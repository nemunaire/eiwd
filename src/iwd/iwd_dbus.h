#ifndef IWD_DBUS_H
#define IWD_DBUS_H

#include <Eldbus.h>
#include <Eina.h>

#define IWD_BUS_NAME            "net.connman.iwd"
#define IWD_IFACE_ADAPTER       "net.connman.iwd.Adapter"
#define IWD_IFACE_DEVICE        "net.connman.iwd.Device"
#define IWD_IFACE_STATION       "net.connman.iwd.Station"
#define IWD_IFACE_NETWORK       "net.connman.iwd.Network"
#define IWD_IFACE_KNOWN_NETWORK "net.connman.iwd.KnownNetwork"
#define IWD_IFACE_AGENT_MANAGER "net.connman.iwd.AgentManager"
#define IWD_IFACE_AGENT         "net.connman.iwd.Agent"

typedef struct _Iwd_Dbus Iwd_Dbus;

/* Callbacks the consumer (iwd_manager) registers to react to bus state. */
typedef struct _Iwd_Dbus_Callbacks
{
   void (*name_appeared) (void *data);
   void (*name_vanished) (void *data);
   /* iface_props is an iter positioned on a{sv} for the given interface. */
   void (*iface_added)   (void *data, const char *path, const char *iface,
                          Eldbus_Message_Iter *iface_props);
   void (*iface_removed) (void *data, const char *path, const char *iface);
} Iwd_Dbus_Callbacks;

Iwd_Dbus *iwd_dbus_new (Eldbus_Connection *conn,
                        const Iwd_Dbus_Callbacks *cbs, void *data);
void      iwd_dbus_free(Iwd_Dbus *d);

Eldbus_Connection *iwd_dbus_conn(const Iwd_Dbus *d);

#endif
