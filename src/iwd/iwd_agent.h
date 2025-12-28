#ifndef IWD_AGENT_H
#define IWD_AGENT_H

#include <Eina.h>
#include <Eldbus.h>

#define IWD_AGENT_PATH "/org/enlightenment/eiwd/agent"

/* Agent structure */
typedef struct _IWD_Agent
{
   Eldbus_Service_Interface *iface;
   const char *pending_network_path;
   const char *pending_passphrase;
} IWD_Agent;

/* Global agent */
extern IWD_Agent *iwd_agent;

/* Agent management */
Eina_Bool iwd_agent_init(void);
void iwd_agent_shutdown(void);

/* Set passphrase for pending request */
void iwd_agent_set_passphrase(const char *passphrase);

/* Cancel pending request */
void iwd_agent_cancel(void);

#endif
