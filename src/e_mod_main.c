#include "e_mod_main.h"
#include "iwd/iwd_manager.h"
#include "e_mod_gadget.h"
#include "e_mod_config.h"

E_Iwd_Module *e_iwd = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "iwd" };

EAPI void *
e_modapi_init(E_Module *m)
{
   e_iwd = E_NEW(E_Iwd_Module, 1);
   e_iwd->module = m;

   if (!eldbus_init())
     {
        E_FREE(e_iwd);
        return NULL;
     }

   e_iwd->conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);
   if (!e_iwd->conn)
     {
        eldbus_shutdown();
        E_FREE(e_iwd);
        return NULL;
     }

   e_iwd_config_load();
   e_iwd->manager = iwd_manager_new(e_iwd->conn);
   e_iwd_gadget_init();

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m EINA_UNUSED)
{
   if (!e_iwd) return 1;

   e_iwd_gadget_shutdown();
   if (e_iwd->manager) iwd_manager_free(e_iwd->manager);
   e_iwd_config_save();
   if (e_iwd->conn) eldbus_connection_unref(e_iwd->conn);
   eldbus_shutdown();
   E_FREE(e_iwd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m EINA_UNUSED)
{
   e_iwd_config_save();
   return 1;
}
