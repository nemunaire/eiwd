#ifndef IWD_NETWORK_H
#define IWD_NETWORK_H

#include <stdint.h>
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

   /* Signal strength in 100*dBm units (iwd convention). Valid iff have_signal. */
   int16_t       signal_dbm;
   Eina_Bool     have_signal;

   Eldbus_Object *obj;
   Eldbus_Proxy  *proxy;
   Eldbus_Signal_Handler *sh_props;

   void          *manager;
};

Iwd_Network *iwd_network_new (Eldbus_Connection *conn, const char *path, void *manager);
void         iwd_network_free(Iwd_Network *n);

void iwd_network_apply_props(Iwd_Network *n, Eldbus_Message_Iter *props);

/* 0 = unknown/no signal, 1..4 = weak..excellent. */
int  iwd_network_signal_tier(const Iwd_Network *n);

void iwd_network_connect (Iwd_Network *n);
/* Forget acts on the KnownNetwork object referenced by this network. */
void iwd_network_forget  (Iwd_Network *n);

#endif
