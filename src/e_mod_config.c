#include "e_mod_main.h"
#include "e_mod_config.h"

E_Iwd_Config *e_iwd_config = NULL;

void
e_iwd_config_load(void)
{
   /* TODO: register E_Config_DD and load saved config */
   e_iwd_config = E_NEW(E_Iwd_Config, 1);
   e_iwd_config->auto_connect = 1;
   e_iwd_config->show_hidden = 0;
   e_iwd_config->refresh_interval = 5;
}

void
e_iwd_config_save(void)
{
   /* TODO: e_config_domain_save */
}

void
e_iwd_config_dialog_show(void)
{
   /* TODO: build E_Config_Dialog */
}
