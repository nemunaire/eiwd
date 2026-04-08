#ifndef IWD_PROPS_H
#define IWD_PROPS_H

#include <Eldbus.h>
#include <Eina.h>

/* Iterate an a{sv} message iter, calling cb for every entry. */
typedef void (*Iwd_Prop_Cb)(void *data, const char *key, Eldbus_Message_Iter *value);
void iwd_props_for_each(Eldbus_Message_Iter *dict, Iwd_Prop_Cb cb, void *data);

/* Extract a string from a variant iter ("s" or "o"). Returns strdup'd copy. */
char *iwd_props_str_dup(Eldbus_Message_Iter *variant);
Eina_Bool iwd_props_bool(Eldbus_Message_Iter *variant);

#endif
