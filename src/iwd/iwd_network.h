#ifndef IWD_NETWORK_H
#define IWD_NETWORK_H

#include <Eina.h>
#include <Eldbus.h>

typedef enum {
   IWD_SEC_OPEN,
   IWD_SEC_PSK,
   IWD_SEC_8021X,
   IWD_SEC_WEP,
   IWD_SEC_UNKNOWN,
} Iwd_Security;

typedef struct _Iwd_Network Iwd_Network;

struct _Iwd_Network
{
   char         *path;
   char         *ssid;
   char         *device_path;
   char         *known_path;
   Iwd_Security  security;
   Eina_Bool     connected;

   Eldbus_Object *obj;
   Eldbus_Proxy  *proxy;
   Eldbus_Signal_Handler *sh_props;

   void          *manager;
};

Iwd_Network *iwd_network_new (Eldbus_Connection *conn, const char *path, void *manager);
void         iwd_network_free(Iwd_Network *n);

void iwd_network_apply_props(Iwd_Network *n, Eldbus_Message_Iter *props);

void iwd_network_connect (Iwd_Network *n);
/* Forget acts on the KnownNetwork object referenced by this network. */
void iwd_network_forget  (Iwd_Network *n);

#endif
