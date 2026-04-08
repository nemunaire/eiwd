#include "wifi_list.h"

/* TODO: Genlist of networks, sorted (known first, then signal desc),
 *       with security icon, signal bars, and click → connect/auth flow. */

Evas_Object *
wifi_list_add(Evas_Object *parent)
{
   Evas_Object *gl = elm_genlist_add(parent);
   return gl;
}

void wifi_list_refresh(Evas_Object *list EINA_UNUSED) { /* TODO */ }
