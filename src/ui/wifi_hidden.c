#include "../e_mod_main.h"

/* Hidden network dialog structure */
typedef struct _Hidden_Dialog
{
   Instance *inst;
   E_Dialog *dialog;
   Evas_Object *ssid_entry;
   Evas_Object *pass_entry;
   char *ssid;
   char *passphrase;
   Eina_Bool has_password;
} Hidden_Dialog;

/* Global hidden dialog */
static Hidden_Dialog *hidden_dialog = NULL;

/* Forward declarations */
static void _hidden_dialog_ok_cb(void *data, E_Dialog *dialog);
static void _hidden_dialog_cancel_cb(void *data, E_Dialog *dialog);
static void _hidden_dialog_free(Hidden_Dialog *hd);

/* Show hidden network dialog */
void
wifi_hidden_dialog_show(Instance *inst)
{
   Hidden_Dialog *hd;
   E_Dialog *dia;
   Evas_Object *o, *list, *ssid_entry, *pass_entry;

   if (!inst) return;

   /* Only one hidden dialog at a time */
   if (hidden_dialog)
   {
      WRN("Hidden network dialog already open");
      return;
   }

   DBG("Showing hidden network dialog");

   hd = E_NEW(Hidden_Dialog, 1);
   if (!hd) return;

   hd->inst = inst;
   hidden_dialog = hd;

   /* Create dialog */
   dia = e_dialog_new(NULL, "E", "iwd_hidden_network");
   if (!dia)
   {
      _hidden_dialog_free(hd);
      return;
   }

   hd->dialog = dia;

   e_dialog_title_set(dia, "Connect to Hidden Network");
   e_dialog_icon_set(dia, "network-wireless", 48);

   /* Create content list */
   list = e_widget_list_add(evas_object_evas_get(dia->win), 0, 0);

   /* SSID label and entry */
   o = e_widget_label_add(evas_object_evas_get(dia->win), "Network Name (SSID):");
   e_widget_list_object_append(list, o, 1, 1, 0.5);

   ssid_entry = e_widget_entry_add(evas_object_evas_get(dia->win), &hd->ssid, NULL, NULL, NULL);
   e_widget_size_min_set(ssid_entry, 280, 30);
   e_widget_list_object_append(list, ssid_entry, 1, 1, 0.5);
   hd->ssid_entry = ssid_entry;

   /* Spacing */
   o = e_widget_label_add(evas_object_evas_get(dia->win), " ");
   e_widget_list_object_append(list, o, 1, 1, 0.5);

   /* Passphrase label and entry */
   o = e_widget_label_add(evas_object_evas_get(dia->win), "Passphrase (leave empty for open network):");
   e_widget_list_object_append(list, o, 1, 1, 0.5);

   pass_entry = e_widget_entry_add(evas_object_evas_get(dia->win), &hd->passphrase, NULL, NULL, NULL);
   e_widget_entry_password_set(pass_entry, 1);
   e_widget_size_min_set(pass_entry, 280, 30);
   e_widget_list_object_append(list, pass_entry, 1, 1, 0.5);
   hd->pass_entry = pass_entry;

   e_dialog_content_set(dia, list, 300, 180);

   /* Buttons */
   e_dialog_button_add(dia, "Connect", NULL, _hidden_dialog_ok_cb, hd);
   e_dialog_button_add(dia, "Cancel", NULL, _hidden_dialog_cancel_cb, hd);

   e_dialog_button_focus_num(dia, 0);
   e_dialog_show(dia);

   INF("Hidden network dialog shown");
}

/* OK button callback */
static void
_hidden_dialog_ok_cb(void *data, E_Dialog *dialog EINA_UNUSED)
{
   Hidden_Dialog *hd = data;

   if (!hd) return;

   DBG("Hidden network dialog OK clicked");

   if (!hd->ssid || strlen(hd->ssid) == 0)
   {
      e_util_dialog_show("Error", "Please enter a network name (SSID).");
      return;
   }

   /* Check if passphrase is provided */
   hd->has_password = (hd->passphrase && strlen(hd->passphrase) > 0);

   if (hd->has_password && strlen(hd->passphrase) < 8)
   {
      e_util_dialog_show("Error",
                         "Passphrase must be at least 8 characters long.");
      return;
   }

   /* Store passphrase if provided */
   if (hd->has_password)
   {
      iwd_agent_set_passphrase(hd->passphrase);
   }

   /* Connect to hidden network */
   if (hd->inst && hd->inst->device)
   {
      INF("Connecting to hidden network: %s", hd->ssid);
      iwd_device_connect_hidden(hd->inst->device, hd->ssid);
   }

   /* Close dialog */
   _hidden_dialog_free(hd);
}

/* Cancel button callback */
static void
_hidden_dialog_cancel_cb(void *data, E_Dialog *dialog EINA_UNUSED)
{
   Hidden_Dialog *hd = data;

   DBG("Hidden network dialog cancelled");

   _hidden_dialog_free(hd);
}

/* Free hidden dialog */
static void
_hidden_dialog_free(Hidden_Dialog *hd)
{
   if (!hd) return;

   DBG("Freeing hidden network dialog");

   if (hd->dialog)
      e_object_del(E_OBJECT(hd->dialog));

   /* Clear sensitive data from memory */
   if (hd->ssid)
   {
      memset(hd->ssid, 0, strlen(hd->ssid));
      E_FREE(hd->ssid);
   }

   if (hd->passphrase)
   {
      memset(hd->passphrase, 0, strlen(hd->passphrase));
      E_FREE(hd->passphrase);
   }

   E_FREE(hd);
   hidden_dialog = NULL;
}

/* Cancel any open hidden dialog */
void
wifi_hidden_dialog_cancel(void)
{
   if (hidden_dialog)
   {
      DBG("Cancelling hidden network dialog from external request");
      _hidden_dialog_free(hidden_dialog);
   }
}
