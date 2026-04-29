#include "e_mod_main.h"
#include "e_mod_config.h"
#include <e_config_data.h>

#define CONFIG_DOMAIN "module.iwd"
#define CONFIG_VERSION 1

E_Iwd_Config *e_iwd_config = NULL;
static E_Config_DD *_edd = NULL;

static void
_edd_setup(void)
{
   if (_edd) return;
   _edd = E_CONFIG_DD_NEW("E_Iwd_Config", E_Iwd_Config);
   E_CONFIG_VAL(_edd, E_Iwd_Config, version,           INT);
   E_CONFIG_VAL(_edd, E_Iwd_Config, auto_connect,      INT);
   E_CONFIG_VAL(_edd, E_Iwd_Config, show_hidden,       INT);
   E_CONFIG_VAL(_edd, E_Iwd_Config, refresh_interval,  INT);
   E_CONFIG_VAL(_edd, E_Iwd_Config, preferred_adapter, STR);
}

void
e_iwd_config_load(void)
{
   _edd_setup();
   e_iwd_config = e_config_domain_load(CONFIG_DOMAIN, _edd);
   if (e_iwd_config && e_iwd_config->version == CONFIG_VERSION) return;

   /* Missing or out-of-date — start fresh with defaults. */
   if (e_iwd_config)
     {
        eina_stringshare_replace(&e_iwd_config->preferred_adapter, NULL);
        E_FREE(e_iwd_config);
     }
   e_iwd_config = E_NEW(E_Iwd_Config, 1);
   e_iwd_config->version          = CONFIG_VERSION;
   e_iwd_config->auto_connect     = 1;
   e_iwd_config->show_hidden      = 0;
   e_iwd_config->refresh_interval = 5;
}

void
e_iwd_config_save(void)
{
   if (!_edd || !e_iwd_config) return;
   e_config_domain_save(CONFIG_DOMAIN, _edd, e_iwd_config);
}

/* ----- Settings dialog ------------------------------------------------ */

struct _E_Config_Dialog_Data
{
   int   auto_connect;
   int   show_hidden;
   int   refresh_interval;
   char *preferred_adapter;
};

static void *
_cfd_create(E_Config_Dialog *cfd EINA_UNUSED)
{
   if (!e_iwd_config) return NULL;
   E_Config_Dialog_Data *c = E_NEW(E_Config_Dialog_Data, 1);
   c->auto_connect      = e_iwd_config->auto_connect;
   c->show_hidden       = e_iwd_config->show_hidden;
   c->refresh_interval  = e_iwd_config->refresh_interval;
   c->preferred_adapter = e_iwd_config->preferred_adapter
                          ? strdup(e_iwd_config->preferred_adapter) : strdup("");
   return c;
}

static void
_cfd_free(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *c)
{
   if (!c) return;
   free(c->preferred_adapter);
   E_FREE(c);
}

static Evas_Object *
_cfd_basic_create(E_Config_Dialog *cfd EINA_UNUSED, Evas *evas, E_Config_Dialog_Data *c)
{
   Evas_Object *o, *of, *ob;
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, "Connection", 0);
   ob = e_widget_check_add(evas, "Auto-connect to known networks",
                           &c->auto_connect);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, "Show hidden networks",
                           &c->show_hidden);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, "Performance", 0);
   ob = e_widget_label_add(evas, "Signal refresh interval (s):");
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, "%1.0f", 1.0, 60.0, 1.0, 0,
                            NULL, &c->refresh_interval, 150);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, "Adapter", 0);
   ob = e_widget_label_add(evas, "Preferred wireless adapter (blank = auto):");
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_entry_add(evas, &c->preferred_adapter, NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int
_cfd_basic_apply(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *c)
{
   if (!e_iwd_config || !c) return 0;
   e_iwd_config->auto_connect     = c->auto_connect;
   e_iwd_config->show_hidden      = c->show_hidden;
   e_iwd_config->refresh_interval = c->refresh_interval;
   if (e_iwd_config->preferred_adapter)
     eina_stringshare_del(e_iwd_config->preferred_adapter);
   e_iwd_config->preferred_adapter =
     (c->preferred_adapter && *c->preferred_adapter)
     ? eina_stringshare_add(c->preferred_adapter) : NULL;
   e_iwd_config_save();
   return 1;
}

void
e_iwd_config_dialog_show(void)
{
   if (e_config_dialog_find("E_Iwd", "extensions/iwd")) return;

   E_Config_Dialog_View *v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;
   v->create_cfdata        = _cfd_create;
   v->free_cfdata          = _cfd_free;
   v->basic.create_widgets = _cfd_basic_create;
   v->basic.apply_cfdata   = _cfd_basic_apply;

   E_Config_Dialog *cfd = e_config_dialog_new(NULL,
       "iwd Wi-Fi Settings", "E_Iwd", "extensions/iwd", NULL, 0, v, NULL);
   if (!cfd) E_FREE(v);
}
