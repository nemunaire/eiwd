#include "wifi_auth.h"
#include <stdlib.h>

typedef struct _Auth_Ctx
{
   Evas_Object *win;     /* top-level window hosting the popup */
   Evas_Object *popup;
   Evas_Object *entry;
   Wifi_Auth_Cb cb;
   void        *data;
   Eina_Bool    fired;
} Auth_Ctx;

static void
_finish(Auth_Ctx *c, Eina_Bool ok, const char *pass)
{
   if (c->fired) return;
   c->fired = EINA_TRUE;
   if (c->cb) c->cb(c->data, pass, ok);
   if (c->win) evas_object_del(c->win);
   free(c);
}

static void
_on_ok(void *data, Evas_Object *o EINA_UNUSED, void *ev EINA_UNUSED)
{
   Auth_Ctx *c = data;
   _finish(c, EINA_TRUE, elm_entry_entry_get(c->entry));
}

static void
_on_cancel(void *data, Evas_Object *o EINA_UNUSED, void *ev EINA_UNUSED)
{
   _finish(data, EINA_FALSE, NULL);
}

static void
_on_del(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *ev EINA_UNUSED)
{
   /* Window closed without cancel/ok — treat as cancel. */
   _finish(data, EINA_FALSE, NULL);
}

Evas_Object *
wifi_auth_prompt(Evas_Object *parent EINA_UNUSED, const char *ssid,
                 Wifi_Auth_Cb cb, void *data)
{
   Auth_Ctx *c = calloc(1, sizeof(*c));
   c->cb = cb; c->data = data;

   /* A floating top-level window so the popup is actually visible —
    * elm_popup parented to a gadcon popup's sub-canvas never shows. */
   Evas_Object *win = elm_win_add(NULL, "eiwd-auth", ELM_WIN_DIALOG_BASIC);
   elm_win_title_set(win, "iwd Wi-Fi");
   elm_win_autodel_set(win, EINA_TRUE);
   elm_win_center(win, EINA_TRUE, EINA_TRUE);
   evas_object_resize(win, 360, 200);
   c->win = win;

   Evas_Object *bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   Evas_Object *p = elm_popup_add(win);
   c->popup = p;
   char title[256];
   snprintf(title, sizeof(title), "Connect to %s", ssid ? ssid : "network");
   elm_object_part_text_set(p, "title,text", title);

   Evas_Object *box = elm_box_add(p);
   elm_box_padding_set(box, 0, 6);

   Evas_Object *entry = elm_entry_add(box);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_password_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, 0);
   elm_box_pack_end(box, entry);
   evas_object_show(entry);
   c->entry = entry;

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
   elm_object_focus_set(entry, EINA_TRUE);
   return win;
}
