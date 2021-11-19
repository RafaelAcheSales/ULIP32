

void start_eth(_Bool dhcp, char * ip_address, char * gateway, char * netmask, void (* got_ip_callback_set)(void));
void release_eth(void);