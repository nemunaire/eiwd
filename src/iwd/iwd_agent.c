#include "iwd_agent.h"
#include "iwd_dbus.h"
#include "../e_mod_main.h"

/* Global agent */
IWD_Agent *iwd_agent = NULL;

/* Forward declarations */
static Eldbus_Message *_agent_request_passphrase(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_agent_cancel(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_agent_release(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static void _agent_register_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending);
static void _agent_unregister(void);

/* Agent interface methods */
static const Eldbus_Method agent_methods[] = {
   {
      "RequestPassphrase", ELDBUS_ARGS({"o", "network"}),
      ELDBUS_ARGS({"s", "passphrase"}),
      _agent_request_passphrase, 0
   },
   {
      "Cancel", ELDBUS_ARGS({"s", "reason"}),
      NULL,
      _agent_cancel, 0
   },
   {
      "Release", NULL, NULL,
      _agent_release, 0
   },
   { NULL, NULL, NULL, NULL, 0 }
};

/* Agent interface description */
static const Eldbus_Service_Interface_Desc agent_desc = {
   IWD_AGENT_MANAGER_INTERFACE, agent_methods, NULL, NULL, NULL, NULL
};

/* Initialize agent */
Eina_Bool
iwd_agent_init(void)
{
   Eldbus_Connection *conn;
   Eldbus_Object *obj;
   Eldbus_Proxy *proxy;

   DBG("Initializing iwd agent");

   if (iwd_agent)
   {
      WRN("Agent already initialized");
      return EINA_TRUE;
   }

   conn = iwd_dbus_conn_get();
   if (!conn)
   {
      ERR("No D-Bus connection available");
      return EINA_FALSE;
   }

   iwd_agent = E_NEW(IWD_Agent, 1);
   if (!iwd_agent)
   {
      ERR("Failed to allocate agent");
      return EINA_FALSE;
   }

   /* Register D-Bus service interface */
   iwd_agent->iface = eldbus_service_interface_register(conn, IWD_AGENT_PATH, &agent_desc);
   if (!iwd_agent->iface)
   {
      ERR("Failed to register agent interface");
      E_FREE(iwd_agent);
      iwd_agent = NULL;
      return EINA_FALSE;
   }

   /* Register agent with iwd */
   obj = eldbus_object_get(conn, IWD_SERVICE, IWD_MANAGER_PATH);
   if (!obj)
   {
      ERR("Failed to get iwd manager object");
      eldbus_service_interface_unregister(iwd_agent->iface);
      E_FREE(iwd_agent);
      iwd_agent = NULL;
      return EINA_FALSE;
   }

   proxy = eldbus_proxy_get(obj, IWD_AGENT_MANAGER_INTERFACE);
   if (!proxy)
   {
      ERR("Failed to get AgentManager proxy");
      eldbus_object_unref(obj);
      eldbus_service_interface_unregister(iwd_agent->iface);
      E_FREE(iwd_agent);
      iwd_agent = NULL;
      return EINA_FALSE;
   }

   eldbus_proxy_call(proxy, "RegisterAgent", _agent_register_cb, NULL, -1, "o", IWD_AGENT_PATH);

   eldbus_object_unref(obj);

   INF("Agent initialized");
   return EINA_TRUE;
}

/* Shutdown agent */
void
iwd_agent_shutdown(void)
{
   DBG("Shutting down iwd agent");

   if (!iwd_agent) return;

   _agent_unregister();

   if (iwd_agent->iface)
      eldbus_service_interface_unregister(iwd_agent->iface);

   eina_stringshare_del(iwd_agent->pending_network_path);
   eina_stringshare_del(iwd_agent->pending_passphrase);

   E_FREE(iwd_agent);
   iwd_agent = NULL;
}

/* Set passphrase for pending request */
void
iwd_agent_set_passphrase(const char *passphrase)
{
   if (!iwd_agent) return;

   eina_stringshare_replace(&iwd_agent->pending_passphrase, passphrase);
   DBG("Passphrase set for pending request");
}

/* Cancel pending request */
void
iwd_agent_cancel(void)
{
   if (!iwd_agent) return;

   eina_stringshare_del(iwd_agent->pending_network_path);
   eina_stringshare_del(iwd_agent->pending_passphrase);
   iwd_agent->pending_network_path = NULL;
   iwd_agent->pending_passphrase = NULL;

   DBG("Agent request cancelled");
}

/* Agent registration callback */
static void
_agent_register_cb(void *data EINA_UNUSED,
                   const Eldbus_Message *msg,
                   Eldbus_Pending *pending EINA_UNUSED)
{
   const char *err_name, *err_msg;

   if (eldbus_message_error_get(msg, &err_name, &err_msg))
   {
      ERR("Failed to register agent: %s: %s", err_name, err_msg);
      return;
   }

   INF("Agent registered with iwd");
}

/* Unregister agent */
static void
_agent_unregister(void)
{
   Eldbus_Connection *conn;
   Eldbus_Object *obj;
   Eldbus_Proxy *proxy;

   conn = iwd_dbus_conn_get();
   if (!conn) return;

   obj = eldbus_object_get(conn, IWD_SERVICE, IWD_MANAGER_PATH);
   if (!obj) return;

   proxy = eldbus_proxy_get(obj, IWD_AGENT_MANAGER_INTERFACE);
   if (proxy)
   {
      eldbus_proxy_call(proxy, "UnregisterAgent", NULL, NULL, -1, "o", IWD_AGENT_PATH);
      DBG("Agent unregistered from iwd");
   }

   eldbus_object_unref(obj);
}

/* Request passphrase method */
static Eldbus_Message *
_agent_request_passphrase(const Eldbus_Service_Interface *iface EINA_UNUSED,
                          const Eldbus_Message *msg)
{
   const char *network_path;

   if (!eldbus_message_arguments_get(msg, "o", &network_path))
   {
      ERR("Failed to get network path from RequestPassphrase");
      return eldbus_message_error_new(msg, "org.freedesktop.DBus.Error.InvalidArgs", "Invalid arguments");
   }

   INF("Passphrase requested for network: %s", network_path);

   /* Store network path for reference */
   eina_stringshare_replace(&iwd_agent->pending_network_path, network_path);

   /* TODO: Show passphrase dialog (Phase 4) */
   /* For now, just return an error to indicate we're not ready */

   return eldbus_message_error_new(msg, "net.connman.iwd.Agent.Error.Canceled", "UI not implemented yet");
}

/* Cancel method */
static Eldbus_Message *
_agent_cancel(const Eldbus_Service_Interface *iface EINA_UNUSED,
             const Eldbus_Message *msg)
{
   const char *reason;

   if (!eldbus_message_arguments_get(msg, "s", &reason))
   {
      WRN("Cancel called with no reason");
      reason = "unknown";
   }

   INF("Agent request cancelled: %s", reason);

   iwd_agent_cancel();

   /* TODO: Close passphrase dialog if open (Phase 4) */

   return eldbus_message_method_return_new(msg);
}

/* Release method */
static Eldbus_Message *
_agent_release(const Eldbus_Service_Interface *iface EINA_UNUSED,
              const Eldbus_Message *msg)
{
   INF("Agent released by iwd");

   iwd_agent_cancel();

   return eldbus_message_method_return_new(msg);
}
