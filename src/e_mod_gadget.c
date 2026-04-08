#include "e_mod_main.h"
#include "e_mod_gadget.h"
#include "e_mod_popup.h"
#include "iwd/iwd_manager.h"
#include <e_gadcon.h>

/* ----- per-instance gadget data --------------------------------------- */

typedef struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_base;   /* themed edje, gcc->o_base */
   Evas_Object     *o_icon;   /* swallowed into o_base */
} Instance;

static Eina_List *_instances = NULL;

/* ----- icon update ----------------------------------------------------- */

static const char *
_icon_name_for_state(Iwd_State s)
{
   switch (s)
     {
      case IWD_STATE_OFF:        return "network-offline";
      case IWD_STATE_IDLE:       return "network-wireless-disconnected";
      case IWD_STATE_SCANNING:   return "network-wireless-acquiring";
      case IWD_STATE_CONNECTING: return "network-wireless-acquiring";
      case IWD_STATE_CONNECTED:  return "network-wireless-signal-excellent";
      case IWD_STATE_ERROR:      return "network-error";
     }
   return "network-wireless";
}

static void
_inst_refresh(Instance *inst)
{
   if (!inst || !inst->o_icon || !e_iwd) return;
   Iwd_State s = iwd_manager_state(e_iwd->manager);
   e_icon_fdo_icon_set(inst->o_icon, _icon_name_for_state(s));
}

/* Listener invoked by iwd_manager whenever state changes. */
static void
_on_manager_change(void *data EINA_UNUSED, Iwd_Manager *m EINA_UNUSED)
{
   Eina_List *l;
   Instance *inst;
   EINA_LIST_FOREACH(_instances, l, inst) _inst_refresh(inst);
}

/* ----- click → popup --------------------------------------------------- */

static void
_on_mouse_down(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Instance *inst = data;
   if (ev->button == 1)
     e_iwd_popup_toggle(inst->gcc->o_base);
}

/* ----- helpers --------------------------------------------------------- */

static char *
_theme_path(void)
{
   static char buf[4096];
   if (!e_iwd || !e_iwd->module) return NULL;
   snprintf(buf, sizeof(buf), "%s/e-module-iwd.edj",
            e_module_dir_get(e_iwd->module));
   return buf;
}

/* ----- gadcon class ---------------------------------------------------- */

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Instance *inst = E_NEW(Instance, 1);
   const char *path = _theme_path();

   /* themed edje is the gadcon o_base — its intrinsic min comes from the
    * theme group, just like the backlight module. */
   Evas_Object *base = edje_object_add(gc->evas);
   edje_object_file_set(base, path, "e/modules/iwd/main");
   evas_object_show(base);
   inst->o_base = base;

   /* the actual fdo icon goes into the swallow part */
   Evas_Object *icon = e_icon_add(gc->evas);
   e_icon_fill_inside_set(icon, EINA_TRUE);
   e_icon_fdo_icon_set(icon, "network-wireless");
   e_icon_preload_set(icon, EINA_TRUE);
   evas_object_show(icon);
   edje_object_part_swallow(base, "e.swallow.content", icon);
   inst->o_icon = icon;

   inst->gcc = e_gadcon_client_new(gc, name, id, style, base);
   inst->gcc->data = inst;

   evas_object_event_callback_add(base, EVAS_CALLBACK_MOUSE_DOWN,
                                  _on_mouse_down, inst);

   _instances = eina_list_append(_instances, inst);
   _inst_refresh(inst);
   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst = gcc->data;
   if (!inst) return;
   _instances = eina_list_remove(_instances, inst);
   if (inst->o_icon) evas_object_del(inst->o_icon);
   if (inst->o_base) evas_object_del(inst->o_base);
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient EINA_UNUSED)
{
   Instance *inst = gcc->data;
   Evas_Coord mw = 0, mh = 0;
   if (!inst || !inst->o_base) return;
   edje_object_size_min_get(inst->o_base, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_base, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *cc EINA_UNUSED) { return "iwd"; }

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *cc EINA_UNUSED, Evas *evas)
{
   const char *path = _theme_path();
   Evas_Object *o = edje_object_add(evas);
   if (path) edje_object_file_set(o, path, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *cc)
{
   static char buf[128];
   snprintf(buf, sizeof(buf), "%s.%d", cc->name,
            eina_list_count(_instances) + 1);
   return buf;
}

static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
   "iwd",
   { _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* ----- public ---------------------------------------------------------- */

void
e_iwd_gadget_init(void)
{
   e_gadcon_provider_register(&_gadcon_class);
   if (e_iwd && e_iwd->manager)
     iwd_manager_listener_add(e_iwd->manager, _on_manager_change, NULL);
}

void
e_iwd_gadget_shutdown(void)
{
   if (e_iwd && e_iwd->manager)
     iwd_manager_listener_del(e_iwd->manager, _on_manager_change, NULL);
   e_gadcon_provider_unregister(&_gadcon_class);
}

void
e_iwd_gadget_update(void)
{
   _on_manager_change(NULL, NULL);
}
