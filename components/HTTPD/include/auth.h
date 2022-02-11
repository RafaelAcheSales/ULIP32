#ifndef AUTH_H
#define AUTH_H

#include "libesphttpd/httpd.h"

#ifndef HTTP_AUTH_REALM
#define HTTP_AUTH_REALM "utech"
#endif

#define AUTH_MAX_USER_LEN   64
#define AUTH_MAX_PASS_LEN   64

//Parameter given to authWhatever functions. This callback returns the usernames/passwords the device
//has.
typedef int (*AuthGetUserPw)(HttpdConnData *connData,
                             char *user, char *pass);

void authSetCallback(AuthGetUserPw authCallback);
int authBasic(HttpdConnData *connData);
int authBasicGetUsername(HttpdConnData *connData,
                                           char *user, int len);
int authBasicGetPassword(HttpdConnData *connData,
                                           char *password, int len);

#endif
