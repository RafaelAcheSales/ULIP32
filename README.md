= Diferenças entre ULIP e ULET por módulo =

antigo => novo

== Mudanças globais: ==

prefixo "os_" removido de várias funções como os_free, os_printf, os_malloc, etc

FALSE, TRUE => false, true

zalloc(x) => calloc(1, x)

=== Timer: ===

os_timer_t => esp_timer_handle_t

os_timer_setfn => esp_timer_create_args_t & esp_timer_create()

os_timer_arm() => esp_timer_start_periodic() & esp_timer_start_once()

os_timer_disarm() => esp_timer_stop()

rtc_localtime() => time() & localtime()

=== Print e Debug ===

os_debug, os_info, etc => ESP_LOGD, ESP_LOGI, ESP_LOGE

= Módulos =

== ACCOUNT: ==

==== Includes adicionados: ====

 * stdbool.h
 * esp_partition.h
 * esp_timer.h, string.h
 * esp_log.h
 * sha1.h
 * base64.h
 * FreeRTOS.h
 * task.h
 * time.h.

==== Mudanças ====

Utilizado o sistema esp_partition para armazenar, ler e modificar as accounts. As partições são encontradas usando esp_partition_find_first()

spi_flash_read() => esp_partition_read()

spi_flash_erase_sector() => esp_partition_erase_range()

spi_flash_write() => esp_partition_write()

==== SHA1 e Base64 ====

SHA1_CTX => mbedtls_sha1_context

SHA1_Init() => mbedtls_sha1_init()

SHA1_Update() => mbedtls_sha1_update_ret()

SHA1_Final() => mbedtls_sha1_finish_ret()

base64Encode() => mbedtls_base64_encode()

== AP: ==

==== Módulo que lida com o wifi ====

Inicialização feita por: 

wifi_init_softap(bool ap_mode, char * ip, char * netmask, char * gateway, bool dhcp, char * ssid, char * password, uint8_t channel, bool disable, void (* got_ip_callback_set)(void))

Onde ap_mode significa se o wifi sera inicializado no modo Station ou AcessPoint (deve ser chamado pelo módulo CTL ao saber se o botao AP_MODE está sendo pressionado durante inicialização). got_ip_callback_set é a função que será chamada quando o módulo wifi receber seu IP .

wifi_event_handler vai servir como máquina de estados por receber o evento atual como descrito [[https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html#_CPPv412wifi_event_t | aqui ]]

== BLUETOOTH: ==

Ainda em desenvolvimento...

== CONFIG: ==

==== Includes adicionados: ====

 * stdint.h
 * string.h
 * config2.h
 * stdbool.h
 * esp_log.h
 * esp_system.h
 * FreeRTOS.h
 * task.h
 * nvs_flash.h
 * nvs.h

==== Configs adicionadas: ====

 * bool eth_enable

 * bool eth_dhcp

 * char eth_ip_adress[16]

 * char eth_netmask[16]

 * char eth_gateway[16]

==== Mudanças ====

Utiliza o sistema nvs_flash para armazenar as configuraçôes. Documentação da espressif [[https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html|aqui]]

nvs_open_from_partition() para iniciar procedimentos na partição utilizando o struct nvs_handle_t

nvs_get_blob() para ler as variáveis da flash

nvs_set_blob() para alterar as variáveis salvas

nvs_commit() para salvar as alteraçôes

nvs_close() para finalizar as alterações

nvs_erase_all() deleta todas as informações da partição

== CTL: ==

==== Includes adicionados: ====

 * esp_log.h
 * esp_timer.h
 * FreeRTOS.h
 * task.h
 * ap.h

== DEBUG: ==

Lida apenas com o log via UDP já que os prints normais são disponibilizados pela espressif (ESP_LOG).

int udp_logging_init() recebe a função customizada que substituirá o print utilizando a função "esp_log_set_vprintf()" e inicializa o socket.

udp_logging_vprintf() é a função customizada que envia os logs via sendto() da biblioteca lwip.

O níveis de debug já são gerenciados pelo esp_log.h


== ETH: ==

Toda lógica é feita no arquivo lan8720.h

eth_start() é chamada para inicializar o módulo.

eth_event_handler() é a função que lida com os eventos de conexão e desconexão via event_id [[https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_eth.html|leia]].

got_ip_event_handler() lida com os eventos de IP.

O clock da placa lan8720 possui o enable atrelado ao GPIO-17 para que só seja iniciado após o boot e não atrapalhe o GPIO-00 que serve como input do clock para referência.

Na inicialização são chamadas as funções: 

 * esp_eth_driver_install()

 * esp_netif_attach()

 * esp_eth_start()

== FPM: ==

==== Includes adicionados: ====

 * sys/time.h
 * string.h
 * esp_timer.h
 * esp_log.h

==== Mudanças ====

Adicionado a opção de baud rate. Durante inicialização ou timeout do comando enviado, o código testa trocar de baud rate pois isto pode estar causando falha na comunicação. Adicionado uma etapa de inicialização onde o baud rate é trocado de 9600 para 115200

== GPIO: ==

==== Includes adicionados: ====

 * FreeRTOS.h
 * task.h
 * freertos/queue.h
 * esp_log.h

==== Mudanças ====

Na inicialização é necessário chamar gpio_config() e gpio_install_isr_service()

Biblioteca para GPIO [[https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html|aqui]]

gpio_drv_release() deve ainda ser implementado, testado e decidido se deve chamar gpio_uninstall_isr_service() ou apenas desabilitar as interrupções

== HTTP: == 

==== Includes adicionados: ====

 * http.h
 * esp_http_client.h
 * esp_log.h
 * string.h

==== Mudanças ====

Toda estrutura está diferente do ULIP. Foi utilizado o exemplo da espressif como base para os requests http, apenas mantendo a convenção de chamada http_raw_request() para evitar mudança em como este módulo é utilizado na aplicação.

Eventos http são recebidos na função _http_event_handle()

esp_http_client_init() recebe como parâmetro um struct com as informações necessárias para inicializar uma conexão.

esp_http_client_set_* é a função que vai settar as informações do request como por exemplo tipo de método (POST, GET, etc...) e Headers

esp_http_client_perform() realiza de fato o request e bloqueia a task atual.

esp_http_client_get_* retorna as informações da resposta do request tal como método e headers


