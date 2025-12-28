#ifndef IWD_DEVICE_H
#define IWD_DEVICE_H

#include <Eina.h>
#include <Eldbus.h>

/* Device structure */
typedef struct _IWD_Device
{
   const char *path;           /* D-Bus object path */
   const char *name;           /* Interface name (e.g., "wlan0") */
   const char *address;        /* MAC address */
   const char *adapter_path;   /* Adapter object path */
   const char *mode;           /* "station", "ap", "ad-hoc" */
   Eina_Bool powered;          /* Device powered state */

   /* Station interface properties */
   Eina_Bool scanning;
   const char *state;          /* "disconnected", "connecting", "connected", "disconnecting" */
   const char *connected_network; /* Connected network object path */

   /* D-Bus objects */
   Eldbus_Proxy *device_proxy;
   Eldbus_Proxy *station_proxy;
   Eldbus_Signal_Handler *properties_changed;
} IWD_Device;

/* Global device list */
extern Eina_List *iwd_devices;

/* Device management */
IWD_Device *iwd_device_new(const char *path);
void iwd_device_free(IWD_Device *dev);
IWD_Device *iwd_device_find(const char *path);

/* Device operations */
void iwd_device_scan(IWD_Device *dev);
void iwd_device_disconnect(IWD_Device *dev);
void iwd_device_connect_hidden(IWD_Device *dev, const char *ssid);

/* Get devices list */
Eina_List *iwd_devices_get(void);

/* Initialize device subsystem */
void iwd_device_init(void);
void iwd_device_shutdown(void);

#endif
