#ifndef IWD_NETWORK_H
#define IWD_NETWORK_H

#include <Eina.h>
#include <Eldbus.h>

typedef enum {
   IWD_SEC_OPEN,
   IWD_SEC_PSK,
   IWD_SEC_8021X,
   IWD_SEC_WEP,
} Iwd_Security;

typedef struct _Iwd_Network Iwd_Network;

struct _Iwd_Network
{
   char         *path;
   char         *ssid;
   Iwd_Security  security;
   int           signal;        /* 0..100 */
   Eina_Bool     known;
   Eina_Bool     connected;
};

Iwd_Network *iwd_network_new (Eldbus_Connection *conn, const char *path);
void         iwd_network_free(Iwd_Network *n);

void iwd_network_connect (Iwd_Network *n);
void iwd_network_forget  (Iwd_Network *n);

#endif
