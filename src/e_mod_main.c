#include "e_mod_main.h"

/* Module metadata */
E_API E_Module_Api e_modapi = {
   E_MODULE_API_VERSION,
   "IWD"
};

/* Global module instance */
Mod *iwd_mod = NULL;

/* Logging domain */
int _e_iwd_log_dom = -1;

/* Forward declarations */
static void _iwd_config_load(void);
static void _iwd_config_free(void);

/* Module initialization */
E_API void *
e_modapi_init(E_Module *m)
{
   Mod *mod;

   /* Initialize logging */
   _e_iwd_log_dom = eina_log_domain_register("e-iwd", EINA_COLOR_CYAN);
   if (_e_iwd_log_dom < 0)
   {
      EINA_LOG_ERR("Could not register log domain 'e-iwd'");
      return NULL;
   }

   INF("IWD Module initializing");

   /* Allocate module structure */
   mod = E_NEW(Mod, 1);
   if (!mod)
   {
      ERR("Failed to allocate module structure");
      eina_log_domain_unregister(_e_iwd_log_dom);
      _e_iwd_log_dom = -1;
      return NULL;
   }

   mod->module = m;
   mod->log_dom = _e_iwd_log_dom;
   iwd_mod = mod;

   /* Initialize configuration */
   e_iwd_config_init();
   _iwd_config_load();

   /* Initialize D-Bus and iwd subsystems (Phase 2 & 5) */
   iwd_state_init();
   iwd_device_init();
   iwd_network_init();

   if (!iwd_dbus_init())
   {
      WRN("Failed to initialize D-Bus connection to iwd");
      iwd_state_set(IWD_STATE_ERROR);
      /* Continue anyway - we'll show error state in UI */
   }

   if (!iwd_agent_init())
   {
      WRN("Failed to initialize iwd agent");
   }

   /* Initialize gadget (Phase 3) */
   e_iwd_gadget_init();

   INF("IWD Module initialized successfully");
   return mod;
}

/* Module shutdown */
E_API int
e_modapi_shutdown(E_Module *m EINA_UNUSED)
{
   Mod *mod = iwd_mod;

   if (!mod) return 0;

   INF("IWD Module shutting down");

   /* Shutdown gadget */
   e_iwd_gadget_shutdown();

   /* Shutdown D-Bus and iwd subsystems */
   iwd_agent_shutdown();
   iwd_dbus_shutdown();
   iwd_network_shutdown();
   iwd_device_shutdown();
   iwd_state_shutdown();

   /* Free configuration */
   _iwd_config_free();
   e_iwd_config_shutdown();

   /* Free module structure */
   E_FREE(mod);
   iwd_mod = NULL;

   /* Unregister logging */
   eina_log_domain_unregister(_e_iwd_log_dom);
   _e_iwd_log_dom = -1;

   INF("IWD Module shutdown complete");
   return 1;
}

/* Module save */
E_API int
e_modapi_save(E_Module *m EINA_UNUSED)
{
   Mod *mod = iwd_mod;

   if (!mod || !mod->conf) return 0;

   DBG("Saving module configuration");
   return e_config_domain_save("module.iwd", mod->conf_edd, mod->conf);
}

/* Configuration management */
void
e_iwd_config_init(void)
{
   Mod *mod = iwd_mod;

   if (!mod) return;

   /* Create configuration descriptor */
   mod->conf_edd = E_CONFIG_DD_NEW("IWD_Config", Config);
   if (!mod->conf_edd)
   {
      ERR("Failed to create config EDD");
      return;
   }

   #undef T
   #undef D
   #define T Config
   #define D mod->conf_edd

   E_CONFIG_VAL(D, T, config_version, INT);
   E_CONFIG_VAL(D, T, auto_connect, UCHAR);
   E_CONFIG_VAL(D, T, show_hidden_networks, UCHAR);
   E_CONFIG_VAL(D, T, signal_refresh_interval, INT);
   E_CONFIG_VAL(D, T, preferred_adapter, STR);

   #undef T
   #undef D
}

void
e_iwd_config_shutdown(void)
{
   Mod *mod = iwd_mod;

   if (!mod) return;

   if (mod->conf_edd)
   {
      E_CONFIG_DD_FREE(mod->conf_edd);
      mod->conf_edd = NULL;
   }
}

static void
_iwd_config_load(void)
{
   Mod *mod = iwd_mod;

   if (!mod || !mod->conf_edd) return;

   /* Load configuration from disk */
   mod->conf = e_config_domain_load("module.iwd", mod->conf_edd);

   if (mod->conf)
   {
      /* Check version */
      if ((mod->conf->config_version >> 16) < MOD_CONFIG_FILE_EPOCH)
      {
         /* Config too old, use defaults */
         WRN("Configuration version too old, using defaults");
         E_FREE(mod->conf);
         mod->conf = NULL;
      }
   }

   /* Create default configuration if needed */
   if (!mod->conf)
   {
      INF("Creating default configuration");
      mod->conf = E_NEW(Config, 1);
      if (mod->conf)
      {
         mod->conf->config_version = MOD_CONFIG_FILE_VERSION;
         mod->conf->auto_connect = EINA_TRUE;
         mod->conf->show_hidden_networks = EINA_FALSE;
         mod->conf->signal_refresh_interval = 5;
         mod->conf->preferred_adapter = NULL;

         /* Save default config */
         e_config_domain_save("module.iwd", mod->conf_edd, mod->conf);
      }
   }
}

static void
_iwd_config_free(void)
{
   Mod *mod = iwd_mod;

   if (!mod || !mod->conf) return;

   if (mod->conf->preferred_adapter)
      eina_stringshare_del(mod->conf->preferred_adapter);

   E_FREE(mod->conf);
   mod->conf = NULL;
}

/* Gadget implementations are in e_mod_gadget.c */
