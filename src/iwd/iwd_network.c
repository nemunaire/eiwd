#include "iwd_network.h"
#include "iwd_dbus.h"
#include "iwd_props.h"
#include "iwd_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Iwd_Security
_sec_from_str(const char *s)
{
   if (!s) return IWD_SEC_UNKNOWN;
   if (!strcmp(s, "open"))   return IWD_SEC_OPEN;
   if (!strcmp(s, "psk"))    return IWD_SEC_PSK;
   if (!strcmp(s, "8021x"))  return IWD_SEC_8021X;
   if (!strcmp(s, "wep"))    return IWD_SEC_WEP;
   return IWD_SEC_UNKNOWN;
}

static void
_prop_cb(void *data, const char *key, Eldbus_Message_Iter *v)
{
   Iwd_Network *n = data;
   if (!strcmp(key, "Name"))           { free(n->ssid);        n->ssid        = iwd_props_str_dup(v); }
   else if (!strcmp(key, "Type"))      { char *s = iwd_props_str_dup(v); n->security = _sec_from_str(s); free(s); }
   else if (!strcmp(key, "Connected")) { n->connected = iwd_props_bool(v); }
   else if (!strcmp(key, "Device"))    { free(n->device_path); n->device_path = iwd_props_str_dup(v); }
   else if (!strcmp(key, "KnownNetwork")) { free(n->known_path); n->known_path = iwd_props_str_dup(v); }
}

void
iwd_network_apply_props(Iwd_Network *n, Eldbus_Message_Iter *props)
{
   iwd_props_for_each(props, _prop_cb, n);
}

static void
_on_props_changed(void *data, const Eldbus_Message *msg)
{
   Iwd_Network *n = data;
   const char *iface;
   Eldbus_Message_Iter *changed, *invalidated;
   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &iface, &changed, &invalidated))
     return;
   if (strcmp(iface, IWD_IFACE_NETWORK) != 0) return;
   iwd_props_for_each(changed, _prop_cb, n);
   if (n->manager) iwd_manager_notify(n->manager);
}

Iwd_Network *
iwd_network_new(Eldbus_Connection *conn, const char *path, void *manager)
{
   Iwd_Network *n = calloc(1, sizeof(*n));
   if (!n) return NULL;
   n->path = path ? strdup(path) : NULL;
   n->manager = manager;
   n->security = IWD_SEC_UNKNOWN;
   n->obj = eldbus_object_get(conn, IWD_BUS_NAME, path);
   if (n->obj)
     {
        n->proxy = eldbus_proxy_get(n->obj, IWD_IFACE_NETWORK);
        if (n->proxy)
          n->sh_props = eldbus_proxy_properties_changed_callback_add(
              n->proxy, _on_props_changed, n);
     }
   return n;
}

void
iwd_network_free(Iwd_Network *n)
{
   if (!n) return;
   if (n->sh_props) eldbus_signal_handler_del(n->sh_props);
   if (n->proxy)    eldbus_proxy_unref(n->proxy);
   if (n->obj)      eldbus_object_unref(n->obj);
   free(n->path);
   free(n->ssid);
   free(n->device_path);
   free(n->known_path);
   free(n);
}

static void
_on_connect_reply(void *data, const Eldbus_Message *msg, Eldbus_Pending *p EINA_UNUSED)
{
   const char *en, *em;
   const char *ssid = data;
   if (eldbus_message_error_get(msg, &en, &em))
     fprintf(stderr, "e_iwd: connect to '%s' failed: %s: %s\n",
             ssid ? ssid : "?", en, em);
   free(data);
}

int
iwd_network_signal_tier(const Iwd_Network *n)
{
   if (!n || !n->have_signal) return 0;
   /* iwd reports signal in 100*dBm. Cutoffs in dBm: -60/-67/-74/-80. */
   int dbm = n->signal_dbm / 100;
   if (dbm >= -60) return 4;
   if (dbm >= -67) return 3;
   if (dbm >= -74) return 2;
   if (dbm >= -80) return 1;
   return 1;
}

void
iwd_network_connect(Iwd_Network *n)
{
   if (!n || !n->proxy) return;
   /* Network.Connect() takes no args; iwd will dial the registered Agent
    * for a passphrase if needed. */
   eldbus_proxy_call(n->proxy, "Connect", _on_connect_reply,
                     n->ssid ? strdup(n->ssid) : NULL, -1, "");
}

void
iwd_network_forget(Iwd_Network *n)
{
   if (!n || !n->known_path || !n->obj) return;
   Eldbus_Connection *conn = eldbus_object_connection_get(n->obj);
   Eldbus_Object *kobj = eldbus_object_get(conn, IWD_BUS_NAME, n->known_path);
   if (!kobj) return;
   Eldbus_Proxy *kp = eldbus_proxy_get(kobj, IWD_IFACE_KNOWN_NETWORK);
   if (kp)
     {
        eldbus_proxy_call(kp, "Forget", NULL, NULL, -1, "");
        eldbus_proxy_unref(kp);
     }
   eldbus_object_unref(kobj);
}
