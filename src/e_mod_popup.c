#include "e_mod_main.h"
#include "e_mod_popup.h"
#include "iwd/iwd_manager.h"
#include "iwd/iwd_device.h"
#include "iwd/iwd_network.h"
#include "iwd/iwd_agent.h"
#include "ui/wifi_auth.h"
#include <e_gadcon_popup.h>
#include <stdlib.h>
#include <string.h>

typedef struct _Popup
{
   E_Gadcon_Popup *gp;
   Evas_Object    *box;
   Evas_Object    *status_lbl;
   Evas_Object    *list;
   Evas_Object    *btn_scan;
   Evas_Object    *btn_toggle;
   Eina_Bool       listening;
} Popup;

static Popup *_popup = NULL;

/* Pending passphrase request from the agent — only one at a time. */
static Iwd_Agent_Request *_pending_req = NULL;

/* ----- helpers --------------------------------------------------------- */

static const char *
_state_label(Iwd_State s)
{
   switch (s) {
    case IWD_STATE_OFF:        return "Wi-Fi disabled";
    case IWD_STATE_IDLE:       return "Disconnected";
    case IWD_STATE_SCANNING:   return "Scanning…";
    case IWD_STATE_CONNECTING: return "Connecting…";
    case IWD_STATE_CONNECTED:  return "Connected";
    case IWD_STATE_ERROR:      return "Error";
   }
   return "";
}

static const char *
_sec_label(Iwd_Security s)
{
   switch (s) {
    case IWD_SEC_OPEN:    return "open";
    case IWD_SEC_PSK:     return "WPA";
    case IWD_SEC_8021X:   return "802.1X";
    case IWD_SEC_WEP:     return "WEP";
    case IWD_SEC_UNKNOWN: return "?";
   }
   return "";
}

static int
_net_cmp(const void *a, const void *b)
{
   const Iwd_Network *na = a, *nb = b;
   if (na->connected != nb->connected) return nb->connected - na->connected;
   if (na->known_path && !nb->known_path) return -1;
   if (!na->known_path && nb->known_path) return  1;
   if (!na->ssid) return 1;
   if (!nb->ssid) return -1;
   return strcasecmp(na->ssid, nb->ssid);
}

/* ----- list rendering -------------------------------------------------- */

static void
_on_net_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *ev EINA_UNUSED)
{
   Iwd_Network *n = data;
   if (!n) return;
   iwd_network_connect(n);
}

static void
_rebuild_list(Popup *p)
{
   if (!p->list || !e_iwd || !e_iwd->manager) return;
   elm_box_clear(p->list);

   /* When the radio is off, hide the (now-stale) network list entirely. */
   if (iwd_manager_state(e_iwd->manager) == IWD_STATE_OFF) return;

   const Eina_Hash *h = iwd_manager_networks(e_iwd->manager);
   if (!h) return;

   /* Snapshot into a list so we can sort. */
   Eina_List *items = NULL;
   Eina_Iterator *it = eina_hash_iterator_data_new((Eina_Hash *)h);
   Iwd_Network *n;
   EINA_ITERATOR_FOREACH(it, n) items = eina_list_append(items, n);
   eina_iterator_free(it);
   items = eina_list_sort(items, eina_list_count(items), _net_cmp);

   Eina_List *l;
   EINA_LIST_FOREACH(items, l, n)
     {
        Evas_Object *btn = elm_button_add(p->list);
        char label[256];
        snprintf(label, sizeof(label), "%s%s  [%s]%s",
                 n->known_path ? "★ " : "  ",
                 n->ssid ? n->ssid : "(hidden)",
                 _sec_label(n->security),
                 n->connected ? "  ✔" : "");
        elm_object_text_set(btn, label);
        evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, 0);
        evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, 0);
        evas_object_smart_callback_add(btn, "clicked", _on_net_clicked, n);
        elm_box_pack_end(p->list, btn);
        evas_object_show(btn);
     }
   eina_list_free(items);
}

static void
_refresh(Popup *p)
{
   if (!p || !e_iwd || !e_iwd->manager) return;
   Iwd_State s = iwd_manager_state(e_iwd->manager);
   if (p->status_lbl)
     elm_object_text_set(p->status_lbl, _state_label(s));
   if (p->btn_toggle)
     elm_object_text_set(p->btn_toggle, s == IWD_STATE_OFF ? "Enable" : "Disable");
   if (p->btn_scan)
     elm_object_disabled_set(p->btn_scan, s == IWD_STATE_OFF);
   _rebuild_list(p);
}

static void
_on_manager_change(void *data, Iwd_Manager *m EINA_UNUSED)
{
   _refresh(data);
}

/* ----- action buttons -------------------------------------------------- */

static void _on_rescan (void *d EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *e EINA_UNUSED)
{
   if (e_iwd && e_iwd->manager) iwd_manager_scan_request(e_iwd->manager);
}
static void _on_toggle(void *d EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *e EINA_UNUSED)
{
   if (!e_iwd || !e_iwd->manager) return;
   Eina_Bool off = (iwd_manager_state(e_iwd->manager) == IWD_STATE_OFF);
   iwd_manager_set_powered(e_iwd->manager, off);
}

/* ----- passphrase plumbing -------------------------------------------- */

static void
_on_auth_done(void *data EINA_UNUSED, const char *pass, Eina_Bool ok)
{
   if (!_pending_req) return;
   if (ok) iwd_agent_reply (_pending_req, pass ? pass : "");
   else    iwd_agent_cancel(_pending_req);
   _pending_req = NULL;
}

static void _on_passphrase_request(void *data, Iwd_Agent_Request *req, const char *netpath);

void
e_iwd_popup_install_passphrase_handler(void)
{
   if (e_iwd && e_iwd->manager)
     iwd_manager_set_passphrase_handler(e_iwd->manager, _on_passphrase_request, NULL);
}

static void
_on_passphrase_request(void *data EINA_UNUSED, Iwd_Agent_Request *req, const char *netpath)
{
   if (_pending_req)
     {
        iwd_agent_cancel(req);
        return;
     }
   _pending_req = req;

   /* Look up the network for a friendly SSID, if we have it. */
   const char *ssid = "network";
   if (e_iwd && e_iwd->manager)
     {
        const Eina_Hash *h = iwd_manager_networks(e_iwd->manager);
        if (h)
          {
             Iwd_Network *n = eina_hash_find(h, netpath);
             if (n && n->ssid) ssid = n->ssid;
          }
     }
   const char *sec = NULL;
   if (e_iwd && e_iwd->manager)
     {
        const Eina_Hash *h = iwd_manager_networks(e_iwd->manager);
        if (h)
          {
             Iwd_Network *n = eina_hash_find(h, netpath);
             if (n) sec = _sec_label(n->security);
          }
     }
   wifi_auth_prompt(_popup ? _popup->box : e_comp->elm, ssid, sec,
                    _on_auth_done, NULL);
}

/* ----- popup lifecycle ------------------------------------------------- */

static void
_destroy(void)
{
   if (!_popup) return;
   if (_popup->listening && e_iwd && e_iwd->manager)
     iwd_manager_listener_del(e_iwd->manager, _on_manager_change, _popup);
   if (_popup->gp) e_object_del(E_OBJECT(_popup->gp));
   free(_popup);
   _popup = NULL;
}

void
e_iwd_popup_close(void) { _destroy(); }

void
e_iwd_popup_refresh(void) { if (_popup) _refresh(_popup); }

void
e_iwd_popup_toggle(E_Gadcon_Client *gcc)
{
   if (_popup) { _destroy(); return; }
   if (!gcc || !e_iwd) return;

   Popup *p = calloc(1, sizeof(*p));
   _popup = p;

   p->gp = e_gadcon_popup_new(gcc, EINA_FALSE);

   Evas *evas = evas_object_evas_get(gcc->o_base);
   Evas_Object *box = elm_box_add(e_comp->elm);
   (void)evas;
   elm_box_padding_set(box, 0, 4);
   evas_object_size_hint_min_set(box, 240, 320);
   p->box = box;

   /* Status line */
   Evas_Object *st = elm_label_add(box);
   p->status_lbl = st;
   elm_box_pack_end(box, st);
   evas_object_show(st);

   /* Network list (vertical box of buttons — keeps deps minimal) */
   Evas_Object *scroller = elm_scroller_add(box);
   evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
   Evas_Object *list_box = elm_box_add(scroller);
   evas_object_size_hint_weight_set(list_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(scroller, list_box);
   evas_object_show(list_box);
   elm_box_pack_end(box, scroller);
   evas_object_show(scroller);
   p->list = list_box;

   /* Action row */
   Evas_Object *row = elm_box_add(box);
   elm_box_horizontal_set(row, EINA_TRUE);
   elm_box_padding_set(row, 4, 0);

   p->btn_scan = elm_button_add(row);
   elm_object_text_set(p->btn_scan, "Rescan");
   evas_object_smart_callback_add(p->btn_scan, "clicked", _on_rescan, NULL);
   elm_box_pack_end(row, p->btn_scan); evas_object_show(p->btn_scan);

   p->btn_toggle = elm_button_add(row);
   elm_object_text_set(p->btn_toggle, "Disable");
   evas_object_smart_callback_add(p->btn_toggle, "clicked", _on_toggle, NULL);
   elm_box_pack_end(row, p->btn_toggle); evas_object_show(p->btn_toggle);

   elm_box_pack_end(box, row);
   evas_object_show(row);

   evas_object_show(box);
   e_gadcon_popup_content_set(p->gp, box);
   e_gadcon_popup_show(p->gp);

   if (e_iwd->manager)
     {
        iwd_manager_listener_add(e_iwd->manager, _on_manager_change, p);
        p->listening = EINA_TRUE;
        iwd_manager_set_passphrase_handler(e_iwd->manager, _on_passphrase_request, NULL);
        iwd_manager_scan_request(e_iwd->manager);
     }
   _refresh(p);
}
