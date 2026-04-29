#ifndef IWD_LABELS_H
#define IWD_LABELS_H

#include "iwd_manager.h"
#include "iwd_network.h"

/* Short, user-facing labels for state and security enums. The pointers
 * returned are static literals — do not free. */
const char *iwd_state_label   (Iwd_State s);
const char *iwd_security_label(Iwd_Security s);

#endif
