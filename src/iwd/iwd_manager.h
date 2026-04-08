#ifndef IWD_MANAGER_H
#define IWD_MANAGER_H

#include <Eldbus.h>
#include <Eina.h>

typedef enum {
   IWD_STATE_OFF,
   IWD_STATE_IDLE,
   IWD_STATE_SCANNING,
   IWD_STATE_CONNECTING,
   IWD_STATE_CONNECTED,
   IWD_STATE_ERROR,
} Iwd_State;

typedef struct _Iwd_Manager Iwd_Manager;

Iwd_Manager *iwd_manager_new (Eldbus_Connection *conn);
void         iwd_manager_free(Iwd_Manager *m);

Iwd_State    iwd_manager_state    (const Iwd_Manager *m);
const Eina_List *iwd_manager_devices (const Iwd_Manager *m);

void         iwd_manager_scan_request    (Iwd_Manager *m);
void         iwd_manager_set_powered     (Iwd_Manager *m, Eina_Bool on);

/* Event callback signature for UI layer */
typedef void (*Iwd_Manager_Cb)(void *data, Iwd_Manager *m);
void iwd_manager_listener_add (Iwd_Manager *m, Iwd_Manager_Cb cb, void *data);
void iwd_manager_listener_del (Iwd_Manager *m, Iwd_Manager_Cb cb, void *data);

#endif
