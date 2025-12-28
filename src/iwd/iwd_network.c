#include "iwd_network.h"
#include "iwd_dbus.h"
#include "../e_mod_main.h"

/* Global network list */
Eina_List *iwd_networks = NULL;

/* Forward declarations */
static void _network_properties_changed_cb(void *data, const Eldbus_Message *msg);
static void _network_parse_properties(IWD_Network *net, Eldbus_Message_Iter *properties);

/* Create new network */
IWD_Network *
iwd_network_new(const char *path)
{
   IWD_Network *net;
   Eldbus_Connection *conn;
   Eldbus_Object *obj;

   if (!path) return NULL;

   conn = iwd_dbus_conn_get();
   if (!conn) return NULL;

   /* Check if network already exists */
   net = iwd_network_find(path);
   if (net)
   {
      DBG("Network already exists: %s", path);
      return net;
   }

   DBG("Creating new network: %s", path);

   net = E_NEW(IWD_Network, 1);
   if (!net) return NULL;

   net->path = eina_stringshare_add(path);

   /* Create D-Bus object */
   obj = eldbus_object_get(conn, IWD_SERVICE, path);
   if (!obj)
   {
      ERR("Failed to get D-Bus object for network");
      eina_stringshare_del(net->path);
      E_FREE(net);
      return NULL;
   }

   /* Get proxy */
   net->network_proxy = eldbus_proxy_get(obj, IWD_NETWORK_INTERFACE);

   /* Subscribe to property changes */
   net->properties_changed =
      eldbus_proxy_signal_handler_add(net->network_proxy,
                                       "PropertiesChanged",
                                       _network_properties_changed_cb,
                                       net);

   /* Add to global list */
   iwd_networks = eina_list_append(iwd_networks, net);

   INF("Created network: %s", path);

   eldbus_object_unref(obj);
   return net;
}

/* Free network */
void
iwd_network_free(IWD_Network *net)
{
   if (!net) return;

   DBG("Freeing network: %s", net->path);

   iwd_networks = eina_list_remove(iwd_networks, net);

   if (net->properties_changed)
      eldbus_signal_handler_del(net->properties_changed);

   eina_stringshare_del(net->path);
   eina_stringshare_del(net->name);
   eina_stringshare_del(net->type);
   eina_stringshare_del(net->last_connected_time);

   E_FREE(net);
}

/* Find network by path */
IWD_Network *
iwd_network_find(const char *path)
{
   Eina_List *l;
   IWD_Network *net;

   if (!path) return NULL;

   EINA_LIST_FOREACH(iwd_networks, l, net)
   {
      if (net->path && strcmp(net->path, path) == 0)
         return net;
   }

   return NULL;
}

/* Connect error callback */
static void
_network_connect_error_cb(void *data EINA_UNUSED,
                          const Eldbus_Message *msg,
                          Eldbus_Pending *pending EINA_UNUSED)
{
   const char *err_name, *err_msg;

   if (eldbus_message_error_get(msg, &err_name, &err_msg))
   {
      ERR("Failed to connect: %s: %s", err_name, err_msg);

      /* Show user-friendly error */
      if (strstr(err_name, "NotAuthorized") || strstr(err_msg, "Not authorized"))
      {
         e_util_dialog_show("Permission Denied",
                           "You do not have permission to manage Wi-Fi.<br>"
                           "Please configure polkit rules for iwd.");
      }
      else if (strstr(err_name, "Failed") || strstr(err_msg, "operation failed"))
      {
         e_util_dialog_show("Connection Failed",
                           "Failed to connect to the network.<br>"
                           "Please check your password and try again.");
      }
      else
      {
         char buf[512];
         snprintf(buf, sizeof(buf), "Connection error:<br>%s", err_msg ? err_msg : err_name);
         e_util_dialog_show("Connection Error", buf);
      }
   }
}

/* Connect to network */
void
iwd_network_connect(IWD_Network *net)
{
   if (!net || !net->network_proxy)
   {
      ERR("Invalid network for connect");
      return;
   }

   DBG("Connecting to network: %s", net->name ? net->name : net->path);

   /* This will trigger agent RequestPassphrase if needed */
   eldbus_proxy_call(net->network_proxy, "Connect",
                     _network_connect_error_cb, NULL, -1, "");
}

/* Forget network */
void
iwd_network_forget(IWD_Network *net)
{
   Eldbus_Proxy *known_proxy;

   if (!net || !net->network_proxy || !net->known)
   {
      ERR("Invalid network for forget or network not known");
      return;
   }

   DBG("Forgetting network: %s", net->name ? net->name : net->path);

   /* Get KnownNetwork proxy (same path, different interface) */
   known_proxy = eldbus_proxy_get(eldbus_proxy_object_get(net->network_proxy),
                                   IWD_KNOWN_NETWORK_INTERFACE);
   if (!known_proxy)
   {
      ERR("Failed to get KnownNetwork proxy");
      return;
   }

   eldbus_proxy_call(known_proxy, "Forget", NULL, NULL, -1, "");
}

/* Get networks list */
Eina_List *
iwd_networks_get(void)
{
   return iwd_networks;
}

/* Initialize network subsystem */
void
iwd_network_init(void)
{
   DBG("Initializing network subsystem");
   /* Networks will be populated from ObjectManager signals */
}

/* Shutdown network subsystem */
void
iwd_network_shutdown(void)
{
   IWD_Network *net;

   DBG("Shutting down network subsystem");

   EINA_LIST_FREE(iwd_networks, net)
      iwd_network_free(net);
}

/* Properties changed callback */
static void
_network_properties_changed_cb(void *data,
                               const Eldbus_Message *msg)
{
   IWD_Network *net = data;
   const char *interface;
   Eldbus_Message_Iter *changed, *invalidated;

   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &interface, &changed, &invalidated))
   {
      ERR("Failed to parse PropertiesChanged signal");
      return;
   }

   DBG("Properties changed for network %s on interface %s", net->path, interface);

   _network_parse_properties(net, changed);

   /* TODO: Notify UI of changes */
}

/* Parse network properties */
static void
_network_parse_properties(IWD_Network *net,
                         Eldbus_Message_Iter *properties)
{
   Eldbus_Message_Iter *entry;

   if (!properties) return;

   while (eldbus_message_iter_get_and_next(properties, 'e', &entry))
   {
      const char *key;
      Eldbus_Message_Iter *var;

      if (!eldbus_message_iter_arguments_get(entry, "sv", &key, &var))
         continue;

      if (strcmp(key, "Name") == 0)
      {
         const char *name;
         if (eldbus_message_iter_arguments_get(var, "s", &name))
         {
            eina_stringshare_replace(&net->name, name);
            DBG("  Name: %s", net->name);
         }
      }
      else if (strcmp(key, "Type") == 0)
      {
         const char *type;
         if (eldbus_message_iter_arguments_get(var, "s", &type))
         {
            eina_stringshare_replace(&net->type, type);
            DBG("  Type: %s", net->type);
         }
      }
      else if (strcmp(key, "Known") == 0)
      {
         Eina_Bool known;
         if (eldbus_message_iter_arguments_get(var, "b", &known))
         {
            net->known = known;
            DBG("  Known: %d", net->known);
         }
      }
      else if (strcmp(key, "AutoConnect") == 0)
      {
         Eina_Bool auto_connect;
         if (eldbus_message_iter_arguments_get(var, "b", &auto_connect))
         {
            net->auto_connect = auto_connect;
            DBG("  Auto-connect: %d", net->auto_connect);
         }
      }
   }
}
