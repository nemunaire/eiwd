#ifndef IWD_NETWORK_H
#define IWD_NETWORK_H

#include <Eina.h>
#include <Eldbus.h>

/* Network structure */
typedef struct _IWD_Network
{
   const char *path;           /* D-Bus object path */
   const char *name;           /* SSID (decoded) */
   const char *type;           /* "open", "psk", "8021x" */
   Eina_Bool known;            /* Is this a known network? */
   int16_t signal_strength;    /* Signal strength in dBm */

   /* Known network properties */
   Eina_Bool auto_connect;
   const char *last_connected_time;

   /* D-Bus objects */
   Eldbus_Proxy *network_proxy;
   Eldbus_Signal_Handler *properties_changed;
} IWD_Network;

/* Global network list */
extern Eina_List *iwd_networks;

/* Network management */
IWD_Network *iwd_network_new(const char *path);
void iwd_network_free(IWD_Network *net);
IWD_Network *iwd_network_find(const char *path);

/* Network operations */
void iwd_network_connect(IWD_Network *net);
void iwd_network_forget(IWD_Network *net);

/* Get networks list */
Eina_List *iwd_networks_get(void);

/* Initialize network subsystem */
void iwd_network_init(void);
void iwd_network_shutdown(void);

#endif
