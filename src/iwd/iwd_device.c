#include "iwd_device.h"

Iwd_Device *
iwd_device_new(Eldbus_Connection *conn EINA_UNUSED, const char *path)
{
   Iwd_Device *d = calloc(1, sizeof(*d));
   if (path) d->path = strdup(path);
   /* TODO: build proxies for Device + Station; subscribe to PropertiesChanged */
   return d;
}

void
iwd_device_free(Iwd_Device *d)
{
   if (!d) return;
   free(d->path);
   free(d->name);
   free(d->address);
   free(d);
}

void
iwd_device_set_powered(Iwd_Device *d EINA_UNUSED, Eina_Bool on EINA_UNUSED)
{
   /* TODO: Set("Powered") on Device interface */
}
