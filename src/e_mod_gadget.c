#include "e_mod_main.h"

/* Forward declarations */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(const E_Gadcon_Client_Class *client_class);

static void _gadget_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _gadget_update(Instance *inst);
static Eina_Bool _gadget_update_timer_cb(void *data);

/* Gadcon class definition */
static const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION,
   "iwd",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* Global gadcon provider */
static E_Gadcon_Client_Class *gadcon_class = NULL;

/* Initialize gadget subsystem */
void
e_iwd_gadget_init(void)
{
   DBG("Initializing gadget");

   gadcon_class = (E_Gadcon_Client_Class *)&_gc_class;
   e_gadcon_provider_register(gadcon_class);
}

/* Shutdown gadget subsystem */
void
e_iwd_gadget_shutdown(void)
{
   DBG("Shutting down gadget");

   if (gadcon_class)
   {
      e_gadcon_provider_unregister(gadcon_class);
      gadcon_class = NULL;
   }
}

/* Gadcon init */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Instance *inst;
   E_Gadcon_Client *gcc;
   Evas_Object *o;

   DBG("Creating gadget instance");

   inst = E_NEW(Instance, 1);
   if (!inst) return NULL;

   /* Create gadcon client */
   gcc = e_gadcon_client_new(gc, name, id, style, NULL);
   if (!gcc)
   {
      E_FREE(inst);
      return NULL;
   }

   gcc->data = inst;
   inst->gcc = gcc;

   /* Create icon */
   o = edje_object_add(gcc->gadcon->evas);
   inst->icon = o;
   inst->gadget = o;

   /* For now, use a simple colored rectangle until we have theme */
   evas_object_color_set(o, 100, 150, 200, 255);
   evas_object_resize(o, 16, 16);
   evas_object_show(o);

   /* Add mouse event handler */
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                   _gadget_mouse_down_cb, inst);

   /* Set gadcon object */
   e_gadcon_client_min_size_set(gcc, 16, 16);
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_show(gcc);

   /* Get first available device */
   Eina_List *devices = iwd_devices_get();
   if (devices && eina_list_count(devices) > 0)
   {
      inst->device = eina_list_data_get(devices);
      DBG("Using device: %s", inst->device->name ? inst->device->name : inst->device->path);
   }
   else
   {
      DBG("No Wi-Fi devices available");
   }

   /* Start update timer */
   inst->update_timer = ecore_timer_add(2.0, _gadget_update_timer_cb, inst);
   _gadget_update(inst);

   /* Add to module instances */
   if (iwd_mod)
      iwd_mod->instances = eina_list_append(iwd_mod->instances, inst);

   return gcc;
}

/* Gadcon shutdown */
static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   DBG("Destroying gadget instance");

   if (!(inst = gcc->data)) return;

   /* Remove from module instances */
   if (iwd_mod)
      iwd_mod->instances = eina_list_remove(iwd_mod->instances, inst);

   /* Delete popup if open */
   if (inst->popup)
   {
      iwd_popup_del(inst);
   }

   /* Stop timer */
   if (inst->update_timer)
   {
      ecore_timer_del(inst->update_timer);
      inst->update_timer = NULL;
   }

   /* Delete icon */
   if (inst->icon)
      evas_object_del(inst->icon);

   E_FREE(inst);
}

/* Gadcon orient */
static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient EINA_UNUSED)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

/* Gadcon label */
static const char *
_gc_label(const E_Gadcon_Client_Class *client_class EINA_UNUSED)
{
   return "IWD Wi-Fi";
}

/* Gadcon icon */
static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class EINA_UNUSED, Evas *evas)
{
   Evas_Object *o;

   o = edje_object_add(evas);
   /* TODO: Load theme icon in Phase 6 */
   /* For now, return a simple colored box */
   evas_object_color_set(o, 100, 150, 200, 255);
   evas_object_resize(o, 16, 16);

   return o;
}

/* Generate new ID */
static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class EINA_UNUSED)
{
   static char buf[32];
   snprintf(buf, sizeof(buf), "%s.%d", _gc_class.name, rand());
   return buf;
}

/* Mouse down callback */
static void
_gadget_mouse_down_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event_info;

   if (!inst) return;

   if (ev->button == 1) /* Left click */
   {
      if (inst->popup)
      {
         /* Close popup */
         iwd_popup_del(inst);
      }
      else
      {
         /* Open popup */
         iwd_popup_new(inst);
      }
   }
}

/* Update gadget icon and tooltip */
static void
_gadget_update(Instance *inst)
{
   char buf[256];

   if (!inst) return;

   /* Update tooltip */
   if (inst->device)
   {
      if (inst->device->state && strcmp(inst->device->state, "connected") == 0)
      {
         snprintf(buf, sizeof(buf), "IWD Wi-Fi\nConnected: %s\nSignal: %s",
                  inst->device->name ? inst->device->name : "Unknown",
                  inst->device->connected_network ? "Good" : "");
      }
      else if (inst->device->state && strcmp(inst->device->state, "connecting") == 0)
      {
         snprintf(buf, sizeof(buf), "IWD Wi-Fi\nConnecting...");
      }
      else
      {
         snprintf(buf, sizeof(buf), "IWD Wi-Fi\nDisconnected");
      }
   }
   else
   {
      snprintf(buf, sizeof(buf), "IWD Wi-Fi\nNo device");
   }

   /* TODO: Update icon appearance based on state (Phase 6 with theme) */
   /* For now, change color based on connection state */
   if (inst->device && inst->device->state)
   {
      if (strcmp(inst->device->state, "connected") == 0)
         evas_object_color_set(inst->icon, 100, 200, 100, 255); /* Green */
      else if (strcmp(inst->device->state, "connecting") == 0)
         evas_object_color_set(inst->icon, 200, 200, 100, 255); /* Yellow */
      else
         evas_object_color_set(inst->icon, 150, 150, 150, 255); /* Gray */
   }
   else
   {
      evas_object_color_set(inst->icon, 200, 100, 100, 255); /* Red - no device */
   }
}

/* Update timer callback */
static Eina_Bool
_gadget_update_timer_cb(void *data)
{
   Instance *inst = data;

   _gadget_update(inst);

   return ECORE_CALLBACK_RENEW;
}
