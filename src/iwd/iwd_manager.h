#ifndef IWD_MANAGER_H
#define IWD_MANAGER_H

#include <Eldbus.h>
#include <Eina.h>
#include "iwd_agent.h"

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

/* Hash<path, Iwd_Device*> */
const Eina_Hash *iwd_manager_devices (const Iwd_Manager *m);
/* Hash<path, Iwd_Network*> */
const Eina_Hash *iwd_manager_networks(const Iwd_Manager *m);

void         iwd_manager_scan_request    (Iwd_Manager *m);
void         iwd_manager_set_powered     (Iwd_Manager *m, Eina_Bool on);

typedef void (*Iwd_Manager_Cb)(void *data, Iwd_Manager *m);
void iwd_manager_listener_add (Iwd_Manager *m, Iwd_Manager_Cb cb, void *data);
void iwd_manager_listener_del (Iwd_Manager *m, Iwd_Manager_Cb cb, void *data);

/* Internal: invoked by sub-objects when their state changes. */
void iwd_manager_notify (Iwd_Manager *m);

/* The UI installs its passphrase prompt here. The handler must
 * eventually call iwd_agent_reply()/iwd_agent_cancel() with the request. */
void iwd_manager_set_passphrase_handler(Iwd_Manager *m,
                                        Iwd_Agent_Passphrase_Cb cb,
                                        void *data);

/* Notified when iwd issues Agent.Cancel — UI should close any open prompt. */
void iwd_manager_set_cancel_handler    (Iwd_Manager *m,
                                        Iwd_Agent_Cancel_Cb cb,
                                        void *data);

#endif
