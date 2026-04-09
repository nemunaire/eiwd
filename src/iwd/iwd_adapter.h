#ifndef IWD_ADAPTER_H
#define IWD_ADAPTER_H

#include <Eina.h>
#include <Eldbus.h>

typedef struct _Iwd_Adapter
{
   char          *path;
   Eina_Bool      powered;
   Eldbus_Object *obj;
   Eldbus_Proxy  *proxy;
   Eldbus_Proxy  *_props_proxy_keepalive;
   Eldbus_Signal_Handler *sh_props;
   void          *manager;
} Iwd_Adapter;

Iwd_Adapter *iwd_adapter_new (Eldbus_Connection *conn, const char *path, void *manager);
void         iwd_adapter_free(Iwd_Adapter *a);

void iwd_adapter_apply_props(Iwd_Adapter *a, Eldbus_Message_Iter *props);
void iwd_adapter_set_powered(Iwd_Adapter *a, Eina_Bool on);

#endif
