#include "iwd_dbus.h"
#include "../e_mod_main.h"

/* Global D-Bus context */
IWD_DBus *iwd_dbus = NULL;

/* Forward declarations */
static void _iwd_dbus_name_owner_changed_cb(void *data, const char *bus EINA_UNUSED, const char *old_id, const char *new_id);
static void _iwd_dbus_managed_objects_get_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending);
static void _iwd_dbus_interfaces_added_cb(void *data, const Eldbus_Message *msg);
static void _iwd_dbus_interfaces_removed_cb(void *data, const Eldbus_Message *msg);
static void _iwd_dbus_connect(void);
static void _iwd_dbus_disconnect(void);

/* Initialize D-Bus connection */
Eina_Bool
iwd_dbus_init(void)
{
   DBG("Initializing iwd D-Bus connection");

   if (iwd_dbus)
   {
      WRN("D-Bus already initialized");
      return EINA_TRUE;
   }

   iwd_dbus = E_NEW(IWD_DBus, 1);
   if (!iwd_dbus)
   {
      ERR("Failed to allocate D-Bus context");
      return EINA_FALSE;
   }

   /* Connect to system bus */
   iwd_dbus->conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);
   if (!iwd_dbus->conn)
   {
      ERR("Failed to connect to system bus");
      E_FREE(iwd_dbus);
      iwd_dbus = NULL;
      return EINA_FALSE;
   }

   /* Monitor iwd daemon availability */
   eldbus_name_owner_changed_callback_add(iwd_dbus->conn,
                                           IWD_SERVICE,
                                           _iwd_dbus_name_owner_changed_cb,
                                           NULL,
                                           EINA_TRUE);

   /* Try to connect to iwd */
   _iwd_dbus_connect();

   return EINA_TRUE;
}

/* Shutdown D-Bus connection */
void
iwd_dbus_shutdown(void)
{
   DBG("Shutting down iwd D-Bus connection");

   if (!iwd_dbus) return;

   _iwd_dbus_disconnect();

   /* Unregister name owner changed callback */
   eldbus_name_owner_changed_callback_del(iwd_dbus->conn, IWD_SERVICE,
                                           _iwd_dbus_name_owner_changed_cb, NULL);

   if (iwd_dbus->conn)
      eldbus_connection_unref(iwd_dbus->conn);

   E_FREE(iwd_dbus);
   iwd_dbus = NULL;
}

/* Check if connected to iwd */
Eina_Bool
iwd_dbus_is_connected(void)
{
   return (iwd_dbus && iwd_dbus->connected);
}

/* Get D-Bus connection */
Eldbus_Connection *
iwd_dbus_conn_get(void)
{
   return iwd_dbus ? iwd_dbus->conn : NULL;
}

/* Request managed objects from iwd */
void
iwd_dbus_get_managed_objects(void)
{
   Eldbus_Proxy *proxy;

   if (!iwd_dbus || !iwd_dbus->manager_obj) return;

   DBG("Requesting managed objects from iwd");

   proxy = eldbus_proxy_get(iwd_dbus->manager_obj, DBUS_OBJECT_MANAGER_INTERFACE);
   if (!proxy)
   {
      ERR("Failed to get ObjectManager proxy");
      return;
   }

   eldbus_proxy_call(proxy, "GetManagedObjects",
                     _iwd_dbus_managed_objects_get_cb,
                     NULL, -1, "");
}

/* Connect to iwd daemon */
static void
_iwd_dbus_connect(void)
{
   if (!iwd_dbus || !iwd_dbus->conn) return;

   DBG("Connecting to iwd daemon");

   /* Create manager object */
   iwd_dbus->manager_obj = eldbus_object_get(iwd_dbus->conn, IWD_SERVICE, IWD_MANAGER_PATH);
   if (!iwd_dbus->manager_obj)
   {
      ERR("Failed to get manager object");
      return;
   }

   /* Subscribe to ObjectManager signals */
   iwd_dbus->interfaces_added =
      eldbus_proxy_signal_handler_add(
         eldbus_proxy_get(iwd_dbus->manager_obj, DBUS_OBJECT_MANAGER_INTERFACE),
         "InterfacesAdded",
         _iwd_dbus_interfaces_added_cb,
         NULL);

   iwd_dbus->interfaces_removed =
      eldbus_proxy_signal_handler_add(
         eldbus_proxy_get(iwd_dbus->manager_obj, DBUS_OBJECT_MANAGER_INTERFACE),
         "InterfacesRemoved",
         _iwd_dbus_interfaces_removed_cb,
         NULL);

   iwd_dbus->connected = EINA_TRUE;
   INF("Connected to iwd daemon");

   /* Get initial state */
   iwd_dbus_get_managed_objects();
}

/* Disconnect from iwd daemon */
static void
_iwd_dbus_disconnect(void)
{
   if (!iwd_dbus) return;

   DBG("Disconnecting from iwd daemon");

   if (iwd_dbus->interfaces_added)
   {
      eldbus_signal_handler_del(iwd_dbus->interfaces_added);
      iwd_dbus->interfaces_added = NULL;
   }

   if (iwd_dbus->interfaces_removed)
   {
      eldbus_signal_handler_del(iwd_dbus->interfaces_removed);
      iwd_dbus->interfaces_removed = NULL;
   }

   if (iwd_dbus->manager_obj)
   {
      eldbus_object_unref(iwd_dbus->manager_obj);
      iwd_dbus->manager_obj = NULL;
   }

   iwd_dbus->connected = EINA_FALSE;
}

/* Name owner changed callback */
static void
_iwd_dbus_name_owner_changed_cb(void *data EINA_UNUSED,
                                 const char *bus EINA_UNUSED,
                                 const char *old_id,
                                 const char *new_id)
{
   DBG("iwd name owner changed: old='%s' new='%s'", old_id, new_id);

   if (new_id && new_id[0])
   {
      /* iwd daemon started */
      INF("iwd daemon started");
      _iwd_dbus_connect();
   }
   else if (old_id && old_id[0])
   {
      /* iwd daemon stopped */
      WRN("iwd daemon stopped");
      _iwd_dbus_disconnect();
      /* TODO: Notify UI to show error state */
   }
}

/* Managed objects callback */
static void
_iwd_dbus_managed_objects_get_cb(void *data EINA_UNUSED,
                                  const Eldbus_Message *msg,
                                  Eldbus_Pending *pending EINA_UNUSED)
{
   Eldbus_Message_Iter *array, *dict_entry;
   const char *err_name, *err_msg;

   if (eldbus_message_error_get(msg, &err_name, &err_msg))
   {
      ERR("Failed to get managed objects: %s: %s", err_name, err_msg);
      return;
   }

   if (!eldbus_message_arguments_get(msg, "a{oa{sa{sv}}}", &array))
   {
      ERR("Failed to parse GetManagedObjects reply");
      return;
   }

   DBG("Processing managed objects from iwd");

   while (eldbus_message_iter_get_and_next(array, 'e', &dict_entry))
   {
      Eldbus_Message_Iter *interfaces;
      const char *path;

      eldbus_message_iter_arguments_get(dict_entry, "o", &path);
      eldbus_message_iter_arguments_get(dict_entry, "a{sa{sv}}", &interfaces);

      DBG("  Object: %s", path);

      /* Parse interfaces and create device/network objects */
      Eldbus_Message_Iter *iface_entry;
      Eina_Bool has_device = EINA_FALSE;
      Eina_Bool has_station = EINA_FALSE;
      Eina_Bool has_network = EINA_FALSE;

      while (eldbus_message_iter_get_and_next(interfaces, 'e', &iface_entry))
      {
         const char *iface_name;
         Eldbus_Message_Iter *properties;

         eldbus_message_iter_arguments_get(iface_entry, "s", &iface_name);
         eldbus_message_iter_arguments_get(iface_entry, "a{sv}", &properties);

         DBG("    Interface: %s", iface_name);

         if (strcmp(iface_name, IWD_DEVICE_INTERFACE) == 0)
            has_device = EINA_TRUE;
         else if (strcmp(iface_name, IWD_STATION_INTERFACE) == 0)
            has_station = EINA_TRUE;
         else if (strcmp(iface_name, IWD_NETWORK_INTERFACE) == 0)
            has_network = EINA_TRUE;
      }

      /* Create device if it has Device and Station interfaces */
      if (has_device && has_station)
      {
         extern IWD_Device *iwd_device_new(const char *path);
         IWD_Device *dev = iwd_device_new(path);
         if (dev)
         {
            /* Parse properties - re-iterate to get them */
            eldbus_message_iter_arguments_get(dict_entry, "o", &path);
            eldbus_message_iter_arguments_get(dict_entry, "a{sa{sv}}", &interfaces);

            while (eldbus_message_iter_get_and_next(interfaces, 'e', &iface_entry))
            {
               const char *iface_name;
               Eldbus_Message_Iter *properties;

               eldbus_message_iter_arguments_get(iface_entry, "s", &iface_name);
               eldbus_message_iter_arguments_get(iface_entry, "a{sv}", &properties);

               if (strcmp(iface_name, IWD_DEVICE_INTERFACE) == 0 ||
                   strcmp(iface_name, IWD_STATION_INTERFACE) == 0)
               {
                  extern void _device_parse_properties(IWD_Device *dev, Eldbus_Message_Iter *properties);
                  /* We need to expose the parse function or call it via a public function */
                  /* For now, properties will be updated via PropertyChanged signals */
               }
            }
         }
      }

      /* Create network if it has Network interface */
      if (has_network)
      {
         extern IWD_Network *iwd_network_new(const char *path);
         iwd_network_new(path);
      }
   }
}

/* Interfaces added callback */
static void
_iwd_dbus_interfaces_added_cb(void *data EINA_UNUSED,
                              const Eldbus_Message *msg)
{
   const char *path;
   Eldbus_Message_Iter *interfaces;

   if (!eldbus_message_arguments_get(msg, "oa{sa{sv}}", &path, &interfaces))
   {
      ERR("Failed to parse InterfacesAdded signal");
      return;
   }

   DBG("Interfaces added at: %s", path);

   /* Check what interfaces were added */
   Eldbus_Message_Iter *iface_entry;
   Eina_Bool has_device = EINA_FALSE;
   Eina_Bool has_station = EINA_FALSE;
   Eina_Bool has_network = EINA_FALSE;

   while (eldbus_message_iter_get_and_next(interfaces, 'e', &iface_entry))
   {
      const char *iface_name;
      Eldbus_Message_Iter *properties;

      eldbus_message_iter_arguments_get(iface_entry, "s", &iface_name);
      eldbus_message_iter_arguments_get(iface_entry, "a{sv}", &properties);

      if (strcmp(iface_name, IWD_DEVICE_INTERFACE) == 0)
         has_device = EINA_TRUE;
      else if (strcmp(iface_name, IWD_STATION_INTERFACE) == 0)
         has_station = EINA_TRUE;
      else if (strcmp(iface_name, IWD_NETWORK_INTERFACE) == 0)
         has_network = EINA_TRUE;
   }

   if (has_device && has_station)
   {
      extern IWD_Device *iwd_device_new(const char *path);
      iwd_device_new(path);
   }

   if (has_network)
   {
      extern IWD_Network *iwd_network_new(const char *path);
      iwd_network_new(path);
   }
}

/* Interfaces removed callback */
static void
_iwd_dbus_interfaces_removed_cb(void *data EINA_UNUSED,
                                const Eldbus_Message *msg)
{
   const char *path;
   Eldbus_Message_Iter *interfaces;

   if (!eldbus_message_arguments_get(msg, "oas", &path, &interfaces))
   {
      ERR("Failed to parse InterfacesRemoved signal");
      return;
   }

   DBG("Interfaces removed at: %s", path);

   /* Check if we should remove device or network */
   extern IWD_Device *iwd_device_find(const char *path);
   extern IWD_Network *iwd_network_find(const char *path);
   extern void iwd_device_free(IWD_Device *dev);
   extern void iwd_network_free(IWD_Network *net);

   IWD_Device *dev = iwd_device_find(path);
   if (dev)
   {
      iwd_device_free(dev);
   }

   IWD_Network *net = iwd_network_find(path);
   if (net)
   {
      iwd_network_free(net);
   }
}
