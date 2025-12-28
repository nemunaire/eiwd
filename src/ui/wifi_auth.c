#include "../e_mod_main.h"

/* Auth dialog structure */
typedef struct _Auth_Dialog
{
   Instance *inst;
   IWD_Network *network;
   E_Dialog *dialog;
   Evas_Object *entry;
   char *passphrase;
} Auth_Dialog;

/* Global auth dialog (only one at a time) */
static Auth_Dialog *auth_dialog = NULL;

/* Forward declarations */
static void _auth_dialog_ok_cb(void *data, E_Dialog *dialog);
static void _auth_dialog_cancel_cb(void *data, E_Dialog *dialog);
static void _auth_dialog_free(Auth_Dialog *ad);

/* Show authentication dialog */
void
wifi_auth_dialog_show(Instance *inst, IWD_Network *net)
{
   Auth_Dialog *ad;
   E_Dialog *dia;
   Evas_Object *o, *entry;
   char buf[512];

   if (!inst || !net) return;

   /* Only one auth dialog at a time */
   if (auth_dialog)
   {
      WRN("Auth dialog already open");
      return;
   }

   DBG("Showing auth dialog for network: %s", net->name ? net->name : net->path);

   ad = E_NEW(Auth_Dialog, 1);
   if (!ad) return;

   ad->inst = inst;
   ad->network = net;
   auth_dialog = ad;

   /* Create dialog */
   dia = e_dialog_new(NULL, "E", "iwd_passphrase");
   if (!dia)
   {
      _auth_dialog_free(ad);
      return;
   }

   ad->dialog = dia;

   e_dialog_title_set(dia, "Wi-Fi Authentication");
   e_dialog_icon_set(dia, "network-wireless", 48);

   /* Message */
   snprintf(buf, sizeof(buf),
            "Enter passphrase for network:<br>"
            "<b>%s</b><br><br>"
            "Security: %s",
            net->name ? net->name : "Unknown",
            net->type ? (strcmp(net->type, "psk") == 0 ? "WPA2/WPA3" : net->type) : "Unknown");

   o = e_widget_label_add(evas_object_evas_get(dia->win), buf);
   e_widget_size_min_set(o, 300, 40);

   /* Entry for passphrase */
   entry = e_widget_entry_add(evas_object_evas_get(dia->win), &ad->passphrase, NULL, NULL, NULL);
   e_widget_entry_password_set(entry, 1);
   e_widget_size_min_set(entry, 280, 30);

   /* Pack into a list */
   Evas_Object *list = e_widget_list_add(evas_object_evas_get(dia->win), 0, 0);
   e_widget_list_object_append(list, o, 1, 1, 0.5);
   e_widget_list_object_append(list, entry, 1, 1, 0.5);

   e_dialog_content_set(dia, list, 300, 120);
   ad->entry = entry;

   /* Buttons */
   e_dialog_button_add(dia, "Connect", NULL, _auth_dialog_ok_cb, ad);
   e_dialog_button_add(dia, "Cancel", NULL, _auth_dialog_cancel_cb, ad);

   e_dialog_button_focus_num(dia, 0);
   e_dialog_show(dia);

   INF("Auth dialog shown");
}

/* OK button callback */
static void
_auth_dialog_ok_cb(void *data, E_Dialog *dialog EINA_UNUSED)
{
   Auth_Dialog *ad = data;

   if (!ad) return;

   DBG("Auth dialog OK clicked");

   if (!ad->passphrase || strlen(ad->passphrase) < 8)
   {
      e_util_dialog_show("Error",
                         "Passphrase must be at least 8 characters long.");
      return;
   }

   /* Store passphrase in agent */
   iwd_agent_set_passphrase(ad->passphrase);

   /* Initiate connection */
   if (ad->network)
   {
      INF("Connecting to network: %s", ad->network->name);
      iwd_network_connect(ad->network);
   }

   /* Close dialog */
   _auth_dialog_free(ad);
}

/* Cancel button callback */
static void
_auth_dialog_cancel_cb(void *data, E_Dialog *dialog EINA_UNUSED)
{
   Auth_Dialog *ad = data;

   DBG("Auth dialog cancelled");

   /* Cancel agent request */
   iwd_agent_cancel();

   _auth_dialog_free(ad);
}

/* Free auth dialog */
static void
_auth_dialog_free(Auth_Dialog *ad)
{
   if (!ad) return;

   DBG("Freeing auth dialog");

   if (ad->dialog)
      e_object_del(E_OBJECT(ad->dialog));

   /* Clear passphrase from memory */
   if (ad->passphrase)
   {
      memset(ad->passphrase, 0, strlen(ad->passphrase));
      E_FREE(ad->passphrase);
   }

   E_FREE(ad);
   auth_dialog = NULL;
}

/* Cancel any open auth dialog */
void
wifi_auth_dialog_cancel(void)
{
   if (auth_dialog)
   {
      DBG("Cancelling auth dialog from external request");
      _auth_dialog_free(auth_dialog);
   }
}
