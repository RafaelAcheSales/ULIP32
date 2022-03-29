#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <lwip/dns.h>
#include <esp_http_client.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#include "osapi.h"
#include "debug.h"
#include "rtc2.h"
#include "http.h"

#define HTTP_USER_AGENT         "uTech"
#define HTTP_DEFAULT_TIMEOUT    30000

typedef struct {
    char *path;
    char *auth;
    int port;
    char *post_data;
    char *headers;
    char *hostname;
    bool secure;
    int retries;
    http_callback user_callback;
    char date[32];
    esp_http_client_handle_t client;
    TaskHandle_t task;
    char *data;
    int size;
    os_timer_t timer;
} request_args;

static char *user_agent = NULL;

static void http_request_retry(request_args *req);


static char *esp_strstrip(char *string)
{
    int len;
    int i;
    int k;

    if (!string) return NULL;

    len = strlen(string);
    for (i = 0; i < len; i++) {
        if (string[i] != ' ')
            break;
    }
    if (i) {
        if (i < len) {
            memcpy(string, string + i, len - i);
            len -= i;
            string[len] = '\0';
        } else {
            string[0] = '\0';
            return string;
        }
    }
    for (k = len - 1; k >= 0; k--) {
        if (string[k] != ' ')
            break;
        string[k] = '\0';
    }

    return string;
}

static esp_err_t http_event(esp_http_client_event_t *e)
{
    request_args *req = (request_args *)e->user_data;

    switch(e->event_id) {
        case HTTP_EVENT_ERROR:
            os_info("HTTP", "HTTP_EVENT_ERROR: %x", e->data);
            break;
        case HTTP_EVENT_ON_CONNECTED:
            os_info("HTTP", "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            os_info("HTTP", "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            os_info("HTTP", "HTTP_EVENT_ON_HEADER");
            break;
        case HTTP_EVENT_ON_DATA:
            req->data = realloc(req->data, req->size + e->data_len + 1);
            if (req->data) {
                memcpy(req->data + req->size, e->data, e->data_len);
                req->size += e->data_len;
                req->data[req->size] = '\0';
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            os_info("HTTP", "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            os_info("HTTP", "HTTP_EVENT_DISCONNECTED");
            break;
    }

    return ESP_OK;
}
const char * errorToString(int8_t error){
    switch(error){
        case 0: return "OK";
        case -1: return "Out of memory error";
        case -2: return "Buffer error";
        case -3: return "Timeout";
        case -4: return "Routing problem";
        case -5: return "Operation in progress";
        case -6: return "Illegal value";
        case -7: return "Operation would block";
        case -8: return "Adress in use";
        case -9: return "Already connecting";
        case -10: return "Already connected";
        case -11: return "Not connected";
        case -12: return "Low-level netif error";
        case -13: return "Connection aborted";
        case -14: return "Connection reset";
        case -15: return "Connection closed";
        case -16: return "Illegal argument";
        default: return "UNKNOWN";

        
    }
}
static void http_destroy(request_args *req)
{
    if (!req) return;

    if (req->task)
        vTaskDelete(req->task);
    if (req->client)
        esp_http_client_cleanup(req->client);
    free(req->post_data);
    free(req->headers);
    free(req->auth);
    free(req->path);
    free(req->hostname);
    if (req->data)
        free(req->data);
    free(req);
}

static void http_task(void *arg)
{
    request_args *req = (request_args *)arg;
    esp_err_t err;
    int status;
    char data[256];
    esp_http_client_get_url(req->client, data, sizeof(data));
    
    os_info("HTTP", "HTTP request: %s", data);
    // req->client = esp_http_client_init();
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(req->client));

    status = esp_http_client_get_status_code(req->client);
    os_debug("HTTP", "HTTP request [%p] status code [%d]",
             req, status);
    if (status >= 200 && status < 300) {
        if (req->user_callback != NULL)
            req->user_callback(req->path, status, req->data,
                               req->size);
        http_destroy(req);
    } else {
        if (req->retries-- <= 0) {
            os_info("HTTP", "HTTP request [%p] failed", req);
            if (req->user_callback != NULL)
                req->user_callback(req->path, status, req->data,
                                   req->size);
            http_destroy(req);
        } else {
            os_info("HTTP", "HTTP request [%p] retry", req);
            req->task = NULL;
            esp_http_client_cleanup(req->client);
            req->client = NULL;
            if (req->data) {
                free(req->data);
                req->data = NULL;
            }
            req->size = 0;
            os_timer_setfn(&req->timer, (os_timer_func_t *)http_request_retry, req);
            os_timer_arm(&req->timer, HTTP_DEFAULT_TIMEOUT, 0);
            vTaskDelete(req->task);
        }
    }
}

static void dns_callback(const char *hostname, const ip_addr_t *addr,
                         void *arg)
{
    request_args * req = (request_args *)arg;
    esp_http_client_config_t config;
    char headers[512];
    char auth[128];
    char *p;
    char *k;
    char *v;
    int rc;
    
    if (addr) {
        os_info("HTTP", "DNS lookup succeeded. IP=%s",
                ip4addr_ntoa((const ip4_addr_t *)addr));
        if (req->task) {
            vTaskDelete(req->task);
            req->task = NULL;
        }
        if (req->client) {
            esp_http_client_cleanup(req->client);
            req->client = NULL;
        }
        memset(&config, 0, sizeof(config));
        config.host = ip4addr_ntoa((const ip4_addr_t *)addr);
        config.path = req->path;
        config.port = req->port;
        config.user_agent = user_agent;
        config.timeout_ms = HTTP_DEFAULT_TIMEOUT;
        config.max_authorization_retries = 1;
        config.event_handler = http_event;
        config.user_data = req;
        config.transport_type = req->secure ?
                                HTTP_TRANSPORT_OVER_SSL :
                                HTTP_TRANSPORT_OVER_TCP;
        config.is_async = FALSE;
        if (req->auth) {
            strcpy(auth, req->auth);
            config.username = auth;
            if ((p = strchr(req->auth, ':'))) {
                *p++ = '\0';
                config.password = p;
            }
        }
        req->client = esp_http_client_init(&config);
        if (!req->client) {
            os_error("HTTP", "HTTP request [%p] client error", req);
            if (req->user_callback != NULL)
                req->user_callback(req->path, -1, NULL, 0);
            http_destroy(req);
            return;
        }
        if (req->post_data) {
            esp_http_client_set_method(req->client, HTTP_METHOD_POST);
            esp_http_client_set_post_field(req->client, req->post_data,
                                           strlen(req->post_data));
        } else {
            esp_http_client_set_method(req->client, HTTP_METHOD_GET);
        }
        esp_http_client_set_header(req->client, "Date", req->date);
        if (req->headers) {
            strcpy(headers, req->headers);
            k = p = headers;
            while ((p = strstr(p, "\r\n"))) {
                *p = '\0';
                if ((v = strchr(k, ':'))) {
                    *v++ = '\0';
                    k = esp_strstrip(k);
                    v = esp_strstrip(v);
                    esp_http_client_set_header(req->client, k, v);
                }
                p += 2;
                k = p;
            }
        }
        // ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(req->client));
        // rc = pdPASS;
        rc = xTaskCreatePinnedToCore(http_task, "HTTP", 4096, req, 3,
                                     &req->task, 1);
        if (rc != pdPASS) {
            os_error("HTTP", "HTTP request [%p] task error",
                     req);
            if (req->user_callback != NULL)
                req->user_callback(req->path, -1, NULL, 0);
            http_destroy(req);
        }
    } else {
        os_warning("HTTP", "HTTP request [%p] DNS failed for [%s]",
                   req, hostname);
        if (req->user_callback != NULL)
            req->user_callback(req->path, -1, NULL, 0);
        http_destroy(req);
    }
}

static void http_request_retry(request_args *req)
{
    ip_addr_t addr;
    err_t err;

    os_info("HTTP", "HTTP request [%p] retry", req);

    err = dns_gethostbyname(req->hostname, &addr,
                            dns_callback, req);
    ESP_LOGE("HTTP", "error %s", errorToString(err));
    switch (err) {
        case ERR_OK:
            dns_callback(req->hostname, &addr, req);
            break;
        case ERR_INPROGRESS:
            break;
        default:
            os_warning("HTTP", "HTTP request [%p] DNS error code [%d]",
                       req, err);
            if (req->retries-- <= 0) {
                dns_callback(req->hostname, NULL, req); // Handle all DNS errors the same way.
            } else {
                os_timer_setfn(&req->timer, (os_timer_func_t *)http_request_retry, req);
                os_timer_arm(&req->timer, HTTP_DEFAULT_TIMEOUT, 0);
            }
            break;
    }
}

void http_raw_request(const char *hostname, int port, bool secure,
                      const char *auth, const char *path, const char *post_data,
                      const char *headers, int retries, http_callback user_callback)
{
    request_args *req;
    ip_addr_t addr;
    struct tm *tm;
    err_t err;

    if (!hostname) return;

    req = (request_args *)calloc(1, sizeof(request_args));
    if (!req) return;
    req->hostname = strdup(hostname);
    req->auth = strdup(auth);
    req->path = strdup(path);
    req->port = port;
    req->secure = secure;
    req->headers = strdup(headers);
    req->post_data = strdup(post_data);
    req->retries = retries;
    req->user_callback = user_callback;

    /* Date */
    tm = rtc_localtime();
    snprintf(req->date, sizeof(req->date), "%s, %02d %s %d %02d:%02d:%02d",
             rtc_weekday(tm), tm->tm_mday, rtc_month(tm),
             tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec);

    err = dns_gethostbyname(hostname, &addr, dns_callback, req);
    ESP_LOGE("HTTP", "error %s", errorToString(err));
    switch (err) {
        case ERR_OK:
            dns_callback(hostname, &addr, req);
            break;
        case ERR_INPROGRESS:
            break;
        default:
            os_warning("HTTP", "HTTP request [%p] DNS error [%d]",
                       req, err);
            if (req->retries-- <= 0) {
                dns_callback(hostname, NULL, req);
            } else {
                os_timer_setfn(&req->timer, (os_timer_func_t *)http_request_retry, req);
                os_timer_arm(&req->timer, HTTP_DEFAULT_TIMEOUT, 0);
            }
            break;
    }
}

void http_init(const char *agent)
{
    if (!agent)
        agent = HTTP_USER_AGENT;
    if (user_agent)
        free(user_agent);
    user_agent = (char *)malloc(strlen(agent) + 1);
    strcpy(user_agent, agent);
}

void http_release(void)
{
    if (user_agent) {
        free(user_agent);
        user_agent = NULL;
    }
}
