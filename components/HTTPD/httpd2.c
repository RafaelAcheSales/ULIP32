#include "http2.h"
void httpdSuspend(HttpdConnData *connData, int timeout)
{
	if (!connData) return;

	connData->timeout = timeout;
}