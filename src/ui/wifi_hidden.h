#ifndef WIFI_HIDDEN_H
#define WIFI_HIDDEN_H

#include <Elementary.h>

/* Called once with ok=EINA_TRUE + ssid (and optional passphrase, may be ""),
 * or ok=EINA_FALSE on cancel. The dialog destroys itself. */
typedef void (*Wifi_Hidden_Cb)(void *data, const char *ssid,
                               const char *passphrase, Eina_Bool ok);

void wifi_hidden_prompt(Evas_Object *parent, Wifi_Hidden_Cb cb, void *data);

#endif
