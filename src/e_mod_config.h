#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

typedef struct _E_Iwd_Config E_Iwd_Config;

struct _E_Iwd_Config
{
   int  auto_connect;
   int  show_hidden;
   int  refresh_interval;
   char *preferred_adapter;
};

extern E_Iwd_Config *e_iwd_config;

void e_iwd_config_load(void);
void e_iwd_config_save(void);
void e_iwd_config_dialog_show(void);

#endif
