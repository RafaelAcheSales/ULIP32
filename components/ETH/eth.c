#include "eth.h"
#include "lan8720.h"
void start_eth(bool dhcp, char * ip_address, char * gateway, char * netmask, void (* got_ip_callback_set)(void)) {
    eth_start(dhcp, ip_address, gateway, netmask, got_ip_callback_set);
}