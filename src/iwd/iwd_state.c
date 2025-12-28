#include "iwd_state.h"
#include "../e_mod_main.h"

/* Global state */
static IWD_State current_state = IWD_STATE_OFF;
static Eina_List *state_change_callbacks = NULL;

/* State change callback structure */
typedef struct _State_Callback
{
   IWD_State_Changed_Cb cb;
   void *data;
} State_Callback;

/* Initialize state subsystem */
void
iwd_state_init(void)
{
   DBG("Initializing state subsystem");
   current_state = IWD_STATE_OFF;
}

/* Shutdown state subsystem */
void
iwd_state_shutdown(void)
{
   State_Callback *scb;

   DBG("Shutting down state subsystem");

   EINA_LIST_FREE(state_change_callbacks, scb)
      E_FREE(scb);
}

/* Get current state */
IWD_State
iwd_state_get(void)
{
   return current_state;
}

/* Set state and notify callbacks */
void
iwd_state_set(IWD_State state)
{
   IWD_State old_state;
   Eina_List *l;
   State_Callback *scb;

   if (current_state == state) return;

   old_state = current_state;
   current_state = state;

   DBG("State changed: %d -> %d", old_state, state);

   /* Notify callbacks */
   EINA_LIST_FOREACH(state_change_callbacks, l, scb)
   {
      if (scb->cb)
         scb->cb(scb->data, old_state, state);
   }
}

/* Add state change callback */
void
iwd_state_callback_add(IWD_State_Changed_Cb cb, void *data)
{
   State_Callback *scb;

   if (!cb) return;

   scb = E_NEW(State_Callback, 1);
   if (!scb) return;

   scb->cb = cb;
   scb->data = data;

   state_change_callbacks = eina_list_append(state_change_callbacks, scb);
}

/* Remove state change callback */
void
iwd_state_callback_del(IWD_State_Changed_Cb cb, void *data)
{
   Eina_List *l, *l_next;
   State_Callback *scb;

   EINA_LIST_FOREACH_SAFE(state_change_callbacks, l, l_next, scb)
   {
      if (scb->cb == cb && scb->data == data)
      {
         state_change_callbacks = eina_list_remove_list(state_change_callbacks, l);
         E_FREE(scb);
         return;
      }
   }
}

/* Update state from device */
void
iwd_state_update_from_device(IWD_Device *dev)
{
   if (!dev)
   {
      iwd_state_set(IWD_STATE_ERROR);
      return;
   }

   if (!dev->powered)
   {
      iwd_state_set(IWD_STATE_OFF);
      return;
   }

   if (dev->scanning)
   {
      iwd_state_set(IWD_STATE_SCANNING);
      return;
   }

   if (dev->state)
   {
      if (strcmp(dev->state, "connected") == 0)
         iwd_state_set(IWD_STATE_CONNECTED);
      else if (strcmp(dev->state, "connecting") == 0)
         iwd_state_set(IWD_STATE_CONNECTING);
      else if (strcmp(dev->state, "disconnecting") == 0)
         iwd_state_set(IWD_STATE_IDLE);
      else
         iwd_state_set(IWD_STATE_IDLE);
   }
   else
   {
      iwd_state_set(IWD_STATE_IDLE);
   }
}

/* Get state name */
const char *
iwd_state_name_get(IWD_State state)
{
   switch (state)
   {
      case IWD_STATE_OFF: return "OFF";
      case IWD_STATE_IDLE: return "IDLE";
      case IWD_STATE_SCANNING: return "SCANNING";
      case IWD_STATE_CONNECTING: return "CONNECTING";
      case IWD_STATE_CONNECTED: return "CONNECTED";
      case IWD_STATE_ERROR: return "ERROR";
      default: return "UNKNOWN";
   }
}
