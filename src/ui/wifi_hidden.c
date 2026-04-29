#include "wifi_hidden.h"
#include <stdlib.h>
#include <string.h>

typedef struct _Hidden_Ctx
{
   Evas_Object   *win;
   Evas_Object   *popup;
   Evas_Object   *e_ssid;
   Evas_Object   *e_pass;
   Wifi_Hidden_Cb cb;
   void          *data;
   Eina_Bool      fired;
} Hidden_Ctx;

static void
_finish(Hidden_Ctx *c, Eina_Bool ok)
{
   if (c->fired) return;
   c->fired = EINA_TRUE;

   /* Copy SSID + passphrase into buffers we own; wipe the passphrase
    * (and overwrite the entry) before the window is destroyed. */
   char *ssid = NULL, *pass = NULL;
   if (ok)
     {
        if (c->e_ssid)
          {
             const char *r = elm_entry_entry_get(c->e_ssid);
             if (r) ssid = strdup(r);
          }
        if (c->e_pass)
          {
             const char *r = elm_entry_entry_get(c->e_pass);
             if (r) pass = strdup(r);
          }
     }
   if (c->cb) c->cb(c->data, ssid, pass, ok);
   if (pass)
     {
        explicit_bzero(pass, strlen(pass));
        free(pass);
     }
   /* SSIDs aren't secret, but wiping keeps the heap consistent with the
    * passphrase handling and avoids leaving identifiable network names in
    * freed memory after a hidden-network prompt. */
   if (ssid)
     {
        explicit_bzero(ssid, strlen(ssid));
        free(ssid);
     }
   if (c->e_ssid) elm_entry_entry_set(c->e_ssid, "");
   if (c->e_pass) elm_entry_entry_set(c->e_pass, "");
   if (c->win) evas_object_del(c->win);
   free(c);
}

static void
_on_ok(void *data, Evas_Object *o EINA_UNUSED, void *ev EINA_UNUSED)
{
   Hidden_Ctx *c = data;
   const char *ssid = elm_entry_entry_get(c->e_ssid);
   if (!ssid || !*ssid) return;   /* require non-empty SSID */
   _finish(c, EINA_TRUE);
}

static void
_on_cancel(void *data, Evas_Object *o EINA_UNUSED, void *ev EINA_UNUSED)
{
   _finish(data, EINA_FALSE);
}

static void
_on_del(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *ev EINA_UNUSED)
{
   Hidden_Ctx *c = data;
   c->win = NULL;
   c->e_ssid = NULL;
   c->e_pass = NULL;
   _finish(c, EINA_FALSE);
}

static Evas_Object *
_labelled_entry(Evas_Object *box, const char *label_text, Eina_Bool password)
{
   Evas_Object *lbl = elm_label_add(box);
   elm_object_text_set(lbl, label_text);
   evas_object_size_hint_align_set(lbl, 0.0, 0.5);
   elm_box_pack_end(box, lbl);
   evas_object_show(lbl);

   Evas_Object *e = elm_entry_add(box);
   elm_entry_single_line_set(e, EINA_TRUE);
   elm_entry_scrollable_set(e, EINA_TRUE);
   if (password) elm_entry_password_set(e, EINA_TRUE);
   evas_object_size_hint_weight_set(e, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(e, EVAS_HINT_FILL, 0);
   elm_box_pack_end(box, e);
   evas_object_show(e);
   return e;
}

void
wifi_hidden_prompt(Evas_Object *parent EINA_UNUSED, Wifi_Hidden_Cb cb, void *data)
{
   Hidden_Ctx *c = calloc(1, sizeof(*c));
   if (!c) { if (cb) cb(data, NULL, NULL, EINA_FALSE); return; }
   c->cb = cb; c->data = data;

   /* Floating top-level so the popup actually shows. */
   Evas_Object *win = elm_win_add(NULL, "eiwd-hidden", ELM_WIN_DIALOG_BASIC);
   elm_win_title_set(win, "iwd Wi-Fi");
   elm_win_autodel_set(win, EINA_TRUE);
   elm_win_center(win, EINA_TRUE, EINA_TRUE);
   evas_object_resize(win, 360, 220);
   c->win = win;

   Evas_Object *bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   Evas_Object *p = elm_popup_add(win);
   c->popup = p;
   elm_object_part_text_set(p, "title,text", "Connect to hidden network");

   Evas_Object *box = elm_box_add(p);
   elm_box_padding_set(box, 0, 4);

   c->e_ssid = _labelled_entry(box, "SSID:",       EINA_FALSE);
   c->e_pass = _labelled_entry(box, "Passphrase (optional):", EINA_TRUE);

   evas_object_show(box);
   elm_object_content_set(p, box);

   Evas_Object *bcancel = elm_button_add(p);
   elm_object_text_set(bcancel, "Cancel");
   elm_object_part_content_set(p, "button1", bcancel);
   evas_object_smart_callback_add(bcancel, "clicked", _on_cancel, c);

   Evas_Object *bok = elm_button_add(p);
   elm_object_text_set(bok, "Connect");
   elm_object_part_content_set(p, "button2", bok);
   evas_object_smart_callback_add(bok, "clicked", _on_ok, c);

   evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _on_del, c);

   evas_object_show(p);
   evas_object_show(win);
   elm_object_focus_set(c->e_ssid, EINA_TRUE);
}
