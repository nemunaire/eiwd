#include "iwd_device.h"
#include "iwd_dbus.h"
#include "iwd_props.h"
#include "iwd_manager.h"
#include "iwd_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _refresh_signals(Iwd_Device *d);

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
   else if (!strcmp(key, "Scanning"))
     {
        Eina_Bool was = d->scanning;
        d->scanning = iwd_props_bool(v);
        /* When a scan finishes, ask iwd for the ranked list with RSSI. */
        if (was && !d->scanning) _refresh_signals(d);
     }
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
        _refresh_signals(d);
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
   if (d->sh_dev_props) eldbus_signal_handler_del(d->sh_dev_props);
   if (d->device_proxy) eldbus_proxy_unref(d->device_proxy);
   if (d->obj)          eldbus_object_unref(d->obj);
   free(d->path);
   free(d->name);
   free(d->address);
   free(d->adapter_path);
   free(d->connected_network);
   free(d);
}

/* Reply to Station.GetOrderedNetworks: a(on) — list of (object_path, RSSI).
 * RSSI is a 16-bit signed value in 100*dBm units. */
static void
_on_ordered_networks(void *data, const Eldbus_Message *msg, Eldbus_Pending *p EINA_UNUSED)
{
   Iwd_Device *d = data;
   const char *en, *em;
   if (eldbus_message_error_get(msg, &en, &em)) return;

   Eldbus_Message_Iter *array = NULL;
   if (!eldbus_message_arguments_get(msg, "a(on)", &array) || !array)
     return;

   const Eina_Hash *nets = d->manager ? iwd_manager_networks(d->manager) : NULL;
   Eldbus_Message_Iter *entry;
   Eina_Bool any = EINA_FALSE;
   while (eldbus_message_iter_get_and_next(array, 'r', &entry))
     {
        const char *path = NULL;
        int16_t rssi = 0;
        if (!eldbus_message_iter_arguments_get(entry, "on", &path, &rssi)) continue;
        if (!nets || !path) continue;
        Iwd_Network *n = eina_hash_find(nets, path);
        if (!n) continue;
        n->signal_dbm  = rssi;
        n->have_signal = EINA_TRUE;
        any = EINA_TRUE;
     }
   if (any && d->manager) iwd_manager_notify(d->manager);
}

static void
_refresh_signals(Iwd_Device *d)
{
   if (!d || !d->station_proxy) return;
   eldbus_proxy_call(d->station_proxy, "GetOrderedNetworks",
                     _on_ordered_networks, d, -1, "");
}

void
iwd_device_scan(Iwd_Device *d)
{
   if (!d || !d->station_proxy) return;
   eldbus_proxy_call(d->station_proxy, "Scan", NULL, NULL, -1, "");
}

void
iwd_device_disconnect(Iwd_Device *d)
{
   if (!d || !d->station_proxy) return;
   eldbus_proxy_call(d->station_proxy, "Disconnect", NULL, NULL, -1, "");
}

static void
_on_connect_hidden_reply(void *data, const Eldbus_Message *msg, Eldbus_Pending *p EINA_UNUSED)
{
   const char *en, *em;
   char *ssid = data;
   if (eldbus_message_error_get(msg, &en, &em))
     fprintf(stderr, "e_iwd: ConnectHiddenNetwork('%s') failed: %s: %s\n",
             ssid ? ssid : "?", en, em);
   free(ssid);
}

void
iwd_device_connect_hidden(Iwd_Device *d, const char *ssid)
{
   if (!d || !d->station_proxy || !ssid || !*ssid) return;
   eldbus_proxy_call(d->station_proxy, "ConnectHiddenNetwork",
                     _on_connect_hidden_reply, strdup(ssid), -1, "s", ssid);
}
