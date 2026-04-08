#include "wifi_status.h"

/* TODO: current connection summary widget (SSID, signal, IP, Disconnect). */

Evas_Object *
wifi_status_add(Evas_Object *parent)
{
   Evas_Object *box = elm_box_add(parent);
   return box;
}

void wifi_status_refresh(Evas_Object *o EINA_UNUSED) { /* TODO */ }
