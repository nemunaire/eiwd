#include "iwd_network.h"

Iwd_Network *
iwd_network_new(Eldbus_Connection *conn EINA_UNUSED, const char *path)
{
   Iwd_Network *n = calloc(1, sizeof(*n));
   if (path) n->path = strdup(path);
   /* TODO: read Name, Type, KnownNetwork properties */
   return n;
}

void
iwd_network_free(Iwd_Network *n)
{
   if (!n) return;
   free(n->path);
   free(n->ssid);
   free(n);
}

void iwd_network_connect(Iwd_Network *n EINA_UNUSED) { /* TODO: Network.Connect() */ }
void iwd_network_forget (Iwd_Network *n EINA_UNUSED) { /* TODO: KnownNetwork.Forget() */ }
