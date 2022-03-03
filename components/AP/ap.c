/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "ap.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#define MAX_STA_CONN 5
#define ESP_MAXIMUM_RETRY 5
#define MAX_AP_LIST_SIZE 20
static wifi_ap_record_t ap_list[MAX_AP_LIST_SIZE];
static uint16_t ap_list_size = MAX_AP_LIST_SIZE;

static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
const char * nvs_partition = "nvs";
static const char *TAG = "wifi softAP"; 
static esp_netif_t *ap_netif;
static esp_netif_t *sta_netif;
static int s_retry_num = 0;
static bool initialized = false;
static void (* got_ip_callback)(void) = NULL;
static void (* wifi_scan_callback)(uint16_t *size, wifi_ap_record_t *) = NULL;
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        if (got_ip_callback != NULL) {
            (*(got_ip_callback))();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        if (wifi_scan_callback != NULL) {
           
            // uint16_t size = 20;
            esp_wifi_scan_get_ap_records(&ap_list_size, &ap_list);
            (*(wifi_scan_callback))(&ap_list_size,&ap_list);
        }
    }
}
void wifi_station_scan(wifi_scan_config_t *scanConf, void (* wifi_scan_callback_set(wifi_ap_record_t *))) {
    wifi_scan_callback = wifi_scan_callback_set;
    ESP_ERROR_CHECK(esp_wifi_scan_start(scanConf, true));
}
void wifi_init_softap(bool ap_mode, char * ip, char * netmask, char * gateway, bool dhcp, char * ssid, char * password, uint8_t channel, bool disable, void (* got_ip_callback_set)(void))
{
    ESP_ERROR_CHECK(nvs_flash_init());
    //already done on ETH module - mentira eh aqui msm kkk te enganei
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    if (disable)
        return;
    got_ip_callback = got_ip_callback_set;
    if (ap_mode) {
        ap_netif = esp_netif_create_default_wifi_ap();
        
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        // cfg.nvs_enable = 1;
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        esp_event_handler_instance_t instance_any_id;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_any_id));

        wifi_config_t wifi_config = {
            .ap = {
                .ssid_len = strlen(ssid),
                .channel = channel,
                .max_connection = MAX_STA_CONN,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK
            },
        };
        if (strlen(password) == 0) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }
        strcpy((char *)wifi_config.ap.ssid, ssid);
        strcpy((char *)wifi_config.ap.password, password);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

        esp_netif_ip_info_t info_t;
        memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
        if (ap_netif)
        {
            ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));
            info_t.ip.addr = esp_ip4addr_aton((const char *)ip);
            info_t.netmask.addr = esp_ip4addr_aton((const char *)netmask);
            info_t.gw.addr = esp_ip4addr_aton((const char *)gateway);
            esp_netif_set_ip_info(ap_netif, &info_t);
            ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));
        }


        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
                ssid, password, channel);

    } else {
            s_wifi_event_group = xEventGroupCreate();

            // ESP_ERROR_CHECK(esp_netif_init());

            // ESP_ERROR_CHECK(esp_event_loop_create_default());
            sta_netif = esp_netif_create_default_wifi_sta();

            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));

            esp_event_handler_instance_t instance_any_id;
            esp_event_handler_instance_t instance_got_ip;
            ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                                ESP_EVENT_ANY_ID,
                                                                &wifi_event_handler,
                                                                NULL,
                                                                &instance_any_id));
            ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                                IP_EVENT_STA_GOT_IP,
                                                                &wifi_event_handler,
                                                                NULL,
                                                                &instance_got_ip));

            wifi_config_t wifi_config = {
                .sta = {
                    .channel = channel,
                    
                    /* Setting a password implies station will connect to all security modes including WEP/WPA.
                    * However these modes are deprecated and not advisable to be used. Incase your Access point
                    * doesn't support WPA2, these mode can be enabled by commenting below line */
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,

                    .pmf_cfg = {
                        .capable = true,
                        .required = false
                    },
                },
            };
            strcpy((char *)wifi_config.sta.ssid, ssid);
            strcpy((char *)wifi_config.sta.password, password);
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
            ESP_ERROR_CHECK(esp_wifi_start());

            if (!dhcp) {
                ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta_netif));
                // vTaskDelay(200);
                esp_netif_ip_info_t info_t;
                memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
                esp_netif_str_to_ip4((const char *)ip, (esp_ip4_addr_t *) &info_t.ip.addr);
                esp_netif_str_to_ip4((const char *)gateway,(esp_ip4_addr_t *) &info_t.gw.addr);
                esp_netif_str_to_ip4((const char *)netmask,(esp_ip4_addr_t *) &info_t.netmask.addr);
                esp_netif_set_ip_info(sta_netif, &info_t);
            }


            ESP_LOGI(TAG, "wifi_init_sta finished.");

            /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
            * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                    pdFALSE,
                    pdFALSE,
                    portMAX_DELAY);

            /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
            * happened. */
            if (bits & WIFI_CONNECTED_BIT) {
                ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                        ssid, password);
            } else if (bits & WIFI_FAIL_BIT) {
                ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                        ssid, password);
            } else {
                ESP_LOGE(TAG, "UNEXPECTED EVENT");
            }

            /* The event will not be processed after unregister */
            // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
            // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
            // vEventGroupDelete(s_wifi_event_group);
    }
    initialized = true;
}
void wifi_reset() {
    ESP_ERROR_CHECK(esp_wifi_restore());
}

void wifi_release(){
    if (initialized) {
        ESP_ERROR_CHECK(esp_wifi_stop());
    }
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    // ESP_ERROR_CHECK(esp_netif_deinit());
    initialized = false;
}
void get_wifi_ip(bool ap_mode, esp_netif_ip_info_t *info){
    if (ap_mode && ap_netif != NULL) {
        esp_netif_get_ip_info(ap_netif, info);
    } else if (!ap_mode && sta_netif != NULL) {
        esp_netif_get_ip_info(sta_netif, info);
    }
}


