idf_component_register(SRCS "httpd2.c" "auth.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES nvs_flash esp_http_server
                    REQUIRES libesphttpd)
