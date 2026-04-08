#ifndef WIFI_AUTH_H
#define WIFI_AUTH_H

#include <Elementary.h>

typedef void (*Wifi_Auth_Cb)(void *data, const char *passphrase, Eina_Bool remember);

void wifi_auth_prompt(Evas_Object *parent, const char *ssid,
                      Wifi_Auth_Cb cb, void *data);

#endif
