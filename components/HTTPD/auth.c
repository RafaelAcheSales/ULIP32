/*
HTTP auth implementation. Only does basic authentication for now.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include "mbedtls/base64.h"
#include "auth.h"

static AuthGetUserPw authGetUser = NULL;


void authSetCallback(AuthGetUserPw authCallback)
{
    authGetUser = authCallback;
}

int authBasic(HttpdConnData *connData)
{
    char userpass[AUTH_MAX_USER_LEN + AUTH_MAX_PASS_LEN + 2];
    char auth[AUTH_MAX_USER_LEN + AUTH_MAX_PASS_LEN + 2];
    char *user = NULL;
    char *pass = NULL;
    char hdr[512];
    int r;

    if (connData->isConnectionClosed) return HTTPD_CGI_DONE;

    if (!authGetUser) {
        httpdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
        httpdStartResponse(connData, 403);
        httpdHeader(connData, "Content-Length", "0");
        httpdEndHeaders(connData);
        return HTTPD_CGI_DONE;
    }

    r = httpdGetHeader(connData, "Authorization", hdr, sizeof(hdr) - 1);
    if (r && strncmp(hdr, "Basic", 5) == 0) {
        // r = base64Decode(strlen(hdr) - 6, hdr + 6, sizeof(auth),
        //                  (unsigned char *)auth);
        unsigned int olen;
        r = mbedtls_base64_decode((uint8_t *)auth, sizeof(auth), &olen
                                    , (uint8_t *)hdr+6, strlen(hdr)-6);
        auth[r] = 0;
        user = strtok_r(auth, ":", &pass);
        if (authGetUser(connData, user, pass))
            return HTTPD_CGI_AUTHENTICATED;
        httpdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
        httpdStartResponse(connData, 403);
        httpdHeader(connData, "Content-Length", "0");
        httpdEndHeaders(connData);
        return HTTPD_CGI_DONE;
    }

    httpdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
    httpdStartResponse(connData, 401);
    httpdHeader(connData, "WWW-Authenticate", "Basic realm=\""HTTP_AUTH_REALM"\"");
    httpdHeader(connData, "Content-Length", "0");
    httpdEndHeaders(connData);
    return HTTPD_CGI_DONE;
}

int authBasicGetUsername(HttpdConnData *connData,
                                           char *username, int len)
{
    char auth[AUTH_MAX_USER_LEN + AUTH_MAX_PASS_LEN + 2];
    char *user = NULL;
    char *pass = NULL;
    char hdr[512];
    int r;

    if (!connData || !username) return -1;

    r = httpdGetHeader(connData, "Authorization", hdr, sizeof(hdr) - 1);
    if (r && strncmp(hdr, "Basic", 5) == 0) {
        // r = base64Decode(strlen(hdr) - 6, hdr + 6, sizeof(auth),
        //                  (unsigned char *)auth);
        unsigned int olen;
        r = mbedtls_base64_decode((uint8_t *)auth, sizeof(auth), &olen
                                    , (uint8_t *)hdr+6, strlen(hdr)-6);
        if (r < 0) r = 0;
        auth[r] = 0;
        user = strtok_r(auth, ":", &pass);
        strncpy(username, user, len - 1);
        username[len - 1] = '\0';
        return 0;
    }

    return -1;
}

int authBasicGetPassword(HttpdConnData *connData,
                                           char *password, int len)
{
    char auth[AUTH_MAX_USER_LEN + AUTH_MAX_PASS_LEN + 2];
    char *user = NULL;
    char *pass = NULL;
    char hdr[512];
    int r;

    if (!connData || !password) return -1;

    r = httpdGetHeader(connData, "Authorization", hdr, sizeof(hdr) - 1);
    if (r && strncmp(hdr, "Basic", 5) == 0) {
        // r = base64Decode(strlen(hdr) - 6, hdr + 6, sizeof(auth),
        //                  (unsigned char *)auth);
        unsigned int olen;
        r = mbedtls_base64_decode((uint8_t *)auth, sizeof(auth), &olen
                                    , (uint8_t *)hdr+6, strlen(hdr)-6);
        if (r < 0) r = 0;
        auth[r] = 0;
        user = strtok_r(auth, ":", &pass);
        strncpy(password, pass, len - 1);
        password[len - 1] = '\0';
        return 0;
    }

    return -1;
}
