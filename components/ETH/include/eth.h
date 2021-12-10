
#pragma once
#include "esp_netif.h"

void start_eth(_Bool dhcp, char * ip_address, char * gateway, char * netmask, void (* got_ip_callback_set)(char * ip_address));
void release_eth(void);

void get_eth_ip(esp_netif_ip_info_t *ip_info);