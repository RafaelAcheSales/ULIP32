/* config.c
*
* Copyright (c) 2018, uTech Tecnologia Ltda <rogerio@utech.com.br>
* All rights reserved.
*
*/
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "stdbool.h"
#include "esp_log.h"
#include "esp_system.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
const char * PARTITION_NAME = "storage";
const char * NAMESPACE = "nvs_nameespace";
typedef struct {
    uint32_t cfg_holder;
    uint8_t ethaddr[18];
    uint8_t serialnum[11];
    char board[8];
    char release[12];
} SYSENV;

typedef struct {
    uint32_t cfg_holder;
    char wifi_ssid[32];
    char wifi_passwd[64];
    bool dhcp;
    char ip_address[16];
    char netmask[16];
    char gateway[16];
    char ota_url[80];
    char web_user[20];
    char web_passwd[20];
    bool rfid_enable;
    uint32_t rfid_timeout;
    bool qrcode_enable;
    uint32_t qrcode_timeout;
    uint8_t control_mode;
    uint32_t control_timeout;
    bool control_external;
    char control_url[256];
    bool rs485_enable;
    uint8_t rs485_hwaddr;
    uint8_t rs485_server_hwaddr;
    char server_ip[200];
    uint16_t server_port;
    char server_user[20];
    char server_passwd[20];
    char server_url[200];
    uint8_t debug;
    uint8_t debug_level;
    char debug_host[16];
    uint16_t debug_port;
    char hostname[20];
    char dns[16];
    char ntp[128];
    bool wifi_disable;
    int8_t timezone;
    bool standalone;
    bool dst;
    char dst_date[32];
    bool ap_mode;
    bool rf433_enable;
    bool hotspot;
    bool qrcode_config;
    bool ssid_hidden;
    bool fingerprint_enable;
    uint32_t fingerprint_timeout;
    uint8_t fingerprint_security;
    bool user_auth;
    char latitude[16];
    char longitude[16];
    bool rfid_nfc;
    bool qrcode_led;
    bool breakin_alarm;
    uint32_t control_acc_timeout;
    uint8_t wifi_channel;
    uint16_t wifi_beacon_interval;
    uint32_t rfid_panic_timeout;
    uint8_t rfid_format;
    uint32_t qrcode_panic_timeout;
    uint8_t fingerprint_identify_retries;
    uint8_t server_retries;
    bool ddns;
    char ddns_domain[256];
    char ddns_user[64];
    char ddns_passwd[64];
    bool rf433_rc;
    uint16_t rf433_hc;
    bool rf433_alarm;
    bool rf433_bc;
    bool button_enable;
    uint8_t rf433_bp;
    uint32_t rf433_panic_timeout;
    bool dht_enable;
    uint32_t dht_timeout;
    int16_t dht_temp_upper;
    int16_t dht_temp_lower;
    int16_t dht_rh_upper;
    int16_t dht_rh_lower;
    bool dht_relay;
    bool dht_alarm;
    char control_desc[128];
    bool mq2_enable;
    uint32_t mq2_timeout;
    uint16_t mq2_limit;
    bool mq2_relay;
    bool mq2_alarm;
    bool pir_enable;
    bool pir_chime;
    bool pir_relay;
    bool pir_alarm;
    bool lora_enable;
    uint8_t lora_channel;
    uint16_t lora_baudrate;
    uint8_t lora_addr;
    uint8_t lora_server_addr;
    uint8_t sensor_type;
    uint16_t sensor_flow;
    uint32_t sensor_cycles;
    uint32_t sensor_limit;
    bool sensor_relay;
    bool sensor_alarm;
    uint32_t pir_timeout;
    bool temt_enable;
    uint32_t temt_timeout;
    uint16_t temt_upper;
    uint16_t temt_lower;
    bool temt_relay;
    bool temt_alarm;
    uint8_t relay_status;
    uint32_t pow_vol_cal;
    uint32_t pow_vol_upper;
    uint32_t pow_vol_lower;
    uint32_t pow_cur_cal;
    uint32_t pow_cur_upper;
    uint32_t pow_cur_lower;
    uint32_t pow_pwr_upper;
    uint32_t pow_pwr_lower;
    bool pow_relay;
    uint16_t pow_alarm_time;
    uint16_t pow_relay_timeout;
    uint64_t energy_daily;
    uint64_t energy_daily_last;
    uint64_t energy_monthly;
    uint64_t energy_monthly_last;
    uint64_t energy_total;
    uint16_t pow_interval;
    uint8_t pow_day;
    uint32_t energy_daily_limit;
    uint32_t energy_monthly_limit;
    uint32_t energy_total_limit;
    bool qrcode_dynamic;
    uint16_t qrcode_validity;
    uint8_t rfid_retries;
    uint32_t rtc_time;
    uint8_t pow_relay_ext;
    uint64_t energy_month[12];
    uint32_t control_doublepass_timeout;
    uint32_t rtc_shutdown;
    char schedule[CFG_SCHEDULE][34];
    uint32_t pow_nrg_cal;
    bool cli_enable;
    uint32_t cli_timeout;
    uint16_t cli_range;
    uint16_t cli_upper;
    uint16_t cli_lower;
    bool cli_relay;
    bool cli_alarm;
    uint8_t rf433_ba;
    uint16_t cli_cal;
} SYSCFG;

typedef struct {
    uint8_t flag;
    uint8_t pad[3];
} SAVE_FLAG;

static SYSENV sysEnv;
static SYSCFG sysCfg;
static SAVE_FLAG saveFlag;
size_t sysEnvSize = sizeof(SYSENV);
size_t sysCfgSize = sizeof(SYSCFG);
size_t saveFlagSize = sizeof(SAVE_FLAG);
// static portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
const char *CFG_version = CFG_RELEASE;


static void
CFG_ENV_Save(void)
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open_from_partition(PARTITION_NAME, NAMESPACE, NVS_READWRITE, &my_handle));
    // ESP_LOGI("CONFIG", "cfg_env_save");
    // ESP_ERROR_CHECK(nvs_erase_key(my_handle, SYSENV_KEY));
    // ESP_ERROR_CHECK(nvs_commit(my_handle));
    ESP_ERROR_CHECK(nvs_set_blob(my_handle, SYSENV_KEY,&sysEnv, sizeof(SYSENV)));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    // ESP_LOGI("CONFIG", "sysenv erase and write");
    nvs_close(my_handle);
}

void
CFG_ENV_Default(void)
{
    // ESP_LOGI("CONFIG", "CFG_ENV_DEFAULOT");
    memset(&sysCfg, 0, sizeof(sysCfg));
    strcpy((char *)sysEnv.ethaddr, CFG_ETHADDR);
    strcpy((char *)sysEnv.serialnum, CFG_SERIALNUM);
    strcpy(sysEnv.board, CFG_BOARD);
    strcpy(sysEnv.release, CFG_RELEASE);
    CFG_ENV_Save();
}

static void
CFG_ENV_Init(void)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_flash_init_partition(PARTITION_NAME);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase_partition(PARTITION_NAME));
        err = nvs_flash_init_partition(PARTITION_NAME);
    }
    ESP_ERROR_CHECK( err );
    ESP_ERROR_CHECK(nvs_open_from_partition(PARTITION_NAME, NAMESPACE ,NVS_READWRITE, &my_handle));
    // ESP_LOGI("CONFIG", "handler is %d", my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_blob(my_handle, SYSENV_KEY, &sysEnv, &sysEnvSize));
    // ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);

    if (sysEnv.cfg_holder == CFG_HOLDER) {
        /* Update environment */
        if (strcmp(sysEnv.release, CFG_version)) {
            strcpy(sysEnv.release, CFG_version);
            CFG_ENV_Save();
        }
    } else {
        // ESP_LOGI("CONFIG", "default env");
        /* Default environment */
        CFG_ENV_Default();
    }
}

void
CFG_Default(void)
{
    ESP_LOGI("CONFIG", "Use default configuration");

    memset(&sysCfg, 0, sizeof(sysCfg));
    sysCfg.cfg_holder = CFG_HOLDER;
    strcpy(sysCfg.wifi_ssid, CFG_WIFI_SSID);
    strcpy(sysCfg.wifi_passwd, CFG_WIFI_PASSWD);
    sysCfg.dhcp = true;
    strcpy(sysCfg.ip_address, CFG_IP_ADDRESS);
    strcpy(sysCfg.netmask, CFG_NETMASK);
    strcpy(sysCfg.gateway, CFG_GATEWAY);
    strcpy(sysCfg.ota_url, CFG_OTA_URL);
    strcpy(sysCfg.web_user, CFG_WEB_USER);
    strcpy(sysCfg.web_passwd, CFG_WEB_PASSWD);
    strcpy(sysCfg.ntp, CFG_NTP);
    sysCfg.rfid_enable = true;
    sysCfg.rfid_timeout = CFG_RFID_TIMEOUT;
    sysCfg.rfid_retries = 0;
    sysCfg.qrcode_enable = true;
    sysCfg.qrcode_timeout = CFG_QRCODE_TIMEOUT;
    sysCfg.control_timeout = CFG_CONTROL_TIMEOUT;
    sysCfg.rs485_enable = false;
    sysCfg.wifi_disable = false;
    sysCfg.server_port = 80;
    sysCfg.standalone = true;
    sysCfg.timezone = CFG_TIMEZONE;
    sysCfg.dst = false;
    strcpy(sysCfg.dst_date, "10/3/0 2/3/0");
    strcpy(sysCfg.hostname, CFG_HOSTNAME);
    sysCfg.rf433_enable = true;
    sysCfg.hotspot = true;
    sysCfg.qrcode_config = true;
    sysCfg.ssid_hidden = false;
    sysCfg.fingerprint_enable = true;
    sysCfg.fingerprint_timeout = CFG_FPM_TIMEOUT;
    sysCfg.fingerprint_security = CFG_FPM_SECURITY;
    sysCfg.user_auth = true;
    sysCfg.rfid_nfc = false;
    sysCfg.rfid_format = CFG_RFID_FORMAT;
    sysCfg.qrcode_led = true;
    sysCfg.breakin_alarm = false;
    sysCfg.control_acc_timeout = CFG_CONTROL_ACC_TIMEOUT;
    sysCfg.wifi_channel = 1;
    sysCfg.wifi_beacon_interval = 100;
    sysCfg.fingerprint_identify_retries = 2;
    sysCfg.rf433_rc = false;
    sysCfg.rf433_hc = CFG_RF433_HC;
    sysCfg.rf433_alarm = false;
    sysCfg.rf433_bc = true;
    sysCfg.button_enable = false;
    sysCfg.dht_enable = true;
    sysCfg.dht_timeout = CFG_DHT_TIMEOUT;
    sysCfg.dht_temp_upper = -128;
    sysCfg.dht_temp_lower = -128;
    sysCfg.dht_rh_upper = -128;
    sysCfg.dht_rh_lower = -128;
    sysCfg.dht_relay = false;
    sysCfg.dht_alarm = true;
    sysCfg.mq2_enable = true;
    sysCfg.mq2_timeout = CFG_MQ2_TIMEOUT;
    sysCfg.mq2_limit = 0;
    sysCfg.mq2_relay = false;
    sysCfg.mq2_alarm = true;
    sysCfg.pir_enable = false;
    sysCfg.pir_chime = false;
    sysCfg.pir_relay = false;
    sysCfg.pir_alarm = false;
    sysCfg.lora_enable = false;
    sysCfg.lora_channel = CFG_LORA_CHANNEL;
    sysCfg.lora_baudrate = CFG_LORA_BAUDRATE;
    sysCfg.sensor_type = CFG_SENSOR_NORMAL;
    sysCfg.sensor_flow = 0;
    sysCfg.sensor_cycles = 0;
    sysCfg.sensor_limit = 0;
    sysCfg.sensor_relay = false;
    sysCfg.sensor_alarm = true;
    sysCfg.pir_timeout = CFG_PIR_TIMEOUT;
    sysCfg.temt_enable = true;
    sysCfg.temt_timeout = CFG_TEMT_TIMEOUT;
    sysCfg.temt_upper = 0;
    sysCfg.temt_lower = 0;
    sysCfg.temt_relay = false;
    sysCfg.temt_alarm = true;
    sysCfg.pow_alarm_time = 1;
    sysCfg.pow_relay_timeout = CFG_POW_RELAY_TIMEOUT;
    sysCfg.pow_interval = CFG_POW_INTERVAL;
    sysCfg.pow_day = 1;
    sysCfg.qrcode_dynamic = false;
    sysCfg.qrcode_validity = CFG_QRCODE_VALIDITY;
    sysCfg.rtc_shutdown = CFG_RTC_SHUTDOWN;
    sysCfg.cli_enable = false;
    sysCfg.cli_timeout = CFG_LOOP_TIMEOUT;
    sysCfg.cli_range = 1;
    sysCfg.cli_upper = 0;
    sysCfg.cli_lower = 0;
    sysCfg.cli_relay = false;
    sysCfg.cli_alarm = false;
    sysCfg.cli_cal = 0;

    CFG_Save();
}
void CFG_reset_all_flash(void) {
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open_from_partition(PARTITION_NAME, NAMESPACE, NVS_READWRITE, &my_handle));
    // ESP_ERROR_CHECK(nvs_erase_all(my_handle));
    ESP_ERROR_CHECK(nvs_flash_erase_partition(PARTITION_NAME));
}
uint16_t CFG_Get_blobs(void) {
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open_from_partition(PARTITION_NAME, NAMESPACE, NVS_READWRITE, &my_handle));
    SYSCFG test_config;
    
    ESP_ERROR_CHECK(nvs_get_blob(my_handle,SYSCFG_KEY, &test_config, &sysCfgSize));
    return test_config.server_port;
}

void
CFG_Save(void)
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open_from_partition(PARTITION_NAME, NAMESPACE, NVS_READWRITE, &my_handle));
    ESP_LOGI("CONFIG", "Save configuration to flash ...");

    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_blob(my_handle, "saveflag", &saveFlag, &saveFlagSize));
    // ESP_LOGI("CONFIG", "read save flag: %d", saveFlag.flag);

    saveFlag.flag = (saveFlag.flag == 0) ? 1 : 0;

    ESP_ERROR_CHECK(nvs_set_blob(my_handle, SYSENV_KEY, &sysEnv, sizeof(SYSENV)));
    // ESP_LOGI("CONFIG", "write sysenv");
    ESP_ERROR_CHECK(nvs_set_blob(my_handle, SYSCFG_KEY, &sysCfg, sizeof(SYSCFG)));
    // ESP_LOGI("CONFIG", "write syscfg");
    ESP_ERROR_CHECK(nvs_set_blob(my_handle, SAVEFLAG_KEY, &saveFlag, sizeof(SAVE_FLAG)));
    // ESP_LOGI("CONFIG", "write save flag");
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
}

void
CFG_Load(void)
{
    // ESP_ERROR_CHECK(nvs_flash_erase_partition(PARTITION_NAME));
    ESP_LOGI("CONFIG", "Load configuration from flash ...");

    CFG_ENV_Init();

    nvs_handle_t load_handler;
    ESP_ERROR_CHECK(nvs_open_from_partition(PARTITION_NAME, NAMESPACE,NVS_READWRITE, &load_handler));
    // ESP_LOGI("CONFIG", "handler is %d", load_handler);

    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_blob(load_handler, SAVEFLAG_KEY, &saveFlag, &saveFlagSize));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_blob(load_handler, SYSCFG_KEY, &sysCfg, &sysCfgSize));

    // ESP_LOGI("CONFIG", "holder %x, port %d", sysCfg.cfg_holder, sysCfg.server_port);
    nvs_close(load_handler);
    // spi_flash_read((CFG_LOCATION + 2) * SPI_FLASH_SEC_SIZE, (uint32_t *)&saveFlag, sizeof(SAVE_FLAG));
    // spi_flash_read((CFG_LOCATION + saveFlag.flag) * SPI_FLASH_SEC_SIZE, (uint32_t *)&sysCfg, sizeof(SYSCFG));
    // taskEXIT_CRITICAL(&myMutex);
    if (sysCfg.cfg_holder != CFG_HOLDER)
        CFG_Default();
}

const char *CFG_get_ethaddr(void)
{
    return (char *)sysEnv.ethaddr;
}

const char *CFG_get_serialnum(void)
{
    return (char *)sysEnv.serialnum;
}

const char *CFG_get_board(void)
{
    return sysEnv.board;
}

const char *CFG_get_release(void)
{
    return sysEnv.release;
}

void CFG_set_wifi_ssid(const char *ssid)
{
    memset(sysCfg.wifi_ssid, 0, sizeof(sysCfg.wifi_ssid));
    if (ssid)
        strcpy(sysCfg.wifi_ssid, ssid);
}

const char *CFG_get_wifi_ssid(void)
{
    return sysCfg.wifi_ssid;
}

void CFG_set_wifi_passwd(const char *passwd)
{
    memset(sysCfg.wifi_passwd, 0, sizeof(sysCfg.wifi_passwd));
    if (passwd)
        strcpy(sysCfg.wifi_passwd, passwd);
}

const char *CFG_get_wifi_passwd(void)
{
    return sysCfg.wifi_passwd;
}

void CFG_set_wifi_channel(uint8_t channel)
{
    sysCfg.wifi_channel = channel;
}

uint8_t CFG_get_wifi_channel(void)
{
    return sysCfg.wifi_channel;
}

void CFG_set_wifi_beacon_interval(uint16_t interval)
{
    sysCfg.wifi_beacon_interval = interval;
}

uint16_t CFG_get_wifi_beacon_interval(void)
{
    return sysCfg.wifi_beacon_interval;
}

void CFG_set_dhcp(bool dhcp)
{
    sysCfg.dhcp = dhcp;
}

bool CFG_get_dhcp(void)
{
    return sysCfg.dhcp;
}

void CFG_set_ip_address(const char *ip)
{
    memset(sysCfg.ip_address, 0, sizeof(sysCfg.ip_address));
    if (ip)
        strcpy(sysCfg.ip_address, ip);
}

const char *CFG_get_ip_address(void)
{
    return sysCfg.ip_address;
}

void CFG_set_netmask(const char *netmask)
{
    memset(sysCfg.netmask, 0, sizeof(sysCfg.netmask));
    if (netmask)
        strcpy(sysCfg.netmask, netmask);
}

const char *CFG_get_netmask(void)
{
    return sysCfg.netmask;
}

void CFG_set_gateway(const char *gateway)
{
    memset(sysCfg.gateway, 0, sizeof(sysCfg.gateway));
    if (gateway)
        strcpy(sysCfg.gateway, gateway);
}

const char *CFG_get_gateway(void)
{
    return sysCfg.gateway;
}

void CFG_set_hostname(const char *hostname)
{
    memset(sysCfg.hostname, 0, sizeof(sysCfg.hostname));
    if (hostname)
        strcpy(sysCfg.hostname, hostname);
}

const char *CFG_get_hostname(void)
{
    if (*sysCfg.hostname == '\0')
        return NULL;

    return sysCfg.hostname;
}

void CFG_set_dns(const char *dns)
{
    memset(sysCfg.dns, 0, sizeof(sysCfg.dns));
    if (dns)
        strcpy(sysCfg.dns, dns);
}

const char *CFG_get_dns(void)
{
    if (*sysCfg.dns == '\0')
        return NULL;

    return sysCfg.dns;
}

void CFG_set_ntp(const char *ntp)
{
    memset(sysCfg.ntp, 0, sizeof(sysCfg.ntp));
    if (ntp)
        strcpy(sysCfg.ntp, ntp);
}

const char *CFG_get_ntp(void)
{
    if (*sysCfg.ntp == '\0')
        return NULL;

    return sysCfg.ntp;
}

void CFG_set_ota_url(const char *url)
{
    memset(sysCfg.ota_url, 0, sizeof(sysCfg.ota_url));
    if (url)
        strcpy(sysCfg.ota_url, url);
}

const char *CFG_get_ota_url(void)
{
    if (*sysCfg.ota_url == '\0')
        return NULL;

    return sysCfg.ota_url;
}

void CFG_set_web_user(const char *user)
{
    memset(sysCfg.web_user, 0, sizeof(sysCfg.web_user));
    if (user)
        strcpy(sysCfg.web_user, user);
}

const char *CFG_get_web_user(void)
{
    if (*sysCfg.web_user == '\0')
        return NULL;

    return sysCfg.web_user;
}

void CFG_set_web_passwd(const char *passwd)
{
    memset(sysCfg.web_passwd, 0, sizeof(sysCfg.web_passwd));
    if (passwd)
        strcpy(sysCfg.web_passwd, passwd);
}

const char *CFG_get_web_passwd(void)
{
    if (*sysCfg.web_passwd == '\0')
        return NULL;

    return sysCfg.web_passwd;
}

void CFG_set_rfid_enable(bool enable)
{
    sysCfg.rfid_enable = enable;
}

bool CFG_get_rfid_enable(void)
{
    return sysCfg.rfid_enable;
}

void CFG_set_rfid_timeout(uint32_t timeout)
{
    sysCfg.rfid_timeout = timeout;
}

uint32_t CFG_get_rfid_timeout(void)
{
    return sysCfg.rfid_timeout;
}

void CFG_set_rfid_retries(uint8_t retries)
{
    sysCfg.rfid_retries = retries;
}

uint8_t CFG_get_rfid_retries(void)
{
    return sysCfg.rfid_retries;
}

void CFG_set_rfid_nfc(bool nfc)
{
    sysCfg.rfid_nfc = nfc;
}

bool CFG_get_rfid_nfc(void)
{
    return sysCfg.rfid_nfc;
}

void CFG_set_rfid_panic_timeout(uint32_t timeout)
{
    sysCfg.rfid_panic_timeout = timeout;
}

uint32_t CFG_get_rfid_panic_timeout(void)
{
    return sysCfg.rfid_panic_timeout;
}

void CFG_set_rfid_format(uint8_t format)
{
    sysCfg.rfid_format = format;
}

uint8_t CFG_get_rfid_format(void)
{
    return sysCfg.rfid_format;
}

void CFG_set_qrcode_enable(bool enable)
{
    sysCfg.qrcode_enable = enable;
}

bool CFG_get_qrcode_enable(void)
{
    return sysCfg.qrcode_enable;
}

void CFG_set_qrcode_timeout(uint32_t timeout)
{
    sysCfg.qrcode_timeout = timeout;
}

uint32_t CFG_get_qrcode_timeout(void)
{
    return sysCfg.qrcode_timeout;
}

void CFG_set_qrcode_panic_timeout(uint32_t timeout)
{
    sysCfg.qrcode_panic_timeout = timeout;
}

uint32_t CFG_get_qrcode_panic_timeout(void)
{
    return sysCfg.qrcode_panic_timeout;
}

void CFG_set_qrcode_led(bool led)
{
    sysCfg.qrcode_led = led;
}

bool CFG_get_qrcode_led(void)
{
    return sysCfg.qrcode_led;
}

void CFG_set_control_external(bool external)
{
    sysCfg.control_external = external;
}

bool CFG_get_control_external(void)
{
    return sysCfg.control_external;
}

void CFG_set_control_url(const char *url)
{
    memset(sysCfg.control_url, 0, sizeof(sysCfg.control_url));
    if (url)
        strncpy(sysCfg.control_url, url,
                   sizeof(sysCfg.control_url) - 1);
}

const char *CFG_get_control_url(void)
{
    if (*sysCfg.control_url == '\0')
        return NULL;

    return sysCfg.control_url;
}

void CFG_set_control_mode(uint8_t mode)
{
    sysCfg.control_mode = mode;
}

uint8_t CFG_get_control_mode(void)
{
    return sysCfg.control_mode;
}

void CFG_set_control_timeout(uint32_t timeout)
{
    sysCfg.control_timeout = timeout;
}

uint32_t CFG_get_control_timeout(void)
{
    return sysCfg.control_timeout;
}

void CFG_set_rs485_enable(bool enable)
{
    sysCfg.rs485_enable = enable;
}

bool CFG_get_rs485_enable(void)
{
    return sysCfg.rs485_enable;
}

void CFG_set_rs485_hwaddr(uint8_t addr)
{
    sysCfg.rs485_hwaddr = addr;
}

uint8_t CFG_get_rs485_hwaddr(void)
{
    return sysCfg.rs485_hwaddr;
}

void CFG_set_rs485_server_hwaddr(uint8_t addr)
{
    sysCfg.rs485_server_hwaddr = addr;
}

uint8_t CFG_get_rs485_server_hwaddr(void)
{
    return sysCfg.rs485_server_hwaddr;
}

void CFG_set_server_ip(const char *ip)
{
    memset(sysCfg.server_ip, 0, sizeof(sysCfg.server_ip));
    if (ip)
        strcpy(sysCfg.server_ip, ip);
}

const char *CFG_get_server_ip(void)
{
    if (*sysCfg.server_ip == '\0')
        return NULL;

    return sysCfg.server_ip;
}

void CFG_set_server_port(uint16_t port)
{
    sysCfg.server_port = port;
}

uint16_t CFG_get_server_port(void)
{
    return sysCfg.server_port;
}

void CFG_set_server_user(const char *user)
{
    memset(sysCfg.server_user, 0, sizeof(sysCfg.server_user));
    if (user)
        strcpy(sysCfg.server_user, user);
}

const char *CFG_get_server_user(void)
{
    if (*sysCfg.server_user == '\0')
        return NULL;

    return sysCfg.server_user;
}

void CFG_set_server_passwd(const char *passwd)
{
    memset(sysCfg.server_passwd, 0, sizeof(sysCfg.server_passwd));
    if (passwd)
        strcpy(sysCfg.server_passwd, passwd);
}

const char *CFG_get_server_passwd(void)
{
    if (*sysCfg.server_passwd == '\0')
        return NULL;

    return sysCfg.server_passwd;
}

void CFG_set_server_url(const char *url)
{
    memset(sysCfg.server_url, 0, sizeof(sysCfg.server_url));
    if (url)
        strcpy(sysCfg.server_url, url);
}

const char *CFG_get_server_url(void)
{
    if (*sysCfg.server_url == '\0')
        return NULL;

    return sysCfg.server_url;
}

void CFG_set_server_retries(uint8_t retries)
{
    sysCfg.server_retries = retries;
}

uint8_t CFG_get_server_retries(void)
{
    return sysCfg.server_retries;
}

void CFG_set_user_auth(bool auth)
{
    sysCfg.user_auth = auth;
}

bool CFG_get_user_auth(void)
{
    return sysCfg.user_auth;
}

void CFG_set_debug(uint8_t mode, uint8_t level,
                   const char *host, uint16_t port)
{
    sysCfg.debug = mode;
    sysCfg.debug_level = level;
    if (!host)
        memset(sysCfg.debug_host, 0, sizeof(sysCfg.debug_host));
    else
        strcpy(sysCfg.debug_host, host);
    sysCfg.debug_port = port;
}

void CFG_get_debug(uint8_t *mode, uint8_t *level,
                   const char **host, uint16_t *port)
{
    if (mode)
        *mode = sysCfg.debug;
    if (level)
        *level = sysCfg.debug_level;
    if (host)
        *host = sysCfg.debug_host;
    if (port)
        *port = sysCfg.debug_port;
}

void CFG_set_wifi_disable(bool disable)
{
    sysCfg.wifi_disable = disable;
}

bool CFG_get_wifi_disable(void)
{
    return sysCfg.wifi_disable;
}

void CFG_set_timezone(int8_t timezone)
{
    sysCfg.timezone = timezone;
}

int8_t CFG_get_timezone(void)
{
    return sysCfg.timezone;
}

void CFG_set_standalone(bool enable)
{
    sysCfg.standalone = enable;
}

bool CFG_get_standalone(void)
{
    return sysCfg.standalone;
}

void CFG_set_dst(bool dst)
{
    sysCfg.dst = dst;
}

bool CFG_get_dst(void)
{
    return sysCfg.dst;
}

void CFG_set_dst_date(const char *date)
{
    memset(sysCfg.dst_date, 0, sizeof(sysCfg.dst_date));
    if (date)
        strncpy(sysCfg.dst_date, date, sizeof(sysCfg.dst_date) - 1);
}

const char *CFG_get_dst_date(void)
{
    if (*sysCfg.dst_date == '\0')
        return NULL;

    return sysCfg.dst_date;
}

void CFG_set_ap_mode(bool enable)
{
    sysCfg.ap_mode = enable;
}

bool CFG_get_ap_mode(void)
{
    return sysCfg.ap_mode;
}

void CFG_set_rf433_enable(bool enable)
{
    sysCfg.rf433_enable = enable;
}

bool CFG_get_rf433_enable(void)
{
    return sysCfg.rf433_enable;
}

void CFG_set_rf433_rc(bool enable)
{
    sysCfg.rf433_rc = enable;
}

bool CFG_get_rf433_rc(void)
{
    return sysCfg.rf433_rc;
}

void CFG_set_rf433_hc(uint16_t hc)
{
    sysCfg.rf433_hc = hc;
}

uint16_t CFG_get_rf433_hc(void)
{
    return sysCfg.rf433_hc;
}

void CFG_set_rf433_alarm(bool alarm)
{
    sysCfg.rf433_alarm = alarm;
}

bool CFG_get_rf433_alarm(void)
{
    return sysCfg.rf433_alarm;
}

void CFG_set_rf433_bc(bool enable)
{
    sysCfg.rf433_bc = enable;
}

bool CFG_get_rf433_bc(void)
{
    return sysCfg.rf433_bc;
}

void CFG_set_hotspot(bool hotspot)
{
    sysCfg.hotspot = hotspot;
}

bool CFG_get_hotspot(void)
{
    return sysCfg.hotspot;
}

void CFG_set_qrcode_config(bool enable)
{
    sysCfg.qrcode_config = enable;
}

bool CFG_get_qrcode_config(void)
{
    return sysCfg.qrcode_config;
}

void CFG_set_ssid_hidden(bool enable)
{
    sysCfg.ssid_hidden = enable;
}

bool CFG_get_ssid_hidden(void)
{
    return sysCfg.ssid_hidden;
}

void CFG_set_fingerprint_enable(bool enable)
{
    sysCfg.fingerprint_enable = enable;
}

bool CFG_get_fingerprint_enable(void)
{
    return sysCfg.fingerprint_enable;
}

void CFG_set_fingerprint_timeout(uint32_t timeout)
{
    sysCfg.fingerprint_timeout = timeout;
}

uint32_t CFG_get_fingerprint_timeout(void)
{
    return sysCfg.fingerprint_timeout;
}

void CFG_set_fingerprint_security(uint8_t security)
{
    sysCfg.fingerprint_security = security;
}

uint8_t CFG_get_fingerprint_security(void)
{
    return sysCfg.fingerprint_security;
}

void CFG_set_fingerprint_identify_retries(uint8_t retries)
{
    sysCfg.fingerprint_identify_retries = retries;
}

uint8_t CFG_get_fingerprint_identify_retries(void)
{
    return sysCfg.fingerprint_identify_retries;
}

void CFG_set_latitude(const char *latitude)
{
    memset(sysCfg.latitude, 0, sizeof(sysCfg.latitude));
    if (latitude)
        strncpy(sysCfg.latitude, latitude,
                   sizeof(sysCfg.latitude) - 1);
}

const char *CFG_get_latitude(void)
{
    if (*sysCfg.latitude == '\0')
        return NULL;

    return sysCfg.latitude;
}

void CFG_set_longitude(const char *longitude)
{
    memset(sysCfg.longitude, 0, sizeof(sysCfg.longitude));
    if (longitude)
        strncpy(sysCfg.longitude, longitude,
                   sizeof(sysCfg.longitude) - 1);
}

const char *CFG_get_longitude(void)
{
    if (*sysCfg.longitude == '\0')
        return NULL;

    return sysCfg.longitude;
}

void CFG_set_breakin_alarm(bool alarm)
{
    sysCfg.breakin_alarm = alarm;
}

bool CFG_get_breakin_alarm(void)
{
    return sysCfg.breakin_alarm;
}

void CFG_set_control_acc_timeout(uint32_t timeout)
{
    sysCfg.control_acc_timeout = timeout;
}

uint32_t CFG_get_control_acc_timeout(void)
{
    return sysCfg.control_acc_timeout;
}

void CFG_set_ddns(bool ddns)
{
    sysCfg.ddns = ddns;
}

bool CFG_get_ddns(void)
{
    return sysCfg.ddns;
}

void CFG_set_ddns_domain(const char *domain)
{
    memset(sysCfg.ddns_domain, 0,
              sizeof(sysCfg.ddns_domain));
    if (domain)
        strncpy(sysCfg.ddns_domain, domain,
                   sizeof(sysCfg.ddns_domain) - 1);
}

const char *CFG_get_ddns_domain(void)
{
    if (*sysCfg.ddns_domain == '\0')
        return NULL;

    return sysCfg.ddns_domain;
}

void CFG_set_ddns_user(const char *user)
{
    memset(sysCfg.ddns_user, 0,
              sizeof(sysCfg.ddns_user));
    if (user)
        strncpy(sysCfg.ddns_user, user,
                   sizeof(sysCfg.ddns_user) - 1);
}

const char *CFG_get_ddns_user(void)
{
    if (*sysCfg.ddns_user == '\0')
        return NULL;

    return sysCfg.ddns_user;
}

void CFG_set_ddns_passwd(const char *passwd)
{
    memset(sysCfg.ddns_passwd, 0,
              sizeof(sysCfg.ddns_passwd));
    if (passwd)
        strncpy(sysCfg.ddns_passwd, passwd,
                   sizeof(sysCfg.ddns_passwd) - 1);
}

const char *CFG_get_ddns_passwd(void)
{
    if (*sysCfg.ddns_passwd == '\0')
        return NULL;

    return sysCfg.ddns_passwd;
}

void CFG_set_button_enable(bool button)
{
    sysCfg.button_enable = button;
}

bool CFG_get_button_enable(void)
{
    return sysCfg.button_enable;
}

void CFG_set_rf433_bp(uint8_t button)
{
    sysCfg.rf433_bp = button;
}

uint8_t CFG_get_rf433_bp(void)
{
    return sysCfg.rf433_bp;
}

void CFG_set_rf433_panic_timeout(uint32_t timeout)
{
    sysCfg.rf433_panic_timeout = timeout;
}

uint32_t CFG_get_rf433_panic_timeout(void)
{
    return sysCfg.rf433_panic_timeout;
}

void CFG_set_rf433_ba(uint8_t button)
{
    sysCfg.rf433_ba = button;
}

uint8_t CFG_get_rf433_ba(void)
{
    uint8_t test = -1;
    if (sysCfg.rf433_ba == test)
        return 0;

    return sysCfg.rf433_ba;
}

void CFG_set_dht_enable(bool enable)
{
    sysCfg.dht_enable = enable;
}

bool CFG_get_dht_enable(void)
{
    return sysCfg.dht_enable;
}

void CFG_set_dht_timeout(uint32_t timeout)
{
    sysCfg.dht_timeout = timeout;
}

uint32_t CFG_get_dht_timeout(void)
{
    return sysCfg.dht_timeout;
}

void CFG_set_dht_temp_upper(int8_t temp)
{
    sysCfg.dht_temp_upper = temp;
}

int8_t CFG_get_dht_temp_upper(void)
{
    return sysCfg.dht_temp_upper;
}

void CFG_set_dht_temp_lower(int8_t temp)
{
    sysCfg.dht_temp_lower = temp;
}

int8_t CFG_get_dht_temp_lower(void)
{
    return sysCfg.dht_temp_lower;
}

void CFG_set_dht_rh_upper(int8_t rh)
{
    sysCfg.dht_rh_upper = rh;
}

int8_t CFG_get_dht_rh_upper(void)
{
    return sysCfg.dht_rh_upper;
}

void CFG_set_dht_rh_lower(int8_t rh)
{
    sysCfg.dht_rh_lower = rh;
}

int8_t CFG_get_dht_rh_lower(void)
{
    return sysCfg.dht_rh_lower;
}

void CFG_set_dht_relay(bool relay)
{
    sysCfg.dht_relay = relay;
}

bool CFG_get_dht_relay(void)
{
    return sysCfg.dht_relay;
}

void CFG_set_dht_alarm(bool alarm)
{
    sysCfg.dht_alarm = alarm;
}

bool CFG_get_dht_alarm(void)
{
    return sysCfg.dht_alarm;
}

void CFG_set_control_description(const char *desc)
{
    memset(sysCfg.control_desc, 0, sizeof(sysCfg.control_desc));
    if (desc)
        strncpy(sysCfg.control_desc, desc,
                   sizeof(sysCfg.control_desc) - 1);
}

const char *CFG_get_control_description(void)
{
    if (*sysCfg.control_desc == '\0')
        return NULL;

    return sysCfg.control_desc;
}

void CFG_set_mq2_enable(bool enable)
{
    sysCfg.mq2_enable = enable;
}

bool CFG_get_mq2_enable(void)
{
    return sysCfg.mq2_enable;
}

void CFG_set_mq2_timeout(uint32_t timeout)
{
    sysCfg.mq2_timeout = timeout;
}

uint32_t CFG_get_mq2_timeout(void)
{
    return sysCfg.mq2_timeout;
}

void CFG_set_mq2_limit(uint16_t limit)
{
    sysCfg.mq2_limit = limit;
}

uint16_t CFG_get_mq2_limit(void)
{
    return sysCfg.mq2_limit;
}

void CFG_set_mq2_relay(bool relay)
{
    sysCfg.mq2_relay = relay;
}

bool CFG_get_mq2_relay(void)
{
    return sysCfg.mq2_relay;
}

void CFG_set_mq2_alarm(bool alarm)
{
    sysCfg.mq2_alarm = alarm;
}

bool CFG_get_mq2_alarm(void)
{
    return sysCfg.mq2_alarm;
}

void CFG_set_pir_enable(bool enable)
{
    sysCfg.pir_enable = enable;
}

bool CFG_get_pir_enable(void)
{
    return sysCfg.pir_enable;
}

void CFG_set_pir_chime(bool chime)
{
    sysCfg.pir_chime = chime;
}

bool CFG_get_pir_chime(void)
{
    return sysCfg.pir_chime;
}

void CFG_set_pir_relay(bool relay)
{
    sysCfg.pir_relay = relay;
}

bool CFG_get_pir_relay(void)
{
    return sysCfg.pir_relay;
}

void CFG_set_pir_alarm(bool alarm)
{
    sysCfg.pir_alarm = alarm;
}

bool CFG_get_pir_alarm(void)
{
    return sysCfg.pir_alarm;
}

void CFG_set_lora_enable(bool enable)
{
    sysCfg.lora_enable = enable;
}

bool CFG_get_lora_enable(void)
{
    return sysCfg.lora_enable;
}

void CFG_set_lora_channel(uint8_t channel)
{
    sysCfg.lora_channel = channel;
}

uint8_t CFG_get_lora_channel(void)
{
    return sysCfg.lora_channel;
}

void CFG_set_lora_baudrate(uint16_t baudrate)
{
    sysCfg.lora_baudrate = baudrate;
}

uint16_t CFG_get_lora_baudrate(void)
{
    return sysCfg.lora_baudrate;
}

void CFG_set_lora_address(uint8_t addr)
{
    sysCfg.lora_addr = addr;
}

uint8_t CFG_get_lora_address(void)
{
    return sysCfg.lora_addr;
}

void CFG_set_lora_server_address(uint8_t addr)
{
    sysCfg.lora_server_addr = addr;
}

uint8_t CFG_get_lora_server_address(void)
{
    return sysCfg.lora_server_addr;
}

void CFG_set_sensor_type(uint8_t type)
{
    sysCfg.sensor_type = type;
}

uint8_t CFG_get_sensor_type(void)
{
    return sysCfg.sensor_type;
}

void CFG_set_sensor_flow(uint16_t flow)
{
    sysCfg.sensor_flow = flow;
}

uint16_t CFG_get_sensor_flow(void)
{
    return sysCfg.sensor_flow;
}

void CFG_set_sensor_cycles(uint32_t cycles)
{
    sysCfg.sensor_cycles = cycles;
}

uint32_t CFG_get_sensor_cycles(void)
{
    return sysCfg.sensor_cycles;
}

void CFG_set_sensor_limit(uint32_t limit)
{
    sysCfg.sensor_limit = limit;
}

uint32_t CFG_get_sensor_limit(void)
{
    return sysCfg.sensor_limit;
}

void CFG_set_sensor_relay(bool relay)
{
    sysCfg.sensor_relay = relay;
}

bool CFG_get_sensor_relay(void)
{
    return sysCfg.sensor_relay;
}

void CFG_set_sensor_alarm(bool alarm)
{
    sysCfg.sensor_alarm = alarm;
}

bool CFG_get_sensor_alarm(void)
{
    return sysCfg.sensor_alarm;
}

void CFG_set_pir_timeout(uint32_t timeout)
{
    sysCfg.pir_timeout = timeout;
}

uint32_t CFG_get_pir_timeout(void)
{
    return sysCfg.pir_timeout;
}

void CFG_set_temt_enable(bool enable)
{
    sysCfg.temt_enable = enable;
}

bool CFG_get_temt_enable(void)
{
    return sysCfg.temt_enable;
}

void CFG_set_temt_timeout(uint32_t timeout)
{
    sysCfg.temt_timeout = timeout;
}

uint32_t CFG_get_temt_timeout(void)
{
    return sysCfg.temt_timeout;
}

void CFG_set_temt_upper(uint16_t value)
{
    sysCfg.temt_upper = value;
}

uint16_t CFG_get_temt_upper(void)
{
    return sysCfg.temt_upper;
}

void CFG_set_temt_lower(uint16_t value)
{
    sysCfg.temt_lower = value;
}

uint16_t CFG_get_temt_lower(void)
{
    return sysCfg.temt_lower;
}

void CFG_set_temt_relay(bool relay)
{
    sysCfg.temt_relay = relay;
}

bool CFG_get_temt_relay(void)
{
    return sysCfg.temt_relay;
}

void CFG_set_temt_alarm(bool alarm)
{
    sysCfg.temt_alarm = alarm;
}

bool CFG_get_temt_alarm(void)
{
    return sysCfg.temt_alarm;
}

uint32_t CFG_get_sensor_volume(void)
{
    if (!sysCfg.sensor_flow)
        return 0;

    return sysCfg.sensor_cycles / sysCfg.sensor_flow;
}

const char *CFG_get_sensor_str_volume(void)
{
    static char vol[32] = { "0.000" };
    uint32_t l;
    uint32_t m;

    if (sysCfg.sensor_flow) {
        /* Liters */
        l = sysCfg.sensor_cycles / sysCfg.sensor_flow;
        /* Mililiters */
        m = (1000 * (sysCfg.sensor_cycles % sysCfg.sensor_flow));
        m = m / sysCfg.sensor_flow;
        sprintf(vol, "%u.%03d", l, m);
        
    }

    return vol;
}

void CFG_set_relay_status(uint8_t status)
{
    sysCfg.relay_status = status;
}

uint8_t CFG_get_relay_status(void)
{
    return sysCfg.relay_status;
}

void CFG_set_pow_voltage_cal(uint32_t cal)
{
    sysCfg.pow_vol_cal = cal;
}

uint32_t CFG_get_pow_voltage_cal(void)
{
    return sysCfg.pow_vol_cal;
}

void CFG_set_pow_voltage_upper(uint32_t vol)
{
    sysCfg.pow_vol_upper = vol;
}

uint32_t CFG_get_pow_voltage_upper(void)
{
    return sysCfg.pow_vol_upper;
}

void CFG_set_pow_voltage_lower(uint32_t vol)
{
    sysCfg.pow_vol_lower = vol;
}

uint32_t CFG_get_pow_voltage_lower(void)
{
    return sysCfg.pow_vol_lower;
}

void CFG_set_pow_current_cal(uint32_t cal)
{
    sysCfg.pow_cur_cal = cal;
}

uint32_t CFG_get_pow_current_cal(void)
{
    return sysCfg.pow_cur_cal;
}

void CFG_set_pow_current_upper(uint32_t cur)
{
    sysCfg.pow_cur_upper = cur;
}

uint32_t CFG_get_pow_current_upper(void)
{
    return sysCfg.pow_cur_upper;
}

void CFG_set_pow_current_lower(uint32_t cur)
{
    sysCfg.pow_cur_lower = cur;
}

uint32_t CFG_get_pow_current_lower(void)
{
    return sysCfg.pow_cur_lower;
}

void CFG_set_pow_power_upper(uint32_t pwr)
{
    sysCfg.pow_pwr_upper = pwr;
}

uint32_t CFG_get_pow_power_upper(void)
{
    return sysCfg.pow_pwr_upper;
}

void CFG_set_pow_power_lower(uint32_t pwr)
{
    sysCfg.pow_pwr_lower = pwr;
}

uint32_t CFG_get_pow_power_lower(void)
{
    return sysCfg.pow_pwr_lower;
}

void CFG_set_pow_relay(bool relay)
{
    sysCfg.pow_relay = relay;
}

bool CFG_get_pow_relay(void)
{
    return sysCfg.pow_relay;
}

void CFG_set_pow_alarm_time(uint16_t time)
{
    sysCfg.pow_alarm_time = time;
}

uint16_t CFG_get_pow_alarm_time(void)
{
    return sysCfg.pow_alarm_time;
}

void CFG_set_pow_relay_timeout(uint16_t timeout)
{
    sysCfg.pow_relay_timeout = timeout;
}

uint16_t CFG_get_pow_relay_timeout(void)
{
    return sysCfg.pow_relay_timeout;
}

void CFG_set_pow_relay_ext(bool relay)
{
    sysCfg.pow_relay_ext = relay;
}

bool CFG_get_pow_relay_ext(void)
{
    return sysCfg.pow_relay_ext;
}

void CFG_set_energy_daily(uint64_t energy)
{
    sysCfg.energy_daily = energy;
}

uint64_t CFG_get_energy_daily(void)
{
    return sysCfg.energy_daily;
}

const char *CFG_get_energy_daily_str(void)
{
    static char buf[16] = { 0 };
    uint32_t energy;

    energy = (100 * sysCfg.energy_daily) / 695507;
    snprintf(buf, sizeof(buf), "%d.%02d",
                energy / 100, energy % 100);

    return buf;
}

void CFG_set_energy_daily_last(uint64_t energy)
{
    sysCfg.energy_daily_last = energy;
}

uint64_t CFG_get_energy_daily_last(void)
{
    return sysCfg.energy_daily_last;
}

const char *CFG_get_energy_daily_last_str(void)
{
    static char buf[16] = { 0 };
    uint32_t energy;

    energy = (100 * sysCfg.energy_daily_last) / 695507;
    snprintf(buf, sizeof(buf), "%d.%02d",
                energy / 100, energy % 100);

    return buf;
}

void CFG_set_energy_monthly(uint64_t energy)
{
    sysCfg.energy_monthly = energy;
}

uint64_t CFG_get_energy_monthly(void)
{
    return sysCfg.energy_monthly;
}

const char *CFG_get_energy_monthly_str(void)
{
    static char buf[16] = { 0 };
    uint32_t energy;

    energy = (100 * sysCfg.energy_monthly) / 695507;
    snprintf(buf, sizeof(buf), "%d.%02d",
                energy / 100, energy % 100);

    return buf;
}

void CFG_set_energy_monthly_last(uint64_t energy)
{
    sysCfg.energy_monthly_last = energy;
}

uint64_t CFG_get_energy_monthly_last(void)
{
    return sysCfg.energy_monthly_last;
}

const char *CFG_get_energy_monthly_last_str(void)
{
    static char buf[16] = { 0 };
    uint32_t energy;

    energy = (100 * sysCfg.energy_monthly_last) / 695507;
    snprintf(buf, sizeof(buf), "%d.%02d",
                energy / 100, energy % 100);

    return buf;
}

void CFG_set_energy_total(uint64_t energy)
{
    sysCfg.energy_total = energy;
}

uint64_t CFG_get_energy_total(void)
{
    return sysCfg.energy_total;
}

const char *CFG_get_energy_total_str(void)
{
    static char buf[16] = { 0 };
    uint32_t energy;

    energy = (100 * sysCfg.energy_total) / 695507;
    snprintf(buf, sizeof(buf), "%d.%02d",
                energy / 100, energy % 100);

    return buf;
}

void CFG_set_pow_interval(uint16_t interval)
{
    sysCfg.pow_interval = interval;
}

uint16_t CFG_get_pow_interval(void)
{
    return sysCfg.pow_interval;
}

void CFG_set_pow_day(uint8_t day)
{
    sysCfg.pow_day = day;
}

uint8_t CFG_get_pow_day(void)
{
    return sysCfg.pow_day;
}

void CFG_set_energy_daily_limit(uint32_t limit)
{
    sysCfg.energy_daily_limit = limit;
}

uint32_t CFG_get_energy_daily_limit(void)
{
    return sysCfg.energy_daily_limit;
}

void CFG_set_energy_monthly_limit(uint32_t limit)
{
    sysCfg.energy_monthly_limit = limit;
}

uint32_t CFG_get_energy_monthly_limit(void)
{
    return sysCfg.energy_monthly_limit;
}

void CFG_set_energy_total_limit(uint32_t limit)
{
    sysCfg.energy_total_limit = limit;
}

uint32_t CFG_get_energy_total_limit(void)
{
    return sysCfg.energy_total_limit;
}

void CFG_set_qrcode_dynamic(bool dynamic)
{
    sysCfg.qrcode_dynamic = dynamic;
}

bool CFG_get_qrcode_dynamic(void)
{
    return sysCfg.qrcode_dynamic;
}

void CFG_set_qrcode_validity(uint16_t validity)
{
    sysCfg.qrcode_validity = validity;
}

uint16_t CFG_get_qrcode_validity(void)
{
    return sysCfg.qrcode_validity;
}

void CFG_set_rtc_time(uint32_t time)
{
    sysCfg.rtc_time = time;
}

uint32_t CFG_get_rtc_time(void)
{
    return sysCfg.rtc_time;
}

void CFG_set_energy_month(uint8_t month, uint64_t energy)
{
    if (month > 11) return;

    sysCfg.energy_month[month] = energy;
}

uint64_t CFG_get_energy_month(uint8_t month)
{
    if (month > 11) return 0;

    return sysCfg.energy_month[month];
}

const char *CFG_get_energy_month_str(uint8_t month)
{
    static char buf[16] = { 0 };
    uint32_t energy;

    if (month < 12) {
        energy = (100 * sysCfg.energy_month[month]) / 695507;
        snprintf(buf, sizeof(buf), "%d.%02d",
                    energy / 100, energy % 100);
    }

    return buf;
}

void CFG_set_control_doublepass_timeout(uint32_t timeout)
{
    sysCfg.control_doublepass_timeout = timeout;
}

uint32_t CFG_get_control_doublepass_timeout(void)
{
    if (sysCfg.control_doublepass_timeout == -1)
        return 0;

    return sysCfg.control_doublepass_timeout;
}

void CFG_set_rtc_shutdown(uint32_t time)
{
    sysCfg.rtc_shutdown = time;
}

uint32_t CFG_get_rtc_shutdown(void)
{
    return sysCfg.rtc_shutdown;
}

void CFG_set_schedule(int index, const char *period)
{
    if (index >= CFG_SCHEDULE)
        return;

    memset(sysCfg.schedule[index], 0,
              sizeof(sysCfg.schedule[index]));
    if (period)
        strcpy(sysCfg.schedule[index], period);
}

const char *CFG_get_schedule(int index)
{
    if (index >= CFG_SCHEDULE)
        return NULL;

    if (*sysCfg.schedule[index] == '\0' ||
        *sysCfg.schedule[index] == 0xff)
        return NULL;

    return sysCfg.schedule[index];
}

void CFG_set_pow_energy_cal(uint32_t cal)
{
    sysCfg.pow_nrg_cal = cal;
}

uint32_t CFG_get_pow_energy_cal(void)
{
    return sysCfg.pow_nrg_cal;
}

void CFG_set_cli_enable(bool enable)
{
    sysCfg.cli_enable = enable;
}

bool CFG_get_cli_enable(void)
{
    return sysCfg.cli_enable;
}

void CFG_set_cli_timeout(uint32_t timeout)
{
    sysCfg.cli_timeout = timeout;
}

uint32_t CFG_get_cli_timeout(void)
{
    return sysCfg.cli_timeout;
}

void CFG_set_cli_range(uint16_t range)
{
    sysCfg.cli_range = range;
}

uint16_t CFG_get_cli_range(void)
{
    return sysCfg.cli_range;
}

void CFG_set_cli_upper(uint16_t value)
{
    sysCfg.cli_upper = value;
}

uint16_t CFG_get_cli_upper(void)
{
    return sysCfg.cli_upper;
}

void CFG_set_cli_lower(uint16_t value)
{
    sysCfg.cli_lower = value;
}

uint16_t CFG_get_cli_lower(void)
{
    return sysCfg.cli_lower;
}

void CFG_set_cli_relay(bool relay)
{
    sysCfg.cli_relay = relay;
}

bool CFG_get_cli_relay(void)
{
    return sysCfg.cli_relay;
}

void CFG_set_cli_alarm(bool alarm)
{
    sysCfg.cli_alarm = alarm;
}

bool CFG_get_cli_alarm(void)
{
    return sysCfg.cli_alarm;
}

void CFG_set_cli_cal(uint16_t value)
{
    sysCfg.cli_cal = value;
}

uint16_t CFG_get_cli_cal(void)
{
    return sysCfg.cli_cal;
}
