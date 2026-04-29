#ifndef E_MOD_POPUP_H
#define E_MOD_POPUP_H

#include <e_gadcon.h>

void e_iwd_popup_install_passphrase_handler(void);
void e_iwd_popup_toggle  (E_Gadcon_Client *gcc);
void e_iwd_popup_close   (void);
void e_iwd_popup_refresh (void);
void e_iwd_popup_shutdown(void);

#endif
