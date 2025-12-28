#include "e_mod_main.h"

/* Configuration dialog structure */
typedef struct _E_Config_Dialog_Data
{
   int auto_connect;
   int show_hidden_networks;
   int signal_refresh_interval;
   char *preferred_adapter;
} E_Config_Dialog_Data;

/* Forward declarations */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

/* Show configuration dialog */
void
e_iwd_config_show(void)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (!iwd_mod || !iwd_mod->conf) return;

   /* Check if dialog already exists */
   if (e_config_dialog_find("IWD", "extensions/iwd"))
      return;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   cfd = e_config_dialog_new(NULL, "IWD Wi-Fi Configuration",
                             "IWD", "extensions/iwd",
                             NULL, 0, v, NULL);

   if (!cfd)
   {
      E_FREE(v);
      return;
   }
}

/* Create config data */
static void *
_create_data(E_Config_Dialog *cfd EINA_UNUSED)
{
   E_Config_Dialog_Data *cfdata;

   if (!iwd_mod || !iwd_mod->conf) return NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;

   /* Copy current config */
   cfdata->auto_connect = iwd_mod->conf->auto_connect;
   cfdata->show_hidden_networks = iwd_mod->conf->show_hidden_networks;
   cfdata->signal_refresh_interval = iwd_mod->conf->signal_refresh_interval;

   if (iwd_mod->conf->preferred_adapter)
      cfdata->preferred_adapter = strdup(iwd_mod->conf->preferred_adapter);
   else
      cfdata->preferred_adapter = NULL;

   return cfdata;
}

/* Free config data */
static void
_free_data(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata) return;

   if (cfdata->preferred_adapter)
      free(cfdata->preferred_adapter);

   E_FREE(cfdata);
}

/* Create basic UI */
static Evas_Object *
_basic_create(E_Config_Dialog *cfd EINA_UNUSED, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;

   o = e_widget_list_add(evas, 0, 0);

   /* Connection settings frame */
   of = e_widget_framelist_add(evas, "Connection Settings", 0);

   ob = e_widget_check_add(evas, "Auto-connect to known networks",
                          &(cfdata->auto_connect));
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(evas, "Show hidden networks",
                          &(cfdata->show_hidden_networks));
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   /* Performance settings frame */
   of = e_widget_framelist_add(evas, "Performance", 0);

   ob = e_widget_label_add(evas, "Signal refresh interval (seconds):");
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_slider_add(evas, 1, 0, "%1.0f", 1.0, 60.0, 1.0, 0,
                           NULL, &(cfdata->signal_refresh_interval), 150);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   /* Adapter settings frame (if multiple adapters available) */
   Eina_List *devices = iwd_devices_get();
   if (eina_list_count(devices) > 1)
   {
      of = e_widget_framelist_add(evas, "Adapter Selection", 0);

      ob = e_widget_label_add(evas, "Preferred wireless adapter:");
      e_widget_framelist_object_append(of, ob);

      /* TODO: Add radio list for adapter selection when multiple devices exist */
      ob = e_widget_label_add(evas, "(Auto-select)");
      e_widget_framelist_object_append(of, ob);

      e_widget_list_object_append(o, of, 1, 1, 0.5);
   }

   return o;
}

/* Apply configuration */
static int
_basic_apply(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   if (!iwd_mod || !iwd_mod->conf) return 0;

   /* Update config */
   iwd_mod->conf->auto_connect = cfdata->auto_connect;
   iwd_mod->conf->show_hidden_networks = cfdata->show_hidden_networks;
   iwd_mod->conf->signal_refresh_interval = cfdata->signal_refresh_interval;

   if (cfdata->preferred_adapter)
   {
      if (iwd_mod->conf->preferred_adapter)
         eina_stringshare_del(iwd_mod->conf->preferred_adapter);
      iwd_mod->conf->preferred_adapter = eina_stringshare_add(cfdata->preferred_adapter);
   }

   /* Save config */
   e_config_save_queue();

   DBG("Configuration updated");

   return 1;
}
