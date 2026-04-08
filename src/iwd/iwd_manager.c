#include "iwd_manager.h"
#include "iwd_dbus.h"

struct _Iwd_Manager
{
   Iwd_Dbus  *dbus;
   Eina_List *devices;    /* Iwd_Device * */
   Eina_List *listeners;
   Iwd_State  state;
};

/* TODO:
 *  - Build the device/station/network tree from iwd_dbus events
 *  - Aggregate state from active station
 *  - Notify listeners on any change
 */

Iwd_Manager *
iwd_manager_new(Eldbus_Connection *conn)
{
   Iwd_Manager *m = calloc(1, sizeof(*m));
   m->dbus = iwd_dbus_new(conn);
   m->state = IWD_STATE_OFF;
   return m;
}

void
iwd_manager_free(Iwd_Manager *m)
{
   if (!m) return;
   iwd_dbus_free(m->dbus);
   eina_list_free(m->devices);
   eina_list_free(m->listeners);
   free(m);
}

Iwd_State iwd_manager_state(const Iwd_Manager *m) { return m ? m->state : IWD_STATE_OFF; }
const Eina_List *iwd_manager_devices(const Iwd_Manager *m) { return m ? m->devices : NULL; }

void iwd_manager_scan_request(Iwd_Manager *m EINA_UNUSED) { /* TODO */ }
void iwd_manager_set_powered (Iwd_Manager *m EINA_UNUSED, Eina_Bool on EINA_UNUSED) { /* TODO */ }

void iwd_manager_listener_add(Iwd_Manager *m EINA_UNUSED, Iwd_Manager_Cb cb EINA_UNUSED, void *data EINA_UNUSED) { /* TODO */ }
void iwd_manager_listener_del(Iwd_Manager *m EINA_UNUSED, Iwd_Manager_Cb cb EINA_UNUSED, void *data EINA_UNUSED) { /* TODO */ }
