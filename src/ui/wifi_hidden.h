#ifndef WIFI_HIDDEN_H
#define WIFI_HIDDEN_H

#include <Eina.h>

/* Forward declarations */
typedef struct _Instance Instance;

/* Show hidden network connection dialog */
void wifi_hidden_dialog_show(Instance *inst);

/* Cancel/close hidden network dialog */
void wifi_hidden_dialog_cancel(void);

#endif
