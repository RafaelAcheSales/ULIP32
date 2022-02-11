#ifndef __ULIP_CGI_H__
#define __ULIP_CGI_H__
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"

#define MENU_NETWORK    1
#define MENU_CONTROL    2
#define MENU_HTTP       3
#define MENU_USER       4
#define MENU_LOG        5
#define MENU_STATUS     6
#define MENU_ADMIN      7
#define MENU_PROG       8

#define MENU_ADMIN_TAB_UPDATE	1
#define MENU_ADMIN_TAB_RESET    2
#define MENU_ADMIN_TAB_SENHA	3
#define MENU_ADMIN_TAB_TIMEZONE	4
#define MENU_ADMIN_TAB_LOCATION 5
#define MENU_ADMIN_TAB_SYSTEM   6
#define MENU_ADMIN_TAB_DEBUG	7
#define MENU_ADMIN_TAB_BACKUP   8
#define MENU_ADMIN_TAB_WIFI     9
#define MENU_ADMIN_TAB_WATCHDOG 10

ICACHE_FLASH_ATTR
int ulip_cgi_process(HttpdInstance *pInstance, HttpdConnData *connData);

#endif  /* __ULIP_CGI_H__ */
