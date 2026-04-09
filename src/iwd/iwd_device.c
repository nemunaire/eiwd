#include "iwd_device.h"
#include "iwd_dbus.h"
#include "iwd_props.h"
#include "iwd_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Iwd_Station_State
_state_from_str(const char *s)
{
   if (!s) return IWD_STATION_DISCONNECTED;
   if (!strcmp(s, "connected"))     return IWD_STATION_CONNECTED;
   if (!strcmp(s, "connecting"))    return IWD_STATION_CONNECTING;
   if (!strcmp(s, "disconnecting")) return IWD_STATION_DISCONNECTING;
   if (!strcmp(s, "roaming"))       return IWD_STATION_ROAMING;
   return IWD_STATION_DISCONNECTED;
}

static void
_dev_prop_cb(void *data, const char *key, Eldbus_Message_Iter *v)
{
   Iwd_Device *d = data;
   if (!strcmp(key, "Name"))         { free(d->name);    d->name    = iwd_props_str_dup(v); }
   else if (!strcmp(key, "Address")) { free(d->address); d->address = iwd_props_str_dup(v); }
   else if (!strcmp(key, "Adapter")) { free(d->adapter_path); d->adapter_path = iwd_props_str_dup(v); }
   else if (!strcmp(key, "Powered")) { d->powered = iwd_props_bool(v); }
}

static void
_sta_prop_cb(void *data, const char *key, Eldbus_Message_Iter *v)
{
   Iwd_Device *d = data;
   if (!strcmp(key, "State"))
     {
        char *s = iwd_props_str_dup(v);
        d->station_state = _state_from_str(s);
        free(s);
     }
   else if (!strcmp(key, "Scanning"))         { d->scanning = iwd_props_bool(v); }
   else if (!strcmp(key, "ConnectedNetwork")) { free(d->connected_network); d->connected_network = iwd_props_str_dup(v); }
}

void
iwd_device_apply_device_props(Iwd_Device *d, Eldbus_Message_Iter *props)
{
   iwd_props_for_each(props, _dev_prop_cb, d);
}

void
iwd_device_apply_station_props(Iwd_Device *d, Eldbus_Message_Iter *props)
{
   d->has_station = EINA_TRUE;
   iwd_props_for_each(props, _sta_prop_cb, d);
}

/* org.freedesktop.DBus.Properties.PropertiesChanged: (s, a{sv}, as) */
static void
_on_dev_props_changed(void *data, const Eldbus_Message *msg)
{
   Iwd_Device *d = data;
   const char *iface;
   Eldbus_Message_Iter *changed, *invalidated;
   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &iface, &changed, &invalidated))
     return;
   if (strcmp(iface, IWD_IFACE_DEVICE) != 0) return;
   iwd_props_for_each(changed, _dev_prop_cb, d);
   if (d->manager) iwd_manager_notify(d->manager);
}

static void
_on_sta_props_changed(void *data, const Eldbus_Message *msg)
{
   Iwd_Device *d = data;
   const char *iface;
   Eldbus_Message_Iter *changed, *invalidated;
   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &iface, &changed, &invalidated))
     return;
   if (strcmp(iface, IWD_IFACE_STATION) != 0) return;
   iwd_props_for_each(changed, _sta_prop_cb, d);
   if (d->manager) iwd_manager_notify(d->manager);
}

Iwd_Device *
iwd_device_new(Eldbus_Connection *conn, const char *path, void *manager)
{
   Iwd_Device *d = calloc(1, sizeof(*d));
   if (!d) return NULL;
   d->path = path ? strdup(path) : NULL;
   d->manager = manager;
   d->obj = eldbus_object_get(conn, IWD_BUS_NAME, path);
   if (d->obj)
     {
        d->device_proxy = eldbus_proxy_get(d->obj, IWD_IFACE_DEVICE);
        if (d->device_proxy)
          d->sh_dev_props = eldbus_proxy_properties_changed_callback_add(
              d->device_proxy, _on_dev_props_changed, d);
     }
   return d;
}

void
iwd_device_attach_station(Iwd_Device *d)
{
   if (!d || d->station_proxy || !d->obj) return;
   d->station_proxy = eldbus_proxy_get(d->obj, IWD_IFACE_STATION);
   if (d->station_proxy)
     {
        d->has_station = EINA_TRUE;
        d->sh_sta_props = eldbus_proxy_properties_changed_callback_add(
            d->station_proxy, _on_sta_props_changed, d);
     }
}

void
iwd_device_detach_station(Iwd_Device *d)
{
   if (!d) return;
   if (d->sh_sta_props) { eldbus_signal_handler_del(d->sh_sta_props); d->sh_sta_props = NULL; }
   if (d->station_proxy) { eldbus_proxy_unref(d->station_proxy); d->station_proxy = NULL; }
   d->has_station = EINA_FALSE;
}

void
iwd_device_free(Iwd_Device *d)
{
   if (!d) return;
   iwd_device_detach_station(d);
   if (d->sh_dev_props)  eldbus_signal_handler_del(d->sh_dev_props);
   if (d->device_proxy)  eldbus_proxy_unref(d->device_proxy);
   if (d->adapter_proxy) eldbus_proxy_unref(d->adapter_proxy);
   if (d->adapter_obj)   eldbus_object_unref(d->adapter_obj);
   if (d->obj)           eldbus_object_unref(d->obj);
   free(d->path);
   free(d->name);
   free(d->address);
   free(d->adapter_path);
   free(d->connected_network);
   free(d);
}

static void
_log_reply(void *data, const Eldbus_Message *msg, Eldbus_Pending *p EINA_UNUSED)
{
   const char *what = data;
   const char *en, *em;
   if (eldbus_message_error_get(msg, &en, &em))
     fprintf(stderr, "e_iwd: %s failed: %s: %s\n", what, en, em);
   else
     fprintf(stderr, "e_iwd: %s ok\n", what);
}

void
iwd_device_set_powered(Iwd_Device *d, Eina_Bool on)
{
   if (!d) return;
   Eina_Bool v = on;

   /* Toggle the radio at the Adapter level — that's what actually takes
    * the interface up/down on modern iwd. Device.Powered is a no-op on
    * many installs. Keep the adapter proxy alive on the device so the
    * pending property_set isn't canceled. */
   if (d->adapter_path && d->obj && !d->adapter_proxy)
     {
        Eldbus_Connection *conn = eldbus_object_connection_get(d->obj);
        d->adapter_obj = eldbus_object_get(conn, IWD_BUS_NAME, d->adapter_path);
        if (d->adapter_obj)
          d->adapter_proxy = eldbus_proxy_get(d->adapter_obj, IWD_IFACE_ADAPTER);
     }
   if (d->adapter_proxy)
     eldbus_proxy_property_set(d->adapter_proxy, "Powered", "b", &v, _log_reply,
                               on ? "Adapter.Powered=true"
                                  : "Adapter.Powered=false");
   else
     fprintf(stderr, "e_iwd: set_powered: no Adapter for %s\n",
             d->path ? d->path : "?");

   if (d->device_proxy)
     eldbus_proxy_property_set(d->device_proxy, "Powered", "b", &v, NULL, NULL);
}

void
iwd_device_scan(Iwd_Device *d)
{
   if (!d)
     {
        fprintf(stderr, "e_iwd: scan: NULL device\n");
        return;
     }
   if (!d->station_proxy)
     {
        fprintf(stderr, "e_iwd: scan: no Station proxy on %s\n",
                d->path ? d->path : "?");
        return;
     }
   eldbus_proxy_call(d->station_proxy, "Scan", _log_reply, "Scan", -1, "");
}

void
iwd_device_disconnect(Iwd_Device *d)
{
   if (!d || !d->station_proxy)
     {
        fprintf(stderr, "e_iwd: disconnect: missing Station proxy\n");
        return;
     }
   eldbus_proxy_call(d->station_proxy, "Disconnect", _log_reply, "Disconnect", -1, "");
}
