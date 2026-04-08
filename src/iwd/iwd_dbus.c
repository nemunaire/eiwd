#include "iwd_dbus.h"
#include <Eina.h>

struct _Iwd_Dbus
{
   Eldbus_Connection      *conn;
   Eldbus_Object          *root;
   Eldbus_Proxy           *object_manager;
   Eldbus_Signal_Handler  *name_owner_sh;
};

/* TODO:
 *  - watch IWD_BUS_NAME owner (NameOwnerChanged) for restart handling
 *  - call org.freedesktop.DBus.ObjectManager.GetManagedObjects on "/"
 *  - listen for InterfacesAdded / InterfacesRemoved
 *  - dispatch new objects to iwd_manager
 */

Iwd_Dbus *
iwd_dbus_new(Eldbus_Connection *conn)
{
   Iwd_Dbus *d = calloc(1, sizeof(*d));
   d->conn = conn;
   return d;
}

void
iwd_dbus_free(Iwd_Dbus *d)
{
   if (!d) return;
   free(d);
}
