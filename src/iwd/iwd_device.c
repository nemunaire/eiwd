#include "iwd_device.h"
#include "iwd_dbus.h"
#include "../e_mod_main.h"

/* Global device list */
Eina_List *iwd_devices = NULL;

/* Forward declarations */
static void _device_properties_changed_cb(void *data, const Eldbus_Message *msg);
static void _device_parse_properties(IWD_Device *dev, Eldbus_Message_Iter *properties);

/* Create new device */
IWD_Device *
iwd_device_new(const char *path)
{
   IWD_Device *dev;
   Eldbus_Connection *conn;
   Eldbus_Object *obj;

   if (!path) return NULL;

   conn = iwd_dbus_conn_get();
   if (!conn) return NULL;

   /* Check if device already exists */
   dev = iwd_device_find(path);
   if (dev)
   {
      DBG("Device already exists: %s", path);
      return dev;
   }

   DBG("Creating new device: %s", path);

   dev = E_NEW(IWD_Device, 1);
   if (!dev) return NULL;

   dev->path = eina_stringshare_add(path);

   /* Create D-Bus object */
   obj = eldbus_object_get(conn, IWD_SERVICE, path);
   if (!obj)
   {
      ERR("Failed to get D-Bus object for device");
      eina_stringshare_del(dev->path);
      E_FREE(dev);
      return NULL;
   }

   /* Get proxies */
   dev->device_proxy = eldbus_proxy_get(obj, IWD_DEVICE_INTERFACE);
   dev->station_proxy = eldbus_proxy_get(obj, IWD_STATION_INTERFACE);

   /* Subscribe to property changes */
   dev->properties_changed =
      eldbus_proxy_signal_handler_add(dev->station_proxy,
                                       "PropertiesChanged",
                                       _device_properties_changed_cb,
                                       dev);

   /* Add to global list */
   iwd_devices = eina_list_append(iwd_devices, dev);

   INF("Created device: %s", path);

   eldbus_object_unref(obj);
   return dev;
}

/* Free device */
void
iwd_device_free(IWD_Device *dev)
{
   if (!dev) return;

   DBG("Freeing device: %s", dev->path);

   iwd_devices = eina_list_remove(iwd_devices, dev);

   if (dev->properties_changed)
      eldbus_signal_handler_del(dev->properties_changed);

   eina_stringshare_del(dev->path);
   eina_stringshare_del(dev->name);
   eina_stringshare_del(dev->address);
   eina_stringshare_del(dev->adapter_path);
   eina_stringshare_del(dev->mode);
   eina_stringshare_del(dev->state);
   eina_stringshare_del(dev->connected_network);

   E_FREE(dev);
}

/* Find device by path */
IWD_Device *
iwd_device_find(const char *path)
{
   Eina_List *l;
   IWD_Device *dev;

   if (!path) return NULL;

   EINA_LIST_FOREACH(iwd_devices, l, dev)
   {
      if (dev->path && strcmp(dev->path, path) == 0)
         return dev;
   }

   return NULL;
}

/* Trigger scan */
void
iwd_device_scan(IWD_Device *dev)
{
   if (!dev || !dev->station_proxy)
   {
      ERR("Invalid device for scan");
      return;
   }

   DBG("Triggering scan on device: %s", dev->name ? dev->name : dev->path);

   eldbus_proxy_call(dev->station_proxy, "Scan", NULL, NULL, -1, "");
}

/* Disconnect from network */
void
iwd_device_disconnect(IWD_Device *dev)
{
   if (!dev || !dev->station_proxy)
   {
      ERR("Invalid device for disconnect");
      return;
   }

   DBG("Disconnecting device: %s", dev->name ? dev->name : dev->path);

   eldbus_proxy_call(dev->station_proxy, "Disconnect", NULL, NULL, -1, "");
}

/* Connect to hidden network */
void
iwd_device_connect_hidden(IWD_Device *dev, const char *ssid)
{
   if (!dev || !dev->station_proxy || !ssid)
   {
      ERR("Invalid parameters for hidden network connect");
      return;
   }

   DBG("Connecting to hidden network: %s", ssid);

   eldbus_proxy_call(dev->station_proxy, "ConnectHiddenNetwork",
                     NULL, NULL, -1, "s", ssid);
}

/* Get devices list */
Eina_List *
iwd_devices_get(void)
{
   return iwd_devices;
}

/* Initialize device subsystem */
void
iwd_device_init(void)
{
   DBG("Initializing device subsystem");
   /* Devices will be populated from ObjectManager signals */
}

/* Shutdown device subsystem */
void
iwd_device_shutdown(void)
{
   IWD_Device *dev;

   DBG("Shutting down device subsystem");

   EINA_LIST_FREE(iwd_devices, dev)
      iwd_device_free(dev);
}

/* Properties changed callback */
static void
_device_properties_changed_cb(void *data,
                              const Eldbus_Message *msg)
{
   IWD_Device *dev = data;
   const char *interface;
   Eldbus_Message_Iter *changed, *invalidated;

   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &interface, &changed, &invalidated))
   {
      ERR("Failed to parse PropertiesChanged signal");
      return;
   }

   DBG("Properties changed for device %s on interface %s", dev->path, interface);

   _device_parse_properties(dev, changed);

   /* TODO: Notify UI of state changes */
}

/* Parse device properties */
static void
_device_parse_properties(IWD_Device *dev,
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
            eina_stringshare_replace(&dev->name, name);
            DBG("  Name: %s", dev->name);
         }
      }
      else if (strcmp(key, "Address") == 0)
      {
         const char *address;
         if (eldbus_message_iter_arguments_get(var, "s", &address))
         {
            eina_stringshare_replace(&dev->address, address);
            DBG("  Address: %s", dev->address);
         }
      }
      else if (strcmp(key, "Powered") == 0)
      {
         Eina_Bool powered;
         if (eldbus_message_iter_arguments_get(var, "b", &powered))
         {
            dev->powered = powered;
            DBG("  Powered: %d", dev->powered);
         }
      }
      else if (strcmp(key, "Scanning") == 0)
      {
         Eina_Bool scanning;
         if (eldbus_message_iter_arguments_get(var, "b", &scanning))
         {
            dev->scanning = scanning;
            DBG("  Scanning: %d", dev->scanning);
         }
      }
      else if (strcmp(key, "State") == 0)
      {
         const char *state;
         if (eldbus_message_iter_arguments_get(var, "s", &state))
         {
            eina_stringshare_replace(&dev->state, state);
            DBG("  State: %s", dev->state);
         }
      }
      else if (strcmp(key, "ConnectedNetwork") == 0)
      {
         const char *network;
         if (eldbus_message_iter_arguments_get(var, "o", &network))
         {
            eina_stringshare_replace(&dev->connected_network, network);
            DBG("  Connected network: %s", dev->connected_network);
         }
      }
      else if (strcmp(key, "Mode") == 0)
      {
         const char *mode;
         if (eldbus_message_iter_arguments_get(var, "s", &mode))
         {
            eina_stringshare_replace(&dev->mode, mode);
            DBG("  Mode: %s", dev->mode);
         }
      }
   }
}
