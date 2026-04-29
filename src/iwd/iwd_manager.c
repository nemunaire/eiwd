#include "iwd_manager.h"
#include "iwd_dbus.h"
#include "iwd_adapter.h"
#include "iwd_device.h"
#include "iwd_network.h"
#include <Ecore.h>
#include <stdlib.h>
#include <string.h>

typedef struct _Listener
{
   Iwd_Manager_Cb cb;
   void          *data;
} Listener;

struct _Iwd_Manager
{
   Iwd_Dbus    *dbus;
   Iwd_Agent   *agent;
   Eina_Hash   *adapters;  /* path → Iwd_Adapter * */
   Eina_Hash   *devices;   /* path → Iwd_Device * */
   Eina_Hash   *networks;  /* path → Iwd_Network * */
   Eina_List   *listeners; /* Listener * */
   Iwd_State    state;
   Ecore_Job   *notify_job;

   Iwd_Agent_Passphrase_Cb pass_cb;
   void                   *pass_data;
};

static void
_passphrase_trampoline(void *data, Iwd_Agent_Request *req, const char *path)
{
   Iwd_Manager *m = data;
   if (m->pass_cb) m->pass_cb(m->pass_data, req, path);
   else iwd_agent_cancel(req);
}

void
iwd_manager_set_passphrase_handler(Iwd_Manager *m, Iwd_Agent_Passphrase_Cb cb, void *data)
{
   if (!m) return;
   m->pass_cb = cb;
   m->pass_data = data;
}

void
iwd_manager_set_cancel_handler(Iwd_Manager *m, Iwd_Agent_Cancel_Cb cb, void *data)
{
   if (!m) return;
   iwd_agent_set_cancel_cb(m->agent, cb, data);
}

static void _recompute_state(Iwd_Manager *m);

/* ----- listeners ------------------------------------------------------- */

void
iwd_manager_listener_add(Iwd_Manager *m, Iwd_Manager_Cb cb, void *data)
{
   if (!m || !cb) return;
   Listener *l = calloc(1, sizeof(*l));
   l->cb = cb; l->data = data;
   m->listeners = eina_list_append(m->listeners, l);
}

void
iwd_manager_listener_del(Iwd_Manager *m, Iwd_Manager_Cb cb, void *data)
{
   if (!m) return;
   Eina_List *l;
   Listener *li;
   EINA_LIST_FOREACH(m->listeners, l, li)
     if (li->cb == cb && li->data == data)
       {
          m->listeners = eina_list_remove_list(m->listeners, l);
          free(li);
          return;
       }
}

/* Coalesce bursts of D-Bus PropertiesChanged into a single UI refresh per
 * main-loop tick. Without this, a busy area (30+ visible networks, scan in
 * progress) can fire hundreds of notifies per second, each rebuilding the
 * popup list synchronously and freezing the whole compositor. */
static void
_notify_job_cb(void *data)
{
   Iwd_Manager *m = data;
   m->notify_job = NULL;
   _recompute_state(m);
   Eina_List *l;
   Listener *li;
   EINA_LIST_FOREACH(m->listeners, l, li)
     li->cb(li->data, m);
}

void
iwd_manager_notify(Iwd_Manager *m)
{
   if (!m || m->notify_job) return;
   m->notify_job = ecore_job_add(_notify_job_cb, m);
}

/* ----- state aggregation ---------------------------------------------- */

static void
_recompute_state(Iwd_Manager *m)
{
   Iwd_State s = IWD_STATE_OFF;
   Eina_Bool any_powered = EINA_FALSE;

   /* Adapter.Powered is the source of truth for radio state — Device.Powered
    * is a no-op on modern iwd, so don't let it lie to us. If we have no
    * tracked adapter at all, fall back to "any device exists". */
   if (eina_hash_population(m->adapters) > 0)
     {
        Eina_Iterator *ait = eina_hash_iterator_data_new(m->adapters);
        Iwd_Adapter *ap;
        EINA_ITERATOR_FOREACH(ait, ap)
          if (ap->powered) any_powered = EINA_TRUE;
        eina_iterator_free(ait);
     }
   else
     any_powered = (eina_hash_population(m->devices) > 0);

   Eina_Iterator *it = eina_hash_iterator_data_new(m->devices);
   Iwd_Device *d;
   EINA_ITERATOR_FOREACH(it, d)
     {
        if (!any_powered) continue;     /* radio is down: ignore stale station */
        if (!d->has_station) continue;
        if (d->scanning && s < IWD_STATE_SCANNING)              s = IWD_STATE_SCANNING;
        if (d->station_state == IWD_STATION_CONNECTING && s < IWD_STATE_CONNECTING) s = IWD_STATE_CONNECTING;
        if (d->station_state == IWD_STATION_CONNECTED  && s < IWD_STATE_CONNECTED)  s = IWD_STATE_CONNECTED;
     }
   eina_iterator_free(it);
   if (s == IWD_STATE_OFF && any_powered) s = IWD_STATE_IDLE;
   m->state = s;
}

/* ----- D-Bus callbacks ------------------------------------------------- */

static void
_on_iface_added(void *data, const char *path, const char *iface, Eldbus_Message_Iter *props)
{
   Iwd_Manager *m = data;
   Eldbus_Connection *conn = iwd_dbus_conn(m->dbus);

   if (!strcmp(iface, IWD_IFACE_ADAPTER))
     {
        Iwd_Adapter *a = eina_hash_find(m->adapters, path);
        if (!a)
          {
             a = iwd_adapter_new(conn, path, m);
             if (a) eina_hash_add(m->adapters, path, a);
          }
        if (a) iwd_adapter_apply_props(a, props);
     }
   else if (!strcmp(iface, IWD_IFACE_DEVICE))
     {
        Iwd_Device *d = eina_hash_find(m->devices, path);
        if (!d)
          {
             d = iwd_device_new(conn, path, m);
             if (d) eina_hash_add(m->devices, path, d);
          }
        if (d) iwd_device_apply_device_props(d, props);
     }
   else if (!strcmp(iface, IWD_IFACE_STATION))
     {
        Iwd_Device *d = eina_hash_find(m->devices, path);
        if (!d)
          {
             d = iwd_device_new(conn, path, m);
             if (d) eina_hash_add(m->devices, path, d);
          }
        if (d)
          {
             iwd_device_attach_station(d);
             iwd_device_apply_station_props(d, props);
          }
     }
   else if (!strcmp(iface, IWD_IFACE_NETWORK))
     {
        Iwd_Network *n = eina_hash_find(m->networks, path);
        if (!n)
          {
             n = iwd_network_new(conn, path, m);
             if (n) eina_hash_add(m->networks, path, n);
          }
        if (n) iwd_network_apply_props(n, props);
     }
   /* Adapter / KnownNetwork: TODO (not needed for first connect path) */

   iwd_manager_notify(m);
}

static void
_on_iface_removed(void *data, const char *path, const char *iface)
{
   Iwd_Manager *m = data;

   if (!strcmp(iface, IWD_IFACE_STATION))
     {
        Iwd_Device *d = eina_hash_find(m->devices, path);
        if (d) iwd_device_detach_station(d);
     }
   else if (!strcmp(iface, IWD_IFACE_DEVICE))
     {
        eina_hash_del(m->devices, path, NULL);
     }
   else if (!strcmp(iface, IWD_IFACE_NETWORK))
     {
        eina_hash_del(m->networks, path, NULL);
     }
   else if (!strcmp(iface, IWD_IFACE_ADAPTER))
     {
        eina_hash_del(m->adapters, path, NULL);
     }
   iwd_manager_notify(m);
}

static void
_on_name_appeared(void *data EINA_UNUSED) { /* GetManagedObjects will populate */ }

static void
_on_name_vanished(void *data)
{
   Iwd_Manager *m = data;
   eina_hash_free_buckets(m->adapters);
   eina_hash_free_buckets(m->devices);
   eina_hash_free_buckets(m->networks);
   m->state = IWD_STATE_OFF;
   iwd_manager_notify(m);
}

/* ----- lifecycle ------------------------------------------------------- */

static void _adapter_free_cb(void *d) { iwd_adapter_free(d); }
static void _device_free_cb (void *d) { iwd_device_free(d); }
static void _network_free_cb(void *d) { iwd_network_free(d); }

Iwd_Manager *
iwd_manager_new(Eldbus_Connection *conn)
{
   Iwd_Manager *m = calloc(1, sizeof(*m));
   if (!m) return NULL;
   m->adapters = eina_hash_string_superfast_new(_adapter_free_cb);
   m->devices  = eina_hash_string_superfast_new(_device_free_cb);
   m->networks = eina_hash_string_superfast_new(_network_free_cb);
   m->state    = IWD_STATE_OFF;

   Iwd_Dbus_Callbacks cbs = {
      .name_appeared = _on_name_appeared,
      .name_vanished = _on_name_vanished,
      .iface_added   = _on_iface_added,
      .iface_removed = _on_iface_removed,
   };
   m->dbus  = iwd_dbus_new(conn, &cbs, m);
   m->agent = iwd_agent_new(conn, _passphrase_trampoline, m);
   return m;
}

void
iwd_manager_free(Iwd_Manager *m)
{
   if (!m) return;
   if (m->notify_job) ecore_job_del(m->notify_job);
   iwd_agent_free(m->agent);
   iwd_dbus_free(m->dbus);
   eina_hash_free(m->adapters);
   eina_hash_free(m->devices);
   eina_hash_free(m->networks);
   Listener *li;
   EINA_LIST_FREE(m->listeners, li) free(li);
   free(m);
}

Iwd_State        iwd_manager_state    (const Iwd_Manager *m) { return m ? m->state : IWD_STATE_OFF; }
const Eina_Hash *iwd_manager_devices  (const Iwd_Manager *m) { return m ? m->devices  : NULL; }
const Eina_Hash *iwd_manager_networks (const Iwd_Manager *m) { return m ? m->networks : NULL; }

void
iwd_manager_scan_request(Iwd_Manager *m)
{
   if (!m) return;
   Eina_Iterator *it = eina_hash_iterator_data_new(m->devices);
   Iwd_Device *d;
   EINA_ITERATOR_FOREACH(it, d) iwd_device_scan(d);
   eina_iterator_free(it);
}

void
iwd_manager_set_powered(Iwd_Manager *m, Eina_Bool on)
{
   if (!m) return;

   if (!on)
     {
        Eina_Iterator *dit = eina_hash_iterator_data_new(m->devices);
        Iwd_Device *d;
        EINA_ITERATOR_FOREACH(dit, d) iwd_device_disconnect(d);
        eina_iterator_free(dit);
     }

   Eina_Iterator *it = eina_hash_iterator_data_new(m->adapters);
   Iwd_Adapter *a;
   EINA_ITERATOR_FOREACH(it, a) iwd_adapter_set_powered(a, on);
   eina_iterator_free(it);
}
