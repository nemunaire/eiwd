#include "iwd_adapter.h"
#include "iwd_dbus.h"
#include "iwd_props.h"
#include "iwd_manager.h"
#include <stdlib.h>
#include <string.h>

static void
_prop_cb(void *data, const char *key, Eldbus_Message_Iter *v)
{
   Iwd_Adapter *a = data;
   if (!strcmp(key, "Powered")) a->powered = iwd_props_bool(v);
}

void
iwd_adapter_apply_props(Iwd_Adapter *a, Eldbus_Message_Iter *props)
{
   iwd_props_for_each(props, _prop_cb, a);
}

static void
_on_props_changed(void *data, const Eldbus_Message *msg)
{
   Iwd_Adapter *a = data;
   const char *iface;
   Eldbus_Message_Iter *changed, *invalidated;
   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &iface, &changed, &invalidated))
     return;
   if (strcmp(iface, IWD_IFACE_ADAPTER) != 0) return;
   iwd_props_for_each(changed, _prop_cb, a);
   if (a->manager) iwd_manager_notify(a->manager);
}

Iwd_Adapter *
iwd_adapter_new(Eldbus_Connection *conn, const char *path, void *manager)
{
   Iwd_Adapter *a = calloc(1, sizeof(*a));
   if (!a) return NULL;
   a->path = path ? strdup(path) : NULL;
   a->manager = manager;
   a->obj = eldbus_object_get(conn, IWD_BUS_NAME, path);
   if (a->obj)
     {
        a->proxy = eldbus_proxy_get(a->obj, IWD_IFACE_ADAPTER);
        if (a->proxy)
          a->sh_props = eldbus_proxy_properties_changed_callback_add(
              a->proxy, _on_props_changed, a);
     }
   return a;
}

void
iwd_adapter_free(Iwd_Adapter *a)
{
   if (!a) return;
   if (a->sh_props) eldbus_signal_handler_del(a->sh_props);
   if (a->_props_proxy_keepalive) eldbus_proxy_unref(a->_props_proxy_keepalive);
   if (a->proxy)    eldbus_proxy_unref(a->proxy);
   if (a->obj)      eldbus_object_unref(a->obj);
   free(a->path);
   free(a);
}

static void
_on_set_powered_reply(void *data, const Eldbus_Message *msg, Eldbus_Pending *p EINA_UNUSED)
{
   Iwd_Adapter *a = data;
   const char *en, *em;
   if (eldbus_message_error_get(msg, &en, &em) && a->manager)
     iwd_manager_report_error(a->manager,
                              "Set Adapter.Powered failed: %s", em ? em : en);
}

void
iwd_adapter_set_powered(Iwd_Adapter *a, Eina_Bool on)
{
   if (!a || !a->obj) return;

   /* Call org.freedesktop.DBus.Properties.Set explicitly so we control the
    * variant marshaling exactly. eldbus_proxy_property_set silently swallows
    * Adapter.Powered on this iwd version. */
   Eldbus_Proxy *props = eldbus_proxy_get(a->obj, "org.freedesktop.DBus.Properties");
   if (!props) return;

   Eldbus_Message *msg = eldbus_proxy_method_call_new(props, "Set");
   Eldbus_Message_Iter *iter = eldbus_message_iter_get(msg);
   const char *iface = IWD_IFACE_ADAPTER;
   const char *prop  = "Powered";
   eldbus_message_iter_basic_append(iter, 's', iface);
   eldbus_message_iter_basic_append(iter, 's', prop);
   Eldbus_Message_Iter *variant = eldbus_message_iter_container_new(iter, 'v', "b");
   Eina_Bool v = on;
   eldbus_message_iter_basic_append(variant, 'b', v);
   eldbus_message_iter_container_close(iter, variant);

   eldbus_proxy_send(props, msg, _on_set_powered_reply, a, -1);
   /* Keep the props proxy alive on the adapter so the call isn't canceled. */
   if (a->_props_proxy_keepalive) eldbus_proxy_unref(a->_props_proxy_keepalive);
   a->_props_proxy_keepalive = props;
}
