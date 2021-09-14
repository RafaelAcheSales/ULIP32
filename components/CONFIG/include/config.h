#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "stdbool.h"
#define SYSENV_KEY              "sysenv"
#define SYSCFG_KEY              "syscfg"
#define SAVEFLAG_KEY            "saveflag"
#define CFG_HOLDER              0x12345678

#define CFG_FIRMWARE            "ulip1.bin"

#define CFG_ETHADDR             "02:80:ad:20:31:e8"
#define CFG_SERIALNUM           "1234567890"
#define CFG_BOARD               "ULIP"
#define CFG_RELEASE             "001.000.001"

#define CFG_WIFI_SSID           "uTech-AP"
#define CFG_WIFI_PASSWD         "adminutech"

#define CFG_IP_ADDRESS          "10.0.0.222"
#define CFG_NETMASK             "255.255.255.0"
#define CFG_GATEWAY             "10.0.0.1"
#define CFG_HOSTNAME            "ulip"

#define CFG_NTP                 "pool.ntp.br"
#define CFG_OTA_URL             ""

#define CFG_WEB_USER            "admin"
#define CFG_WEB_PASSWD          "admin"

#define CFG_RFID_TIMEOUT        1000
#define CFG_QRCODE_TIMEOUT      1000
#define CFG_QRCODE_VALIDITY     30
#define CFG_FPM_TIMEOUT         200000
#define CFG_DHT_TIMEOUT         60000
#define CFG_MQ2_TIMEOUT         60000
#define CFG_PIR_TIMEOUT         30000
#define CFG_TEMT_TIMEOUT        60000
#define CFG_LOOP_TIMEOUT        10000

#define CFG_RFID_FORMAT         1

#define CFG_FPM_SECURITY        2

#define CFG_RF433_HC            256

#define CFG_LORA_CHANNEL        23
#define CFG_LORA_BAUDRATE       2400

#define CFG_CONTROL_MODE_AUTO   0
#define CFG_CONTROL_MODE_MANUAL 1
#define CFG_CONTROL_TIMEOUT     3000
#define CFG_CONTROL_ACC_TIMEOUT 30000

#define CFG_TIMEZONE            -3

#define CFG_SENSOR_NORMAL       0
#define CFG_SENSOR_LEVEL        1
#define CFG_SENSOR_VOLUME       2

#define CFG_POW_INTERVAL        60
#define CFG_POW_ALARM_TIME      1
#define CFG_POW_RELAY_TIMEOUT   60

#define CFG_RTC_SHUTDOWN        24

#define CFG_SCHEDULE            5

extern const char *CFG_version;

void  CFG_ENV_Default(void);

void  CFG_Save();
void  CFG_Load();
void  CFG_Default();


const char *CFG_get_ethaddr(void);

const char *CFG_get_serialnum(void);

const char *CFG_get_board(void);

const char *CFG_get_release(void);
uint16_t CFG_Get_blobs(void);
void CFG_reset_all_flash(void);
void CFG_set_wifi_disable(bool disable);

bool CFG_get_wifi_disable(void);

void CFG_set_wifi_ssid(const char *ssid);

const char *CFG_get_wifi_ssid(void);

void CFG_set_wifi_passwd(const char *passwd);

const char *CFG_get_wifi_passwd(void);

void CFG_set_wifi_channel(uint8_t channel);

uint8_t CFG_get_wifi_channel(void);

void CFG_set_wifi_beacon_interval(uint16_t channel);

uint16_t CFG_get_wifi_beacon_interval(void);

void CFG_set_dhcp(bool dhcp);

bool CFG_get_dhcp(void);

void CFG_set_ip_address(const char *ip);

const char *CFG_get_ip_address(void);

void CFG_set_netmask(const char *netmask);

const char *CFG_get_netmask(void);

void CFG_set_gateway(const char *gateway);

const char *CFG_get_gateway(void);

void CFG_set_hostname(const char *hostname);

const char *CFG_get_hostname(void);

void CFG_set_dns(const char *dns);

const char *CFG_get_dns(void);

void CFG_set_ntp(const char *ntp);

const char *CFG_get_ntp(void);

void CFG_set_timezone(int8_t timezone);

int8_t CFG_get_timezone(void);

void CFG_set_ota_url(const char *url);

const char *CFG_get_ota_url(void);

void CFG_set_web_user(const char *user);

const char *CFG_get_web_user(void);

void CFG_set_web_passwd(const char *passwd);

const char *CFG_get_web_passwd(void);

void CFG_set_rfid_enable(bool enable);

bool CFG_get_rfid_enable(void);

void CFG_set_rfid_timeout(uint32_t timeout);

uint32_t CFG_get_rfid_timeout(void);
void CFG_set_rfid_retries(uint8_t retries);

uint8_t CFG_get_rfid_retries(void);

void CFG_set_rfid_nfc(bool nfc);

bool CFG_get_rfid_nfc(void);

void CFG_set_rfid_panic_timeout(uint32_t timeout);

uint32_t CFG_get_rfid_panic_timeout(void);

void CFG_set_rfid_format(uint8_t format);

uint8_t CFG_get_rfid_format(void);

void CFG_set_qrcode_enable(bool enable);

bool CFG_get_qrcode_enable(void);

void CFG_set_qrcode_timeout(uint32_t timeout);

uint32_t CFG_get_qrcode_timeout(void);

void CFG_set_qrcode_panic_timeout(uint32_t timeout);

uint32_t CFG_get_qrcode_panic_timeout(void);

void CFG_set_qrcode_led(bool led);

bool CFG_get_qrcode_led(void);

void CFG_set_control_mode(uint8_t mode);

uint8_t CFG_get_control_mode(void);

void CFG_set_control_timeout(uint32_t timeout);

uint32_t CFG_get_control_timeout(void);

void CFG_set_control_accessibility_timeout(uint32_t timeout);

uint32_t CFG_get_control_accessibility_timeout(void);

void CFG_set_control_external(bool external);

bool CFG_get_control_external(void);

void CFG_set_control_url(const char *url);

const char *CFG_get_control_url(void);

void CFG_set_rs485_enable(bool enable);

bool CFG_get_rs485_enable(void);

void CFG_set_rs485_hwaddr(uint8_t addr);

uint8_t CFG_get_rs485_hwaddr(void);

void CFG_set_rs485_server_hwaddr(uint8_t addr);

uint8_t CFG_get_rs485_server_hwaddr(void);

void CFG_set_server_ip(const char *ip);

const char *CFG_get_server_ip(void);

void CFG_set_server_port(uint16_t port);

uint16_t CFG_get_server_port(void);

void CFG_set_server_user(const char *user);

const char *CFG_get_server_user(void);

void CFG_set_server_passwd(const char *user);

const char *CFG_get_server_passwd(void);

void CFG_set_server_url(const char *url);

const char *CFG_get_server_url(void);

void CFG_set_user_auth(bool auth);

bool CFG_get_user_auth(void);

void CFG_set_server_retries(uint8_t retries);

uint8_t CFG_get_server_retries(void);

void CFG_set_debug(uint8_t mode, uint8_t level,
                   const char *host, uint16_t port);

void CFG_get_debug(uint8_t *mode, uint8_t *level,
                   const char **host, uint16_t *port);

void CFG_set_standalone(bool enable);

bool CFG_get_standalone(void);

void CFG_set_dst(bool dst);

bool CFG_get_dst(void);

void CFG_set_dst_date(const char *date);

const char *CFG_get_dst_date(void);

void CFG_set_ap_mode(bool enable);

bool CFG_get_ap_mode(void);

void CFG_set_rf433_enable(bool enable);

bool CFG_get_rf433_enable(void);

void CFG_set_rf433_rc(bool enable);

bool CFG_get_rf433_rc(void);

void CFG_set_rf433_hc(uint16_t hc);

uint16_t CFG_get_rf433_hc(void);

void CFG_set_rf433_alarm(bool enable);

bool CFG_get_rf433_alarm(void);

void CFG_set_rf433_bc(bool enable);

bool CFG_get_rf433_bc(void);

void CFG_set_hotspot(bool hotspot);

bool CFG_get_hotspot(void);

void CFG_set_qrcode_config(bool enable);

bool CFG_get_qrcode_config(void);

void CFG_set_ssid_hidden(bool enable);

bool CFG_get_ssid_hidden(void);

void CFG_set_fingerprint_enable(bool enable);

bool CFG_get_fingerprint_enable(void);

void CFG_set_fingerprint_timeout(uint32_t timeout);

uint32_t CFG_get_fingerprint_timeout(void);

void CFG_set_fingerprint_security(uint8_t security);

uint8_t CFG_get_fingerprint_security(void);

void CFG_set_fingerprint_identify_retries(uint8_t retries);

uint8_t CFG_get_fingerprint_identify_retries(void);

void CFG_set_latitude(const char *latitude);

const char *CFG_get_latitude(void);

void CFG_set_longitude(const char *longitude);

const char *CFG_get_longitude(void);

void CFG_set_breakin_alarm(bool enable);

bool CFG_get_breakin_alarm(void);

void CFG_set_ddns(bool ddns);

bool CFG_get_ddns(void);

void CFG_set_ddns_domain(const char *domain);

const char *CFG_get_ddns_domain(void);

void CFG_set_ddns_user(const char *user);

const char *CFG_get_ddns_user(void);

void CFG_set_ddns_passwd(const char *passwd);

const char *CFG_get_ddns_passwd(void);

void CFG_set_button_enable(bool button);

bool CFG_get_button_enable(void);

void CFG_set_rf433_bp(uint8_t button);

uint8_t CFG_get_rf433_bp(void);

void CFG_set_rf433_panic_timeout(uint32_t timeout);

uint32_t CFG_get_rf433_panic_timeout(void);

void CFG_set_rf433_ba(uint8_t button);

uint8_t CFG_get_rf433_ba(void);

void CFG_set_dht_enable(bool enable);

bool CFG_get_dht_enable(void);

void CFG_set_dht_timeout(uint32_t timeout);

uint32_t CFG_get_dht_timeout(void);

void CFG_set_dht_temp_upper(int8_t temp);

int8_t CFG_get_dht_temp_upper(void);

void CFG_set_dht_temp_lower(int8_t temp);

int8_t CFG_get_dht_temp_lower(void);

void CFG_set_dht_rh_upper(int8_t rh);

int8_t CFG_get_dht_rh_upper(void);

void CFG_set_dht_rh_lower(int8_t rh);

int8_t CFG_get_dht_rh_lower(void);

void CFG_set_dht_relay(bool relay);

bool CFG_get_dht_relay(void);

void CFG_set_dht_alarm(bool alarm);

bool CFG_get_dht_alarm(void);

void CFG_set_control_description(const char *desc);

const char *CFG_get_control_description(void);

void CFG_set_mq2_enable(bool enable);

bool CFG_get_mq2_enable(void);

void CFG_set_mq2_timeout(uint32_t timeout);

uint32_t CFG_get_mq2_timeout(void);

void CFG_set_mq2_limit(uint16_t limit);

uint16_t CFG_get_mq2_limit(void);

void CFG_set_mq2_relay(bool relay);

bool CFG_get_mq2_relay(void);

void CFG_set_mq2_alarm(bool alarm);

bool CFG_get_mq2_alarm(void);

void CFG_set_pir_enable(bool enable);

bool CFG_get_pir_enable(void);

void CFG_set_pir_relay(bool relay);

bool CFG_get_pir_relay(void);

void CFG_set_pir_alarm(bool alarm);

bool CFG_get_pir_alarm(void);

void CFG_set_lora_enable(bool enable);

bool CFG_get_lora_enable(void);

void CFG_set_lora_channel(uint8_t channel);

uint8_t CFG_get_lora_channel(void);

void CFG_set_lora_baudrate(uint16_t baudrate);

uint16_t CFG_get_lora_baudrate(void);

void CFG_set_lora_address(uint8_t addr);

uint8_t CFG_get_lora_address(void);

void CFG_set_lora_server_address(uint8_t addr);

uint8_t CFG_get_lora_server_address(void);

void CFG_set_sensor_type(uint8_t type);

uint8_t CFG_get_sensor_type(void);

void CFG_set_sensor_flow(uint16_t flow);

uint16_t CFG_get_sensor_flow(void);

void CFG_set_sensor_cycles(uint32_t cycles);

uint32_t CFG_get_sensor_cycles(void);

void CFG_set_sensor_limit(uint32_t limit);

uint32_t CFG_get_sensor_limit(void);

void CFG_set_sensor_relay(bool relay);

bool CFG_get_sensor_relay(void);

void CFG_set_sensor_alarm(bool alarm);

bool CFG_get_sensor_alarm(void);

void CFG_set_pir_timeout(uint32_t timeout);

uint32_t CFG_get_pir_timeout(void);

void CFG_set_temt_enable(bool enable);

bool CFG_get_temt_enable(void);

void CFG_set_temt_timeout(uint32_t timeout);

uint32_t CFG_get_temt_timeout(void);

void CFG_set_temt_upper(uint16_t temp);

uint16_t CFG_get_temt_upper(void);

void CFG_set_temt_lower(uint16_t temp);

uint16_t CFG_get_temt_lower(void);

void CFG_set_temt_relay(bool relay);

bool CFG_get_temt_relay(void);

void CFG_set_temt_alarm(bool alarm);

bool CFG_get_temt_alarm(void);

void CFG_set_relay_status(uint8_t status);

uint8_t CFG_get_relay_status(void);

void CFG_set_pow_voltage_cal(uint32_t cal);

uint32_t CFG_get_pow_voltage_cal(void);

void CFG_set_pow_voltage_upper(uint32_t vol);

uint32_t CFG_get_pow_voltage_upper(void);

void CFG_set_pow_voltage_lower(uint32_t vol);

uint32_t CFG_get_pow_voltage_lower(void);

void CFG_set_pow_current_cal(uint32_t cal);

uint32_t CFG_get_pow_current_cal(void);

void CFG_set_pow_current_upper(uint32_t cur);

uint32_t CFG_get_pow_current_upper(void);

void CFG_set_pow_current_lower(uint32_t cur);

uint32_t CFG_get_pow_current_lower(void);

void CFG_set_pow_power_upper(uint32_t pwr);

uint32_t CFG_get_pow_power_upper(void);

void CFG_set_pow_power_lower(uint32_t pwr);

uint32_t CFG_get_pow_power_lower(void);

void CFG_set_pow_relay(bool relay);

bool CFG_get_pow_relay(void);

void CFG_set_pow_alarm_time(uint16_t time);

uint16_t CFG_get_pow_alarm_time(void);

void CFG_set_pow_relay_timeout(uint16_t timeout);

uint16_t CFG_get_pow_relay_timeout(void);

void CFG_set_pow_relay_ext(bool relay);

bool CFG_get_pow_relay_ext(void);

void CFG_set_schedule(int index, const char *period);

const char *CFG_get_schedule(int index);


void CFG_set_energy_daily(uint64_t energy);

uint64_t CFG_get_energy_daily(void);

const char *CFG_get_energy_daily_str(void);

void CFG_set_energy_daily_last(uint64_t energy);

uint64_t CFG_get_energy_daily_last(void);

const char *CFG_get_energy_daily_last_str(void);

void CFG_set_energy_monthly(uint64_t energy);

uint64_t CFG_get_energy_monthly(void);

const char *CFG_get_energy_monthly_str(void);

void CFG_set_energy_monthly_last(uint64_t energy);

uint64_t CFG_get_energy_monthly_last(void);

const char *CFG_get_energy_monthly_last_str(void);

void CFG_set_energy_total(uint64_t energy);

uint64_t CFG_get_energy_total(void);

const char *CFG_get_energy_total_str(void);

void CFG_set_pow_interval(uint16_t interval);

uint16_t CFG_get_pow_interval(void);

void CFG_set_pow_day(uint8_t day);

uint8_t CFG_get_pow_day(void);

void CFG_set_energy_daily_limit(uint32_t limit);

uint32_t CFG_get_energy_daily_limit(void);

void CFG_set_energy_monthly_limit(uint32_t limit);

uint32_t CFG_get_energy_monthly_limit(void);

void CFG_set_energy_total_limit(uint32_t limit);

uint32_t CFG_get_energy_total_limit(void);

void CFG_set_qrcode_dynamic(bool dynamic);

bool CFG_get_qrcode_dynamic(void);

void CFG_set_qrcode_validity(uint16_t validity);

uint16_t CFG_get_qrcode_validity(void);

void CFG_set_rtc_time(uint32_t time);

uint32_t CFG_get_rtc_time(void);

void CFG_set_energy_month(uint8_t month, uint64_t energy);

uint64_t CFG_get_energy_month(uint8_t month);

const char *CFG_get_energy_month_str(uint8_t month);

void CFG_set_doublepass_timeout(uint32_t timeout);

uint32_t CFG_get_doublepass_timeout(void);

void CFG_set_rtc_shutdown(uint32_t time);

uint32_t CFG_get_rtc_shutdown(void);

void CFG_set_pow_energy_cal(uint32_t cal);

uint32_t CFG_get_pow_energy_cal(void);

void CFG_set_cli_enable(bool enable);

bool CFG_get_cli_enable(void);

void CFG_set_cli_timeout(uint32_t timeout);

uint32_t CFG_get_cli_timeout(void);

void CFG_set_cli_range(uint16_t range);

uint16_t CFG_get_cli_range(void);

void CFG_set_cli_upper(uint16_t value);

uint16_t CFG_get_cli_upper(void);

void CFG_set_cli_lower(uint16_t value);

uint16_t CFG_get_cli_lower(void);

void CFG_set_cli_relay(bool relay);

bool CFG_get_cli_relay(void);

void CFG_set_cli_alarm(bool alarm);

bool CFG_get_cli_alarm(void);

void CFG_set_cli_cal(uint16_t value);

uint16_t CFG_get_cli_cal(void);


uint32_t CFG_get_sensor_volume(void);

const char *CFG_get_sensor_str_volume(void);

#endif /* __CONFIG_H__ */
