#ifndef IWD_DEVICE_H
#define IWD_DEVICE_H

#include <Eina.h>
#include <Eldbus.h>

typedef struct _Iwd_Device Iwd_Device;

typedef enum {
   IWD_STATION_DISCONNECTED,
   IWD_STATION_CONNECTING,
   IWD_STATION_CONNECTED,
   IWD_STATION_DISCONNECTING,
   IWD_STATION_ROAMING,
} Iwd_Station_State;

struct _Iwd_Device
{
   char              *path;
   char              *name;
   char              *address;
   char              *adapter_path;
   Eina_Bool          powered;

   /* Station-side state, valid when station_proxy != NULL */
   Eina_Bool          has_station;
   Iwd_Station_State  station_state;
   Eina_Bool          scanning;
   char              *connected_network;

   Eldbus_Object     *obj;
   Eldbus_Proxy      *device_proxy;
   Eldbus_Proxy      *station_proxy;
   Eldbus_Object     *adapter_obj;
   Eldbus_Proxy      *adapter_proxy;
   Eldbus_Signal_Handler *sh_dev_props;
   Eldbus_Signal_Handler *sh_sta_props;

   void              *manager;   /* back-ref, opaque (Iwd_Manager *) */
};

Iwd_Device *iwd_device_new (Eldbus_Connection *conn, const char *path, void *manager);
void        iwd_device_free(Iwd_Device *d);

void iwd_device_apply_device_props (Iwd_Device *d, Eldbus_Message_Iter *props);
void iwd_device_apply_station_props(Iwd_Device *d, Eldbus_Message_Iter *props);
void iwd_device_attach_station     (Iwd_Device *d);
void iwd_device_detach_station     (Iwd_Device *d);

void iwd_device_set_powered(Iwd_Device *d, Eina_Bool on);
void iwd_device_scan       (Iwd_Device *d);
void iwd_device_disconnect (Iwd_Device *d);

#endif
