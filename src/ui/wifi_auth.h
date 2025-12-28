#ifndef WIFI_AUTH_H
#define WIFI_AUTH_H

#include <Eina.h>

/* Forward declarations */
typedef struct _Instance Instance;
typedef struct _IWD_Network IWD_Network;

/* Show authentication dialog for a network */
void wifi_auth_dialog_show(Instance *inst, IWD_Network *net);

/* Cancel/close authentication dialog */
void wifi_auth_dialog_cancel(void);

#endif
