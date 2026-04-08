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
        if (e_iwd_config->preferred_adapter)
          eina_stringshare_del(e_iwd_config->preferred_adapter);
        free(e_iwd_config);
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

void
e_iwd_config_dialog_show(void)
{
   /* TODO: full E_Config_Dialog with checkboxes/spinners.
    * Settings are persisted; only the GUI is missing. */
}
