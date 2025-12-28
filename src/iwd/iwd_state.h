#ifndef IWD_STATE_H
#define IWD_STATE_H

#include <Eina.h>

/* Forward declaration */
typedef struct _IWD_Device IWD_Device;

/* Connection states */
typedef enum
{
   IWD_STATE_OFF,        /* Powered = false */
   IWD_STATE_IDLE,       /* Powered = true, disconnected */
   IWD_STATE_SCANNING,   /* Scanning in progress */
   IWD_STATE_CONNECTING, /* Connecting to network */
   IWD_STATE_CONNECTED,  /* Connected to network */
   IWD_STATE_ERROR       /* iwd not running or error */
} IWD_State;

/* State change callback */
typedef void (*IWD_State_Changed_Cb)(void *data, IWD_State old_state, IWD_State new_state);

/* Initialize/shutdown */
void iwd_state_init(void);
void iwd_state_shutdown(void);

/* Get/set state */
IWD_State iwd_state_get(void);
void iwd_state_set(IWD_State state);

/* State callbacks */
void iwd_state_callback_add(IWD_State_Changed_Cb cb, void *data);
void iwd_state_callback_del(IWD_State_Changed_Cb cb, void *data);

/* Update state from device */
void iwd_state_update_from_device(IWD_Device *dev);

/* Get state name string */
const char *iwd_state_name_get(IWD_State state);

#endif
