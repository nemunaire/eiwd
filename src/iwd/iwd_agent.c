#include "iwd_agent.h"
#include "iwd_dbus.h"
#include "iwd_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IWD_AGENT_PATH "/net/eiwd/agent"

struct _Iwd_Agent
{
   Eldbus_Connection            *conn;
   Eldbus_Service_Interface     *svc;
   Eldbus_Object                *am_obj;
   Eldbus_Proxy                 *am_proxy;
   Iwd_Agent_Passphrase_Cb       cb;
   void                         *data;
   Iwd_Agent_Cancel_Cb           cancel_cb;
   void                         *cancel_data;
};

struct _Iwd_Agent_Request
{
   Iwd_Agent      *agent;
   Eldbus_Message *msg;       /* original RequestPassphrase message */
};

static Iwd_Agent *_self = NULL;  /* one agent per process is plenty */

/* ----- Method handlers ------------------------------------------------- */

static Eldbus_Message *
_release_cb(const Eldbus_Service_Interface *iface EINA_UNUSED,
            const Eldbus_Message *msg)
{
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_cancel_cb(const Eldbus_Service_Interface *iface EINA_UNUSED,
           const Eldbus_Message *msg)
{
   /* iwd dropped the auth attempt; let the UI tear down its dialog. */
   const char *reason = NULL;
   if (!eldbus_message_arguments_get(msg, "s", &reason)) reason = NULL;
   if (_self && _self->cancel_cb)
     _self->cancel_cb(_self->cancel_data, reason);
   return eldbus_message_method_return_new(msg);
}

/* iwd may also call these for EAP networks. We don't have UI for them yet,
 * so politely refuse — that just fails the connect attempt instead of
 * getting our agent unregistered. */
static Eldbus_Message *
_unsupported_cb(const Eldbus_Service_Interface *iface EINA_UNUSED,
                const Eldbus_Message *msg)
{
   return eldbus_message_error_new(msg,
       "net.connman.iwd.Agent.Error.Canceled",
       "Method not supported by this agent");
}

static Eldbus_Message *
_request_passphrase_cb(const Eldbus_Service_Interface *iface EINA_UNUSED,
                       const Eldbus_Message *msg)
{
   const char *path = NULL;
   if (!eldbus_message_arguments_get(msg, "o", &path))
     return eldbus_message_error_new(msg, "net.connman.iwd.Error.InvalidArgs",
                                     "Expected object path");
   if (!_self || !_self->cb)
     return eldbus_message_error_new(msg, "net.connman.iwd.Agent.Error.Canceled",
                                     "No UI handler");

   Iwd_Agent_Request *req = calloc(1, sizeof(*req));
   if (!req)
     return eldbus_message_error_new(msg, "net.connman.iwd.Agent.Error.Canceled",
                                     "Out of memory");
   req->agent = _self;
   req->msg   = eldbus_message_ref((Eldbus_Message *)msg);
   _self->cb(_self->data, req, path);
   /* Deferred reply: returning NULL keeps the message pending. */
   return NULL;
}

static const Eldbus_Method _methods[] = {
   { "Release",           NULL,
     NULL, _release_cb, 0 },
   { "RequestPassphrase",
     ELDBUS_ARGS({ "o", "network" }),
     ELDBUS_ARGS({ "s", "passphrase" }),
     _request_passphrase_cb, 0 },
   { "Cancel",
     ELDBUS_ARGS({ "s", "reason" }),
     NULL, _cancel_cb, 0 },
   { "RequestPrivateKeyPassphrase",
     ELDBUS_ARGS({ "o", "network" }),
     ELDBUS_ARGS({ "s", "passphrase" }),
     _unsupported_cb, 0 },
   { "RequestUserNameAndPassword",
     ELDBUS_ARGS({ "o", "network" }),
     ELDBUS_ARGS({ "s", "user" }, { "s", "password" }),
     _unsupported_cb, 0 },
   { "RequestUserPassword",
     ELDBUS_ARGS({ "o", "network" }, { "s", "user" }),
     ELDBUS_ARGS({ "s", "password" }),
     _unsupported_cb, 0 },
   { NULL, NULL, NULL, NULL, 0 }
};

static const Eldbus_Service_Interface_Desc _iface_desc = {
   IWD_IFACE_AGENT, _methods, NULL, NULL, NULL, NULL
};

/* ----- Reply / cancel from the UI ------------------------------------- */

void
iwd_agent_reply(Iwd_Agent_Request *req, const char *passphrase)
{
   if (!req) return;
   /* The passphrase is copied into the eldbus/libdbus marshalled message
    * buffer here. We can't wipe that buffer ourselves — eldbus owns it and
    * frees it asynchronously after the call is sent. Callers are expected
    * to explicit_bzero their own copy of `passphrase` after this returns;
    * the residue inside the outbound D-Bus message is unavoidable at this
    * boundary. */
   Eldbus_Message *r = eldbus_message_method_return_new(req->msg);
   eldbus_message_arguments_append(r, "s", passphrase ? passphrase : "");
   eldbus_connection_send(req->agent->conn, r, NULL, NULL, -1);
   eldbus_message_unref(req->msg);
   free(req);
}

void
iwd_agent_cancel(Iwd_Agent_Request *req)
{
   if (!req) return;
   Eldbus_Message *e = eldbus_message_error_new(req->msg,
                                                "net.connman.iwd.Agent.Error.Canceled",
                                                "User canceled");
   eldbus_connection_send(req->agent->conn, e, NULL, NULL, -1);
   eldbus_message_unref(req->msg);
   free(req);
}

/* ----- Registration with iwd ------------------------------------------ */

static void
_on_register(void *data, const Eldbus_Message *msg,
             Eldbus_Pending *p EINA_UNUSED)
{
   /* `data` is the manager — same pointer the trampoline carries. */
   Iwd_Manager *m = data;
   const char *en, *em;
   if (!eldbus_message_error_get(msg, &en, &em)) return;
   if (m)
     iwd_manager_report_error(m,
        "Wi-Fi agent registration refused (another agent running?): %s",
        em ? em : en);
   else
     fprintf(stderr, "e_iwd: agent register failed: %s: %s\n", en, em);
}

void
iwd_agent_register(Iwd_Agent *a)
{
   if (!a || !a->am_proxy) return;
   eldbus_proxy_call(a->am_proxy, "RegisterAgent", _on_register, a->data, -1,
                     "o", IWD_AGENT_PATH);
}

Iwd_Agent *
iwd_agent_new(Eldbus_Connection *conn, Iwd_Agent_Passphrase_Cb cb, void *data)
{
   Iwd_Agent *a = calloc(1, sizeof(*a));
   if (!a) return NULL;
   a->conn = conn;
   a->cb   = cb;
   a->data = data;
   _self = a;

   a->svc = eldbus_service_interface_register(conn, IWD_AGENT_PATH, &_iface_desc);
   if (!a->svc) { free(a); _self = NULL; return NULL; }

   a->am_obj = eldbus_object_get(conn, IWD_BUS_NAME, "/net/connman/iwd");
   if (a->am_obj)
     a->am_proxy = eldbus_proxy_get(a->am_obj, IWD_IFACE_AGENT_MANAGER);
   iwd_agent_register(a);
   return a;
}

void
iwd_agent_set_cancel_cb(Iwd_Agent *a, Iwd_Agent_Cancel_Cb cb, void *data)
{
   if (!a) return;
   a->cancel_cb   = cb;
   a->cancel_data = data;
}

void
iwd_agent_free(Iwd_Agent *a)
{
   if (!a) return;
   /* Politely deregister so iwd doesn't keep dispatching to a dead service
    * during shutdown. Fire-and-forget: the connection may already be torn
    * down by the time the call would land, and there's nothing to do with
    * the reply anyway. */
   if (a->am_proxy)
     eldbus_proxy_call(a->am_proxy, "UnregisterAgent", NULL, NULL, -1,
                       "o", IWD_AGENT_PATH);
   if (a->svc)      eldbus_service_interface_unregister(a->svc);
   if (a->am_proxy) eldbus_proxy_unref(a->am_proxy);
   if (a->am_obj)   eldbus_object_unref(a->am_obj);
   if (_self == a)  _self = NULL;
   free(a);
}
