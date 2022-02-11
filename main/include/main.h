#pragma once
char *weekday[7];
char *monthday[12];
void set_time_names() {
    weekday[0] = "Sun";
    weekday[1] = "Mon";
    weekday[2] = "Tue";
    weekday[3] = "Wed";
    weekday[4] = "Thu";
    weekday[5] = "Fri";
    weekday[6] = "Sat";
    monthday[0] = "Jan";
    monthday[1] = "Feb";
    monthday[2] = "Mar";
    monthday[3] = "Apr";
    monthday[4] = "May";
    monthday[5] = "Jun";
    monthday[6] = "Jul";
    monthday[7] = "Aug";
    monthday[8] = "Sep";
    monthday[9] = "Oct";
    monthday[10] = "Nov";
    monthday[11] = "Dec";
}
bool ulip_core_probe_status(void);
bool ulip_core_erase_status(void);
int ulip_core_log2html(char *html, int len);
void ulip_core_system_update(const char *ota_url);
void ulip_core_restore_config(bool restart);
void ulip_core_log_remove(void);
void ulip_core_probe_user(bool status);
void ulip_core_erase_user(bool status);