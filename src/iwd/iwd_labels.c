#include "iwd_labels.h"

const char *
iwd_state_label(Iwd_State s)
{
   switch (s) {
    case IWD_STATE_OFF:        return "Wi-Fi disabled";
    case IWD_STATE_IDLE:       return "Disconnected";
    case IWD_STATE_SCANNING:   return "Scanning…";
    case IWD_STATE_CONNECTING: return "Connecting…";
    case IWD_STATE_CONNECTED:  return "Connected";
    case IWD_STATE_ERROR:      return "Error";
   }
   return "";
}

const char *
iwd_security_label(Iwd_Security s)
{
   switch (s) {
    case IWD_SEC_OPEN:    return "open";
    case IWD_SEC_PSK:     return "WPA";
    case IWD_SEC_8021X:   return "802.1X";
    case IWD_SEC_WEP:     return "WEP";
    case IWD_SEC_UNKNOWN: return "?";
   }
   return "";
}
