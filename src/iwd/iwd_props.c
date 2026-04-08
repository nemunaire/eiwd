#include "iwd_props.h"
#include <string.h>

void
iwd_props_for_each(Eldbus_Message_Iter *dict, Iwd_Prop_Cb cb, void *data)
{
   if (!dict || !cb) return;
   Eldbus_Message_Iter *entry;
   while (eldbus_message_iter_get_and_next(dict, 'e', &entry))
     {
        const char *key;
        Eldbus_Message_Iter *variant;
        if (!eldbus_message_iter_arguments_get(entry, "sv", &key, &variant))
          continue;
        cb(data, key, variant);
     }
}

char *
iwd_props_str_dup(Eldbus_Message_Iter *variant)
{
   if (!variant) return NULL;
   const char *sig = eldbus_message_iter_signature_get(variant);
   if (!sig || (sig[0] != 's' && sig[0] != 'o')) { free((void *)sig); return NULL; }
   const char *s = NULL;
   char want[2] = { sig[0], 0 };
   free((void *)sig);
   if (!eldbus_message_iter_arguments_get(variant, want, &s)) return NULL;
   return s ? strdup(s) : NULL;
}

Eina_Bool
iwd_props_bool(Eldbus_Message_Iter *variant)
{
   Eina_Bool b = EINA_FALSE;
   if (variant) eldbus_message_iter_arguments_get(variant, "b", &b);
   return b;
}
