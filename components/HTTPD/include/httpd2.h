#pragma once
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
void httpdSuspend(HttpdConnData *connData, int timeout);