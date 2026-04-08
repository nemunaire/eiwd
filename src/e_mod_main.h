#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include <e.h>
#include <Eldbus.h>
#include <Elementary.h>

typedef struct _E_Iwd_Module E_Iwd_Module;

struct _E_Iwd_Module
{
   E_Module     *module;
   Eldbus_Connection *conn;
   void         *manager;   /* Iwd_Manager * */
   void         *gadget;    /* gadget instance */
   void         *config;    /* E_Config_Dialog data */
};

extern E_Iwd_Module *e_iwd;

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

#endif
