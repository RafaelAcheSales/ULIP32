#pragma once
#include "esp_netif.h"
void wifi_init_softap(bool ap_mode, char * ip, char * netmask, char * gateway, bool dhcp, char * ssid, char * password, uint8_t channel, bool disable, void (* got_ip_callback_set)(void));
void wifi_release();
void get_wifi_ip(bool ap_mode, esp_netif_ip_info_t *info);
void wifi_reset();