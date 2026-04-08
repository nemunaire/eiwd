#ifndef IWD_DBUS_H
#define IWD_DBUS_H

#include <Eldbus.h>

#define IWD_BUS_NAME            "net.connman.iwd"
#define IWD_IFACE_ADAPTER       "net.connman.iwd.Adapter"
#define IWD_IFACE_DEVICE        "net.connman.iwd.Device"
#define IWD_IFACE_STATION       "net.connman.iwd.Station"
#define IWD_IFACE_NETWORK       "net.connman.iwd.Network"
#define IWD_IFACE_KNOWN_NETWORK "net.connman.iwd.KnownNetwork"
#define IWD_IFACE_AGENT_MANAGER "net.connman.iwd.AgentManager"
#define IWD_IFACE_AGENT         "net.connman.iwd.Agent"

typedef struct _Iwd_Dbus Iwd_Dbus;

Iwd_Dbus *iwd_dbus_new (Eldbus_Connection *conn);
void      iwd_dbus_free(Iwd_Dbus *d);

#endif
