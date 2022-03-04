#pragma once
bool ulip_core_probe_status(void);
bool ulip_core_erase_status(void);
int ulip_core_log2html(char *html, int len);
void ulip_core_system_update(const char *ota_url);
void ulip_core_restore_config(bool restart);
void ulip_core_log_remove(void);
void ulip_core_probe_user(bool status);
void ulip_core_erase_user(bool status);
void ulip_core_capture_finger(bool status, int index);
bool ulip_core_capture_finger_status(void);
const char *rtc_weekday(struct tm *tm);
const char *rtc_month(struct tm *tm);