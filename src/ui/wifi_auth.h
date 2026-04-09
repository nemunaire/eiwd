#ifndef WIFI_AUTH_H
#define WIFI_AUTH_H

#include <Elementary.h>

typedef void (*Wifi_Auth_Cb)(void *data, const char *passphrase, Eina_Bool ok);

/* Show a modal passphrase dialog. security is an optional human label
 * (e.g. "WPA", "WEP") shown alongside the SSID; pass NULL to omit it.
 * cb is called exactly once with ok=EINA_TRUE + passphrase, or
 * ok=EINA_FALSE on cancel. The dialog destroys itself. */
/* Returns the popup widget so the caller can dismiss it externally
 * (e.g. on Agent.Cancel from iwd). The widget self-deletes on user
 * action; treat the returned pointer as a weak reference. */
Evas_Object *wifi_auth_prompt(Evas_Object *parent, const char *ssid,
                              const char *security,
                              Wifi_Auth_Cb cb, void *data);

#endif
