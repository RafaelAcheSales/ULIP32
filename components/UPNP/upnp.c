#include <stdio.h>
#include <string.h>
#include <lwip/sockets.h>
#include <esp_netif.h>

#include "osapi.h"
#include "config2.h"
#include "debug.h"
#include "upnp.h"

#define UPNP_ETH        0
#define UPNP_WIFI       1

#define UPNP_PORT       50000
#define UPNP_REQUESTS   3

#define UPNP_DISCOVERY  "WHOIS"
#define UPNP_REPLY      "IAM"

static char upnp_board[32];
static uint32_t upnp_cnt = 0;
static struct sockaddr_in upnp_addr;
static int upnp_fdes = -1;
TaskHandle_t upnp_handle = NULL;


static void upnp_receive_data(void)
{
    esp_netif_t *iface;
    esp_netif_ip_info_t ip_info;
    struct sockaddr_in addr;
    socklen_t slen;
    char buf[32];
    char cmd[512];
    char host[16];
    char netmask[16];
    char gateway[16];
    const char *mac;
    const char *serial;
    const char *release;
    int netif = -1;
    uint32_t network;
    int size;
    int len;
    char *p;

    slen = sizeof(addr);
    len = recvfrom(upnp_fdes, buf, sizeof(buf) - 1, 0,
                   (struct sockaddr *)&addr, &slen);
    if (len <= 0) return;
    buf[len] = '\0';

    size = strlen(UPNP_DISCOVERY);
    if (len != size) return;
    if (strncmp((char *)buf, UPNP_DISCOVERY, size)) return;

    /* Debounce */
    if ((upnp_cnt++ % UPNP_REQUESTS)) return;

    /* ETH */
    iface = esp_netif_get_handle_from_ifkey("ETH_DEF");
    if (iface) {
        memset(&ip_info, 0, sizeof(ip_info));
        if (esp_netif_get_ip_info(iface, &ip_info) == ESP_OK) {
            network = addr.sin_addr.s_addr & ip_info.netmask.addr;
            if (network == (ip_info.ip.addr & ip_info.netmask.addr))
                netif = UPNP_ETH;
        }
    }
    /* WIFI */
    if (netif == -1) {
        iface = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (iface) {
            memset(&ip_info, 0, sizeof(ip_info));
            if (esp_netif_get_ip_info(iface, &ip_info) == ESP_OK) {
                network = addr.sin_addr.s_addr & ip_info.netmask.addr;
                if (network == (ip_info.ip.addr & ip_info.netmask.addr))
                    netif = UPNP_WIFI;
            }
        }
    }
    if (netif == -1) return;

    esp_ip4addr_ntoa(&ip_info.ip, host, sizeof(host));
    esp_ip4addr_ntoa(&ip_info.netmask, netmask, sizeof(netmask));
    esp_ip4addr_ntoa(&ip_info.gw, gateway, sizeof(gateway));
    mac = CFG_get_ethaddr();
    serial = CFG_get_serialnum();
    release = CFG_get_release();
    // os_info("UPNP", "Host: %s, Netmask: %s, Gateway: %s, MAC: %s, Serial: %s, Release: %s Netif %d",
    //         host, netmask, gateway, mac, serial, release, netif);
    p = cmd;
    size = strlen(UPNP_REPLY);
    memcpy(p, UPNP_REPLY, size);
    p += size;
    /* Network info */
    size = strlen(host) + 1;
    memcpy(p, host, size);
    p += size;
    size = strlen(netmask) + 1;
    memcpy(p, netmask, size);
    p += size;
    size = strlen(gateway) + 1;
    memcpy(p, gateway, size);
    p += size;
    /* Board info */
    size = strlen(upnp_board) + 1;
    memcpy(p, upnp_board, size);
    p += size;
    size = strlen(mac) + 1;
    memcpy(p, mac, size);
    p += size;
    size = strlen(serial) + 1;
    memcpy(p, serial, size);
    p += size;
    size = strlen(release) + 1;
    memcpy(p, release, size);
    p += size;

    size = p - cmd;
    sendto(upnp_fdes, cmd, size, 0, (struct sockaddr *)&addr, slen);
}

static void upnp_task(void *arg)
{
    struct sockaddr_in addr;
    fd_set rfds;
    int rc;

    FD_ZERO(&rfds);
    FD_SET(upnp_fdes, &rfds);
    os_info("UPNP", "current core: %d", xPortGetCoreID());
    while (true) {
        rc = select(upnp_fdes + 1, &rfds, NULL, NULL, NULL);
        if (rc == -1) break;
        if (rc > 0)
            upnp_receive_data();
    }
}

void upnp_init(const char *board)
{
    int rc;

    if (!board) return;

    if (upnp_fdes != -1)
        close(upnp_fdes);
    upnp_fdes = socket(AF_INET, SOCK_DGRAM, 0);
    if (upnp_fdes == -1) return;
    fcntl(upnp_fdes, F_SETFL, O_NONBLOCK);

    /* Configure UDP */
    memset(&upnp_addr, 0, sizeof(upnp_addr));
    upnp_addr.sin_family = AF_INET;
    upnp_addr.sin_port = htons(UPNP_PORT);
    upnp_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(upnp_fdes, (struct sockaddr *)&upnp_addr, sizeof(upnp_addr))) {
        close(upnp_fdes);
        upnp_fdes = -1;
        return;
    }
    rc = xTaskCreatePinnedToCore(upnp_task, "UPNP", 3072, NULL, 1,
                                 &upnp_handle, 1);
    if (rc != pdPASS) {
        close(upnp_fdes);
        upnp_fdes = -1;
        return;
    }

    strncpy(upnp_board, board, sizeof(upnp_board) - 1);
}

void upnp_release(void)
{
    if (upnp_handle) {
        vTaskDelete(upnp_handle);
        upnp_handle = NULL;
    }
    if (upnp_fdes != -1) {
        close(upnp_fdes);
        upnp_fdes = -1;
    }
}
