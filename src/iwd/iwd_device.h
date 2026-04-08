#ifndef IWD_DEVICE_H
#define IWD_DEVICE_H

#include <Eina.h>
#include <Eldbus.h>

typedef struct _Iwd_Device Iwd_Device;

struct _Iwd_Device
{
   char      *path;        /* D-Bus object path */
   char      *name;
   char      *address;
   Eina_Bool  powered;
   Eldbus_Proxy *station;  /* may be NULL */
};

Iwd_Device *iwd_device_new (Eldbus_Connection *conn, const char *path);
void        iwd_device_free(Iwd_Device *d);
void        iwd_device_set_powered(Iwd_Device *d, Eina_Bool on);

#endif
