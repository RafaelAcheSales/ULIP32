idf_component_register(SRCS "httpd.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES nvs_flash esp_http_server)
