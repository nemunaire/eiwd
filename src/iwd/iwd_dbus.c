#include "iwd_dbus.h"
#include <stdlib.h>
#include <string.h>

#define FDO_OBJECT_MANAGER "org.freedesktop.DBus.ObjectManager"

struct _Iwd_Dbus
{
   Eldbus_Connection        *conn;
   Iwd_Dbus_Callbacks        cbs;
   void                     *data;

   Eldbus_Object            *root_obj;
   Eldbus_Proxy             *root_om;
   Eldbus_Signal_Handler    *sh_added;
   Eldbus_Signal_Handler    *sh_removed;
   Eina_Bool                 present;
};

Eldbus_Connection *
iwd_dbus_conn(const Iwd_Dbus *d) { return d ? d->conn : NULL; }

/* Walk the a{oa{sa{sv}}} reply from GetManagedObjects, emitting iface_added
 * for every (path, interface) pair. */
static void
_emit_managed(Iwd_Dbus *d, Eldbus_Message_Iter *objects)
{
   Eldbus_Message_Iter *entry;
   while (eldbus_message_iter_get_and_next(objects, 'e', &entry))
     {
        const char *path;
        Eldbus_Message_Iter *ifaces;
        if (!eldbus_message_iter_arguments_get(entry, "oa{sa{sv}}", &path, &ifaces))
          continue;

        Eldbus_Message_Iter *iface_entry;
        while (eldbus_message_iter_get_and_next(ifaces, 'e', &iface_entry))
          {
             const char *iface;
             Eldbus_Message_Iter *props;
             if (!eldbus_message_iter_arguments_get(iface_entry, "sa{sv}", &iface, &props))
               continue;
             if (d->cbs.iface_added)
               d->cbs.iface_added(d->data, path, iface, props);
          }
     }
}

static void
_on_get_managed(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   Iwd_Dbus *d = data;
   if (eldbus_message_error_get(msg, NULL, NULL)) return;
   Eldbus_Message_Iter *objects;
   if (!eldbus_message_arguments_get(msg, "a{oa{sa{sv}}}", &objects)) return;
   _emit_managed(d, objects);
}

static void
_on_iface_added(void *data, const Eldbus_Message *msg)
{
   Iwd_Dbus *d = data;
   const char *path;
   Eldbus_Message_Iter *ifaces, *iface_entry;
   if (!eldbus_message_arguments_get(msg, "oa{sa{sv}}", &path, &ifaces))
     return;
   while (eldbus_message_iter_get_and_next(ifaces, 'e', &iface_entry))
     {
        const char *iface;
        Eldbus_Message_Iter *props;
        if (!eldbus_message_iter_arguments_get(iface_entry, "sa{sv}", &iface, &props))
          continue;
        if (d->cbs.iface_added)
          d->cbs.iface_added(d->data, path, iface, props);
     }
}

static void
_on_iface_removed(void *data, const Eldbus_Message *msg)
{
   Iwd_Dbus *d = data;
   const char *path;
   Eldbus_Message_Iter *ifaces;
   if (!eldbus_message_arguments_get(msg, "oas", &path, &ifaces))
     return;
   const char *iface;
   while (eldbus_message_iter_get_and_next(ifaces, 's', &iface))
     {
        if (d->cbs.iface_removed)
          d->cbs.iface_removed(d->data, path, iface);
     }
}

static void
_bind_root(Iwd_Dbus *d)
{
   if (d->root_om) return;
   d->root_obj = eldbus_object_get(d->conn, IWD_BUS_NAME, "/");
   if (!d->root_obj) return;
   d->root_om = eldbus_proxy_get(d->root_obj, FDO_OBJECT_MANAGER);
   if (!d->root_om) return;

   d->sh_added = eldbus_proxy_signal_handler_add(d->root_om, "InterfacesAdded",
                                                 _on_iface_added, d);
   d->sh_removed = eldbus_proxy_signal_handler_add(d->root_om, "InterfacesRemoved",
                                                   _on_iface_removed, d);
   eldbus_proxy_call(d->root_om, "GetManagedObjects", _on_get_managed, d, -1, "");
}

static void
_unbind_root(Iwd_Dbus *d)
{
   if (d->sh_added)   { eldbus_signal_handler_del(d->sh_added);   d->sh_added = NULL; }
   if (d->sh_removed) { eldbus_signal_handler_del(d->sh_removed); d->sh_removed = NULL; }
   if (d->root_om)    { eldbus_proxy_unref(d->root_om);  d->root_om = NULL; }
   if (d->root_obj)   { eldbus_object_unref(d->root_obj); d->root_obj = NULL; }
}

static void
_on_name_owner(void *data, const char *bus EINA_UNUSED, const char *old_id, const char *new_id)
{
   Iwd_Dbus *d = data;
   Eina_Bool now = (new_id && new_id[0]);
   Eina_Bool was = (old_id && old_id[0]);

   if (now && !d->present)
     {
        d->present = EINA_TRUE;
        _bind_root(d);
        if (d->cbs.name_appeared) d->cbs.name_appeared(d->data);
     }
   else if (!now && (was || d->present))
     {
        d->present = EINA_FALSE;
        _unbind_root(d);
        if (d->cbs.name_vanished) d->cbs.name_vanished(d->data);
     }
}

Iwd_Dbus *
iwd_dbus_new(Eldbus_Connection *conn, const Iwd_Dbus_Callbacks *cbs, void *data)
{
   Iwd_Dbus *d = calloc(1, sizeof(*d));
   if (!d) return NULL;
   d->conn = conn;
   if (cbs) d->cbs = *cbs;
   d->data = data;

   eldbus_name_owner_changed_callback_add(conn, IWD_BUS_NAME,
                                          _on_name_owner, d, EINA_TRUE);
   return d;
}

void
iwd_dbus_free(Iwd_Dbus *d)
{
   if (!d) return;
   eldbus_name_owner_changed_callback_del(d->conn, IWD_BUS_NAME, _on_name_owner, d);
   _unbind_root(d);
   free(d);
}
