#ifndef IWD_AGENT_H
#define IWD_AGENT_H

#include <Eldbus.h>

typedef struct _Iwd_Agent         Iwd_Agent;
typedef struct _Iwd_Agent_Request Iwd_Agent_Request;

/* The UI registers a single handler that is called whenever iwd asks for
 * a passphrase. The handler must eventually call iwd_agent_reply() or
 * iwd_agent_cancel() with the request token. */
typedef void (*Iwd_Agent_Passphrase_Cb)(void *data,
                                        Iwd_Agent_Request *req,
                                        const char *network_path);

Iwd_Agent *iwd_agent_new (Eldbus_Connection *conn,
                          Iwd_Agent_Passphrase_Cb cb, void *data);
void       iwd_agent_free(Iwd_Agent *a);

void iwd_agent_reply (Iwd_Agent_Request *req, const char *passphrase);
void iwd_agent_cancel(Iwd_Agent_Request *req);

#endif
