#include "e_mod_main.h"
#include "e_mod_gadget.h"
#include "e_mod_popup.h"
#include "e_mod_config.h"
#include "iwd/iwd_manager.h"
#include "iwd/iwd_device.h"
#include "iwd/iwd_network.h"
#include "iwd/iwd_labels.h"
#include <e_gadcon.h>
#include <limits.h>

/* ----- per-instance gadget data --------------------------------------- */

typedef struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_base;   /* themed edje, gcc->o_base */
   Evas_Object     *o_icon;   /* swallowed into o_base */
} Instance;

static Eina_List *_instances = NULL;

/* ----- icon update ----------------------------------------------------- */

/* Walk the manager state to find the network we're currently connected to,
 * if any. Used both for the signal-tier icon and for the tooltip. */
static Iwd_Network *
_active_network(void)
{
   if (!e_iwd || !e_iwd->manager) return NULL;
   const Eina_Hash *devs = iwd_manager_devices(e_iwd->manager);
   const Eina_Hash *nets = iwd_manager_networks(e_iwd->manager);
   if (!devs || !nets) return NULL;
   Eina_Iterator *it = eina_hash_iterator_data_new((Eina_Hash *)devs);
   Iwd_Device *d;
   Iwd_Network *found = NULL;
   EINA_ITERATOR_FOREACH(it, d)
     {
        if (!d->connected_network) continue;
        found = eina_hash_find(nets, d->connected_network);
        if (found) break;
     }
   eina_iterator_free(it);
   return found;
}

static const char *
_icon_for_signal_tier(int tier)
{
   switch (tier)
     {
      case 4: return "network-wireless-signal-excellent";
      case 3: return "network-wireless-signal-good";
      case 2: return "network-wireless-signal-ok";
      case 1: return "network-wireless-signal-weak";
      default: return "network-wireless-signal-none";
     }
}

static const char *
_icon_name_for_state(Iwd_State s)
{
   switch (s)
     {
      case IWD_STATE_OFF:        return "network-offline";
      case IWD_STATE_IDLE:       return "network-wireless-disconnected";
      case IWD_STATE_SCANNING:   return "network-wireless-acquiring";
      case IWD_STATE_CONNECTING: return "network-wireless-acquiring";
      case IWD_STATE_CONNECTED:
        {
           Iwd_Network *n = _active_network();
           return _icon_for_signal_tier(n ? iwd_network_signal_tier(n) : 0);
        }
      case IWD_STATE_ERROR:      return "network-error";
     }
   return "network-wireless";
}

static void
_build_tooltip(Instance *inst, Iwd_State s)
{
   char buf[256];
   if (s == IWD_STATE_CONNECTED)
     {
        Iwd_Network *n = _active_network();
        if (n)
          snprintf(buf, sizeof(buf), "Wi-Fi: %s — %s — signal %d/4",
                   n->ssid ? n->ssid : "?",
                   iwd_security_label(n->security),
                   iwd_network_signal_tier(n));
        else
          snprintf(buf, sizeof(buf), "Wi-Fi: connected");
     }
   else
     snprintf(buf, sizeof(buf), "Wi-Fi: %s", iwd_state_label(s));
   elm_object_tooltip_text_set(inst->o_base, buf);
}

static void
_inst_refresh(Instance *inst)
{
   if (!inst || !inst->o_icon || !e_iwd) return;
   Iwd_State s = iwd_manager_state(e_iwd->manager);
   e_icon_fdo_icon_set(inst->o_icon, _icon_name_for_state(s));
   _build_tooltip(inst, s);
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
_menu_settings_cb(void *data EINA_UNUSED, E_Menu *m EINA_UNUSED, E_Menu_Item *mi EINA_UNUSED)
{
   e_iwd_config_dialog_show();
}

static void
_show_menu(Instance *inst, Evas_Event_Mouse_Down *ev)
{
   E_Zone *zone = e_zone_current_get();
   E_Menu *m = e_menu_new();
   E_Menu_Item *mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, "Settings");
   e_util_menu_item_theme_icon_set(mi, "preferences-system");
   e_menu_item_callback_set(mi, _menu_settings_cb, inst);

   int x, y;
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(m, zone, x + ev->output.x, y + ev->output.y,
                         1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
}

static void
_on_mouse_down(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Instance *inst = data;
   if (ev->button == 1)
     e_iwd_popup_toggle(inst->gcc);
   else if (ev->button == 3)
     _show_menu(inst, ev);
}

/* ----- helpers --------------------------------------------------------- */

static Eina_Bool
_theme_path(char *buf, size_t len)
{
   if (!e_iwd || !e_iwd->module) return EINA_FALSE;
   snprintf(buf, len, "%s/e-module-iwd.edj", e_module_dir_get(e_iwd->module));
   return EINA_TRUE;
}

/* ----- gadcon class ---------------------------------------------------- */

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Instance *inst = E_NEW(Instance, 1);
   char path[PATH_MAX];
   if (!_theme_path(path, sizeof(path))) path[0] = '\0';

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
   if (inst->o_base)
     evas_object_event_callback_del_full(inst->o_base, EVAS_CALLBACK_MOUSE_DOWN,
                                         _on_mouse_down, inst);
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
   char path[PATH_MAX];
   Evas_Object *o = edje_object_add(evas);
   if (_theme_path(path, sizeof(path)))
     edje_object_file_set(o, path, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *cc)
{
   char buf[128];
   snprintf(buf, sizeof(buf), "%s.%d", cc->name,
            eina_list_count(_instances) + 1);
   return eina_stringshare_add(buf);
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
