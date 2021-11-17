#pragma once
void wifi_init_softap(bool ap_mode, char * ip, char * netmask, char * gateway, bool dhcp, char * ssid, char * password, uint8_t channel, bool disable, void (* got_ip_callback_set)(void));
void wifi_release();