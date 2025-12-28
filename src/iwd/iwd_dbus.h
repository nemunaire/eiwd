#ifndef IWD_DBUS_H
#define IWD_DBUS_H

#include <Eina.h>
#include <Eldbus.h>

/* iwd D-Bus service and interfaces */
#define IWD_SERVICE "net.connman.iwd"
#define IWD_MANAGER_PATH "/"
#define IWD_MANAGER_INTERFACE "net.connman.iwd.Manager"
#define IWD_ADAPTER_INTERFACE "net.connman.iwd.Adapter"
#define IWD_DEVICE_INTERFACE "net.connman.iwd.Device"
#define IWD_STATION_INTERFACE "net.connman.iwd.Station"
#define IWD_NETWORK_INTERFACE "net.connman.iwd.Network"
#define IWD_KNOWN_NETWORK_INTERFACE "net.connman.iwd.KnownNetwork"
#define IWD_AGENT_MANAGER_INTERFACE "net.connman.iwd.AgentManager"

#define DBUS_OBJECT_MANAGER_INTERFACE "org.freedesktop.DBus.ObjectManager"
#define DBUS_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"

/* D-Bus context */
typedef struct _IWD_DBus
{
   Eldbus_Connection *conn;
   Eldbus_Object *manager_obj;
   Eldbus_Proxy *manager_proxy;
   Eldbus_Signal_Handler *interfaces_added;
   Eldbus_Signal_Handler *interfaces_removed;

   Eina_Bool connected;
} IWD_DBus;

/* Global D-Bus context */
extern IWD_DBus *iwd_dbus;

/* Initialization and shutdown */
Eina_Bool iwd_dbus_init(void);
void iwd_dbus_shutdown(void);

/* Connection state */
Eina_Bool iwd_dbus_is_connected(void);

/* Helper to get D-Bus connection */
Eldbus_Connection *iwd_dbus_conn_get(void);

/* Get managed objects */
void iwd_dbus_get_managed_objects(void);

#endif
