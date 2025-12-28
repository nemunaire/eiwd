#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include <e.h>
#include <Eina.h>
#include <Eldbus.h>

/* Module version information */
#define MOD_CONFIG_FILE_EPOCH 0x0001
#define MOD_CONFIG_FILE_GENERATION 0x0001
#define MOD_CONFIG_FILE_VERSION \
   ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

/* Forward declarations for iwd types */
typedef struct _IWD_Device IWD_Device;

/* Configuration structure */
typedef struct _Config
{
   int config_version;
   Eina_Bool auto_connect;
   Eina_Bool show_hidden_networks;
   int signal_refresh_interval;
   const char *preferred_adapter;
} Config;

/* Module instance structure (gadget) */
typedef struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object *gadget;
   Evas_Object *icon;
   void *popup; /* E_Gadcon_Popup - void to avoid circular dependency */

   IWD_Device *device;
   Ecore_Timer *update_timer;
} Instance;

/* Global module context */
typedef struct _Mod
{
   E_Module *module;
   E_Config_DD *conf_edd;
   Config *conf;
   Eina_List *instances;

   /* D-Bus connection (will be initialized in Phase 2) */
   Eldbus_Connection *dbus_conn;

   /* Logging domain */
   int log_dom;
} Mod;

/* Global module instance */
extern Mod *iwd_mod;

/* Logging macros */
extern int _e_iwd_log_dom;
#undef DBG
#undef INF
#undef WRN
#undef ERR
#define DBG(...) EINA_LOG_DOM_DBG(_e_iwd_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_e_iwd_log_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_e_iwd_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_e_iwd_log_dom, __VA_ARGS__)

/* Module API functions */
E_API extern E_Module_Api e_modapi;
E_API void *e_modapi_init(E_Module *m);
E_API int e_modapi_shutdown(E_Module *m);
E_API int e_modapi_save(E_Module *m);

/* Configuration functions */
void e_iwd_config_init(void);
void e_iwd_config_shutdown(void);
void e_iwd_config_show(void);

/* Gadget functions */
void e_iwd_gadget_init(void);
void e_iwd_gadget_shutdown(void);

/* Popup functions */
void iwd_popup_new(Instance *inst);
void iwd_popup_del(Instance *inst);

/* UI dialog functions */
#include "ui/wifi_auth.h"
#include "ui/wifi_hidden.h"

/* D-Bus functions */
#include "iwd/iwd_dbus.h"
#include "iwd/iwd_device.h"
#include "iwd/iwd_network.h"
#include "iwd/iwd_agent.h"
#include "iwd/iwd_state.h"

#endif
