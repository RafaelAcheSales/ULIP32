#include "http.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "string.h"
#define MAX_HTTP_buffer 5000
#define MAX_URL_SIZE 2048
static const char * TAG = "HTTP";
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    static char * buffer;
    static int total_len ;
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            // ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            // ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            total_len = 0;
            break;
        case HTTP_EVENT_HEADER_SENT:
            // ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            // ESP_LOGI(TAG, "HTTP_EsVENT_ON_HEADER");
            // ESP_LOGI(TAG,"%s", (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            // ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // ESP_LOGI(TAG,"%s", (char*)evt->data);
            if (!esp_http_client_is_chunked_response(evt->client) && total_len < 2048) {
                if (evt->user_data) {
                    memcpy(evt->user_data + total_len, evt->data, evt->data_len);
                    
                } else {
                    if (buffer == NULL) {
                        buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        total_len = 0;
                        if (buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(buffer + total_len, evt->data, evt->data_len);
                }
                total_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            // ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            if (buffer != NULL) {
                printf("%s", buffer);
                free(buffer);
                buffer = NULL;
            } else {
               
            }
            total_len = 0;

            break;
        case HTTP_EVENT_DISCONNECTED:
            // ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            if (buffer != NULL) {
                free(buffer);
                buffer = NULL;
            }
            break;
    }
    return ESP_OK;
}
void start_http_client() {
    
    esp_http_client_config_t config = {
        .url = "http://api.plos.org/search?q=title:DNA",
        .event_handler = _http_event_handle
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        int size = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),size);
        // int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_buffer);
        // ESP_LOGI(TAG, "%s", );
    }
    esp_http_client_cleanup(client);
}


void http_raw_request(const char *hostname, int port, bool secure,
                                        const char *user, const char *passwd, const char *path, const char *post_data,
                                        const char *header_key, const char *header_value, int retries, http_callback user_callback) {
    static char local_response_buffer[MAX_HTTP_buffer] = {0};
    char url[MAX_URL_SIZE];
    char key[256];
    char *value;
    ESP_LOGD(TAG, "http_raw_request: hostname=%s Header=%s:%s", hostname, header_key, header_value);

    if (secure)
        ESP_LOGD("HTTP", "user %s, passwd %s, path %s", user, passwd, path);
    

    esp_http_client_config_t config = {
        .port = port,
        .max_authorization_retries = retries,
        .host = hostname,
        .path = path,
        .user_data = local_response_buffer,
        .event_handler = _http_event_handle,
        .username = user,
        .password = passwd,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (post_data != NULL)
    {
        //POST
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
        esp_http_client_set_method(client, HTTP_METHOD_POST);
    } else {
        //GET
        esp_http_client_set_method(client, HTTP_METHOD_GET);
    }
    if (!strcmp(header_key, ""))
    {
        esp_http_client_set_header(client, header_key, header_value);
    }
    // esp_http_client_set_header(client, header_key, header_value);

    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "HTTP Basic Auth Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_get_header(client, key, &value);
    esp_http_client_get_url(client, url,MAX_URL_SIZE);
    user_callback(url,local_response_buffer,esp_http_client_get_status_code(client),key, value, esp_http_client_get_content_length(client));
    esp_http_client_cleanup(client);
    
}