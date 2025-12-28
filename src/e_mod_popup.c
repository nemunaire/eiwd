#include "e_mod_main.h"

/* Forward declarations */
static void _popup_comp_del_cb(void *data, Evas_Object *obj);
static void _button_rescan_cb(void *data, Evas_Object *obj, void *event_info);
static void _button_disconnect_cb(void *data, Evas_Object *obj, void *event_info);
static void _button_hidden_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _popup_reopen_cb(void *data);
static void _network_selected_cb(void *data, Evas_Object *obj, void *event_info);

/* Create popup */
void
iwd_popup_new(Instance *inst)
{
   Evas_Object *list, *box, *button, *label;
   E_Gadcon_Popup *popup;
   Evas_Coord w, h;
   IWD_Network *net;
   Eina_List *l;

   if (!inst) return;
   if (inst->popup) return;

   DBG("Creating popup");

   /* Create popup */
   popup = e_gadcon_popup_new(inst->gcc, 0);
   if (!popup) return;

   inst->popup = (void *)popup;

   /* Create main box */
   box = elm_box_add(e_comp->elm);
   elm_box_horizontal_set(box, EINA_FALSE);
   elm_box_padding_set(box, 0, 5);
   evas_object_show(box);

   /* Title */
   label = elm_label_add(box);
   elm_object_text_set(label, "<b>IWD Wi-Fi Manager</b>");
   elm_box_pack_end(box, label);
   evas_object_show(label);

   /* Current connection status */
   if (inst->device)
   {
      Evas_Object *frame = elm_frame_add(box);
      elm_object_text_set(frame, "Current Connection");

      Evas_Object *status_box = elm_box_add(frame);

      char buf[256];
      if (inst->device->state && strcmp(inst->device->state, "connected") == 0)
      {
         snprintf(buf, sizeof(buf), "Connected to: %s",
                  inst->device->name ? inst->device->name : "Unknown");
      }
      else if (inst->device->state && strcmp(inst->device->state, "connecting") == 0)
      {
         snprintf(buf, sizeof(buf), "Connecting...");
      }
      else
      {
         snprintf(buf, sizeof(buf), "Disconnected");
      }

      Evas_Object *status_label = elm_label_add(status_box);
      elm_object_text_set(status_label, buf);
      elm_box_pack_end(status_box, status_label);
      evas_object_show(status_label);

      /* Disconnect button if connected */
      if (inst->device->state && strcmp(inst->device->state, "connected") == 0)
      {
         button = elm_button_add(status_box);
         elm_object_text_set(button, "Disconnect");
         evas_object_smart_callback_add(button, "clicked", _button_disconnect_cb, inst);
         elm_box_pack_end(status_box, button);
         evas_object_show(button);
      }

      elm_object_content_set(frame, status_box);
      evas_object_show(status_box);
      elm_box_pack_end(box, frame);
      evas_object_show(frame);
   }

   /* Available networks */
   Evas_Object *frame = elm_frame_add(box);
   elm_object_text_set(frame, "Available Networks");

   list = elm_list_add(frame);
   elm_list_mode_set(list, ELM_LIST_COMPRESS);

   /* Add networks to list */
   Eina_List *networks = iwd_networks_get();
   int count = 0;

   EINA_LIST_FOREACH(networks, l, net)
   {
      if (net->name)
      {
         char item_text[256];
         const char *security = "Open";

         if (net->type)
         {
            if (strcmp(net->type, "psk") == 0)
               security = "WPA2";
            else if (strcmp(net->type, "8021x") == 0)
               security = "Enterprise";
         }

         snprintf(item_text, sizeof(item_text), "%s (%s)%s",
                  net->name, security,
                  net->known ? " *" : "");

         elm_list_item_append(list, item_text, NULL, NULL, _network_selected_cb, net);
         count++;
      }
   }

   if (count == 0)
   {
      elm_list_item_append(list, "No networks found", NULL, NULL, NULL, NULL);
   }

   /* Set select mode to always */
   elm_list_select_mode_set(list, ELM_OBJECT_SELECT_MODE_ALWAYS);
   elm_list_go(list);
   elm_object_content_set(frame, list);
   evas_object_show(list);
   elm_box_pack_end(box, frame);
   evas_object_show(frame);

   /* Action buttons */
   Evas_Object *button_box = elm_box_add(box);
   elm_box_horizontal_set(button_box, EINA_TRUE);
   elm_box_padding_set(button_box, 5, 0);

   /* Rescan button */
   button = elm_button_add(button_box);
   elm_object_text_set(button, "Rescan");
   evas_object_smart_callback_add(button, "clicked", _button_rescan_cb, inst);
   elm_box_pack_end(button_box, button);
   evas_object_show(button);

   /* Hidden network button */
   button = elm_button_add(button_box);
   elm_object_text_set(button, "Hidden...");
   evas_object_smart_callback_add(button, "clicked", _button_hidden_cb, inst);
   elm_box_pack_end(button_box, button);
   evas_object_show(button);

   elm_box_pack_end(box, button_box);
   evas_object_show(button_box);

   /* Set popup content */
   e_gadcon_popup_content_set(popup, box);

   /* Size the popup */
   evas_object_geometry_get(box, NULL, NULL, &w, &h);
   if (w < 200) w = 200;
   if (h < 150) h = 150;
   if (w > 400) w = 400;
   if (h > 500) h = 500;

   evas_object_resize(box, w, h);

   /* Show popup */
   e_gadcon_popup_show(popup);

   /* Auto-close on escape */
   e_comp_object_util_autoclose(popup->comp_object,
                                 _popup_comp_del_cb,
                                 e_comp_object_util_autoclose_on_escape,
                                 inst);

   DBG("Popup created");
}

/* Delete popup */
void
iwd_popup_del(Instance *inst)
{
   E_Gadcon_Popup *popup;

   if (!inst) return;
   if (!inst->popup) return;

   DBG("Deleting popup");

   popup = (E_Gadcon_Popup *)inst->popup;
   e_object_del(E_OBJECT(popup));
   inst->popup = NULL;
}

/* Comp delete callback */
static void
_popup_comp_del_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   Instance *inst = data;

   if (inst && inst->popup)
      iwd_popup_del(inst);
}

/* Rescan button callback */
static void
_button_rescan_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Instance *inst = data;

   if (!inst || !inst->device) return;

   DBG("Rescan requested");
   iwd_device_scan(inst->device);

   /* Close and reopen popup to refresh */
   iwd_popup_del(inst);
   ecore_timer_add(0.5, _popup_reopen_cb, inst);
}

/* Timer callback to reopen popup */
static Eina_Bool
_popup_reopen_cb(void *data)
{
   Instance *inst = data;
   if (inst)
      iwd_popup_new(inst);
   return ECORE_CALLBACK_CANCEL;
}

/* Disconnect button callback */
static void
_button_disconnect_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Instance *inst = data;

   if (!inst || !inst->device) return;

   DBG("Disconnect requested");
   iwd_device_disconnect(inst->device);

   /* Close popup */
   iwd_popup_del(inst);
}

/* Hidden network button callback */
static void
_button_hidden_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Instance *inst = data;

   if (!inst) return;

   DBG("Hidden network button clicked");

   extern void wifi_hidden_dialog_show(Instance *inst);
   wifi_hidden_dialog_show(inst);

   /* Close popup */
   iwd_popup_del(inst);
}

/* Network selected callback */
static void
_network_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   IWD_Network *net = data;

   if (!net || !net->name)
   {
      DBG("Invalid network selected");
      return;
   }

   INF("Network selected: %s (type: %s)", net->name, net->type ? net->type : "unknown");

   /* Check if network requires authentication */
   if (net->type && (strcmp(net->type, "psk") == 0 || strcmp(net->type, "8021x") == 0))
   {
      /* Secured network - need to show auth dialog first */
      /* Get instance from module */
      if (iwd_mod && iwd_mod->instances)
      {
         Instance *inst = eina_list_data_get(iwd_mod->instances);
         if (inst)
         {
            extern void wifi_auth_dialog_show(Instance *inst, IWD_Network *net);
            wifi_auth_dialog_show(inst, net);
         }
      }
   }
   else
   {
      /* Open network - connect directly */
      DBG("Connecting to open network");
      iwd_network_connect(net);
   }
}
