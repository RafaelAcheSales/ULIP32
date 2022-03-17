/* vim: set ts=4 et sta noai cin: */
#include <sys/queue.h>
#include <time.h>   
#include "http.h"
#include "config2.h"
#include "qrcode2.h"
#include "ctl.h"
#include "account.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "ap.h"
#include "main.h"
#include "esp_sntp.h"
#include "mbedtls/base64.h"
#include "udp_logging.h"    
#include "fpm.h"

// #include "ulip_model.h"
// #include "ulip_core.h"
#include "ulip_cgi.h"


#define CGI_PAGE_TOP        (1 << 28)
#define CGI_PAGE_BODY       (2 << 28)
#define CGI_PAGE_BOTTOM     (4 << 28)

#define CGI_CACHE_SIZE      8
#define DCACHE_FLASH_ATTR \
__attribute__ ((section (".flash.text"))) \
__attribute__ ((aligned (4)))

typedef struct cgi_cache {
    uint32_t ip;
    char url[32];
    uint32_t etag;
    uint32_t timestamp;
} cgi_cache_t;

static cgi_cache_t cache[CGI_CACHE_SIZE] = { 0 };
static esp_timer_handle_t update_timer;
static esp_timer_handle_t scan_timer;
static char *scan_html = NULL;

DCACHE_FLASH_ATTR
static const char STYLE[] = {
"body{\
margin:0;\
}\
body {\
font-family:\"Helvetica Neue\",Helvetica,Arial,sans-serif;\
font-size:16px;\
line-height:1.428571429;\
color:#333;\
background-color:#fff;\
border-top:none;\
}\
a {\
color:#428bca;\
text-decoration:none\
}\
a:hover,a:focus {\
color:#2a6496;\
text-decoration:underline;\
}\
a:focus {\
outline:thin dotted;\
outline:5px auto -webkit-focus-ring-color;\
outline-offset:-2px;\
}\
.panel {\
margin-bottom:20px;\
background-color:#fff;\
border:1px solid transparent;\
border-color:#428bca;\
border-radius:4px;\
-webkit-box-shadow:0 1px 1px rgba(0,0,0,0.05);\
box-shadow:0 1px 1px rgba(0,0,0,0.05);\
}\
.panel-body {\
padding:15px;\
display:table;\
width:100%;\
}\
.panel-heading {\
padding:10px 15px;\
border-bottom:1px solid transparent;\
border-top-right-radius:3px;\
border-top-left-radius:3px;\
border-top:none;\
}\
.panel-title {\
margin-top:0;\
margin-bottom:0;\
font-size:16px;\
color:inherit;\
}\
panel-primary {\
border-color:#428bca;\
}\
.panel-primary>.panel-heading {\
color:#fff;\
background-color:#428bca;\
border-color:#428bca;\
}\
.pull-right {\
float:right!important;\
}\
.form-control {\
display:inline;\
width:50%;\
height:32px;\
padding:2px 12px;\
font-size:14px;\
line-height:1.428571429;\
color:#555;\
vertical-align:middle;\
background-color:#fff;\
background-image:none;\
border:1px solid #ccc;\
border-radius:4px;\
-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,0.075);\
box-shadow:inset 0 1px 1px rgba(0,0,0,0.075);\
-webkit-transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s;\
transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s;\
}\
.btn {\
display:inline-block;\
padding:6px 12px;\
margin-bottom:0;\
font-size:14px;\
font-weight:normal;\
line-height:1.428571429;\
text-align:center;\
white-space:nowrap;\
vertical-align:middle;\
cursor:pointer;\
background-image:none;\
border:1px solid transparent;\
border-radius:4px;\
-webkit-user-select:none;\
-moz-user-select:none;\
-ms-user-select:none;\
-o-user-select:none;\
user-select:none;\
}\
.btn-default {\
color:#333;\
background-color:#ebebeb;\
border-color:#ccc;\
}\
.btn-default:hover,.btn-default:focus,.btn-default:active,.btn-default.active,.open .dropdown-toggle.btn-default {\
color:#333;\
background-color:#ebebeb;\
border-color:#adadad;\
}\
.btn-primary {\
color:#fff;\
background-color:#428bca;\
border-color:#357ebd;\
}\
.btn-primary:hover,.btn-primary:focus,.btn-primary:active,.btn-primary.active,.open .dropdown-toggle.btn-primary {\
color:#fff;\
background-color:#3276b1;\
border-color:#285e8e;\
}\
.alert-info {\
color: #31708f;\
background-color: #d9edf7;\
border-color: #bce8f1;\
padding: 6px;\
}\
.stat {\
color: #fff !important;\
background-color: #28a745;\
font-weight:bold;\
text-align:center;\
height:20px !important;\
padding:6px 12px;\
-webkit-box-sizing: content-box;\
-moz-box-sizing: content-box;\
box-sizing: content-box;\
}\
.info {\
color: #31708f;\
background-color: #d9edf7;\
border-color: #bce8f1;\
font-weight:bold;\
text-align:center;\
width:350px !important;\
height:20px !important;\
padding:6px 12px;\
-webkit-box-sizing: content-box;\
-moz-box-sizing: content-box;\
box-sizing: content-box;\
}\
.week {\
width:40%;\
}\
.date {\
width:16%;\
}\
.hour {\
width:20%;\
}\
.danger {\
background-color:#dc3545;\
}\
.success {\
background-color:#28a745;\
}\
.default {\
background-color:#ccc;\
border-color:#ccc;\
}\
.fullscreen {\
min-width:100%;\
min-height:calc(100vh - 15px);\
overflow:hidden;\
}\
input, select {\
height:38px !important;\
-webkit-box-sizing: border-box;\
-moz-box-sizing: border-box;\
box-sizing: border-box;\
}"
};

DCACHE_FLASH_ATTR
static const char PAGE_TOP[] = {
"<!DOCTYPE html>\
<html>\
<head>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
<meta http-equiv=\"Content-Type\" content=\"text/html;charset=iso-8859-1\" >\
<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\"/>"
#ifdef CONFIG__MLI_1W_TYPE__
"<TITLE>MLI-1W - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WQ_TYPE__)
"<TITLE>MLI-1WQ - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WQB_TYPE__)
"<TITLE>MLI-1WQB - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WQF_TYPE__)
"<TITLE>MLI-1WQF - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WB_TYPE__)
"<TITLE>MLI-1WB - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WF_TYPE__)
"<TITLE>MLI-1WF - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WRQ_TYPE__)
"<TITLE>MLI-1WRQ - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WR_TYPE__)
"<TITLE>MLI-1WR - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WRF_TYPE__)
"<TITLE>MLI-1WRF - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WRS_TYPE__)
"<TITLE>MLI-1WRS - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WLS_TYPE__)
"<TITLE>MLI-1WLS - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WRG_TYPE__)
"<TITLE>MLI-1WRG - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WLG_TYPE__)
"<TITLE>MLI-1WLG - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WRP_TYPE__)
"<TITLE>MLI-1WRP - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WRC_TYPE__)
"<TITLE>MLI-1WRC - &micro;Tech Tecnologia</TITLE>"
#elif defined(CONFIG__MLI_1WLC_TYPE__)
"<TITLE>MLI-1WLC - &micro;Tech Tecnologia</TITLE>"
#endif
"</head>\
<body onload=\"load_cfg();\">\
<table border=\"0\" bgcolor=\"#ffffff\" width=\"100%\" height=\"100%\" class=\"fullscreen\">\
<tr bgcolor=\"#f8f8f8\" style=\"height:50px;\">\
<td colspan=\"2\">"
#if 0
"<a href=\"http://www.utech.com.br/\"><img style=\"max-width:120px;padding:10px;vertical-align:middle;\" src=imgs/logo_utech.png border=0></a>"
#endif
#ifdef CONFIG__MLI_1W_TYPE__
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1W - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WQ_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WQ - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WQB_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WQB - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WQF_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WQF - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WB_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WB - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WF_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WF - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WRQ_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRQ - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WR_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WR - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WRF_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRF - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WRS_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRS - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WLS_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WLS - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WRG_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRG - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WLG_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WLG - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WRP_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRP - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WRC_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRC - Leitor IP</b></a>"
#elif defined(CONFIG__MLI_1WLC_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WLC - Leitor IP</b></a>"
#endif
"</td>\
</tr>\
<tr valign=\"top\">\
<td rowspan=\"200\" bgcolor=\"#0075be\" valign=\"top\" width=\"200\">\
<table align=\"center\">\
<tr id=\"net\" bgcolor=\"#032449\"><td style=\"padding:12px 40px;\"><a style=\"color:#ffffff\" href=\"?menuopt=1\"><b>Rede</b></a></td></tr>\
<tr id=\"acc\" bgcolor=\"#032449\"><td style=\"padding:12px 40px;\"><a style=\"color:#ffffff\" href=\"?menuopt=2\"><b>Configura&ccedil;&otilde;es</b></a></td></tr>\
<tr id=\"http\" bgcolor=\"#032449\"><td style=\"padding:12px 40px;\"><a style=\"color:#ffffff\" href=\"?menuopt=3\"><b>Integra&ccedil;&atilde;o</b></a></td></tr>\
<tr id=\"prog\" bgcolor=\"#032449\"><td style=\"padding:12px 40px;\"><a style=\"color:#ffffff\" href=\"?menuopt=8\"><b>Programa&ccedil;&atilde;o</b></a></td></tr>\
<tr id=\"user\" bgcolor=\"#032449\"><td style=\"padding:12px 40px;\"><a style=\"color:#ffffff\" href=\"?menuopt=4\"><b>Usu&aacute;rios</b></a></td></tr>"
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
"<tr id=\"log\" bgcolor=\"#032449\"><td style=\"padding:12px 40px;\"><a style=\"color:#ffffff\" href=\"?menuopt=5\"><b>Acessos</b></a></td></tr>"
#else
"<tr id=\"log\" bgcolor=\"#032449\"><td style=\"padding:12px 40px;\"><a style=\"color:#ffffff\" href=\"?menuopt=5\"><b>Telemetria</b></a></td></tr>"
#endif
"<tr id=\"stat\" bgcolor=\"#032449\"><td style=\"padding:12px 40px;\"><a style=\"color:#ffffff\" href=\"?menuopt=6\"><b>Status</b></a></td></tr>\
<tr id=\"admin\" bgcolor=\"#032449\"><td style=\"padding:12px 40px;\"><a style=\"color:#ffffff\" href=\"?menuopt=7&subopt=1\"><b>Admin</b></a></td></tr>\
</table>\
</td>\
</tr>"
};

#define PAGE_BOTTOM \
"</body></html>"

 
DCACHE_FLASH_ATTR static const char INDEXREDE[] = {
"<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\"> \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Configura&ccedil;&atilde;o de Rede</b></h3> \
</div> \
<div class=\"panel-body\"> \
<FORM name=\"NETWORK\" action=\"save\" method=\"POST\">   \
<table border=\"0\" class=\"table\" align=\"left\" width=\"98%\" height=\"100%\"> \
<tr> \
<td colspan=\"2\" class=\"alert-info\"><b>Wireless</b></td> \
</tr> \
<tr> \
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Hotspot</b></td> \
<td><INPUT type=\"checkbox\" name=\"hotspot\"></td> \
</tr><tr> \
<td><b>Habilitar Ponto de Acesso</b></td> \
<td><INPUT type=\"checkbox\" name=\"apmode\"></td> \
</tr><tr> \
<td><b>SSID</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"ssid\" maxlength=\"20\"></td> \
</tr><tr> \
<td><b>Senha</b></td> \
<td><INPUT type=\"password\" class=\"form-control\" name=\"passwd\" maxlength=\"20\"></td> \
</tr><tr> \
<td><b>Canal</b></td> \
<td> \
<SELECT class=\"form-control\" name=\"channel\"> \
<option value=\"1\">1</option> \
<option value=\"2\">2</option> \
<option value=\"3\">3</option> \
<option value=\"4\">4</option> \
<option value=\"5\">5</option> \
<option value=\"6\">6</option> \
<option value=\"7\">7</option> \
<option value=\"8\">8</option> \
<option value=\"9\">9</option> \
<option value=\"10\">10</option> \
<option value=\"11\">11</option> \
<option value=\"12\">12</option> \
<option value=\"13\">13</option> \
</SELECT> \
</td> \
</tr><tr> \
<td><b>Intervalo de Beacon</b></td> \
<td><INPUT type=\"number\" class=\"form-control\" name=\"beacon\" maxlength=\"5\"><b> ms</b></td> \
</tr><tr> \
<td style=\"padding-bottom:10px;\"><b>Esconder SSID</b></td> \
<td><INPUT type=\"checkbox\" name=\"hidessid\"></td> \
</tr> \
<tr> \
<td colspan=\"2\">&nbsp;</td> \
</tr> \
<tr> \
<td colspan=\"2\" class=\"alert-info\"><b>Rede Wifi</b></td> \
</tr> \
<tr> \
<td style=\"padding:10px 0px 10px 0px;\"><b>Enable</b></td> \
<td><INPUT type=\"checkbox\" name=\"wifi_enable\"></td> \
</tr> \
<tr> \
<td style=\"padding:10px 0px 10px 0px;\"><b>DHCP</b></td> \
<td><INPUT type=\"checkbox\" name=\"dhcp\"></td> \
</tr> \
<tr> \
<td><b>Endere&ccedil;o IP</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"ip\" maxlength=\"15\"></td> \
</tr> \
<tr> \
<td><b>M&aacute;scara</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"netmask\" maxlength=\"15\"></td> \
</tr> \
<tr> \
<td><b>Gateway</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"gateway\" maxlength=\"15\"></td> \
</tr> \
<tr> \
<td colspan=\"2\">&nbsp;</td> \
</tr> \
<tr> \
<td colspan=\"2\" class=\"alert-info\"><b>Rede Ethernet</b></td> \
</tr> \
<tr> \
<td style=\"padding:10px 0px 10px 0px;\"><b>Enable</b></td> \
<td><INPUT type=\"checkbox\" name=\"eth_enable\"></td> \
</tr> \
<tr> \
<td style=\"padding:10px 0px 10px 0px;\"><b>DHCP</b></td> \
<td><INPUT type=\"checkbox\" name=\"eth_dhcp\"></td> \
</tr> \
<tr> \
<td><b>Endere&ccedil;o IP</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"eth_ip\" maxlength=\"15\"></td> \
</tr> \
<tr> \
<td><b>M&aacute;scara</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"eth_netmask\" maxlength=\"15\"></td> \
</tr> \
<tr> \
<td><b>Gateway</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"eth_gateway\" maxlength=\"15\"></td> \
</tr> \
<tr> \
<tr> \
<td colspan=\"2\">&nbsp;</td> \
</tr> \
<tr> \
<td colspan=\"2\" class=\"alert-info\"><b>Rede geral</b></td> \
</tr> \
<tr> \
<td><b>DNS</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"dns\" maxlength=\"15\"></td> \
</tr> \
<tr> \
<td><b>NTP</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"ntp\" maxlength=\"50\"></td> \
</tr> \
<tr> \
<td><b>Hostname</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"hostname\" maxlength=\"20\"></td> \
</tr> \
<tr> \
<td colspan=\"2\" class=\"alert-info\"><b>DDNS</b></td> \
</tr> \
<tr> \
<td style=\"padding:10px 0px 10px 0px;\"><b>Servi&ccedil;o</b></td> \
<td> \
<SELECT class=\"form-control\" name=\"ddns\" style=\"margin-top:5px;\"> \
<option value=\"0\">Desabilitado</option> \
<option value=\"1\">No-IP</option> \
<option value=\"2\">DNSdynamic</option> \
</SELECT> \
</td> \
</tr> \
<tr> \
<td><b>Dom&iacute;nio</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"ddns_domain\" maxlength=\"256\"></td> \
</tr> \
<tr> \
<td><b>Usu&aacute;rio</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"ddns_user\" maxlength=\"64\"></td> \
</tr> \
<tr> \
<td><b>Senha</b></td> \
<td><INPUT type=\"password\" class=\"form-control\" name=\"ddns_passwd\" maxlength=\"64\"></td> \
</tr> \
<tr> \
<td colspan=\"2\" align=\"center\" valign=\"middle\" ><BR> \
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Salvar\" onclick=\"alert('Reinicie o equipamento para validar as configura&ccedil;&otilde;es!');\"> \
<INPUT type=\"hidden\" name=\"menuopt\" value=\"1\"> \
</td> \
</tr> \
</table> \
</FORM> \
</div> \
</div> \
</td> \
</tr> \
</table>"
};

 
DCACHE_FLASH_ATTR static const char INDEXACIONAMENTO[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\">\
<div class=\"panel-heading\">\
<h3 class=\"panel-title\"><b>Configura&ccedil;&otilde;es Gerais</b></h3>\
</div>\
<div class=\"panel-body\">\
<FORM name=\"CONTROL\" action=\"save\" method=\"POST\">\
<table border=\"0\" align=\"left\" width=\"98%\" height=\"100%\">"
#if defined(CONFIG__MLI_1WRQ_TYPE__) || defined(CONFIG__MLI_1WR_TYPE__) || \
    defined(CONFIG__MLI_1WRF_TYPE__) || defined(CONFIG__MLI_1WRS_TYPE__) || \
    defined(CONFIG__MLI_1WLS_TYPE__) || defined(CONFIG__MLI_1WRP_TYPE__) || \
    defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
"<script type=\"text/javascript\">\
function select_address(ff)\
{\
for(var i=0;i<30;i++)\
ff.options[i]=new Option(i+1,i+1);\
}\n\
</script>"
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || \
    defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
"<script type=\"text/javascript\">\
function select_temperature(ff)\
{\
ff.options[0] = new Option('Desabilitado','-128');\
var temp = -40;\
var i = 0;\
while(temp<=80){\
ff.options[i+1]=new Option(temp+' \\272C',temp);\
temp++;\
i++;\
}\n\
}\n\
function select_humidity(ff)\
{\
ff.options[0] = new Option('Desabilitado','-128');\
for(var i=0;i<=100;i++)\
ff.options[i+1]=new Option(i+'%',i);\
}\n\
function select_channel(ff)\
{\
for(var i=0;i<=31;i++)\
ff.options[i]=new Option(i,i);\
}\n\
</script>"
#endif
#if defined(CONFIG__MLI_1WRP_TYPE__)
"<script type=\"text/javascript\">\
function select_day(ff)\
{\
for(var i=0;i<31;i++)\
ff.options[i]=new Option(i+1,i+1);\
}\n\
</script>"
#endif
"<tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Acionamento</b></td>\
</tr><tr>\
<td><b>Descri&ccedil;&atilde;o</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"desc\" maxlength=\"128\" style=\"margin-top:5px;\"></td>\
</tr>"
#if !defined(CONFIG__MLI_1WRP_TYPE__)
"<tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Modo Standalone</b></td>\
<td><INPUT type=\"checkbox\" name=\"standalone\"></td>\
</tr><tr>\
<td><b>Tipo de Acionamento</b></td>\
<td>\
<SELECT class=\"form-control\" name=\"mode\">\
<option value=\"0\">Cont&iacute;nuo</option>\
<option value=\"1\">Manual</option>\
</SELECT>\
</td>\
</tr><tr>\
<td><b>Tempo de Acionamento</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"timeout\"><b> ms</b></td>\
</tr>"
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRC_TYPE__) && !defined(CONFIG__MLI_1WLC_TYPE__)
"<tr>\
<td><b>Tempo de Acionamento de Acessibilidade</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"acc_timeout\"><b> ms</b></td>\
</tr>"
#endif
"<tr>\
<td style=\"padding-bottom:10px;\"><b>Acionamento Externo</b></td>\
<td><INPUT type=\"checkbox\" name=\"external\"></td>\
</tr><tr>\
<td style=\"padding-bottom:10px;\"><b>URL para Acionamento Externo</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"external_url\" maxlength=\"256\"></td>\
</tr><tr>\
<td style=\"padding-bottom:10px;\"><b>Habilitar Alarme de Arrombamento</b></td>\
<td><INPUT type=\"checkbox\" name=\"breakin\"></td>\
</tr><tr>\
<td><b>Habilitar Fun&ccedil;&atilde;o Botoeira no Sensor</b></td>\
<td><INPUT type=\"checkbox\" name=\"button_enable\"></td>\
</tr>"
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRC_TYPE__) && !defined(CONFIG__MLI_1WLC_TYPE__)
"<tr>\
<td><b>Tempo de Bloqueio de Dupla Passagem</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"doublepass_timeout\" min=\"0\"><b> s</b></td>\
</tr>"
#endif
#else
"<tr>\
<td><b>Estado Inicial de Rel&ecirc;</b></td>\
<td>\
<SELECT class=\"form-control\" name=\"relay_status\">\
<option value=\"0\">Aberto</option>\
<option value=\"1\">Fechado</option>\
</SELECT>\
</td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Bot&atilde;o de Acionamento de Rel&ecirc;</b></td>\
<td><INPUT type=\"checkbox\" name=\"button_enable\"></td>\
</tr><tr>\
<td><b>Tempo de Acionamento de Rel&ecirc; Auxiliar</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"timeout\"><b> ms</b></td>\
</tr><tr>"
#endif
#if !defined(CONFIG__MLI_1WF_TYPE__) && !defined(CONFIG__MLI_1WQF_TYPE__) && !defined(CONFIG__MLI_1WRF_TYPE__) && \
    !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && !defined(CONFIG__MLI_1WRG_TYPE__) && \
    !defined(CONFIG__MLI_1WLG_TYPE__) && !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>NFC</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Leitor NFC</b></td>\
<td><INPUT type=\"checkbox\" name=\"rfid\"></td>\
</tr><tr>\
<td><b>Timeout Leitor NFC</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"rfid_timo\"><b> ms</b></td>\
</tr><tr>\
<td><b>N&uacute;mero de Tentativas de Leitura</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"rfid_retries\" min=\"0\" max=\"255\"></td>\
</tr><tr>\
<td style=\"padding-bottom:10px;\"><b>Habilitar Acesso NFC M&oacute;vel</b></td>\
<td><INPUT type=\"checkbox\" name=\"rfid_nfc\"></td>\
</tr><tr>\
<td><strong>Timeout para P&acirc;nico no Leitor NFC</strong></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"rfid_panic_timo\"><b> ms</b></td>\
</tr><tr>\
<td><b>Formato da Tag MIFARE</b></td>\
<td>\
<SELECT class=\"form-control\" name=\"rfid_format\">\
<option value=\"0\">Big Endian</option>\
<option value=\"1\">Little Endian</option>\
</SELECT>\
</td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WQ_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRQ_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>QRCODE</b></td>\
</tr><tr>\
<tr><td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Leitor QRCODE</b></td>\
<td><INPUT type=\"checkbox\" name=\"qrcode\"></td>\
</tr><tr>\
<td style=\"padding-bottom:10px;\"><b>Habilitar Ilumina&ccedil;&atilde;o do Leitor QRCODE</b></td>\
<td><INPUT type=\"checkbox\" name=\"qrcode_led\"></td>\
</tr><tr>\
<td><b>Habilitar Configura&ccedil;&atilde;o pelo Leitor QRCODE</b></td>\
<td><INPUT type=\"checkbox\" name=\"qrcode_config\"></td>\
</tr><tr>\
<td><b>Timeout Leitor QRCODE</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"qrcode_timo\"><b> ms</b></td>\
</tr><tr>\
<td><strong>Timeout para P&acirc;nico no Leitor QRCODE</strong></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"qrcode_panic_timo\"><b> ms</b></td>\
</tr><tr>\
<td><strong>Habilitar QRCODE Din&acirc;mico</strong></td>\
<td><INPUT type=\"checkbox\" name=\"qrcode_dynamic\"></td>\
</tr><tr>\
<td><strong>Validade de QRCODE Din&acirc;mico</strong></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"qrcode_validity\"><b> s</b></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WF_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRF_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>RF433</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Leitor RF433</b></td>\
<td><INPUT type=\"checkbox\" name=\"rf433\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Rolling Code</b></td>\
<td><INPUT type=\"checkbox\" name=\"rf433_rc\"></td>\
</tr><tr>\
<td><b>Toler&acirc;ncia do Hopping Code</b></td>\
<td><SELECT class=\"form-control\" name=\"rf433_hc\">\
<option value=\"8\">8</option>\
<option value=\"16\">16</option>\
<option value=\"64\">64</option>\
<option value=\"128\">128</option>\
<option value=\"256\">256</option>\
<option value=\"512\">512</option>\
<option value=\"1024\">1024</option>\
<option value=\"2048\">2048</option>\
</SELECT>\
</td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Alarme Sonoro</b></td>\
<td><INPUT type=\"checkbox\" name=\"rf433_alarm\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Inserir C&oacute;digo do Bot&atilde;o</b></td>\
<td><INPUT type=\"checkbox\" name=\"rf433_bc\"></td>\
</tr><tr>\
<td><b>Bot&atilde;o de Acionamento</b></td>\
<td><SELECT class=\"form-control\" name=\"rf433_ba\">\
<option value=\"0\">Nenhum</option>\
<option value=\"1\">Bot&atilde;o 1</option>\
<option value=\"2\">Bot&atilde;o 2</option>\
<option value=\"3\">Bot&atilde;o 3</option>\
<option value=\"4\">Bot&atilde;o 4</option>\
</SELECT>\
</td>\
</tr><tr>\
<td><b>Bot&atilde;o de P&acirc;nico</b></td>\
<td><SELECT class=\"form-control\" name=\"rf433_bp\">\
<option value=\"0\">Nenhum</option>\
<option value=\"1\">Bot&atilde;o 1</option>\
<option value=\"2\">Bot&atilde;o 2</option>\
<option value=\"3\">Bot&atilde;o 3</option>\
<option value=\"4\">Bot&atilde;o 4</option>\
</SELECT>\
</td>\
</tr><tr>\
<td><strong>Timeout para P&acirc;nico</strong></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"rf433_panic_timo\"><b> ms</b></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Biometria</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Leitor Biom&eacute;trico</b></td>\
<td><INPUT type=\"checkbox\" name=\"fpm\"></td>\
</tr><tr>\
<td><b>Timeout Leitor Biom&eacute;trico</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"fpm_timo\"><b> ms</b></td>\
</tr><tr>\
<td><b>N&iacute;vel de Seguran&ccedil;a de Biometria</b></td>\
<td><select class=\"form-control\" name=\"fpm_sec\">\
<option value=\"1\">1</option>\
<option value=\"2\">2</option>\
<option value=\"3\">3</option>\
<option value=\"4\">4</option>\
<option value=\"5\">5</option>\
</select></td>\
</tr><tr>\
<td><b>N&uacute;mero de Identifica&ccedil;&otilde;es de Biometria</b></td>\
<td><select class=\"form-control\" name=\"fpm_id\">\
<option value=\"1\">1</option>\
<option value=\"2\">2</option>\
<option value=\"3\">3</option>\
<option value=\"4\">4</option>\
<option value=\"5\">5</option>\
</select></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WRQ_TYPE__) || defined(CONFIG__MLI_1WR_TYPE__) || \
    defined(CONFIG__MLI_1WRF_TYPE__) || defined(CONFIG__MLI_1WRS_TYPE__) || \
    defined(CONFIG__MLI_1WRP_TYPE__) || defined(CONFIG__MLI_1WRC_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>RS485</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Interface RS485</b></td>\
<td><INPUT type=\"checkbox\" name=\"rs485\"></td>\
</tr><tr>\
<td><b>Endere&ccedil;o F&iacute;sico Local RS485</b></td>\
<td><select class=\"form-control\" name=\"rs485_addr\">\
<script>\
select_address(document.CONTROL.rs485_addr);\
</script>\
</select></td>\
</tr><tr>\
<td><b>Endere&ccedil;o F&iacute;sico Remoto RS485</b></td>\
<td><select class=\"form-control\" name=\"rs485_server_addr\">\
<script>\
select_address(document.CONTROL.rs485_server_addr);\
</script>\
</select></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WLS_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>LoRA</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar LoRA</b></td>\
<td><INPUT type=\"checkbox\" name=\"lora\"></td>\
</tr><tr>\
<td><b>Canal</b></td>\
<td><select class=\"form-control\" name=\"lora_channel\">\
<script>\
select_channel(document.CONTROL.lora_channel);\
</script>\
</select></td> \
</tr><tr> \
<td><b>Taxa de Transmiss&atilde;o</b></td>\
<td><select class=\"form-control\" name=\"lora_baudrate\">\
<option value=\"300\">300 bps</option>\
<option value=\"1200\">1200 bps</option>\
<option value=\"2400\">2400 bps</option>\
<option value=\"4800\">4800 bps</option>\
<option value=\"9600\">9600 bps</option>\
<option value=\"19200\">19200 bps</option>\
</select></td>\
</tr><tr>\
<td><b>Endere&ccedil;o F&iacute;sico Local</b></td>\
<td><select class=\"form-control\" name=\"lora_addr\">\
<script>\
select_address(document.CONTROL.lora_addr);\
</script>\
</select></td>\
</tr><tr>\
<td><b>Endere&ccedil;o F&iacute;sico Remoto</b></td>\
<td><select class=\"form-control\" name=\"lora_server_addr\">\
<script>\
select_address(document.CONTROL.lora_server_addr);\
</script>\
</select></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || \
    defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Temperatura e Umidade</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Sensor</b></td>\
<td><INPUT type=\"checkbox\" name=\"dht_enable\"></td>\
</tr><tr>\
<td><b>Tempo de Leitura</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"dht_timo\"><b> s</b></td>\
</tr><tr> \
<td><b>Limite Superior de Temperatura</b></td>\
<td><select class=\"form-control\" name=\"dht_temp_upper\">\
<script>\
select_temperature(document.CONTROL.dht_temp_upper);\
</script>\
</select></td> \
</tr><tr> \
<td><b>Limite Inferior de Temperatura</b></td>\
<td><select class=\"form-control\" name=\"dht_temp_lower\">\
<script>\
select_temperature(document.CONTROL.dht_temp_lower);\
</script>\
</select></td>\
</tr><tr>\
<td><b>Limite Superior de Umidade</b></td>\
<td><select class=\"form-control\" name=\"dht_rh_upper\">\
<script>\
select_humidity(document.CONTROL.dht_rh_upper);\
</script>\
</select></td>\
</tr><tr>\
<td><b>Limite Inferior de Umidade</b></td>\
<td><select class=\"form-control\" name=\"dht_rh_lower\">\
<script>\
select_humidity(document.CONTROL.dht_rh_lower);\
</script>\
</select></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Acionamento de Rel&ecirc;</b></td>\
<td><INPUT type=\"checkbox\" name=\"dht_relay\"></td>\
</tr><tr> \
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Alarme Sonoro</b></td>\
<td><INPUT type=\"checkbox\" name=\"dht_alarm\"></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Luminosidade</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Sensor</b></td>\
<td><INPUT type=\"checkbox\" name=\"temt_enable\"></td>\
</tr><tr>\
<td><b>Tempo de Leitura</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"temt_timo\"><b> s</b></td>\
</tr><tr>\
<td><b>Limite Superior de Luminosidade</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"temt_upper\"><b> lx</b></td>\
</tr><tr>\
<td><b>Limite Inferior de Luminosidade</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"temt_lower\"><b> lx</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Acionamento de Rel&ecirc;</b></td>\
<td><INPUT type=\"checkbox\" name=\"temt_relay\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Alarme Sonoro</b></td>\
<td><INPUT type=\"checkbox\" name=\"temt_alarm\"></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WLG_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Fuma&ccedil;a e G&aacute;s</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Sensor</b></td>\
<td><INPUT type=\"checkbox\" name=\"mq2_enable\"></td>\
</tr><tr>\
<td><b>Tempo de Leitura</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"mq2_timo\"><b> s</b></td>\
</tr><tr>\
<td><b>Limite de Detec&ccedil;&atilde;o</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"mq2_limit\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Acionamento de Rel&ecirc;</b></td>\
<td><INPUT type=\"checkbox\" name=\"mq2_relay\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Alarme Sonoro</b></td>\
<td><INPUT type=\"checkbox\" name=\"mq2_alarm\"></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Loop de Corrente</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Sensor</b></td>\
<td><INPUT type=\"checkbox\" name=\"cli_enable\"></td>\
</tr><tr>\
<td><b>Tempo de Leitura</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"cli_timo\"><b> s</b></td>\
</tr><tr>\
<td><b>Fundo de Escala</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"cli_range\" min=\"1\" max=\"600\"></td>\
</tr><tr>\
<td><b>Limite Superior</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"cli_upper\"><b> x100</b></td>\
</tr><tr>\
<td><b>Limite Inferior</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"cli_lower\"><b> x100</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Acionamento de Rel&ecirc;</b></td>\
<td><INPUT type=\"checkbox\" name=\"cli_relay\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Alarme Sonoro</b></td>\
<td><INPUT type=\"checkbox\" name=\"cli_alarm\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Calibrar Sensor</b></td>\
<td><INPUT type=\"submit\" class=\"btn btn-primary\" name=\"cli_cal\" value=\"Calibrar\">&nbsp;\
<INPUT type=\"submit\" class=\"btn btn-primary\" name=\"cli_reset\" value=\"Resetar\"></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || \
    defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WLG_TYPE__) || \
    defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Sensor de Movimento</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Sensor</b></td>\
<td><INPUT type=\"checkbox\" name=\"pir_enable\"></td>\
</tr><tr>\
<td><b>Tempo de Leitura</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"pir_timo\"><b> s</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Fun&ccedil;&atilde;o Chime</b></td>\
<td><INPUT type=\"checkbox\" name=\"pir_chime\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Acionamento de Rel&ecirc;</b></td>\
<td><INPUT type=\"checkbox\" name=\"pir_relay\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Alarme Sonoro</b></td>\
<td><INPUT type=\"checkbox\" name=\"pir_alarm\"></td>\
</tr><tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Sensor de N&iacute;vel e Volume</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Fun&ccedil;&atilde;o do Sensor</b></td>\
<td><SELECT class=\"form-control\" name=\"sensor_type\">\
<option value=\"0\">Desabilitado</option>\
<option value=\"1\">N&iacute;vel</option>\
<option value=\"2\">Volume</option>\
</SELECT></td>\
</tr><tr>\
<td><b>Fluxo do Sensor de Volume</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"sensor_flow\"><b> P/L</b></td>\
</tr><tr>\
<td><b>Limite de Volume</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"sensor_limit\"><b> L</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Acionamento de Rel&ecirc;</b></td>\
<td><INPUT type=\"checkbox\" name=\"sensor_relay\"></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Habilitar Alarme Sonoro</b></td>\
<td><INPUT type=\"checkbox\" name=\"sensor_alarm\"></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WRP_TYPE__)
"<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Pot&ecirc;ncia e Energia</b></td>\
</tr><tr>\
<td><b>Fator de Corre&ccedil;&atilde;o de Tens&atilde;o</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"pow_vol_cal\" style=\"margin-top:5px;\"><b> x1000</b></td>\
</tr><tr>\
<td><b>Limite Superior de Tens&atilde;o</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"pow_vol_upper\"><b> V</b></td>\
</tr><tr>\
<td><b>Limite Inferior de Tens&atilde;o</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"pow_vol_lower\"><b> V</b></td>\
</tr><tr>\
</tr><tr>\
<td><b>Fator de Corre&ccedil;&atilde;o de Corrente</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"pow_cur_cal\"><b> x1000</b></td>\
</tr><tr>\
<td><b>Limite Superior de Corrente</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"pow_cur_upper\"><b> mA</b></td>\
</tr><tr>\
<td><b>Limite Inferior de Corrente</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"pow_cur_lower\"><b> mA</b></td>\
</tr><tr>\
<td><b>Limite Superior de Pot&ecirc;ncia</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"pow_pwr_upper\" style=\"margin-top:5px;\"><b> W</b></td>\
</tr><tr>\
<td><b>Limite Inferior de Pot&ecirc;ncia</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"pow_pwr_lower\"><b> W</b></td>\
</tr><tr>\
<td><b>Fator de Corre&ccedil;&atilde;o de Consumo</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"pow_nrg_cal\" style=\"margin-top:5px;\"><b> x1000</b></td>\
</tr><tr>\
<td><b>Limite de Consumo Di&aacute;rio</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"daily_limit\"><b> kWh</b></td>\
</tr><tr>\
<td><b>Limite de Consumo Mensal</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"monthly_limit\"><b> kWh</b></td>\
</tr><tr>\
<td><b>Limite de Consumo Total</b></td>\
<td><input type=\"number\" class=\"form-control\" name=\"total_limit\"><b> kWh</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Desabilitar Rel&ecirc; em Alarme</b></td>\
<td><INPUT type=\"checkbox\" name=\"pow_relay\"></td>\
</tr><tr>\
<td><b>Tempo para Desarme de Rel&ecirc; em Alarme</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"pow_alarm_time\"><b> s</b></td>\
</tr><tr>\
<td><b>Tempo para Rearme de Rel&ecirc; em Alarme</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"pow_relay_timo\"><b> s</b></td>\
</tr><tr>\
<td style=\"padding:10px 0px 10px 0px;\"><b>Acionamento de Rel&ecirc; Auxiliar em Alarme</b></td>\
<td><INPUT type=\"checkbox\" name=\"pow_relay_ext\"></td>\
</tr><tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Telemetria</b></td>\
</tr><tr>\
<td><b>Intervalo de Telemetria</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"pow_interval\" style=\"margin-top:5px;\"><b> s</b></td>\
</tr><tr>\
<td><b>Dia do M&ecirc;s para Zerar Estat&iacute;sticas</b></td>\
<td><select class=\"form-control\" name=\"pow_day\">\
<script>\
select_day(document.CONTROL.pow_day);\
</script>\
</select></td>\
</tr>"
#endif
"<tr>\
<td colspan=\"2\" align=\"center\" valign=\"middle\" ><BR>\
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Salvar\" onclick=\"alert('Reinicie o equipamento para validar as configura&ccedil;&otilde;es!');\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"2\">\
</td>\
</tr>\
</table>\
</FORM>\
</div>\
</div>\
</td>\
</tr>\
</table>"
};

 
DCACHE_FLASH_ATTR static const char INDEXHTTP[] = {
"<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\"> \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Integra&ccedil;&atilde;o</b></h3> \
</div> \
<div class=\"panel-body\"> \
<FORM name=\"HTTP\" action=\"save\" method=\"POST\">   \
<table border=\"0\" align=\"left\" width=\"100%\" height=\"100%\"> \
<tr> \
<td><b>Servidor</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"server\" size=\"20\" maxlength=\"200\"></td> \
</tr> \
<tr> \
<td><b>Porta</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"port\" size=\"20\" maxlength=\"5\"></td> \
</tr> \
<tr> \
<td><b>Usu&aacute;rio</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"user\" size=\"20\" maxlength=\"20\"></td> \
</tr> \
<tr> \
<td><b>Senha</b></td> \
<td><INPUT type=\"password\" class=\"form-control\" name=\"pass\" size=\"20\" maxlength=\"20\"></td> \
</tr> \
<tr> \
<td><b>URL</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"url\" size=\"20\" maxlength=\"200\"></td> \
</tr> \
<tr> \
<td><b>N&uacute;mero de Tentativas</b></td> \
<td><INPUT type=\"text\" class=\"form-control\" name=\"retries\" size=\"20\" maxlength=\"2\"></td> \
</tr> \
<tr> \
<td><b>Autentica&ccedil;&atilde;o de Usu&aacute;rios</b></td> \
<td><INPUT type=\"checkbox\" name=\"auth\"></td> \
</tr> \
<tr> \
<td colspan=\"2\" align=\"center\" valign=\"middle\"><BR> \
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Salvar\" onclick=\"alert('Reinicie o equipamento para validar as configura&ccedil;&otilde;es!');\"> \
<INPUT type=\"hidden\" name=\"menuopt\" value=\"3\"> \
</td> \
</tr> \
</table> \
</FORM> \
</div> \
</div> \
</td> \
</tr> \
</table>"
};

 
DCACHE_FLASH_ATTR static const char INDEXUSER[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div id=\"load\" class=\"panel panel-primary\" style=\"display:none;\">\
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Aguarde...</b></h3>\
</div> \
<div id=\"load-body\" class=\"panel-body\">\
</div>\
</div>\
<div id=\"page\" class=\"panel panel-primary\">\
<div class=\"panel-heading\">\
<h3 class=\"panel-title\"><b>Usu&aacute;rios</b></h3>\
</div>\
<div class=\"panel-body\">\
<FORM name=\"USER\" method=\"POST\">\
<table width=\"98%\" height=\"100%\">\
<tr> \
<td> \
<FORM name=\"USRADD\" method=\"POST\">\
<INPUT type=\"submit\" name=\"user_Add\" value=\"Adicionar\" class=\"btn btn-primary\">&nbsp;"
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRC_TYPE__) && !defined(CONFIG__MLI_1WLC_TYPE__)
"<INPUT type=\"submit\" name=\"user_Det\" value=\"Detectar\" class=\"btn btn-primary\" onclick=\"return confirm('Deseja detectar usuario?');\">&nbsp;\
<INPUT type=\"submit\" name=\"user_Erase\" value=\"Apagar\" class=\"btn btn-primary\" onclick=\"return confirm('Deseja apagar usuario?');\">&nbsp;"
#endif
"<INPUT type=\"submit\" name=\"user_Clean\" value=\"Remover Todos\" class=\"btn btn-primary\" onclick=\"return confirm('Deseja remover usuarios?');\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"4\">\
</FORM>\
</td>\
<td>\
<FORM name=\"USRSRC\" method=\"POST\">\
<a href=\"/?request=users&file=csv\" class=\"btn btn-primary\" style=\"text-decoration:none\">Exportar</a>&nbsp;\
<INPUT type=\"button\" class=\"btn btn-primary\" name=\"user_Imp\" value=\"Importar\" onclick=\"load();\">&nbsp;\
<INPUT type=\"file\" id=\"upfile\" name=\"upfile\">\
<INPUT type=\"submit\" name=\"user_Search\" value=\"Procurar\" class=\"btn btn-primary pull-right\" style=\"margin-left:2px;\">\
<INPUT type=\"text\" name=\"filter\" style=\"width:200px;height:28px;\" class=\"form-control pull-right\" value=\"\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"4\">\
</FORM>\
</td>\
</tr><tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td><b>N&iacute;vel de Acesso</b></td>\
<td><SELECT class=\"form-control\" name=\"level\">\
<option value=\"0\">Usu&aacute;rio</option>\
<option value=\"1\">Administrador</option>\
</SELECT></td>\
</tr><tr>\
<td><b>Nome</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"name\" maxlength=\"64\"></td>\
</tr><tr>\
<td><b>Usu&aacute;rio</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"user\" maxlength=\"20\"></td>\
</tr><tr>\
<td><b>Senha</b></td>\
<td><INPUT type=\"number\" class=\"form-control\" name=\"pass\" maxlength=\"20\"></td>\
</tr>"
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
"<tr>\
<td><b>Cart&atilde;o</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"card\" maxlength=\"32\"></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WQ_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRQ_TYPE__)
"<tr>\
<td><b>C&oacute;digo QR</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"code\" maxlength=\"128\"></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WF_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRF_TYPE__)
"<tr>\
<td><b>C&oacute;digo RF</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"rfcode\" maxlength=\"16\"></td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
"<tr>\
<td><b>Biometria</b></td>\
<td>\
<textarea class=\"form-control\" style=\"height:200px;\" name=\"fingerprint\" maxlength=\"664\"></textarea>&nbsp;\
<INPUT type=\"submit\" style=\"display:none;\" name=\"user_Finger\" value=\"Capturar\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" style=\"display:none;\" name=\"user_Update\" value=\"Atualizar\" class=\"btn btn-primary\">\
</td>\
</tr>\
<tr>\
<td><b>Identifica&ccedil;&atilde;o de Biometria</b></td>\
<td>\
<SELECT class=\"form-control\" name=\"finger\">\
<option value=\"0\">Nenhum</option>\
<option value=\"A\">Polegar Direito</option>\
<option value=\"B\">Indicador Direito</option>\
<option value=\"C\">M&eacute;dio Direito</option>\
<option value=\"D\">Anelar Direito</option>\
<option value=\"E\">M&iacute;nimo Direito</option>\
<option value=\"F\">Polegar Esquerdo</option>\
<option value=\"G\">Indicador Esquerdo</option>\
<option value=\"H\">M&eacute;dio Esquerdo</option>\
<option value=\"I\">Anelar Esquerdo</option>\
<option value=\"J\">M&iacute;nimo Esquerdo</option>\
</SELECT>\
</td>\
</tr>"
#endif
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
"<tr>\
<td><b>N&uacute;mero de Acessos Permitidos</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"lifecount\" maxlength=\"5\"></td>\
</tr>\
<tr>\
<td><b>Habilitar Acessibilidade</b></td>\
<td><INPUT type=\"checkbox\" name=\"accessibility\"></td>\
</tr>\
<tr>\
<td><b>Habilitar P&acirc;nico</b></td>\
<td><INPUT type=\"checkbox\" name=\"panic\"></td>\
</tr>\
<tr>\
<td><b>Visitante</b></td>\
<td><INPUT type=\"checkbox\" name=\"visitor\"></td>\
</tr>\
<tr>\
<td><b>Chave Privada</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"key\" maxlength=\"16\"></td>\
<tr>\
<td colspan=\"2\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" align=\"center\" class=\"alert-info\"><b>Permiss&otilde;es de Acesso</b></td>\
</tr><tr>\
<td colspan=\"2\">\
<table align=\"center\" width=\"100%\">\
<script type=\"text/javascript\">\
function select_hour(ff)\
{\
ff.options[0]=new Option('-','');\
for(var i=0;i<24;i++)\
ff.options[i+1]=new Option(i+'h',i);\
}\n\
function select_minute(ff)\
{\
ff.options[0]=new Option('-','');\
for(var i=0;i<60;i++)\
ff.options[i+1]=new Option(i,i);\
}\n\
function select_week(ff)\
{\
var weekarr = ['Dom','Seg','Ter','Qua',\
'Qui','Sex','Sab'];\
var weeklen = weekarr.length;\
ff.options[0]=new Option('-','');\
for(var i=0;i<weeklen;i++)\
ff.options[i+1]=new Option(weekarr[i],i);\
}\n\
function select_day(ff)\
{\
ff.options[0]=new Option('-','');\
for(var i=1;i<=31;i++)\
ff.options[i]=new Option(i,i);\
}\n\
function select_month(ff)\
{\
var month = ['Jan','Fev','Mar','Abr',\
'Mai','Jun','Jul','Ago','Set','Out','Nov','Dez'];\
ff.options[0] = new Option('-','');\
for (var i=0;i<month.length;i++)\
ff.options[i+1]=new Option(month[i],i+1);\
}\n\
function select_year(ff)\
{\
ff.options[0] = new Option('-','');\
for (var i=2017;i<=2050;i++)\
ff.options[i-2016]=new Option(i,i);\
}\n\
</script>\
<tr>\
<td align=\"center\"><b>Semana</b></td>\
<td align=\"center\"><b>Data</b></td>\
<td align=\"center\"><b>Hor&aacute;rio</b></td>\
</tr>\
<tr>\
<td align=\"center\">\
<select class=\"form-control week\" name=\"w1i\">\
<script>\
select_week(document.USER.w1i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control week\" name=\"w1e\">\
<script>\
select_week(document.USER.w1e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control date\" name=\"d1i\">\
<script>\
select_day(document.USER.d1i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t1i\">\
<script>\
select_month(document.USER.t1i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y1i\">\
<script>\
select_year(document.USER.y1i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control date\" name=\"d1e\">\
<script>\
select_day(document.USER.d1e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t1e\">\
<script>\
select_month(document.USER.t1e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y1e\">\
<script>\
select_year(document.USER.y1e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control hour\" name=\"h1i\">\
<script>\
select_hour(document.USER.h1i);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m1i\">\
<script>\
select_minute(document.USER.m1i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control hour\" name=\"h1e\">\
<script>\
select_hour(document.USER.h1e);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m1e\">\
<script>\
select_minute(document.USER.m1e);\
</script>\
</select>\
</td></tr>\
<tr>\
<td align=\"center\">\
<select class=\"form-control week\" name=\"w2i\">\
<script>\
select_week(document.USER.w2i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control week\" name=\"w2e\">\
<script>\
select_week(document.USER.w2e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control date\" name=\"d2i\">\
<script>\
select_day(document.USER.d2i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t2i\">\
<script>\
select_month(document.USER.t2i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y2i\">\
<script>\
select_year(document.USER.y2i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control date\" name=\"d2e\">\
<script>\
select_day(document.USER.d2e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t2e\">\
<script>\
select_month(document.USER.t2e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y2e\">\
<script>\
select_year(document.USER.y2e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control hour\" name=\"h2i\">\
<script>\
select_hour(document.USER.h2i);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m2i\">\
<script>\
select_minute(document.USER.m2i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control hour\" name=\"h2e\">\
<script>\
select_hour(document.USER.h2e);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m2e\">\
<script>\
select_minute(document.USER.m2e);\
</script>\
</select>\
</td></tr>\
</table>\
</td>\
</tr>"
#endif
"<tr>\
<td id=\"total\" colspan=\"2\" align=\"right\"></td>\
</tr><tr>\
<td align=\"center\" colspan=\"2\"><BR>\
<INPUT type=\"submit\" name=\"user_First\" value=\"Primeiro\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" name=\"user_Prev\" value=\"Anterior\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" name=\"user_Next\" value=\"Pr&oacute;ximo\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" name=\"user_Last\" value=\"&Uacute;ltimo\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" name=\"save\" value=\"Salvar\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" name=\"user_Del\" value=\"Remover\" class=\"btn btn-primary\" onclick=\"return confirm('Deseja remover usuario?');\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"0\">\
</td>\
</tr>\
</table>\
</FORM>\
</div>\
</div>\
</td>\
</tr>\
</table>\
<script>\
function load(){\
var file=document.getElementById('upfile').files[0];\
if(!file){\
alert('Selecione o arquivo de usuarios!');\
return;\
}\
if(!confirm('Deseja importar usuarios?'))\
return;\
var reader=new FileReader();\
reader.onloadend=function(){\
var acc=reader.result.split(\"\\r\\n\");\
if(!acc.length)\
return;\
if (acc[acc.length-1]==''){\
acc.splice(acc.length-1,1);\
}\
document.getElementById('page').style.display='none';\
document.getElementById('load').style.display='block';\
var i=0;\
var timer=setInterval(function(){\
if(i==acc.length-1){\
clearInterval(timer);\
document.getElementById('load-body').innerHTML='<b>Carregando...</b>';\
setTimeout(function(){window.location.reload();},3000);\
}\
var p=acc[i++].split(',');\
if(p.length<11)\
return;\
document.getElementById('load-body').innerHTML='<b>Transferindo '+i+' de '+acc.length+'...</b>';\
for(k=p.length;k<19;k++)\
p[k]='';\
if (p[8] != 0) {\
p[8]='true';\
} else {\
p[8]='false';\
}\
if (p[9] != 0) {\
p[9]='true';\
} else {\
p[9]='false';\
}\
if (p[11] != 0) {\
p[11]='true';\
} else {\
p[11]='false';\
}\
if (p[12] != 0) {\
p[12]='true';\
} else {\
p[12]='false';\
}\
var json={\"name\":p[0],\"user\":p[1],\"password\":p[2],\
\"card\":p[3],\"qrcode\":p[4],\"rfcode\":p[5],\"fingerprint\":p[6],\
\"lifecount\":p[7],\"accessibility\":p[8],\"panic\":p[9],\"key\":p[10],\
\"administrator\":p[11],\"visitor\":p[12],\"finger\":p[13],\
\"perm1\":p[14],\"perm2\":p[15],\"perm3\":p[16],\"perm4\":p[17],\"perm5\":p[18]};\
var xhr=new XMLHttpRequest();\
xhr.open('POST','/?request=adduser',true);\
xhr.setRequestHeader('Content-type','application/json');\
xhr.send(JSON.stringify(json));\
},1000);\
}\n\
reader.readAsText(file);\
}\
</script>"
};

#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
 
DCACHE_FLASH_ATTR static const char INDEXLOG[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div class=\"panel panel-primary\" style=\"marign: 0 auto;\">\
<div class=\"panel-heading\">\
<h3 class=\"panel-title\"><b>Acessos</b></h3>\
</div>\
<div class=\"panel-body\">\
<FORM name=\"LOG\" action=\"status\" method=\"POST\">\
<div style=\"margin-bottom:10px;\">\
<a href=\"/?request=accesslog&file=csv\" class=\"btn btn-primary\" style=\"text-decoration:none\">Exportar</a>&nbsp;\
<INPUT type=\"submit\" name=\"log_Remove\" class=\"btn btn-primary\" value=\"Remover\" onclick=\"return confirm('Deseja remover os registros?');\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"5\">\
</div>\
<table border=\"0\" align=\"left\" width=\"98%\" height=\"100%\">\
<tr>\
<td class=\"alert-info\" align=\"center\"><b>Hor&aacute;rio</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Nome</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Identifica&ccedil;&atilde;o</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Estado</b></td>\
</tr>"
};
#else
 
DCACHE_FLASH_ATTR static const char INDEXLOG[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div class=\"panel panel-primary\" style=\"marign: 0 auto;\">\
<div class=\"panel-heading\">\
<h3 class=\"panel-title\"><b>Telemetria</b></h3>\
</div>\
<div class=\"panel-body\">\
<FORM name=\"LOG\" action=\"status\" method=\"POST\">\
<div style=\"margin-bottom:10px;\">\
<a href=\"/?request=telemetrylog&file=csv\" class=\"btn btn-primary\" style=\"text-decoration:none\">Exportar</a>&nbsp;\
<INPUT type=\"submit\" name=\"log_Remove\" class=\"btn btn-primary\" value=\"Remover\" onclick=\"return confirm('Deseja remover os registros?');\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"5\">\
</div>\
<table border=\"0\" align=\"left\" width=\"98%\" height=\"100%\">\
<tr>"
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__)
"<td class=\"alert-info\" align=\"center\"><b>Hor&aacute;rio</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Temperatura</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Humidade</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Luminosidade</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Movimento</b></td>\
<td class=\"alert-info\" align=\"center\"><b>N&iacute;vel</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Volume</b></td>\
</tr>"
#elif defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WLG_TYPE__)
"<td class=\"alert-info\" align=\"center\"><b>Hor&aacute;rio</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Fuma&ccedil;a e G&aacute;s</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Movimento</b></td>\
<td class=\"alert-info\" align=\"center\"><b>N&iacute;vel</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Volume</b></td>\
</tr>"
#elif defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
"<td class=\"alert-info\" align=\"center\"><b>Hor&aacute;rio</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Temperatura</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Humidade</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Loop Corrente</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Movimento</b></td>\
<td class=\"alert-info\" align=\"center\"><b>N&iacute;vel</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Volume</b></td>\
</tr>"
#else
"<td class=\"alert-info\" align=\"center\"><b>Hor&aacute;rio</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Tens&atilde;o</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Corrente</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Pot&ecirc;ncia</b></td>\
</tr>"
#endif
};
#endif

 
DCACHE_FLASH_ATTR static const char INDEXSTATUS[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div class=\"panel panel-primary\" style=\"marign: 0 auto;\">\
<div class=\"panel-heading\">\
<h3 class=\"panel-title\"><b>Status</b></h3>\
</div>\
<div class=\"panel-body\">\
<FORM name=\"STATUS\" action=\"status\" method=\"POST\">\
<table border=\"0\" align=\"left\" width=\"98%\" height=\"100%\">"
#if !defined(CONFIG__MLI_1WRP_TYPE__)
"<tr>\
<td class=\"alert-info\" align=\"center\"><b>Alarme</b></td>\
<td class=\"alert-info\" align=\"center\"><b>P&acirc;nico</b></td>\
<td class=\"alert-info\" align=\"center\" colspan=\"2\"><b>A&ccedil;&otilde;es</b></td>\
</tr>\
<tr>\
<td colspan=\"3\">&nbsp;</td>\
</tr>\
<tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" id=\"alarm\" name=\"alarm\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" id=\"panic\" name=\"panic\"></td>\
<td align=\"center\">\
<INPUT type=\"submit\" name=\"stat_Alarm\" value=\"Alarme\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" name=\"stat_Panic\" value=\"P&acirc;nico\" class=\"btn btn-primary\">\
</td>\
</tr>\
<tr>\
<td colspan=\"3\">&nbsp;</td>\
</tr>"
#endif
#if !defined(CONFIG__MLI_1WRP_TYPE__)
"<tr>\
<td class=\"alert-info\" align=\"center\"><b>Rel&ecirc;</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Sensor</b></td>\
<td class=\"alert-info\" align=\"center\"><b>A&ccedil;&otilde;es</b></td>\
</tr><tr>\
<td colspan=\"3\">&nbsp;</td>\
</tr><tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"relay\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"sensor\"></td>\
<td align=\"center\">\
<INPUT type=\"submit\" name=\"stat_Open\" value=\"Abrir\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" name=\"stat_Close\" value=\"Fechar\" class=\"btn btn-primary\">\
</td>\
</tr>"
#else
"<tr>\
<td colspan=\"2\" class=\"alert-info\" align=\"center\"><b>Rel&ecirc;</b></td>\
<td class=\"alert-info\" align=\"center\"><b>A&ccedil;&otilde;es</b></td>\
</tr><tr>\
<td colspan=\"3\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" style=\"width:25%;\" readonly=\"true\" name=\"relay\"></td>\
<td align=\"center\">\
<INPUT type=\"submit\" name=\"stat_Open\" value=\"Ligar\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" name=\"stat_Close\" value=\"Desligar\" class=\"btn btn-primary\">\
</td>\
</tr><tr>\
<td colspan=\"3\">&nbsp;</td>\
</tr><tr>\
<td class=\"alert-info\" align=\"center\"><b>Rel&ecirc; Auxiliar</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Sensor</b></td>\
<td class=\"alert-info\" align=\"center\"><b>A&ccedil;&otilde;es</b></td>\
</tr><tr>\
<td colspan=\"3\">&nbsp;</td>\
</tr><tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"relayaux\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"sensor\"></td>\
<td align=\"center\">\
<INPUT type=\"submit\" name=\"stat_AuxOpen\" value=\"Abrir\" class=\"btn btn-primary\">&nbsp;\
<INPUT type=\"submit\" name=\"stat_AuxClose\" value=\"Fechar\" class=\"btn btn-primary\">\
</td>\
</tr>"
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || \
    defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WRG_TYPE__) || \
    defined(CONFIG__MLI_1WRP_TYPE__) || defined(CONFIG__MLI_1WRC_TYPE__) || \
    defined(CONFIG__MLI_1WLC_TYPE__)
"<tr> \
<td colspan=\"3\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"3\">\
<table width=\"100%\">"
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__)
"<tr>\
<td class=\"alert-info\" align=\"center\"><b>Temperatura</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Umidade</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Luminosidade</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Movimento</b></td>\
<td class=\"alert-info\" align=\"center\"><b>N&iacute;vel</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Volume</b></td>\
</tr>\
<tr>\
<td colspan=\"6\">&nbsp;</td>\
</tr>\
<tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"temp\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"rh\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"lux\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"pir\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"level\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"volume\"></td>\
</tr>"
#elif defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WLG_TYPE__)
"<tr>\
<td class=\"alert-info\" align=\"center\"><b>Temperatura</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Umidade</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Fuma&ccedil;a e G&aacute;s</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Movimento</b></td>\
<td class=\"alert-info\" align=\"center\"><b>N&iacute;vel</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Volume</b></td>\
</tr>\
<tr>\
<td colspan=\"6\">&nbsp;</td>\
</tr>\
<tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"temp\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"rh\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"gas\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"pir\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"level\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"volume\"></td>\
</tr>"
#elif defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
"<tr>\
<td class=\"alert-info\" align=\"center\"><b>Temperatura</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Umidade</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Loop Corrente</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Movimento</b></td>\
<td class=\"alert-info\" align=\"center\"><b>N&iacute;vel</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Volume</b></td>\
</tr>\
<tr>\
<td colspan=\"6\">&nbsp;</td>\
</tr>\
<tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"temp\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"rh\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"loop\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"pir\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"level\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"volume\"></td>\
</tr>"
#elif defined(CONFIG__MLI_1WRP_TYPE__)
"<tr>\
<td class=\"alert-info\" align=\"center\"><b>Tens&atilde;o</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Corrente</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Pot&ecirc;ncia Ativa</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Pot&ecirc;ncia Aparente</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Fator de Pot&ecirc;ncia</b></td>\
</tr>\
<tr>\
<td colspan=\"5\">&nbsp;</td>\
</tr>\
<tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"vol\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"cur\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"act_power\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"app_power\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"pf\"></td>\
<tr>\
<td colspan=\"5\">&nbsp;</td>\
</tr>\
<tr>\
<td class=\"alert-info\" align=\"center\"><b>Di&aacute;rio</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Di&aacute;rio Anterior</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Mensal</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Mensal Anterior</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Total</b></td>\
</tr>\
<tr>\
<td colspan=\"5\">&nbsp;</td>\
</tr>\
<tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_daily\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_daily_last\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_monthly\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_monthly_last\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_total\"></td>\
</tr>\
<tr>\
<td colspan=\"5\">&nbsp;</td>\
</tr>\
<tr>\
<td colspan=\"5\">\
<table width=\"100%\">\
<tr>\
<td class=\"alert-info\" align=\"center\"><b>Janeiro</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Fevereiro</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Mar&ccedil;o</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Abril</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Maio</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Junho</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Julho</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Agosto</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Setembro</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Outubro</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Novembro</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Dezembro</b></td>\
</tr>\
<tr>\
<td colspan=\"12\">&nbsp;</td>\
</tr>\
<tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon0\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon1\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon2\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon3\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon4\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon5\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon6\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon7\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon8\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon9\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon10\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"eng_mon11\"></td>\
</tr>\
</table>\
</td>\
</tr>"
#else
"<tr>\
<td class=\"alert-info\" align=\"center\"><b>Fuma&ccedil;a e G&aacute;s</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Movimento</b></td>\
<td class=\"alert-info\" align=\"center\"><b>N&iacute;vel</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Volume</b></td>\
</tr>\
<tr>\
<td colspan=\"4\">&nbsp;</td>\
</tr>\
<tr>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"gas\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"pir\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"level\"></td>\
<td align=\"center\"><INPUT type=\"text\" class=\"form-control stat\" readonly=\"true\" name=\"volume\"></td>\
</tr>"
#endif
"</table>\
</td>\
</tr>"
#endif
"<tr>\
<td colspan=\"3\">&nbsp;</td>\
</tr>\
<tr>\
<td colspan=\"3\" align=\"center\" valign=\"middle\"><BR>\
<INPUT type=\"submit\" name=\"update\" class=\"btn btn-primary\" value=\"Atualizar\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"6\">"
#if defined(CONFIG__MLI_1WRP_TYPE__)
"&nbsp;<INPUT type=\"submit\" name=\"stat_Reset\" class=\"btn btn-primary\" value=\"Resetar\" onclick=\"return confirm('Deseja resetar contadores?');\">"
#endif
"</td>\
</tr>\
</table>\
</FORM>\
</div>\
</div>\
</td>\
</tr>"
};

 
DCACHE_FLASH_ATTR static const char INDEXADMIN_UPDATE[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" >\
<div class=\"panel-heading\">\
<h3 class=\"panel-title\"><b>Admin</b></h3>\
</div>\
<div class=\"panel-body panel-form\">\
<FORM name=\"INDEXADMIN\" method=\"POST\">\
<table border=\"0\" width=\"100%\" height=\"100%\">\
<tr><td colspan=\"2\">\
<div class=\"erase\">\
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp;\
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp;\
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp;\
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp;\
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp;\
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</div>\
</td></tr>\
<tr>\
<td colpan=\"2\">&nbsp;</td>\
</tr>\
<TD><section class=\"erase\" id=\"countdown\"><b>Protocolo</b></section></TD>\
<TD>\
<div class=\"erase\">\
<SELECT class=\"form-control\" name=\"proto\">\
<OPTION value=\"HTTP\">HTTP</OPTION>\
</SELECT>\
</div>\
</TD>\
</tr>\
<tr>\
<TD><div class=\"erase\"><b>URL</b></div></TD>\
<TD><div class=\"erase\"><INPUT type=\"text\" class=\"form-control\" name=\"uri\" maxlength=\"80\"></div></TD>\
</tr>\
<tr>\
<TD colspan=2 align=\"center\" valign=\"middle\"><BR>\
<div class=\"erase\">\
<INPUT type=\"button\" name=\"save\" class=\"btn btn-primary\" value=\"Atualizar\" onClick=\"update();\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"1\">\
</div>\
</TD>\
</tr>\
</table>\
</FORM>\
</div>\
</div>\
</td>\
</tr>\
<script type=\"text/javascript\">\
function update() {\
if (confirm('Voce deseja atualizar o sistema?')) {\
var xhr = new XMLHttpRequest();\
xhr.open('POST', '/', true);\
xhr.onload = function () {\
var span = document.getElementsByClassName(\"erase\");\
for(var i=0;i<span.length;i++){\
span[i].innerHTML = \"\";\
}\
var counter = 60;\
var timer = setInterval(function() {\
if (counter > 0) {\
var span = document.getElementById(\"countdown\");\
span.innerHTML = \"<b>Voce ser&aacute; reconectado em \" + counter + \" segundos...</b><BR>\";\
span.innerHTML += \"<b>N&atilde;o desligue o equipamento!</b>\";\
counter--;\
return;\
}\
clearInterval(timer);\
window.location.replace('/');\
}, 1000);\
}\n\
var param = 'menuopt=7&subopt=1&save=1&proto='+INDEXADMIN.proto.value+'&uri='+INDEXADMIN.uri.value;\
xhr.send(param);\
}\
}\
</script>"
};

 
DCACHE_FLASH_ATTR static const char INDEXADMIN_RESET[] = {
"<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" > \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Admin</b></h3> \
</div> \
<div class=\"panel-body panel-form\"> \
<table border=\"0\" width=\"100%\" height=\"100%\"> \
<tr><td colspan=\"2\"> \
<div class=\"erase\">\
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp; \
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp; \
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp; \
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp; \
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp; \
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</div>\
</td> \
</tr> \
<FORM name=\"INDEXADMIN\" method=\"POST\" onsubmit=\"setTimeout(function(){ wait_for_reset(); }, 100);\" >   \
<tr> \
<td colspan=\"2\">&nbsp;</td> \
</tr>\
<tr>\
<TD><section class=\"erase\" id=\"countdown\"><b>Tipo de Reboot</b></section></TD>\
<td>\
<div class=\"erase\">\
<SELECT class=\"form-control\" name=\"reboot_mode\">\
<option value=0>Normal</option>\
<option value=1>Restaurar configura&ccedil;&atilde;o</option>\
</SELECT>\
</div>\
</td>\
</tr>\
<tr>\
<TD colspan=2 align=\"center\" valign=\"middle\"><BR>\
<div class=\"erase\">\
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Reiniciar\" onClick=\"return confirm('Voce realmente deseja reiniciar o sistema?');\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"2\">\
</div>\
</TD>\
</tr>\
</FORM>\
</table>\
</div> \
</div> \
</td> \
</tr>\
<script type=\"text/javascript\">\
function wait_for_reset(){\
var span = document.getElementsByClassName(\"erase\");\
for(var i=0;i<span.length;i++){\
span[i].innerHTML = \"\";\
}\
var counter = 10;\
var timer = setInterval(function() {\
if (counter > 0) {\
var span = document.getElementById(\"countdown\");\
span.innerHTML = \"<b>Voce ser&aacute; reconectado em \" + counter + \" segundos...</b>\";\
counter--;\
return; \
}\
clearInterval(timer);\
window.location.replace(\"/\");\
}, 1000);\
}\
</script>"
};

 
DCACHE_FLASH_ATTR static const char INDEXADMIN_SENHA[] = {
"<!-Admin--> \
<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" > \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Admin</b></h3> \
</div> \
<div class=\"panel-body panel-form\"> \
<table border=\"0\" width=\"100%\" height=\"100%\"> \
<tr><td colspan=\"2\"> \
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp; \
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp; \
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp; \
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp; \
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp; \
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</td> \
</tr> \
<FORM name=\"INDEXADMIN\" action=\"saveweb\" method=\"POST\">   \
<tr>\
<td colpan=\"2\">&nbsp;</td>\
</tr><tr>\
<TD><b>Usu&aacute;rio</b></TD>\
<TD><INPUT type=\"text\" class=\"form-control\" name=\"user\" size=\"50\" maxlength=\"100\"></TD>\
</tr>\
<tr>\
<TD><b>Nova Senha</b></TD>\
<TD><INPUT type=\"password\" class=\"form-control\" name=\"npasswd\" size=\"50\" maxlength=\"100\"></TD>\
</tr>\
<tr>\
<TD><b>Confirmar Senha</b></TD>\
<TD><INPUT type=\"password\" class=\"form-control\" name=\"repasswd\" size=\"50\" maxlength=\"100\"></TD>\
</tr><tr>\
<TD colspan=2 align=\"center\" valign=\"middle\"><BR>\
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Salvar\" onClick=\"return change_passwd(document.INDEXADMIN);\" >\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"3\">\
</TD>\
</tr>\
</table>\
</FORM>\
</div> \
</div> \
</td> \
</tr>\
<script>\
function change_passwd(f)\
{\
var val = f.user.value;\
if (f.user.value == \"\") {\
alert(\"Usuario ou Senha invalidos!\");\
return false;\
}\
if(val.match(/^[a-zA-Z0-9]+$/) == null) {\
alert(\"Somente caracteres alfanumericos!\");\
return false;\
}\
val = f.npasswd.value;\
if (f.npasswd.value == \"\") {\
alert(\"Usuario ou Senha invalidos!\");\
return false;\
}\
if(val.match(/^[a-zA-Z0-9]+$/) == null) {\
alert(\"Somente caracteres alfanumericos!\");\
return false;\
}\
if(f.npasswd.value != f.repasswd.value) {\
alert(\"Senhas nao conferem!\");\
return false;\
}\
if(confirm(\"Alterar senha de acesso?\") == true)\
return true;\
else\
 return false;\
}\
</script>"
};

 
DCACHE_FLASH_ATTR static const char INDEXADMIN_TIMEZONE[] = {
"<!-Admin--> \
<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" > \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Admin</b></h3> \
</div> \
<div class=\"panel-body panel-form\"> \
<table border=\"0\" width=\"98%\" height=\"100%\"> \
<tr><td colspan=\"2\"> \
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp; \
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp; \
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp; \
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp; \
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp; \
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</td> \
</tr> \
<FORM name=\"INDEXADMIN\" action=\"save\" method=\"POST\">  \
<tr> \
<td colspan=\"2\">&nbsp;</td> \
</tr><tr>\
<td><b>Fuso Hor&aacute;rio</b></td>\
<td>\
<select id=\"timeZone\" class=\"form-control\" name=\"settimezone\"></select>\
</td>\
</tr><tr>\
<td><b>Hor&aacute;rio de Ver&atilde;o</b></td>\
<td><INPUT type=\"checkbox\" name=\"dst_enable\"></td>\
</tr><tr> \
<td colspan=\"2\">&nbsp;</td> \
</tr><tr>\
<td class=\"alert-info\" colspan=\"2\"><b>Data de In&iacute;cio</b></td>\
</tr><tr>\
<td><b>M&ecirc;s</b></td>\
<td>\
<select id=\"startMonth\" class=\"form-control\" name=\"start_mon\" style=\"margin-top:5px;\"></select>\
</td>\
</tr><tr>\
<td><b>Semana</b></td>\
<td>\
<select id=\"startWeek\"class=\"form-control\" name=\"start_week\"></select>\
</td>\
</tr><tr>\
<td><b>Dia da Semana</b></td>\
<td>\
<select id=\"startDay\" class=\"form-control\" name=\"start_day\"></select>\
</td>\
</tr><tr> \
<td colspan=\"2\">&nbsp;</td> \
</tr><tr>\
<td class=\"alert-info\" colspan=\"2\"><b>Data do T&eacute;rmino</b></td>\
</tr><tr>\
<td><b>M&ecirc;s</b></td>\
<td>\
<select id=\"endMonth\" class=\"form-control\" name=\"end_mon\" style=\"margin-top:5px;\"></select>\
</td>\
</tr><tr>\
<td><b>Semana</b></td>\
<td>\
<select id=\"endWeek\"class=\"form-control\" name=\"end_week\"></select>\
</td>\
</tr><tr>\
<td><b>Dia da Semana</b></td>\
<td>\
<select id=\"endDay\" class=\"form-control\" name=\"end_day\"></select>\
</td>\
</tr>\
<tr>\
<TD colspan=3 align=\"center\" valign=\"middle\"><BR>\
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Salvar\" onclick=\"alert('Reinicie o equipamento para validar as configura&ccedil;&otilde;es!');\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"4\">\
</TD>\
</tr>\
</FORM>\
</table>\
</div> \
</div> \
</td> \
</tr>\
<script>\
var months=[\"Janeiro\",\"Fevereiro\",\"Março\",\"Abril\",\"Maio\",\"Junho\",\"Julho\",\"Agosto\",\"Setembro\",\"Outubro\",\"Novembro\",\"Dezembro\"];\
var days=[\"Domingo\",\"Segunda\",\"Terça\",\"Quarta\",\"Quinta\",\"Sexta\",\"sábado\"];\
for(var i=0; i<25; i++){\
var c=document.createElement(\"option\");\
c.text=\"GMT\";\
c.value = i;\
if(i>12)c.text+=\"+\"+(i-12);\
else if(i<12)c.text+=(i-12);\
document.getElementById(\"timeZone\").options.add(c);\
if(i<12){\
var c = document.createElement(\"option\");\
var d = document.createElement(\"option\");\
c.text = months[i];\
d.text = months[i];\
c.value = i+1;\
d.value = i+1;\
document.getElementById(\"startMonth\").options.add(c);\
document.getElementById(\"endMonth\").options.add(d);\
}\
if(i<6){\
var c=document.createElement(\"option\");\
var d=document.createElement(\"option\");\
c.text=i+1;\
d.text=i+1;\
c.value = i+1;\
d.value = i+1;\
document.getElementById(\"startWeek\").options.add(c);\
document.getElementById(\"endWeek\").options.add(d);\
}\
if(i<7){\
var c = document.createElement(\"option\");\
var d = document.createElement(\"option\");\
c.text=days[i];\
d.text=days[i];\
c.value = i;\
d.value = i;\
document.getElementById(\"startDay\").options.add(c);\
document.getElementById(\"endDay\").options.add(d);\
}\
}\
</script>"
};

 
DCACHE_FLASH_ATTR static const char INDEXADMIN_LOCATION[] = {
"<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" > \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Admin</b></h3> \
</div> \
<div class=\"panel-body panel-form\"> \
<table border=\"0\" width=\"100%\" height=\"100%\"> \
<tr><td colspan=\"2\"> \
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp; \
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp; \
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp; \
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp; \
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp; \
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</td> \
</tr> \
<FORM name=\"INDEXADMIN\" method=\"POST\">   \
<tr>\
<td colpan=\"2\">&nbsp;</td>\
</tr><tr>\
<td><b>Latitude</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"latitude\" maxlength=\"15\"><b> graus</b></td>\
</tr><tr>\
<td><b>Longitude</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"longitude\" maxlength=\"15\"><b> graus</b></td>\
</tr><tr>\
<td colspan=2 align=\"center\" valign=\"middle\"><BR>\
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Salvar\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"5\">\
</td>\
</tr>\
</table>\
</FORM>\
</div> \
</div> \
</td> \
</tr>"
};

 
DCACHE_FLASH_ATTR static const char INDEXADMIN_SYSTEM[] = {
"<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" > \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Admin</b></h3> \
</div> \
<div class=\"panel-body panel-form\"> \
<table border=\"0\" width=\"100%\" height=\"100%\"> \
<tr><td colspan=\"2\"> \
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp; \
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp; \
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp; \
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp; \
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp; \
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</td> \
</tr> \
<FORM name=\"INDEXADMIN\" method=\"POST\">   \
<tr>\
<td colpan=\"2\">&nbsp;</td>\
</tr><tr>\
<TD><b>N&uacute;mero de S&eacute;rie</b></TD>\
<TD><INPUT type=\"text\" class=\"form-control info\" readonly=\"true\" name=\"serial\"></TD>\
</tr><tr>\
<TD><b>Endere&ccedil;o MAC</b></TD>\
<TD><INPUT type=\"text\" class=\"form-control info\" readonly=\"true\" name=\"mac\"></TD>\
</tr><tr>\
<TD><b>Release</b></TD>\
<TD><INPUT type=\"text\" class=\"form-control info\" readonly=\"true\" name=\"release\"></TD>\
</tr><tr>\
<TD><b>Hor&aacute;rio</b></TD>\
<TD><INPUT type=\"text\" class=\"form-control info\" readonly=\"true\" name=\"date\"></TD>\
</tr><tr>\
<TD><b>Uptime</b></TD>\
<TD><INPUT type=\"text\" class=\"form-control info\" readonly=\"true\" name=\"uptime\"></TD>\
</tr> \
</table>\
</FORM>\
</div> \
</div> \
</td> \
</tr>"
};

 
DCACHE_FLASH_ATTR static const char INDEXADMIN_DEBUG[] = {
"<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" > \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Admin</b></h3> \
</div> \
<div class=\"panel-body panel-form\"> \
<table border=\"0\" width=\"100%\" height=\"100%\"> \
<tr><td colspan=\"2\"> \
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp; \
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp; \
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp; \
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp; \
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp; \
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</td> \
</tr> \
<FORM name=\"INDEXADMIN\" action=\"savedebug\" method=\"POST\">   \
<tr>\
<td colpan=\"2\">&nbsp;</td>\
</tr><tr>\
<td><b>Habilitar Debug</b></td>\
<td><INPUT type=\"checkbox\" name=\"debug_enable\"></td>\
</tr><tr>\
<TD><b>IP</b></TD>\
<TD><INPUT type=\"text\" class=\"form-control\" name=\"ip\" size=\"50\" maxlength=\"16\"></TD>\
</tr><tr>\
<TD><b>Porta</b></TD>\
<TD><INPUT type=\"text\" class=\"form-control\" name=\"port\" size=\"50\" maxlength=\"5\"></TD>\
</tr><tr>\
<TD><b>N&iacute;vel</b></TD>\
<TD>\
<SELECT class=\"form-control\" name=\"debug_level\">\
<OPTION value=\"0\">Nenhum</OPTION>\
<OPTION value=\"2\">ERROR</OPTION>\
<OPTION value=\"3\">CRITICAL</OPTION>\
<OPTION value=\"4\">WARNING</OPTION>\
<OPTION value=\"5\">MESSAGE</OPTION>\
<OPTION value=\"6\">INFO</OPTION>\
<OPTION value=\"7\">DEBUG</OPTION>\
</SELECT>\
</TD>\
</tr><tr>\
<TD colspan=2 align=\"center\" valign=\"middle\"><BR>\
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Salvar\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"7\">\
</TD>\
</tr>\
</table>\
</FORM>\
</div> \
</div> \
</td> \
</tr>"
};

 
DCACHE_FLASH_ATTR static const char INDEXADMIN_BACKUP[] = {
"<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" > \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Admin</b></h3> \
</div> \
<div class=\"panel-body panel-form\"> \
<table border=\"0\" width=\"100%\" height=\"100%\"> \
<tr><td colspan=\"2\"> \
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp; \
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp; \
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp; \
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp; \
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp; \
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</td> \
</tr> \
<tr>\
<td colpan=\"2\">&nbsp;</td>\
</tr><tr>\
<td><b>Salvar Configura&ccedil;&otilde;es</b></td>\
<FORM name=\"BACKUP\" method=\"POST\">   \
<td><INPUT type=\"button\" class=\"btn btn-primary\" id=\"backup\" name=\"backup\" value=\"Backup\" style=\"width:100px;\" onclick=\"save();\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"8\">\
</td>\
</FORM>\
</tr><tr>\
<td><b>Carregar Configura&ccedil;&otilde;es</b></td>\
<FORM name=\"LOAD\" method=\"POST\" ENCTYPE=\"multipart/form-data\">\
<td>\
<INPUT type=\"button\" class=\"btn btn-primary\" name=\"restore\" value=\"Carregar\" style=\"width:100px;\" onclick=\"load();\">&nbsp;\
<INPUT type=\"file\" id=\"upfile\" name=\"upfile\" style=\"display:inline !important;\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"8\">\
</td>\
</FORM>\
</tr>\
<script>\
function save() {\
var xhr = new XMLHttpRequest();\
xhr.open('GET', '/?request=getconfig', true);\
xhr.onload = function () {\
var json = xhr.response;\
var link = document.createElement('a');\
link.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(json));\
link.setAttribute('download', 'config.json');\
link.style.display = 'none';\
document.body.appendChild(link);\
link.click();\
document.body.removeChild(link);\
}\n\
xhr.send();\
}\n\
function load() {\
var xhr = new XMLHttpRequest();\
xhr.open('POST', '/?menuopt=7&subopt=8&request=setconfig', true);\
xhr.setRequestHeader('Content-type', 'application/json');\
xhr.onload = function() {\
alert('Reinicie o equipamento para validar as configuracoes!');\
}\n\
var file = document.getElementById('upfile').files[0];\
if (!file) {\
alert('Selecione o arquivo de configuracao!');\
return;\
}\
if (!confirm('Deseja carregar as configuracoes?'))\
return;\
var reader = new FileReader();\
reader.onloadend = function () {\
var config = reader.result;\
try {\
var obj = JSON.parse(config);\
} catch(e) {\
alert('Arquivo de configuracao invalido!');\
return;\
}\n\
xhr.send(config);\
}\n\
reader.readAsText(file);\
}\
</script>\
</table>\
</div> \
</div> \
</td> \
</tr>"
};

 
DCACHE_FLASH_ATTR static const char INDEXADMIN_WIFI[] = {
"<tr valign=\"top\" height=\"100%\"> \
<td valign=\"top\"> \
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" > \
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Admin</b></h3> \
</div> \
<div class=\"panel-body panel-form\"> \
<table border=\"0\" width=\"98%\" height=\"100%\"> \
<tr><td colspan=\"2\"> \
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp; \
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp; \
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp; \
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp; \
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp; \
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</td> \
</tr> \
<FORM name=\"INDEXADMIN\" method=\"POST\">   \
<tr>\
<td>&nbsp;</td>\
</tr><tr>\
<td colspan=\"2\" class=\"alert-info\"><b>Redes Dispon&iacute;veis</b></td>\
</tr><tr>\
<td colspan=\"2\">\
<table align=\"center\" width=\"100%\" style=\"text-align:center;\">\
<thead>\
<tr>\
<td style=\"text-align:left;\"><b>Rede</b></td>\
<td><b>Autentica&ccedil;&atilde;o</b></td>\
<td><b>Canal</b></td>\
<td><b>N&iacute;vel de Sinal</b></td>\
</tr>\
</thead>\
<tbody id=\"wifi\">\
</tbody>\
</table>\
</td>\
</tr><tr>\
<TD colspan=2 align=\"center\" valign=\"middle\"><BR>\
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Atualizar\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"9\">\
</TD>\
</tr>\
</table>\
</FORM>\
</div>\
</div>\
</td>\
</tr>\
<script>\
setInterval(function () { location.reload(); }, 10000);\
</script>"
};


DCACHE_FLASH_ATTR static const char INDEXADMIN_WATCHDOG[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\" valign=\"top\" >\
<div class=\"panel-heading\">\
<h3 class=\"panel-title\"><b>Admin</b></h3>\
</div>\
<div class=\"panel-body panel-form\">\
<table border=\"0\" width=\"98%\" height=\"100%\">\
<tr><td colspan=\"2\">\
<a id=\"tabUpdate\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=1\"><b>Atualizar</b></a>&nbsp;\
<a id=\"tabReset\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=2\"><b>Reiniciar</b></a>&nbsp;\
<a id=\"tabPasswd\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=3\"><b>Senha</b></a>&nbsp;\
<a id=\"tabTimezone\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=4\"><b>Fuso Hor&aacute;rio</b></a>&nbsp;\
<a id=\"tabLocation\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=5\"><b>Localiza&ccedil;&atilde;o</b></a>&nbsp;\
<a id=\"tabSystem\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=6\"><b>Sistema</b></a>&nbsp;\
<a id=\"tabDebug\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=7\"><b>Debug</b></a>&nbsp;\
<a id=\"tabBackup\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=8\"><b>Backup</b></a>&nbsp;\
<a id=\"tabWifi\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=9\"><b>Wireless</b></a>&nbsp;\
<a id=\"tabWatchdog\" class=\"btn btn-default\" href=\"/?menuopt=7&subopt=10\"><b>Watchdog</b></a>\
</td>\
</tr>\
<FORM name=\"INDEXADMIN\" method=\"POST\">   \
<tr>\
<td colpan=\"2\">&nbsp;</td>\
</tr><tr>\
<TD><b>Reboot Peri&oacute;dico</b></TD>\
<TD><INPUT type=\"number\" class=\"form-control\" name=\"shutdown\" size=\"50\" maxlength=\"16\"> <b>hora(s)</b></TD>\
</tr><tr>\
<td>&nbsp;</td>\
<td align=\"center\" id=\"shutdown\" class=\"danger\" style=\"display:none;width:50%;color:#fff;padding:4px 0px 4px 0px;border-radius:6px;\"></td>\
</tr><tr>\
<TD colspan=2 align=\"center\" valign=\"middle\"><BR>\
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Salvar\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"7\">\
<INPUT type=\"hidden\" name=\"subopt\" value=\"10\">\
</TD>\
</tr>\
</table>\
</FORM>\
</div> \
</div> \
</td> \
</tr>"
};


DCACHE_FLASH_ATTR static const char INDEXPROG[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\">\
<div class=\"panel-heading\"> \
<h3 class=\"panel-title\"><b>Programa&ccedil;&atilde;o de Acionamento</b></h3>\
</div> \
<div class=\"panel-body\"> \
<FORM name=\"PROG\" action=\"save\" method=\"POST\">\
<table border=\"0\" class=\"table\" align=\"left\" width=\"98%\" height=\"100%\">\
<tr>\
<td colspan=\"2\">\
<table align=\"center\" width=\"100%\">\
<script type=\"text/javascript\">\
function select_hour(ff)\
{\
ff.options[0]=new Option('-','');\
for(var i=0;i<24;i++)\
ff.options[i+1]=new Option(i+'h',i);\
}\n\
function select_minute(ff)\
{\
ff.options[0]=new Option('-','');\
for(var i=0;i<60;i++)\
ff.options[i+1]=new Option(i,i);\
}\n\
function select_week(ff)\
{\
var weekarr = ['Dom','Seg','Ter','Qua',\
'Qui','Sex','Sab'];\
var weeklen = weekarr.length;\
ff.options[0]=new Option('-','');\
for(var i=0;i<weeklen;i++)\
ff.options[i+1]=new Option(weekarr[i],i);\
}\n\
function select_day(ff)\
{\
ff.options[0]=new Option('-','');\
for(var i=1;i<=31;i++)\
ff.options[i]=new Option(i,i);\
}\n\
function select_month(ff)\
{\
var month = ['Jan','Fev','Mar','Abr',\
'Mai','Jun','Jul','Ago','Set','Out','Nov','Dez'];\
ff.options[0] = new Option('-','');\
for (var i=0;i<month.length;i++)\
ff.options[i+1]=new Option(month[i],i+1);\
}\n\
function select_year(ff)\
{\
ff.options[0] = new Option('-','');\
for (var i=2017;i<=2050;i++)\
ff.options[i-2016]=new Option(i,i);\
}\n\
</script>\
<tr>\
<td align=\"center\"><b>Semana</b></td>\
<td align=\"center\"><b>Data</b></td>\
<td align=\"center\"><b>Hor&aacute;rio</b></td>\
</tr>\
<tr>\
<td align=\"center\">\
<select class=\"form-control week\" name=\"w1i\">\
<script>\
select_week(document.PROG.w1i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control week\" name=\"w1e\">\
<script>\
select_week(document.PROG.w1e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control date\" name=\"d1i\">\
<script>\
select_day(document.PROG.d1i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t1i\">\
<script>\
select_month(document.PROG.t1i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y1i\">\
<script>\
select_year(document.PROG.y1i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control date\" name=\"d1e\">\
<script>\
select_day(document.PROG.d1e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t1e\">\
<script>\
select_month(document.PROG.t1e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y1e\">\
<script>\
select_year(document.PROG.y1e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control hour\" name=\"h1i\">\
<script>\
select_hour(document.PROG.h1i);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m1i\">\
<script>\
select_minute(document.PROG.m1i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control hour\" name=\"h1e\">\
<script>\
select_hour(document.PROG.h1e);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m1e\">\
<script>\
select_minute(document.PROG.m1e);\
</script>\
</select>\
</td></tr>\
<tr>\
<td align=\"center\">\
<select class=\"form-control week\" name=\"w2i\">\
<script>\
select_week(document.PROG.w2i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control week\" name=\"w2e\">\
<script>\
select_week(document.PROG.w2e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control date\" name=\"d2i\">\
<script>\
select_day(document.PROG.d2i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t2i\">\
<script>\
select_month(document.PROG.t2i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y2i\">\
<script>\
select_year(document.PROG.y2i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control date\" name=\"d2e\">\
<script>\
select_day(document.PROG.d2e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t2e\">\
<script>\
select_month(document.PROG.t2e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y2e\">\
<script>\
select_year(document.PROG.y2e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control hour\" name=\"h2i\">\
<script>\
select_hour(document.PROG.h2i);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m2i\">\
<script>\
select_minute(document.PROG.m2i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control hour\" name=\"h2e\">\
<script>\
select_hour(document.PROG.h2e);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m2e\">\
<script>\
select_minute(document.PROG.m2e);\
</script>\
</select>\
</td></tr>\
<tr>\
<td align=\"center\">\
<select class=\"form-control week\" name=\"w3i\">\
<script>\
select_week(document.PROG.w3i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control week\" name=\"w3e\">\
<script>\
select_week(document.PROG.w3e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control date\" name=\"d3i\">\
<script>\
select_day(document.PROG.d3i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t3i\">\
<script>\
select_month(document.PROG.t3i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y3i\">\
<script>\
select_year(document.PROG.y3i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control date\" name=\"d3e\">\
<script>\
select_day(document.PROG.d3e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t3e\">\
<script>\
select_month(document.PROG.t3e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y3e\">\
<script>\
select_year(document.PROG.y3e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control hour\" name=\"h3i\">\
<script>\
select_hour(document.PROG.h3i);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m3i\">\
<script>\
select_minute(document.PROG.m3i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control hour\" name=\"h3e\">\
<script>\
select_hour(document.PROG.h3e);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m3e\">\
<script>\
select_minute(document.PROG.m3e);\
</script>\
</select>\
</td></tr>\
<tr>\
<td align=\"center\">\
<select class=\"form-control week\" name=\"w4i\">\
<script>\
select_week(document.PROG.w4i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control week\" name=\"w4e\">\
<script>\
select_week(document.PROG.w4e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control date\" name=\"d4i\">\
<script>\
select_day(document.PROG.d4i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t4i\">\
<script>\
select_month(document.PROG.t4i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y4i\">\
<script>\
select_year(document.PROG.y4i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control date\" name=\"d4e\">\
<script>\
select_day(document.PROG.d4e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t4e\">\
<script>\
select_month(document.PROG.t4e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y4e\">\
<script>\
select_year(document.PROG.y4e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control hour\" name=\"h4i\">\
<script>\
select_hour(document.PROG.h4i);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m4i\">\
<script>\
select_minute(document.PROG.m4i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control hour\" name=\"h4e\">\
<script>\
select_hour(document.PROG.h4e);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m4e\">\
<script>\
select_minute(document.PROG.m4e);\
</script>\
</select>\
</td></tr>\
<tr>\
<td align=\"center\">\
<select class=\"form-control week\" name=\"w5i\">\
<script>\
select_week(document.PROG.w5i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control week\" name=\"w5e\">\
<script>\
select_week(document.PROG.w5e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control date\" name=\"d5i\">\
<script>\
select_day(document.PROG.d5i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t5i\">\
<script>\
select_month(document.PROG.t5i);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y5i\">\
<script>\
select_year(document.PROG.y5i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control date\" name=\"d5e\">\
<script>\
select_day(document.PROG.d5e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"t5e\">\
<script>\
select_month(document.PROG.t5e);\
</script>\
</select>\
<select class=\"form-control date\" name=\"y5e\">\
<script>\
select_year(document.PROG.y5e);\
</script>\
</select>\
</td>\
<td align=\"center\">\
<select class=\"form-control hour\" name=\"h5i\">\
<script>\
select_hour(document.PROG.h5i);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m5i\">\
<script>\
select_minute(document.PROG.m5i);\
</script>\
</select>\
&nbsp;\
<select class=\"form-control hour\" name=\"h5e\">\
<script>\
select_hour(document.PROG.h5e);\
</script>\
</select>\
<select class=\"form-control hour\" name=\"m5e\">\
<script>\
select_minute(document.PROG.m5e);\
</script>\
</select>\
</td></tr>\
</table>\
</td>\
</tr><tr>\
<td colspan=\"2\" align=\"center\" valign=\"middle\" ><BR>\
<INPUT type=\"submit\" name=\"save\" class=\"btn btn-primary\" value=\"Salvar\" onclick=\"alert('Reinicie o equipamento para validar as configura&ccedil;&otilde;es!');\">\
<INPUT type=\"hidden\" name=\"menuopt\" value=\"8\">\
</td>\
</tr>\
</table>\
</FORM>\
</div>\
</div>\
</td>\
</tr>\
</table>"
};

#if 0

static const uint8_t LOGO[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 
    0x00, 0x00, 0x00, 0x5A, 0x00, 0x00, 0x00, 0x21, 0x08, 0x06, 0x00, 0x00, 0x00, 0x59, 0x4D, 0xBC, 
    0x00, 0x00, 0x00, 0x0E, 0x64, 0x49, 0x44, 0x41, 0x54, 0x68, 0xDE, 0xE5, 0x9A, 0x79, 0xB0, 0x65, 
    0x55, 0x75, 0xC6, 0x7F, 0x6B, 0xEF, 0x7D, 0x86, 0x3B, 0xBC, 0xF9, 0xF5, 0x40, 0xCB, 0xD4, 0x4C, 
    0x8A, 0x28, 0x21, 0x0E, 0x90, 0x44, 0x05, 0xEC, 0xAA, 0x98, 0x50, 0x31, 0x81, 0xC4, 0x10, 0xA5, 
    0x0C, 0x15, 0x07, 0xEC, 0x50, 0xA5, 0x49, 0x45, 0x63, 0x55, 0x52, 0x58, 0x6A, 0x99, 0xA8, 0x19, 
    0x2C, 0x33, 0x10, 0x21, 0x89, 0x14, 0x60, 0xA9, 0x49, 0x80, 0x24, 0x96, 0x03, 0x04, 0x29, 0x11, 
    0x5B, 0x19, 0x12, 0x45, 0x5B, 0x91, 0x58, 0xD2, 0x0D, 0xDD, 0x22, 0x3D, 0x77, 0xBF, 0xDB, 0x6F, 
    0xBE, 0xF7, 0x9E, 0x61, 0xEF, 0x95, 0x3F, 0xCE, 0x7D, 0xC3, 0x7D, 0xEF, 0x01, 0x8D, 0x5A, 0xFC, 
    0x91, 0xB7, 0xAB, 0xEE, 0x7B, 0xF7, 0xD6, 0xD9, 0x67, 0x9F, 0x73, 0xBE, 0xFD, 0xED, 0x6F, 0x7D, 
    0x6B, 0xED, 0x23, 0xAC, 0x6C, 0x7F, 0x72, 0x9F, 0x61, 0xF6, 0xF1, 0x06, 0xB5, 0xE1, 0x41, 0xE6, 
    0x5C, 0x4C, 0x11, 0xC0, 0xD8, 0xDE, 0x41, 0xED, 0xFD, 0x93, 0xC5, 0xAF, 0x7D, 0xCD, 0x2C, 0xFB, 
    0xAE, 0x00, 0x01, 0x8C, 0x17, 0x88, 0x02, 0x36, 0x2F, 0x90, 0xCE, 0x0C, 0x9D, 0x62, 0x9E, 0x4F, 
    0x6D, 0x0F, 0xAC, 0xB3, 0x26, 0x7D, 0xBF, 0xAE, 0xFD, 0x97, 0x8D, 0xE4, 0x66, 0x1B, 0x14, 0xBF, 
    0x45, 0x5C, 0xBF, 0x14, 0x89, 0x37, 0x50, 0x2A, 0x88, 0xF4, 0xBA, 0x6A, 0xFF, 0x69, 0xFA, 0x74, 
    0xA3, 0x2D, 0x3F, 0x10, 0x7A, 0x33, 0xE0, 0x8F, 0x91, 0xB7, 0x77, 0x60, 0xCB, 0x2F, 0xD1, 0x88, 
    0xEF, 0xE5, 0xFA, 0xAB, 0x0E, 0xAD, 0x4F, 0xA0, 0x7F, 0xE7, 0x6F, 0xCE, 0xA0, 0x36, 0xFE, 0x7E, 
    0xA2, 0xFA, 0xEF, 0x61, 0x62, 0x41, 0x62, 0xB0, 0x11, 0x84, 0xF0, 0x0C, 0x80, 0xAE, 0x68, 0x2B, 
    0xE6, 0xA1, 0x9A, 0xA0, 0x05, 0xB0, 0x05, 0x7C, 0x0E, 0xBE, 0x0B, 0x3E, 0xBB, 0x8D, 0xCE, 0x91, 
    0x0F, 0x73, 0xFB, 0xBB, 0x7F, 0xB0, 0xBE, 0x80, 0xBE, 0xEA, 0x1F, 0xC6, 0x21, 0xFE, 0x0B, 0x6A, 
    0x63, 0xD7, 0x90, 0x0E, 0x81, 0x06, 0xD0, 0x90, 0xA1, 0x3A, 0x4F, 0xF0, 0x8A, 0x51, 0x81, 0x9E, 
    0x5C, 0xC8, 0x22, 0xA8, 0xAB, 0xE1, 0x97, 0x15, 0xB0, 0x6B, 0x6F, 0x19, 0x88, 0x08, 0x62, 0x1A, 
    0x60, 0x13, 0x8C, 0x85, 0xBC, 0x03, 0xF3, 0xAD, 0xDB, 0xC8, 0x8E, 0xBF, 0x87, 0xCF, 0xBD, 0x67, 
    0x5D, 0x30, 0xDB, 0xF1, 0xAE, 0x7F, 0x32, 0x4C, 0xB1, 0x8D, 0x68, 0xF8, 0xAD, 0xD4, 0x06, 0xA1, 
    0xCC, 0x21, 0xE4, 0xDF, 0x45, 0xF3, 0xFF, 0xA2, 0x6C, 0xEF, 0x27, 0x94, 0x8A, 0x33, 0x06, 0x55, 
    0xA5, 0xC2, 0x5B, 0x10, 0x14, 0xAF, 0xFD, 0x50, 0x8B, 0x08, 0x46, 0x04, 0x51, 0x50, 0xD5, 0x3E, 
    0x79, 0x71, 0x89, 0x01, 0xBB, 0x19, 0xEC, 0xEB, 0xB1, 0xCD, 0x9F, 0x27, 0xAA, 0x43, 0xE2, 0x7F, 
    0x1D, 0xC7, 0xFD, 0xC0, 0x8D, 0xEB, 0x03, 0x68, 0xA2, 0x3A, 0xD6, 0x5E, 0x41, 0xD4, 0xB0, 0x04, 
    0x20, 0xEF, 0xEE, 0xC4, 0xB4, 0xDF, 0x47, 0xDC, 0x79, 0x80, 0xA7, 0x7E, 0x98, 0x91, 0x3D, 0x05, 
    0x1B, 0x36, 0x0A, 0xED, 0x39, 0x05, 0x07, 0x71, 0x2A, 0x68, 0xAE, 0x74, 0x7D, 0x15, 0x14, 0x17, 
    0x5A, 0xE4, 0xC0, 0x5A, 0xC1, 0x29, 0xF8, 0x4C, 0x11, 0x27, 0x04, 0x81, 0x06, 0xE0, 0x4E, 0x06, 
    0x1D, 0x8D, 0xD0, 0xDA, 0x57, 0xC9, 0xC2, 0xDF, 0x51, 0x1F, 0xBD, 0x80, 0x24, 0x69, 0xD0, 0x35, 
    0xAF, 0xE7, 0x8D, 0x1F, 0xFD, 0x12, 0xB7, 0x5F, 0xB7, 0xEF, 0xFF, 0x3F, 0xD0, 0x61, 0x60, 0x88, 
    0x58, 0x2E, 0xC5, 0x38, 0xD0, 0xA2, 0x83, 0x9F, 0xBF, 0x9B, 0xE3, 0xBB, 0x1F, 0xE0, 0x0B, 0x1F, 
    0x9E, 0xFB, 0x19, 0x5F, 0x2B, 0xE7, 0xDA, 0xBF, 0x7F, 0x90, 0x28, 0xFD, 0x0C, 0xE5, 0xFC, 0xB9, 
    0x44, 0x8D, 0x04, 0x9B, 0xBC, 0x98, 0x64, 0xF8, 0x14, 0x60, 0x1D, 0x00, 0xAD, 0xC6, 0x21, 0x66, 
    0xAC, 0xB7, 0xCE, 0xE7, 0x89, 0x65, 0x1F, 0xE9, 0x63, 0xDD, 0x55, 0x3D, 0x6F, 0xF8, 0x42, 0x8D, 
    0x83, 0x7B, 0x23, 0x3E, 0xF2, 0xEE, 0x99, 0x67, 0x1C, 0xF1, 0xA6, 0x3B, 0x6A, 0xEC, 0x3E, 0x68, 
    0xF8, 0xD8, 0x1F, 0xCD, 0xAF, 0x3A, 0x36, 0xB2, 0xD5, 0x33, 0xDD, 0xDD, 0x45, 0x3B, 0x9B, 0xC2, 
    0xD6, 0x37, 0x61, 0xD2, 0x26, 0x6E, 0xA4, 0xB6, 0xBA, 0xDF, 0x86, 0x51, 0x8A, 0x68, 0x90, 0x08, 
    0x28, 0x15, 0x82, 0x42, 0x62, 0x04, 0x1B, 0x83, 0x2F, 0x40, 0x0B, 0xA0, 0x06, 0x3E, 0x40, 0xE9, 
    0xA7, 0x69, 0x1F, 0x3C, 0xFE, 0xBC, 0x23, 0x57, 0x1B, 0x3A, 0x9D, 0xA4, 0x79, 0x36, 0xCD, 0xFA, 
    0x4E, 0xF6, 0x3F, 0xDE, 0x3A, 0x01, 0x46, 0xB7, 0x21, 0xD4, 0x03, 0xC6, 0x56, 0x41, 0x90, 0x06, 
    0xA4, 0xAF, 0x01, 0xFE, 0x63, 0xA9, 0xD7, 0x35, 0xB7, 0x5C, 0xCC, 0x03, 0x47, 0x6E, 0xA5, 0x3E, 
    0xF6, 0x38, 0x6F, 0xBF, 0xE9, 0x3A, 0x6E, 0x7E, 0xC7, 0xCE, 0x55, 0x23, 0x5D, 0xFB, 0x8F, 0x42, 
    0x1E, 0x9D, 0xCF, 0xD7, 0xDA, 0x7F, 0x8D, 0x0C, 0x8F, 0x73, 0xD5, 0xDF, 0x7E, 0x80, 0x7F, 0x7B, 
    0xF7, 0x5D, 0xFD, 0xE1, 0x31, 0x01, 0x09, 0x01, 0xC9, 0x15, 0x51, 0x90, 0x52, 0x30, 0x45, 0x7F, 
    0x08, 0x1D, 0x1C, 0x7B, 0x25, 0xC1, 0x5E, 0x4B, 0xE4, 0x46, 0x31, 0xC6, 0xE3, 0x02, 0x08, 0x06, 
    0x23, 0x3D, 0x7F, 0x69, 0x41, 0x0C, 0x18, 0x04, 0x6B, 0x04, 0x29, 0x5B, 0x0C, 0x6D, 0xFC, 0x34, 
    0xD3, 0x47, 0xBF, 0xFE, 0xBC, 0x81, 0x5C, 0x1F, 0x3F, 0x1D, 0x67, 0xDF, 0x8B, 0x31, 0x2F, 0x67, 
    0x7E, 0x7E, 0x0F, 0xC3, 0xC9, 0xD5, 0x4C, 0x65, 0xFA, 0x4C, 0xA7, 0x18, 0x42, 0x07, 0x28, 0x03, 
    0x62, 0x81, 0xA0, 0x98, 0x42, 0x71, 0x53, 0xFD, 0xBD, 0x42, 0x7C, 0x23, 0xF5, 0x53, 0xCF, 0xC0, 
    0x8C, 0xBE, 0x16, 0x06, 0xDE, 0xC4, 0x07, 0x6F, 0x4A, 0x57, 0x8D, 0xF4, 0xC4, 0x6E, 0xCB, 0x5C, 
    0x7E, 0x25, 0xE9, 0xD0, 0xEB, 0xA8, 0x8F, 0xBF, 0x0C, 0xD2, 0x37, 0xB3, 0x6D, 0xFB, 0x70, 0xFF, 
    0xB4, 0xB6, 0xC1, 0x66, 0x20, 0xBA, 0x64, 0xFB, 0xA4, 0x5C, 0xA6, 0xF3, 0x83, 0x31, 0x2A, 0xAF, 
    0xC0, 0x45, 0xA7, 0x92, 0x34, 0x12, 0xE2, 0x5A, 0x5A, 0x7D, 0xD2, 0x88, 0x28, 0x76, 0xD8, 0x24, 
    0x22, 0x4A, 0x1D, 0x2E, 0xB1, 0xB8, 0xD4, 0xE1, 0x6A, 0x0E, 0x17, 0xBF, 0x00, 0xEC, 0xCB, 0x19, 
    0x3A, 0xB5, 0xFE, 0xFC, 0xE9, 0x80, 0xD9, 0x84, 0x4D, 0x46, 0x89, 0x07, 0x8E, 0x22, 0xE6, 0x74, 
    0xB4, 0xB1, 0xF5, 0xD9, 0x19, 0x6D, 0x63, 0xF0, 0xD6, 0xA2, 0xBE, 0xF2, 0xBD, 0x41, 0xA0, 0x30, 
    0x2B, 0x9D, 0x5B, 0x93, 0x38, 0xEE, 0x39, 0x88, 0x7A, 0x13, 0xDF, 0x88, 0x81, 0x7E, 0x79, 0x39, 
    0xE7, 0x6C, 0xCB, 0x5C, 0x3D, 0xC6, 0x1A, 0x30, 0x11, 0xC4, 0xB5, 0x98, 0xB1, 0x0D, 0x35, 0x60, 
    0x69, 0xD6, 0x8A, 0x08, 0xCA, 0xA0, 0xA8, 0x2E, 0xC5, 0x62, 0x4D, 0x96, 0x98, 0x90, 0x6C, 0x2C, 
    0xD0, 0xE9, 0x5D, 0x94, 0xFA, 0x24, 0xAE, 0xAC, 0xE3, 0x4D, 0x80, 0x42, 0x71, 0xA1, 0x81, 0xB1, 
    0x31, 0x5E, 0x02, 0x65, 0x99, 0xE3, 0x7D, 0x1B, 0xA7, 0x15, 0xD3, 0xF1, 0x19, 0x4E, 0x76, 0x71, 
    0x72, 0xD9, 0xE5, 0x91, 0x9F, 0xA9, 0xED, 0x7D, 0x06, 0x86, 0x4A, 0xC0, 0x8A, 0x62, 0x10, 0x7C, 
    0xE8, 0x92, 0xBB, 0x13, 0x71, 0x1D, 0x2B, 0x86, 0x17, 0x20, 0x5B, 0xC9, 0x7B, 0x29, 0x08, 0x0A, 
    0x82, 0x62, 0x28, 0x49, 0xD7, 0xB8, 0x89, 0xB2, 0x54, 0x54, 0x43, 0xE5, 0x44, 0x14, 0x44, 0x02, 
    0x26, 0x0E, 0x6B, 0x3E, 0xC3, 0x22, 0xA3, 0x95, 0x3E, 0xEB, 0x32, 0xF7, 0x84, 0x52, 0x1B, 0x79, 
    0x08, 0xB1, 0x93, 0xE4, 0xD9, 0x18, 0x94, 0x25, 0xD8, 0x84, 0xE0, 0x7E, 0x0D, 0x2B, 0x67, 0x22, 
    0x6A, 0x31, 0x7E, 0x37, 0x9A, 0x7D, 0x05, 0xD5, 0x1C, 0x4A, 0x4B, 0xD9, 0x99, 0xA6, 0x1E, 0xFD, 
    0x90, 0x47, 0x8E, 0x2E, 0x5D, 0xAB, 0x31, 0x32, 0x4C, 0x52, 0x1F, 0xA6, 0x28, 0x06, 0x10, 0x4A, 
    0x42, 0x76, 0x9C, 0xB9, 0xE9, 0x16, 0x50, 0x3E, 0xBD, 0xE6, 0x26, 0x86, 0x68, 0x60, 0x8C, 0xC0, 
    0x30, 0x1A, 0x1A, 0x58, 0x33, 0x4F, 0x91, 0x4F, 0xD3, 0x99, 0x39, 0xB6, 0x0A, 0x74, 0xE9, 0x25, 
    0x63, 0x1A, 0x14, 0x31, 0xE0, 0x2C, 0xD4, 0x06, 0xC6, 0x89, 0x92, 0x0D, 0x98, 0x28, 0xC2, 0x97, 
    0xB3, 0xF8, 0xEC, 0x28, 0xED, 0x99, 0xF9, 0x35, 0x80, 0x96, 0xD5, 0x99, 0xDD, 0xF2, 0x94, 0x4F, 
    0x17, 0x06, 0x17, 0x28, 0xCD, 0xEA, 0x1B, 0x2D, 0x7B, 0x34, 0x5D, 0xBC, 0x89, 0x35, 0x72, 0x74, 
    0x59, 0x8B, 0x2B, 0x2B, 0xFA, 0x74, 0x26, 0xBB, 0xC0, 0x77, 0x97, 0x34, 0xFB, 0xCC, 0x14, 0x5F, 
    0x5E, 0x88, 0xD8, 0xB3, 0x30, 0xC1, 0x10, 0xC2, 0x61, 0xDA, 0xD9, 0x0E, 0xBA, 0x87, 0xF3, 0xC5, 
    0x3E, 0xD3, 0xCB, 0xA4, 0x70, 0x60, 0xE3, 0x45, 0xB8, 0xF8, 0x12, 0xC4, 0x6D, 0x21, 0x76, 0x11, 
    0x12, 0x94, 0x10, 0xCD, 0x30, 0x94, 0x7E, 0x07, 0xCC, 0x0E, 0xA6, 0x0F, 0x1D, 0x5B, 0x75, 0xEF, 
    0xE3, 0x5B, 0x87, 0x29, 0x3B, 0xDB, 0x30, 0xF6, 0x02, 0xD4, 0x8C, 0xA1, 0x58, 0x8C, 0xF7, 0x20, 
    0x2D, 0xD8, 0xF8, 0x3D, 0xEA, 0xD1, 0xBD, 0xB4, 0x0E, 0xCC, 0xAC, 0x7A, 0x98, 0x10, 0x04, 0x17, 
    0x95, 0x48, 0x7C, 0x09, 0x26, 0x9C, 0x46, 0x94, 0x6E, 0x42, 0x9C, 0x43, 0xB2, 0x36, 0x96, 0x3D, 
    0x44, 0xE3, 0x5F, 0xA1, 0x93, 0x3D, 0x46, 0x3E, 0xAB, 0xEE, 0x04, 0x97, 0x92, 0xF6, 0x33, 0x52, 
    0x7E, 0xA2, 0x52, 0xCA, 0x4F, 0xD4, 0x4C, 0x37, 0xC2, 0x3A, 0x87, 0xA8, 0x10, 0x50, 0xD4, 0x08, 
    0x03, 0x91, 0xA3, 0x4B, 0xBE, 0xEA, 0x62, 0x43, 0x9B, 0x2E, 0xC7, 0xA5, 0xBF, 0x8D, 0x8B, 0x47, 
    0x80, 0x94, 0xE0, 0x1D, 0x36, 0x0A, 0x60, 0x32, 0x8A, 0xFC, 0x5C, 0x7C, 0x7E, 0x16, 0x83, 0x5B, 
    0x6E, 0x61, 0xE6, 0xE0, 0x91, 0x25, 0x26, 0x8F, 0x8D, 0x50, 0xE4, 0xEF, 0x24, 0xAA, 0x5D, 0x88, 
    0xB1, 0xAE, 0xAA, 0xA0, 0x05, 0x01, 0x0D, 0xB8, 0xDA, 0xE9, 0x44, 0xE5, 0xF9, 0x14, 0x9D, 0xF3, 
    0x49, 0x86, 0xAE, 0x27, 0x9B, 0x3E, 0xDE, 0x47, 0x3C, 0x54, 0xB0, 0x2E, 0x43, 0xF4, 0x37, 0xA0, 
    0x16, 0xE3, 0x12, 0x0B, 0x06, 0x8C, 0x04, 0xD4, 0xBE, 0x88, 0x2C, 0x7B, 0x21, 0x31, 0x1F, 0x27, 
    0x9F, 0xDD, 0x6B, 0x9E, 0xBB, 0x64, 0x85, 0xCA, 0x6E, 0xAD, 0x8A, 0xC4, 0xB6, 0x4A, 0xD3, 0x17, 
    0x4A, 0x78, 0x52, 0xC5, 0xD8, 0x9F, 0xBA, 0x69, 0xB7, 0x72, 0x18, 0x66, 0xA1, 0x3C, 0x68, 0x04, 
    0x4D, 0x56, 0xCF, 0x60, 0x63, 0xF4, 0x17, 0x88, 0x92, 0x2B, 0x89, 0xEB, 0xE3, 0x18, 0x93, 0xA0, 
    0xE5, 0x3E, 0x7C, 0x79, 0x2F, 0x5E, 0x77, 0x12, 0x54, 0x71, 0x51, 0x82, 0x73, 0xAF, 0xC2, 0xC9, 
    0x65, 0x6C, 0xD8, 0x92, 0x2C, 0x9E, 0x17, 0x47, 0x57, 0x13, 0xC5, 0x17, 0xE3, 0x52, 0x03, 0xC6, 
    0x10, 0xCA, 0x92, 0x50, 0x1C, 0xA3, 0x2C, 0x40, 0xC4, 0x90, 0xD4, 0x1D, 0x69, 0xE3, 0x22, 0xEA, 
    0xB5, 0x6B, 0x96, 0x26, 0xDF, 0xF6, 0x0A, 0x0E, 0xBD, 0x9A, 0x84, 0x91, 0x1A, 0xC1, 0x37, 0x28, 
    0xB2, 0x92, 0x32, 0x03, 0xD4, 0x60, 0xD3, 0x0C, 0x97, 0xBC, 0x14, 0x97, 0x6C, 0x63, 0xEC, 0x4C, 
    0x77, 0x62, 0x8C, 0x16, 0xDB, 0x1B, 0x34, 0x54, 0x6C, 0x76, 0x6B, 0x00, 0x5D, 0x18, 0x2A, 0x26, 
    0x84, 0x4A, 0x83, 0x83, 0x06, 0x3A, 0x5E, 0x7F, 0x6A, 0xA0, 0xAD, 0xA9, 0x52, 0x7A, 0x45, 0xB0, 
    0x52, 0x4D, 0xF2, 0xD4, 0x8A, 0x20, 0x62, 0x47, 0x07, 0x71, 0xC9, 0x6B, 0x31, 0x6E, 0x88, 0x50, 
    0x18, 0x7C, 0xFE, 0x2D, 0xE0, 0x53, 0x04, 0x3D, 0x80, 0x24, 0x11, 0x7E, 0xE6, 0xB5, 0x58, 0xFB, 
    0x36, 0x6C, 0x9C, 0x40, 0xF1, 0x1A, 0xF2, 0x72, 0x07, 0xF0, 0x24, 0x9B, 0x5E, 0x78, 0x0E, 0xA1, 
    0x7D, 0x11, 0x62, 0x0B, 0xCA, 0x0C, 0x8A, 0x7C, 0x2F, 0xC6, 0x7C, 0x96, 0x50, 0xEE, 0xC7, 0xCA, 
    0x38, 0x59, 0xFB, 0x0F, 0xA8, 0x0F, 0x6C, 0xC6, 0xD5, 0x3D, 0x65, 0x76, 0x16, 0xD6, 0x9D, 0x86, 
    0x2F, 0x7F, 0x5C, 0xD9, 0x60, 0x96, 0xCA, 0x0C, 0x79, 0x67, 0x02, 0x9F, 0x7F, 0x12, 0x64, 0x02, 
    0x6B, 0xB7, 0x60, 0xE5, 0x72, 0x48, 0xB6, 0xE2, 0xE2, 0x36, 0xBE, 0x78, 0x15, 0xED, 0x89, 0xFF, 
    0x3C, 0x41, 0xE9, 0x30, 0xD5, 0x88, 0xAA, 0xE0, 0x0B, 0x25, 0x5F, 0x03, 0xC0, 0xEE, 0xB4, 0x12, 
    0xAC, 0x12, 0x02, 0x04, 0x53, 0x4D, 0x4E, 0x9A, 0xFC, 0xF4, 0xDA, 0x61, 0xDC, 0x6A, 0x1D, 0x5F, 
    0xB9, 0x50, 0x46, 0x46, 0x86, 0x41, 0x4F, 0x42, 0xC4, 0x52, 0x76, 0x5A, 0x64, 0xDD, 0x3B, 0x99, 
    0x6B, 0xED, 0x5A, 0xD2, 0xF9, 0x93, 0x1E, 0x42, 0xCC, 0xCB, 0x31, 0xF2, 0x8B, 0x98, 0x68, 0x84, 
    0x54, 0x36, 0x33, 0xCD, 0x93, 0xE4, 0xD3, 0x67, 0x12, 0xA5, 0x4D, 0x7C, 0x08, 0xF8, 0x7C, 0x0A, 
    0x53, 0xDC, 0xCA, 0xE4, 0x91, 0x87, 0x7B, 0x67, 0x3D, 0x85, 0x1B, 0xF9, 0x33, 0x34, 0x7C, 0x98, 
    0x28, 0x1D, 0x26, 0xCF, 0x5A, 0x68, 0xF3, 0xE0, 0x72, 0x13, 0x85, 0x38, 0x25, 0x9F, 0x69, 0x60, 
    0xEB, 0x7F, 0xCC, 0xE4, 0xFE, 0x27, 0xAA, 0x49, 0xAF, 0xED, 0xA6, 0x39, 0x9C, 0x22, 0x5C, 0x85, 
    0xB5, 0x23, 0x88, 0x19, 0x65, 0x60, 0x38, 0x76, 0x27, 0xBE, 0x84, 0x59, 0x52, 0x06, 0x67, 0xD7, 
    0x58, 0xBA, 0x1B, 0x61, 0xB6, 0x27, 0xDE, 0xA2, 0xA0, 0x08, 0xA1, 0x78, 0x7E, 0x7C, 0xAD, 0x4A, 
    0x13, 0x91, 0x14, 0x54, 0xB1, 0x69, 0x49, 0xB3, 0xB1, 0x89, 0x81, 0x4D, 0x17, 0x50, 0x12, 0x51, 
    0xB4, 0x0B, 0xC4, 0xD7, 0x31, 0x26, 0xC6, 0x1A, 0x45, 0x42, 0x8C, 0xCA, 0x60, 0xEF, 0xBC, 0x06, 
    0x18, 0x87, 0x35, 0xE0, 0xCD, 0x14, 0x24, 0xFB, 0xFB, 0x03, 0xFC, 0xE4, 0x8F, 0x28, 0x06, 0x3E, 
    0x8E, 0xF8, 0x73, 0x31, 0x6E, 0x07, 0xA1, 0x55, 0x2C, 0x12, 0x4E, 0x43, 0x85, 0x47, 0x50, 0xC8, 
    0x5A, 0x4B, 0x56, 0xD7, 0x77, 0x4A, 0x7C, 0x7D, 0x3F, 0x3E, 0xEA, 0x60, 0xE2, 0x21, 0x4A, 0x19, 
    0xA2, 0x3D, 0x67, 0xDD, 0x89, 0xC5, 0xAE, 0x05, 0x94, 0xAD, 0xA0, 0x36, 0xA6, 0xE8, 0xAE, 0xEE, 
    0x79, 0xFA, 0x06, 0xC3, 0x63, 0x33, 0xAE, 0x72, 0x2F, 0xBD, 0xFE, 0x91, 0x5D, 0x63, 0x1C, 0x7D, 
    0x8E, 0x81, 0xF2, 0x04, 0xD4, 0x47, 0x34, 0x82, 0x20, 0xA8, 0x2D, 0x31, 0x76, 0x10, 0xD5, 0xB7, 
    0x51, 0xE6, 0x4A, 0x10, 0x30, 0x46, 0x21, 0x18, 0xAC, 0x18, 0x8C, 0x06, 0xCA, 0x1C, 0x7C, 0xE6, 
    0x17, 0x9C, 0x3B, 0x22, 0x0E, 0x8C, 0xC7, 0x9A, 0x40, 0x2D, 0x09, 0x4C, 0xAE, 0x18, 0xBB, 0x3D, 
    0xBB, 0x93, 0xF6, 0xEC, 0xCE, 0xD5, 0x16, 0x75, 0x41, 0xA2, 0xC5, 0x20, 0x36, 0xE9, 0x97, 0x32, 
    0x4C, 0xCF, 0xC2, 0x56, 0x95, 0x4F, 0x22, 0x71, 0x6B, 0x3E, 0xD3, 0xAA, 0x10, 0x19, 0x04, 0x2C, 
    0x40, 0x4C, 0x67, 0x6E, 0x2B, 0x7B, 0x0F, 0x0F, 0x01, 0xFD, 0x76, 0x67, 0xFF, 0xC4, 0x28, 0x65, 
    0xF4, 0x12, 0x62, 0x0B, 0xA6, 0xCA, 0x94, 0xB1, 0xA6, 0x7F, 0x91, 0x1B, 0x0F, 0x12, 0x9E, 0xA3, 
    0x9C, 0xE8, 0x32, 0x5F, 0xD8, 0x8B, 0xB7, 0x2B, 0xD3, 0x25, 0x63, 0x02, 0x01, 0x41, 0x55, 0xF0, 
    0x45, 0x06, 0x7E, 0x16, 0x95, 0x0C, 0x54, 0x70, 0x4E, 0x20, 0x06, 0xD5, 0x92, 0xAC, 0x70, 0xE0, 
    0xF7, 0xA1, 0xD9, 0xAE, 0xBE, 0x27, 0xD5, 0x00, 0xA1, 0x14, 0xBA, 0x6B, 0xCC, 0xFE, 0xC0, 0x19, 
    0x35, 0x9C, 0x19, 0xA0, 0x3E, 0x7D, 0x9C, 0x03, 0xC7, 0xCA, 0x55, 0xB7, 0x85, 0x06, 0xB4, 0x58, 
    0x7B, 0x6B, 0xA4, 0x2A, 0x24, 0x5B, 0x50, 0x56, 0x4B, 0x47, 0x40, 0x57, 0x25, 0x2C, 0x8A, 0x62, 
    0x15, 0xC4, 0x82, 0x49, 0x2E, 0x24, 0x1D, 0x7D, 0x33, 0xEF, 0xFA, 0xC4, 0xCD, 0x7C, 0xE2, 0x5D, 
    0x95, 0x27, 0x7D, 0xFB, 0xC7, 0x4F, 0xA2, 0xA3, 0xDB, 0x71, 0xB5, 0x6D, 0xC4, 0xB5, 0xAA, 0xF0, 
    0x63, 0x6B, 0x5B, 0x18, 0x7A, 0xC1, 0x46, 0x60, 0xC9, 0xB7, 0xBA, 0x08, 0xAC, 0x17, 0x8C, 0x3F, 
    0x71, 0x56, 0x97, 0x19, 0x58, 0xA7, 0x55, 0xAD, 0xA3, 0xD2, 0x24, 0x6A, 0x2B, 0x80, 0xB6, 0x3E, 
    0x23, 0x68, 0x89, 0x8A, 0x45, 0x8B, 0x2E, 0x9A, 0x7F, 0x16, 0xDC, 0x1E, 0xF0, 0xAE, 0x5A, 0x44, 
    0x91, 0x52, 0x04, 0x8F, 0x2F, 0x20, 0x36, 0x73, 0x1C, 0x9F, 0x9C, 0x58, 0x28, 0x40, 0xA0, 0xDA, 
    0x2B, 0x3F, 0x48, 0x1D, 0x9F, 0x35, 0xFB, 0xAE, 0x1D, 0x0D, 0xD4, 0x31, 0xC5, 0x1B, 0x10, 0x7B, 
    0x06, 0xF3, 0xFC, 0x0F, 0x70, 0xCF, 0xAA, 0x95, 0x29, 0x56, 0x58, 0xC1, 0x27, 0xC4, 0x81, 0x73, 
    0x06, 0x0D, 0x8A, 0x0D, 0x06, 0x59, 0x9E, 0xB0, 0xC8, 0xB2, 0x59, 0x5A, 0x39, 0x3F, 0x1A, 0x02, 
    0x14, 0x95, 0xAD, 0x49, 0x06, 0x46, 0x28, 0xDD, 0x7B, 0x98, 0x4D, 0xCF, 0xE1, 0x75, 0x37, 0x7C, 
    0x83, 0x66, 0x12, 0x43, 0xB2, 0x0D, 0x69, 0xFC, 0x2A, 0x69, 0xD3, 0x21, 0x5A, 0xF5, 0xD3, 0xFA, 
    0x05, 0x74, 0xCB, 0xF7, 0xF2, 0xCB, 0x7F, 0xFE, 0x0D, 0x5C, 0xE7, 0x21, 0xEE, 0xFE, 0xE8, 0x2E, 
    0x08, 0x82, 0x59, 0xA4, 0xE7, 0x89, 0xB5, 0xD9, 0x0E, 0x0C, 0xD4, 0x21, 0xEA, 0xF1, 0x43, 0x01, 
    0xEF, 0x57, 0x38, 0x9E, 0xB9, 0x59, 0x34, 0x9A, 0xC7, 0x35, 0x04, 0x35, 0x83, 0x74, 0xCA, 0xD3, 
    0x98, 0x3D, 0x70, 0x3F, 0x2C, 0xF3, 0xDA, 0x03, 0x63, 0xA7, 0x82, 0x0E, 0xD2, 0xE5, 0xF0, 0x32, 
    0x37, 0xB5, 0x87, 0xD2, 0x77, 0x89, 0x4D, 0x8C, 0x91, 0x61, 0x0A, 0xBF, 0x8D, 0xFA, 0x58, 0x8B, 
    0x76, 0x6B, 0x12, 0x93, 0x0C, 0x10, 0xA5, 0x6F, 0x20, 0x72, 0x57, 0xE0, 0x6C, 0x8D, 0xAE, 0xD9, 
    0x04, 0xD1, 0xF7, 0xA1, 0x38, 0x84, 0x4A, 0xB5, 0x9B, 0xA4, 0xBD, 0x24, 0xCD, 0x38, 0xD6, 0xCC, 
    0x1C, 0x83, 0x56, 0x3E, 0x42, 0x44, 0x5C, 0xAF, 0xEE, 0xB0, 0xF4, 0x10, 0x3D, 0xB2, 0x2F, 0x81, 
    0xAC, 0xC2, 0xEF, 0xDF, 0x61, 0x50, 0x03, 0x12, 0xAA, 0x7D, 0x44, 0xE2, 0x0D, 0x48, 0xFA, 0x66, 
    0xB6, 0x34, 0x5E, 0x8F, 0xA8, 0xC1, 0xB9, 0x21, 0x88, 0x1D, 0xEA, 0x7A, 0x0C, 0x04, 0x22, 0x1A, 
    0x48, 0xF3, 0x4D, 0x6C, 0x7E, 0xE9, 0x15, 0x14, 0x73, 0xDF, 0xE6, 0xCA, 0x8F, 0x6C, 0xE7, 0xD4, 
    0x17, 0x3D, 0xC9, 0xF7, 0x76, 0xEA, 0x9A, 0x3E, 0xFC, 0xE9, 0x5A, 0x92, 0xF6, 0xA6, 0x45, 0x2A, 
    0x8B, 0xA7, 0x0A, 0x21, 0x5B, 0x99, 0x4D, 0x1E, 0xC3, 0x0D, 0xFF, 0x2F, 0x92, 0x9C, 0x47, 0xAD, 
    0x16, 0xE1, 0xC2, 0x65, 0xD8, 0xCD, 0x67, 0x51, 0x94, 0xFB, 0xB0, 0xB1, 0x41, 0x74, 0x03, 0xD6, 
    0x6E, 0x40, 0xA9, 0x51, 0x66, 0x5F, 0xA4, 0x3E, 0x78, 0x17, 0xED, 0x99, 0x02, 0xEC, 0xF7, 0xF1, 
    0xD9, 0x8F, 0xF0, 0xF6, 0xE7, 0x70, 0x69, 0x81, 0x98, 0x6D, 0xD8, 0xF2, 0x1C, 0xDC, 0xA6, 0x16, 
    0x84, 0x21, 0x6C, 0xB2, 0x95, 0x38, 0x75, 0x88, 0x7A, 0x44, 0x62, 0x88, 0x67, 0xA1, 0xA8, 0x4A, 
    0x11, 0x5A, 0x01, 0x88, 0xAA, 0x41, 0x56, 0x68, 0xAD, 0xEA, 0x82, 0xFB, 0x33, 0x80, 0xAF, 0x8C, 
    0xF5, 0xB3, 0x25, 0x74, 0x22, 0x8A, 0xB7, 0x95, 0x9C, 0x05, 0x81, 0xAC, 0x03, 0xE5, 0x0C, 0x38, 
    0x8D, 0xA9, 0xA5, 0x1B, 0x48, 0x93, 0x31, 0x6C, 0xEA, 0xF0, 0x39, 0x74, 0x0E, 0x1D, 0xC7, 0xCC, 
    0xDE, 0x45, 0xD9, 0x3E, 0x4C, 0xA9, 0x10, 0xC5, 0x29, 0xCD, 0xE6, 0x30, 0x49, 0xE3, 0x12, 0x8C, 
    0x7B, 0x09, 0xD7, 0x9C, 0xAD, 0x48, 0xA4, 0x18, 0xAB, 0xCF, 0x5A, 0xB7, 0x59, 0x94, 0x8E, 0x00, 
    0x8A, 0xA9, 0x56, 0x9B, 0xF6, 0x92, 0x81, 0x46, 0xFF, 0x8A, 0x98, 0x2F, 0x4A, 0x34, 0x7C, 0x99, 
    0x32, 0x7F, 0x94, 0xA2, 0x13, 0xE3, 0xA2, 0x06, 0xB5, 0xC1, 0x97, 0xD2, 0x18, 0xFA, 0x15, 0x6A, 
    0xB5, 0xD7, 0x91, 0xD4, 0x5F, 0x46, 0x5C, 0x3F, 0x0D, 0x1B, 0x9F, 0x06, 0x72, 0x26, 0x4A, 0x04, 
    0x40, 0xEB, 0xC7, 0x6D, 0xC4, 0xDE, 0x48, 0xC8, 0x8E, 0xA0, 0xBE, 0x89, 0x4B, 0x1B, 0xC4, 0xF5, 
    0xB3, 0x49, 0x9A, 0x17, 0x51, 0x1B, 0x3A, 0x97, 0xB8, 0x56, 0xA3, 0xC8, 0x20, 0x9B, 0x53, 0x9C, 
    0xFB, 0x77, 0x98, 0x9F, 0xEB, 0xAD, 0x70, 0x83, 0xA8, 0x05, 0x91, 0x2A, 0x28, 0xC6, 0xFD, 0x38, 
    0x8A, 0xA9, 0xFE, 0xA8, 0x08, 0x48, 0x40, 0x44, 0xCD, 0xE2, 0x03, 0x4B, 0x58, 0x8A, 0xA4, 0x66, 
    0xC5, 0xCA, 0x96, 0xAE, 0x22, 0x25, 0x28, 0x05, 0x49, 0xF1, 0x00, 0xE5, 0xCC, 0xE7, 0x98, 0x6A, 
    0xED, 0x67, 0xAA, 0x35, 0xCF, 0xE4, 0xD4, 0x1C, 0xD3, 0x47, 0x0F, 0xE2, 0x8F, 0xFE, 0x2B, 0x9D, 
    0x89, 0xCB, 0xC9, 0xEE, 0xB9, 0x9C, 0xE2, 0xE0, 0x5B, 0x68, 0x4F, 0xDC, 0x47, 0xEB, 0xC8, 0x34, 
    0x93, 0x07, 0x0A, 0xCA, 0x89, 0xBB, 0x49, 0x93, 0xFB, 0x78, 0xFF, 0x0E, 0x41, 0xAD, 0x10, 0xA4, 
    0x62, 0xA6, 0x4A, 0xFF, 0x76, 0xD8, 0x5A, 0xED, 0xBC, 0xDF, 0x9C, 0x43, 0xCC, 0xE3, 0x94, 0x45, 
    0x44, 0x59, 0x46, 0x88, 0x39, 0xC0, 0xF4, 0x91, 0xD5, 0xBB, 0x3F, 0x33, 0xAD, 0x83, 0x48, 0xF9, 
    0x09, 0xDA, 0xED, 0xFB, 0x99, 0x69, 0x17, 0xE4, 0xDE, 0x22, 0x26, 0xC5, 0x48, 0x8A, 0x68, 0x44, 
    0x51, 0x1A, 0x42, 0xB9, 0x0F, 0xEB, 0x1E, 0x80, 0x74, 0x49, 0xE1, 0xC7, 0x47, 0xF7, 0x22, 0xE1, 
    0xAF, 0x28, 0xE6, 0x1E, 0xC7, 0x77, 0x23, 0x54, 0x63, 0x20, 0x25, 0x32, 0x35, 0xD4, 0x47, 0x74, 
    0xB3, 0x63, 0x84, 0xF2, 0x63, 0x1C, 0xDB, 0x7F, 0xCF, 0x32, 0x6F, 0x3F, 0x45, 0x60, 0x9A, 0x50, 
    0xC4, 0x04, 0xDF, 0x21, 0xD4, 0x0E, 0xAC, 0x28, 0x3E, 0x4F, 0x13, 0x7C, 0x17, 0x9F, 0x0F, 0x10, 
    0xF4, 0x29, 0xD4, 0x74, 0x1D, 0xB9, 0x40, 0x24, 0x82, 0x04, 0x08, 0x95, 0xA0, 0x90, 0xAC, 0xF0, 
    0x38, 0x26, 0xAA, 0x00, 0x11, 0x1F, 0x30, 0x7C, 0x8B, 0x53, 0xFC, 0x07, 0x79, 0xF0, 0x50, 0xC2, 
    0x46, 0x7B, 0x0A, 0x03, 0xA3, 0x05, 0x4F, 0x3C, 0x39, 0x41, 0x78, 0xB8, 0xC5, 0xD7, 0xEF, 0x59, 
    0x88, 0xCA, 0xF7, 0xF0, 0x4B, 0xEF, 0xF8, 0x06, 0x9B, 0x5E, 0x74, 0x12, 0xE3, 0x71, 0x4A, 0x38, 
    0xF4, 0x14, 0x37, 0x7F, 0x74, 0x9E, 0x0F, 0x7C, 0x51, 0x30, 0xBD, 0x7A, 0xB4, 0xF4, 0x28, 0xAA, 
    0xEE, 0x99, 0x69, 0xFD, 0xDD, 0x4F, 0x2B, 0xE3, 0x27, 0x7F, 0x9E, 0xF6, 0x71, 0x10, 0x6B, 0x31, 
    0xF5, 0x3B, 0x9F, 0xB6, 0xEF, 0xE4, 0xE1, 0x27, 0x49, 0x87, 0x3E, 0x82, 0xCE, 0xBF, 0x12, 0x89, 
    0x5E, 0x41, 0xBD, 0x3E, 0x04, 0x21, 0x50, 0x94, 0x73, 0x78, 0xBF, 0x0B, 0x93, 0x7E, 0x9B, 0xB9, 
    0x83, 0x13, 0xFD, 0x75, 0xF4, 0x47, 0x03, 0xF0, 0x08, 0x63, 0x63, 0xD7, 0x51, 0x74, 0x5E, 0x45, 
    0xE8, 0xBE, 0x18, 0x91, 0x04, 0xAF, 0x9E, 0xBC, 0x7C, 0x0C, 0x86, 0xEE, 0xE3, 0xF8, 0x9E, 0xFE, 
    0x02, 0xFD, 0xB6, 0x8B, 0x77, 0xF3, 0x95, 0x7B, 0x3E, 0x4D, 0xC6, 0xAB, 0x49, 0xD2, 0x2F, 0x31, 
    0xB5, 0xA7, 0xD3, 0x77, 0x7C, 0xCB, 0xCC, 0x1E, 0x7E, 0x9C, 0xDE, 0x4A, 0x19, 0x5E, 0x43, 0xE0, 
    0xEB, 0xA8, 0x9F, 0x76, 0xC4, 0x4D, 0x4F, 0xC9, 0x2C, 0x86, 0x04, 0x29, 0x53, 0xFC, 0xF4, 0x46, 
    0x5A, 0x33, 0xAE, 0xAF, 0xA4, 0xE8, 0xBD, 0x2E, 0xA6, 0xBF, 0x85, 0xC4, 0x0C, 0x8C, 0x08, 0x5F, 
    0x7D, 0x4B, 0x0B, 0x78, 0xFA, 0x2D, 0x9C, 0x87, 0x6E, 0xEA, 0x00, 0x7B, 0xFB, 0x57, 0x46, 0xCB, 
    0x50, 0xD4, 0x4F, 0xC6, 0xBA, 0x26, 0x12, 0xC0, 0xE4, 0x5D, 0x4C, 0x3B, 0x7F, 0x56, 0xF9, 0x98, 
    0xD8, 0x3F, 0x0B, 0x7C, 0xA6, 0xFA, 0xF1, 0x2C, 0xBB, 0x56, 0xDD, 0xE9, 0x02, 0x78, 0x08, 0x78, 
    0x88, 0xEE, 0x73, 0xD8, 0xE1, 0x6A, 0xB5, 0x26, 0x81, 0x3B, 0x7B, 0x9F, 0xE5, 0x17, 0x5F, 0xDD, 
    0xF7, 0x73, 0xB7, 0x2B, 0xF0, 0x20, 0xF0, 0x20, 0x6B, 0xED, 0xAC, 0x3E, 0xDA, 0x56, 0x68, 0x7F, 
    0x13, 0xF8, 0xE6, 0x12, 0xC9, 0x31, 0xB3, 0xE0, 0x1F, 0xAE, 0x02, 0x94, 0x69, 0x12, 0x6A, 0x97, 
    0x90, 0xBE, 0xF8, 0x3C, 0xAE, 0xFF, 0xD3, 0x25, 0xDD, 0x31, 0x21, 0x54, 0xBA, 0x63, 0xC0, 0x19, 
    0x65, 0xB3, 0x2D, 0x9F, 0x73, 0xF6, 0x76, 0xC3, 0x27, 0x0D, 0x87, 0xDA, 0xE7, 0x32, 0x5F, 0xBC, 
    0x11, 0xD3, 0x68, 0xA0, 0x06, 0x8A, 0xB0, 0x97, 0xD9, 0xE9, 0xC3, 0xEB, 0xE3, 0x75, 0x03, 0xD3, 
    0x9D, 0xA7, 0x6C, 0x7F, 0x9E, 0x60, 0x2E, 0x23, 0x1D, 0x83, 0x78, 0xF0, 0x62, 0xB4, 0xFC, 0x10, 
    0xF7, 0x0F, 0xDD, 0xC1, 0x5B, 0x6F, 0xDF, 0x87, 0xE2, 0x91, 0x05, 0x06, 0xAA, 0x41, 0xCB, 0x93, 
    0xF9, 0xDA, 0xCC, 0xAB, 0xB9, 0xEA, 0x96, 0x36, 0xCE, 0x3A, 0x7C, 0x00, 0x23, 0x82, 0x1A, 0x30, 
    0x25, 0x18, 0xB3, 0x94, 0x58, 0x68, 0x01, 0x51, 0x13, 0xCA, 0xC8, 0xF0, 0xE0, 0xF1, 0x93, 0x48, 
    0x06, 0xAF, 0x22, 0x6D, 0x5E, 0x8A, 0xB1, 0x50, 0x74, 0x72, 0x7C, 0xF1, 0x65, 0xE2, 0x23, 0x3F, 
    0x5A, 0x0F, 0x40, 0x57, 0x91, 0xE8, 0xEA, 0x1B, 0x4E, 0x41, 0x9A, 0xFF, 0x4C, 0x32, 0x72, 0x19, 
    0x71, 0x02, 0x45, 0x50, 0xB2, 0xFC, 0x28, 0xB1, 0x9F, 0x25, 0x98, 0x80, 0xAB, 0x6D, 0xC5, 0xB9, 
    0x08, 0x55, 0x25, 0x74, 0xA7, 0x29, 0xCA, 0x6A, 0xD7, 0x41, 0x44, 0x08, 0xAA, 0x55, 0xC9, 0x70, 
    0x21, 0xF1, 0x17, 0x59, 0x04, 0x7A, 0xA1, 0xDA, 0x87, 0x35, 0x14, 0x79, 0x83, 0xB8, 0xB6, 0x99, 
    0xA8, 0x0E, 0xF9, 0x1C, 0xE4, 0x53, 0xF7, 0x62, 0xDA, 0xEF, 0xE4, 0xD6, 0xED, 0xBB, 0xD7, 0x0F, 
    0xD0, 0x37, 0xFF, 0x40, 0x78, 0xF8, 0x91, 0x8B, 0xE8, 0x16, 0x7F, 0x89, 0x34, 0x2E, 0x21, 0x1E, 
    0xAA, 0xD2, 0x68, 0x2D, 0x41, 0x7B, 0xD6, 0x4E, 0xA4, 0x7F, 0xB7, 0x05, 0xD3, 0x2B, 0xA3, 0xF9, 
    0x15, 0x1B, 0x02, 0xBA, 0xAC, 0x9C, 0xB1, 0xF0, 0x3B, 0x54, 0x59, 0x25, 0xA6, 0x7A, 0x1D, 0xCC, 
    0x4F, 0xFD, 0x37, 0xB6, 0xF3, 0x3E, 0x6E, 0xD9, 0xFE, 0x35, 0xD6, 0x49, 0x5B, 0xF2, 0x56, 0xD7, 
    0xDF, 0x6B, 0xF9, 0xCE, 0x9E, 0xB3, 0x09, 0xF1, 0xEF, 0x12, 0xD5, 0xDF, 0x48, 0x11, 0x4E, 0x25, 
    0x8E, 0x63, 0xAC, 0xAB, 0x5E, 0x74, 0x44, 0xFA, 0xB7, 0xB4, 0x16, 0x7D, 0x70, 0xE8, 0x1F, 0x4A, 
    0x42, 0x7F, 0x1F, 0xB5, 0xD5, 0x84, 0xF9, 0xB2, 0x00, 0x7F, 0x80, 0xD0, 0xB9, 0x4D, 0xC2, 0xCC, 
    0x67, 0x18, 0x9A, 0xDC, 0xA5, 0x37, 0x7C, 0xC8, 0xAF, 0x3F, 0xA0, 0x01, 0xAE, 0xB9, 0x51, 0x98, 
    0x99, 0x8B, 0x18, 0x3D, 0x6B, 0x8C, 0xB9, 0x89, 0x33, 0x18, 0x6F, 0x9C, 0x44, 0x2D, 0x8D, 0x29, 
    0xBA, 0x1E, 0x4C, 0x95, 0x3F, 0x06, 0x96, 0xE4, 0x61, 0x39, 0x9B, 0x17, 0x37, 0x5D, 0x97, 0x31, 
    0xDA, 0x58, 0x70, 0xC6, 0x92, 0xCD, 0xE7, 0xCC, 0xCD, 0x1E, 0xC5, 0xD4, 0xF7, 0x32, 0x35, 0x3F, 
    0xC1, 0x1D, 0x7F, 0x98, 0xB1, 0xCE, 0xDA, 0xFF, 0x01, 0x4E, 0x8C, 0x3E, 0xB2, 0x2D, 0xF2, 0xDA, 
    0x8B, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};
#endif


static void ulip_cgi_scan_cleanup(void *arg)
{
    ESP_LOGD("CGI", "Scan cleanup");

    if (scan_html) {
        free(scan_html);
        scan_html = NULL;
    }
}

 
static void ulip_cgi_scan_callback(uint16_t *list_size,  wifi_ap_record_t * list)
{
    ESP_LOGI("CGI", "Scan callback %d", *list_size);
    const char *auth;
    int size = 0;
    if (scan_html)
        free(scan_html);
    scan_html = (char *)calloc(1,1280);
    if (!scan_html) return;
    int max_size = *list_size;
    for (int i = 0; i < max_size; i++)
    {
        // ESP_LOGI("CGI", "wifi ssid %s", list[i].ssid);
        switch (list[i].authmode)
        {
            case WIFI_AUTH_OPEN:
                auth = "Aberta";
                break;
            case WIFI_AUTH_WEP:
                auth = "WEP";
                break;
            case WIFI_AUTH_WPA_PSK:
                auth = "WPA";
                break;
            case WIFI_AUTH_WPA2_PSK:
                auth = "WPA2";
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                auth = "WPA/WPA2";
                break;
            default:
                auth = " - ";
                break;
        }
        uint8_t channel;
        esp_wifi_get_channel(&channel, NULL);
        if (!strcmp((char *)list[i].ssid, CFG_get_wifi_ssid()))
            size += snprintf(scan_html + size, 1280 - size,
                                "<tr style=\\\"background-color:#d2d2d2;\\\"><td>%s</td><td>%s</td><td>%d</td><td>%d</td></tr>",
                                list[i].ssid, auth, channel, list[i].rssi);
        else if (i < 15)
            size += snprintf(scan_html + size, 1280 - size,
                                "<tr><td>%s</td><td>%s</td><td>%d</td><td>%d</td></tr>",
                                list[i].ssid, auth, list[i].primary, list[i].rssi);

    }
    // while (bss != NULL) {
    //     switch (bss->authmode) {
    //         case WIFI_AUTH_OPEN:
    //             auth = "Aberta";
    //             break;
    //         case WIFI_AUTH_WEP:
    //             auth = "WEP";
    //             break;
    //         case WIFI_AUTH_WPA_PSK:
    //             auth = "WPA";
    //             break;
    //         case WIFI_AUTH_WPA2_PSK:
    //             auth = "WPA2";
    //             break;
    //         case WIFI_AUTH_WPA_WPA2_PSK:
    //             auth = "WPA/WPA2";
    //             break;
    //         default:
    //             auth = "-";
    //             break;
    //     }
    //     uint8_t channel;
    //     esp_wifi_get_channel(&channel, NULL);
    //     wifi_sta_list_t list;
    //     esp_wifi_ap_get_sta_list(&list);
    //     if (!strcmp((char *)bss->ssid, CFG_get_wifi_ssid()))

    //         size += snprintf(scan_html + size, 1280 - size,
    //                             "<tr style=\\\"background-color:#d2d2d2;\\\"><td>%s</td><td>%s</td><td>%d</td><td>%d</td></tr>",
    //                             bss->ssid, auth, channel, list.sta[0].rssi);
    //     else if (i < 15)
    //         size += snprintf(scan_html + size, 1280 - size,
    //                             "<tr><td>%s</td><td>%s</td><td>%d</td><td>%d</td></tr>",
    //                             bss->ssid, auth, bss->primary, bss->rssi);
    //     bss = STAILQ_NEXT(bss, next);
    //     i++;
    // }
    // timer_disarm(&scan_timer);
    // timer_setfn(&scan_timer, (timer_func_t *)ulip_cgi_scan_cleanup, NULL);
    // timer_arm(&scan_timer, 30000, false);
    esp_timer_stop(scan_timer);
    esp_timer_create_args_t timer_args = {
        .callback = &ulip_cgi_scan_cleanup,
        .arg = NULL,
        .name = "scan_cleanup"
    };
    esp_timer_create(&timer_args, &scan_timer);
    esp_timer_start_once(scan_timer, 30000*1000);
}

 
static bool ulip_cgi_cache_lookup(const char *url, uint32_t ip,
                                  uint32_t *etag)
{
    cgi_cache_t *c;
    int rc = false;
    time_t now;
    int d;
    int i;

    if (!url) return false;

    // now = system_get_time();
    time(&now);
    for (i = 0; i < CGI_CACHE_SIZE; i++) {
        c = &cache[i];
        if (*c->url != '\0') {
            d = ((now - c->timestamp) / 1000000);
            if (!strcmp(url, c->url) && ip == c->ip) {
                if (d < 3600) {
                    if (etag)
                        *etag = c->etag;
                    rc = true;
                }
            }
            if (d > 3600)
                memset(c, 0, sizeof(cgi_cache_t));
        }
    }

    return rc;
}

 
static int ulip_cgi_cache_insert(const char *url, uint32_t ip, uint32_t *etag)
{
    cgi_cache_t *c;
    int i;

    if (!url) return -1;

    for (i = 0; i < CGI_CACHE_SIZE; i++) {
        c = &cache[i];
        if (*c->url == '\0') {
            strcpy(c->url, url);
            c->ip = ip;
            c->etag = random();
            c->timestamp = time(NULL);
            if (etag)
                *etag = c->etag;
            return 0;
        }
    }

    return -1;
}

 
static int ulip_cgi_send_data(HttpdInstance *pInstance, HttpdConnData *connData,
                              const uint8_t *data, int size)
{
    int len;

    while (size > 0) {
        len = size;
        if (len > (HTTPD_MAX_SENDBUFF_LEN >> 1))
            len = HTTPD_MAX_SENDBUFF_LEN >> 1;
        if (!httpdSend(connData, (const char *)data, len)) {
            httpdFlushSendBuffer(pInstance, connData);
            continue;
        }
        size -= len;
        data += len;
    }

    return HTTPD_CGI_DONE;
}

static int testadress = 0x40090000;
void changeTestAdress(int value)
{
    testadress = value;
    ESP_LOGI("CGI", "Testadress changed to %x", testadress);
}

static int ulip_cgi_send_data_from_flash(HttpdInstance *pInstance, HttpdConnData *connData,
                                         const uint8_t *data, int size)
{
    uint32_t limit = 0;
    char buf[1024];
    uint32_t addr;
    uint32_t off;
    int len;

    addr = (uint32_t)data - testadress;
    off = (uint32_t)connData->cgiData & 0x0fffffff;



    /* Check offset */
    if (off) {
        size -= (off - addr);
        addr = off;
    }
    // ESP_LOGI("CGI", "off %x addr %x size %x", off, addr, size);
    while (size > 0 && limit < (HTTPD_MAX_SENDBUFF_LEN >> 1)) {
        len = size;
        if (len > (HTTPD_MAX_SENDBUFF_LEN >> 1))
            len = HTTPD_MAX_SENDBUFF_LEN >> 1;
        // ESP_LOGI("CGI", "CGI flash data size [%p] [0x%x] [0x%x] [%d]",
        //     connData, addr, off, size);
        // ets_intr_lock();
        spi_flash_read(addr, (uint32 *)buf, len);
        // snprintf(buf, HTTPD_MAX_SENDBUFF_LEN, "TEST1%s", buf);
        // ESP_LOGE("addr", "addr [0x%x] data [%p]", addr, data);
        // ESP_LOG_BUFFER_CHAR("CGI", buf, len);
        // ets_intr_unlock();
        if (!httpdSend(connData, buf, len)) {
            httpdFlushSendBuffer(pInstance, connData);
            continue;
        }
        limit += len;
        size -= len;
        addr += len;
    }

    connData->cgiData = (void *)((uint32_t)connData->cgiData & 0xf0000000);
    if (size) {
        connData->cgiData = (void *)((uint32_t)connData->cgiData | addr);
        return HTTPD_CGI_MORE;
    }

    return HTTPD_CGI_DONE;
}


static void ulip_cgi_response(HttpdConnData *connData, int status,
                              uint32_t etag, const char *content_type,
                              int content_length)
{
    uint32_t now = time(NULL);
    struct tm *tm;
    char header[128];
    char date[64];
    char slen[32];

    if (content_length != -1)
        httpdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
    httpdStartResponse(connData, status);
    /* CORS */
    if (httpdGetHeader(connData, "Origin", header, sizeof(header))) {
        httpdHeader(connData, "Access-Control-Allow-Origin", "*");
        httpdHeader(connData, "Access-Control-Allow-Credentials", "true");
        if (httpdGetHeader(connData, "Access-Control-Request-Method", header, sizeof(header)))
            httpdHeader(connData, "Access-Control-Allow-Methods", "GET, POST");
        if (httpdGetHeader(connData, "Access-Control-Request-Headers", header, sizeof(header)))
            httpdHeader(connData, "Access-Control-Allow-Headers", header);
    }
    if (!content_type) {
        httpdHeader(connData, "Content-Type", "text/html");
    } else {
        /* Cache control */
        tm = localtime(&now);
        sprintf(date, "%s, %02d %s %d %02d:%02d:%02d GMT",
                   rtc_weekday(tm), tm->tm_mday, rtc_month(tm),
                   tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec);
        httpdHeader(connData, "Date", date);
        now += 31536000;
        tm = localtime(&now);
        sprintf(date, "%s, %02d %s %d %02d:%02d:%02d GMT",
                   rtc_weekday(tm), tm->tm_mday, rtc_month(tm),
                   tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec);
        httpdHeader(connData, "Expires", date);
        if (etag) {
            sprintf(header, "\"%u\"", etag);
            httpdHeader(connData, "ETag", header);
        }
        /* Content type */
        httpdHeader(connData, "Content-Type", content_type);
    }
    if (content_length != -1) {
        sprintf(slen, "%d", content_length);
        httpdHeader(connData, "Content-Length", slen);
    }
    httpdEndHeaders(connData);
}


static void ulip_cgi_include_script(HttpdInstance *pInstance,  HttpdConnData *connData,
                                    int menuopt, int subopt)
{
    char buf[512];
    
    if (subopt != -1) {
        sprintf(buf, "<script type=\"text/javascript\"" \
                   "src=\"/js/init.js?menuopt=%d&subopt=%d&loadtok=%lu\"></script>",
                   menuopt, subopt, random());
    } else {
        sprintf(buf, "<script type=\"text/javascript\"" \
                   "src=\"/js/init.js?menuopt=%d&loadtok=%lu\"></script>",
                   menuopt, random());
    }
    ulip_cgi_send_data(pInstance,connData, (uint8_t *) buf, strlen(buf));
}


static void ulip_cgi_init_js(HttpdInstance *pInstance, HttpdConnData *connData, int menuopt, int subopt)
{
    wifi_scan_config_t scanConf; 
    account_t *acc;
    char buf[32];
    char *js;
    int size;
    struct tm *tm;
    uint32_t time_value;
    uint8_t mode;
    uint8_t level;
    const char *host;
    uint16_t port;
    char date[64];
    uint8_t mac[6];
    char fingerprint[700];
    acc_permission_t *perm;
    const char *period;
    int day;
    int hour;
    int min;
    int sec;
    char *p;
    char *t;
    char *l;
    int w1;
    int w2;
    int h1;
    int m1;
    int h2;
    int m2;
    int y1;
    int t1;
    int d1;
    int y2;
    int t2;
    int d2;
    int i;

    js = (char *)malloc(2048);
    if (!js) return;

    size = sprintf(js, "%s",
                      "function load_cfg() {\n");
    if (menuopt == MENU_NETWORK) {
        size += sprintf(js + size, "document.NETWORK.hotspot.checked=%s;\n",
                           CFG_get_hotspot() ? "true" : "false");
        size += sprintf(js + size, "document.NETWORK.apmode.checked=%s;\n",
                           CFG_get_ap_mode() ? "true" : "false");
        size += sprintf(js + size, "document.NETWORK.ssid.value=\"%s\";\n",
                           CFG_get_wifi_ssid() ? CFG_get_wifi_ssid() : "");
        size += sprintf(js + size, "document.NETWORK.passwd.value=\"%s\";\n",
                           CFG_get_wifi_passwd() ? CFG_get_wifi_passwd() : "");
        size += sprintf(js + size, "document.NETWORK.channel.value=\"%d\";\n",
                           CFG_get_wifi_channel());
        size += sprintf(js + size, "document.NETWORK.beacon.value=\"%d\";\n",
                           CFG_get_wifi_beacon_interval());
        size += sprintf(js + size, "document.NETWORK.hidessid.checked=%s;\n",
                           CFG_get_ssid_hidden() ? "true" : "false");
        size += sprintf(js + size, "document.NETWORK.wifi_enable.checked=%s;\n",
                           CFG_get_wifi_disable() ? "false" : "true");
        size += sprintf(js + size, "document.NETWORK.dhcp.checked=%s;\n",
                           CFG_get_dhcp() ? "true" : "false");
        size += sprintf(js + size, "document.NETWORK.ip.value=\"%s\";\n",
                           CFG_get_ip_address() ? CFG_get_ip_address() : "");
        size += sprintf(js + size, "document.NETWORK.netmask.value=\"%s\";\n",
                           CFG_get_netmask() ? CFG_get_netmask() : "");
        size += sprintf(js + size, "document.NETWORK.gateway.value=\"%s\";\n",
                           CFG_get_gateway() ? CFG_get_gateway() : "");
        size += sprintf(js + size, "document.NETWORK.eth_enable.checked=%s;\n",
                           CFG_get_eth_enable()? "true" : "false");
        size += sprintf(js + size, "document.NETWORK.eth_dhcp.checked=%s;\n",
                           CFG_get_eth_dhcp() ? "true" : "false");
        size += sprintf(js + size, "document.NETWORK.eth_ip.value=\"%s\";\n",
                           CFG_get_eth_ip_address() ? CFG_get_eth_ip_address() : "");
        size += sprintf(js + size, "document.NETWORK.eth_netmask.value=\"%s\";\n",
                           CFG_get_eth_netmask() ? CFG_get_eth_netmask() : "");
        size += sprintf(js + size, "document.NETWORK.eth_gateway.value=\"%s\";\n",
                           CFG_get_eth_gateway() ? CFG_get_eth_gateway() : "");
        size += sprintf(js + size, "document.NETWORK.dns.value=\"%s\";\n",
                           CFG_get_dns() ? CFG_get_dns() : "");
        size += sprintf(js + size, "document.NETWORK.ntp.value=\"%s\";\n",
                           CFG_get_ntp() ? CFG_get_ntp() : "");
        size += sprintf(js + size, "document.NETWORK.hostname.value=\"%s\";\n",
                           CFG_get_hostname() ? CFG_get_hostname() : "");
        size += sprintf(js + size, "document.NETWORK.ddns.value=\"%d\";\n",
                           CFG_get_ddns());
        size += sprintf(js + size, "document.NETWORK.ddns_domain.value=\"%s\";\n",
                           CFG_get_ddns_domain() ? CFG_get_ddns_domain() : "");
        size += sprintf(js + size, "document.NETWORK.ddns_user.value=\"%s\";\n",
                           CFG_get_ddns_user() ? CFG_get_ddns_user() : "");
        size += sprintf(js + size, "document.NETWORK.ddns_passwd.value=\"%s\";\n",
                           CFG_get_ddns_passwd() ? CFG_get_ddns_passwd() : "");
        size += sprintf(js + size, "%s", "document.getElementById('net').style.backgroundColor='#0075be';\n");
    } else if (menuopt == MENU_CONTROL) {
        size += sprintf(js + size, "document.CONTROL.desc.value=\"%s\";\n",
                           CFG_get_control_description() ? CFG_get_control_description() : "");
#if !defined(CONFIG__MLI_1WRP_TYPE__)
        size += sprintf(js + size, "document.CONTROL.standalone.checked=%s;\n",
                           CFG_get_standalone() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.mode.value=\"%d\";\n",
                           CFG_get_control_mode());
        size += sprintf(js + size, "document.CONTROL.timeout.value=\"%d\";\n",
                           CFG_get_control_timeout());
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRC_TYPE__) && !defined(CONFIG__MLI_1WLC_TYPE__)
        size += sprintf(js + size, "document.CONTROL.acc_timeout.value=\"%d\";\n",
                           CFG_get_control_acc_timeout());
#endif
        size += sprintf(js + size, "document.CONTROL.external.checked=%s;\n",
                           CFG_get_control_external() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.external_url.value=\"%s\";\n",
                           CFG_get_control_url() ? CFG_get_control_url() : "");
        size += sprintf(js + size, "document.CONTROL.breakin.checked=%s;\n",
                           CFG_get_breakin_alarm() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.button_enable.checked=%s;\n",
                           CFG_get_button_enable() ? "true" : "false");
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRC_TYPE__) && !defined(CONFIG__MLI_1WLC_TYPE__)
        size += sprintf(js + size, "document.CONTROL.doublepass_timeout.value=\"%d\";\n",
                           CFG_get_control_doublepass_timeout());
#endif
#endif
#if defined(CONFIG__MLI_1WRP_TYPE__)
        size += sprintf(js + size, "document.CONTROL.relay_status.value=\"%d\";\n",
                           CFG_get_relay_status());
        size += sprintf(js + size, "document.CONTROL.button_enable.checked=%s;\n",
                           CFG_get_button_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.timeout.value=\"%d\";\n",
                           CFG_get_control_timeout());
#endif
#if !defined(CONFIG__MLI_1WF_TYPE__) && !defined(CONFIG__MLI_1WQF_TYPE__) && !defined(CONFIG__MLI_1WRF_TYPE__) && \
    !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && !defined(CONFIG__MLI_1WRG_TYPE__) && \
    !defined(CONFIG__MLI_1WLG_TYPE__) && !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
        size += sprintf(js + size, "document.CONTROL.rfid.checked=%s;\n",
                           CFG_get_rfid_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.rfid_timo.value=\"%d\";\n",
                           CFG_get_rfid_timeout());
        size += sprintf(js + size, "document.CONTROL.rfid_retries.value=\"%d\";\n",
                           CFG_get_rfid_retries());
        size += sprintf(js + size, "document.CONTROL.rfid_nfc.checked=%s;\n",
                           CFG_get_rfid_nfc() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.rfid_panic_timo.value=\"%d\";\n",
                           CFG_get_rfid_panic_timeout());
        size += sprintf(js + size, "document.CONTROL.rfid_format.value=\"%d\";\n",
                           CFG_get_rfid_format());
#endif
#if defined(CONFIG__MLI_1WQ_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRQ_TYPE__)
        size += sprintf(js + size, "document.CONTROL.qrcode.checked=%s;\n",
                           CFG_get_qrcode_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.qrcode_timo.value=\"%d\";\n",
                           CFG_get_qrcode_timeout());
        size += sprintf(js + size, "document.CONTROL.qrcode_panic_timo.value=\"%d\";\n",
                           CFG_get_qrcode_panic_timeout());
        size += sprintf(js + size, "document.CONTROL.qrcode_config.checked=%s;\n",
                           CFG_get_qrcode_config() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.qrcode_led.checked=%s;\n",
                           CFG_get_qrcode_led() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.qrcode_dynamic.checked=%s;\n",
                           CFG_get_qrcode_dynamic() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.qrcode_validity.value=\"%d\";\n",
                           CFG_get_qrcode_validity());
#endif
#if defined(CONFIG__MLI_1WF_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRF_TYPE__)
        size += sprintf(js + size, "document.CONTROL.rf433.checked=%s;\n",
                           CFG_get_rf433_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.rf433_rc.checked=%s;\n",
                           CFG_get_rf433_rc() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.rf433_hc.value=\"%d\";\n",
                           CFG_get_rf433_hc());
        size += sprintf(js + size, "document.CONTROL.rf433_alarm.checked=%s;\n",
                           CFG_get_rf433_alarm() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.rf433_bc.checked=%s;\n",
                           CFG_get_rf433_bc() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.rf433_bp.value=\"%d\";\n",
                           CFG_get_rf433_bp());
        size += sprintf(js + size, "document.CONTROL.rf433_panic_timo.value=\"%d\";\n",
                           CFG_get_rf433_panic_timeout());
        size += sprintf(js + size, "document.CONTROL.rf433_ba.value=\"%d\";\n",
                           CFG_get_rf433_ba());
#endif
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
        size += sprintf(js + size, "document.CONTROL.fpm.checked=%s;\n",
                           CFG_get_fingerprint_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.fpm_timo.value=\"%d\";\n",
                           CFG_get_fingerprint_timeout());
        size += sprintf(js + size, "document.CONTROL.fpm_sec.value=\"%d\";\n",
                           CFG_get_fingerprint_security());
        size += sprintf(js + size, "document.CONTROL.fpm_id.value=\"%d\";\n",
                           CFG_get_fingerprint_identify_retries());
#endif
#if defined(CONFIG__MLI_1WRQ_TYPE__) || defined(CONFIG__MLI_1WR_TYPE__) || defined(CONFIG__MLI_1WRF_TYPE__) || \
    defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WRP_TYPE__) || defined(CONFIG__MLI_1WRC_TYPE__)
        size += sprintf(js + size, "document.CONTROL.rs485.checked=%s;\n",
                           CFG_get_rs485_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.rs485_addr.value=\"%d\";\n",
                           CFG_get_rs485_hwaddr());
        size += sprintf(js + size, "document.CONTROL.rs485_server_addr.value=\"%d\";\n",
                           CFG_get_rs485_server_hwaddr());
#endif
#if defined(CONFIG__MLI_1WLS_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
        size += sprintf(js + size, "document.CONTROL.lora.checked=%s;\n",
                           CFG_get_lora_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.lora_channel.value=\"%d\";\n",
                           CFG_get_lora_channel());
        size += sprintf(js + size, "document.CONTROL.lora_baudrate.value=\"%d\";\n",
                           CFG_get_lora_baudrate());
        size += sprintf(js + size, "document.CONTROL.lora_addr.value=\"%d\";\n",
                           CFG_get_lora_address());
        size += sprintf(js + size, "document.CONTROL.lora_server_addr.value=\"%d\";\n",
                           CFG_get_lora_server_address());
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || defined(CONFIG__MLI_1WRC_TYPE__) || \
    defined(CONFIG__MLI_1WLC_TYPE__)
        size += sprintf(js + size, "document.CONTROL.dht_enable.checked=%s;\n",
                           CFG_get_dht_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.dht_timo.value=\"%d\";\n",
                           CFG_get_dht_timeout() / 1000);
        size += sprintf(js + size, "document.CONTROL.dht_temp_upper.value=\"%d\";\n",
                           CFG_get_dht_temp_upper());
        size += sprintf(js + size, "document.CONTROL.dht_temp_lower.value=\"%d\";\n",
                           CFG_get_dht_temp_lower());
        size += sprintf(js + size, "document.CONTROL.dht_rh_upper.value=\"%d\";\n",
                           CFG_get_dht_rh_upper());
        size += sprintf(js + size, "document.CONTROL.dht_rh_lower.value=\"%d\";\n",
                           CFG_get_dht_rh_lower());
        size += sprintf(js + size, "document.CONTROL.dht_relay.checked=%s;\n",
                           CFG_get_dht_relay() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.dht_alarm.checked=%s;\n",
                           CFG_get_dht_alarm() ? "true" : "false");
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__)
        size += sprintf(js + size, "document.CONTROL.temt_enable.checked=%s;\n",
                           CFG_get_temt_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.temt_timo.value=\"%d\";\n",
                           CFG_get_temt_timeout() / 1000);
        size += sprintf(js + size, "document.CONTROL.temt_upper.value=\"%d\";\n",
                           CFG_get_temt_upper());
        size += sprintf(js + size, "document.CONTROL.temt_lower.value=\"%d\";\n",
                           CFG_get_temt_lower());
        size += sprintf(js + size, "document.CONTROL.temt_relay.checked=%s;\n",
                           CFG_get_temt_relay() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.temt_alarm.checked=%s;\n",
                           CFG_get_temt_alarm() ? "true" : "false");
#endif
#if defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WLG_TYPE__)
        size += sprintf(js + size, "document.CONTROL.mq2_enable.checked=%s;\n",
                           CFG_get_mq2_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.mq2_timo.value=\"%d\";\n",
                           CFG_get_mq2_timeout() / 1000);
        size += sprintf(js + size, "document.CONTROL.mq2_limit.value=\"%d\";\n",
                           CFG_get_mq2_limit());
        size += sprintf(js + size, "document.CONTROL.mq2_relay.checked=%s;\n",
                           CFG_get_mq2_relay() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.mq2_alarm.checked=%s;\n",
                           CFG_get_mq2_alarm() ? "true" : "false");
#endif
#if defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
        size += sprintf(js + size, "document.CONTROL.cli_enable.checked=%s;\n",
                           CFG_get_cli_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.cli_timo.value=\"%d\";\n",
                           CFG_get_cli_timeout() / 1000);
        size += sprintf(js + size, "document.CONTROL.cli_range.value=\"%d\";\n",
                           CFG_get_cli_range());
        size += sprintf(js + size, "document.CONTROL.cli_upper.value=\"%d\";\n",
                           CFG_get_cli_upper());
        size += sprintf(js + size, "document.CONTROL.cli_lower.value=\"%d\";\n",
                           CFG_get_cli_lower());
        size += sprintf(js + size, "document.CONTROL.cli_relay.checked=%s;\n",
                           CFG_get_cli_relay() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.cli_alarm.checked=%s;\n",
                           CFG_get_cli_alarm() ? "true" : "false");
        if (!CFG_get_cli_cal()) {
            size += sprintf(js + size, "%s", "document.CONTROL.cli_cal.value='Calibrar';\n");
            size += sprintf(js + size, "%s", "document.CONTROL.cli_reset.disabled=true;\n");
        } else {
            size += sprintf(js + size, "%s", "document.CONTROL.cli_cal.value='Recalibrar';\n");
            size += sprintf(js + size, "%s", "document.CONTROL.cli_reset.disabled=false;\n");
        }
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || \
    defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WLG_TYPE__) || \
    defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
        size += sprintf(js + size, "document.CONTROL.pir_enable.checked=%s;\n",
                           CFG_get_pir_enable() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.pir_timo.value=\"%d\";\n",
                           CFG_get_pir_timeout() / 1000);
        size += sprintf(js + size, "document.CONTROL.pir_chime.checked=%s;\n",
                           CFG_get_pir_chime() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.pir_relay.checked=%s;\n",
                           CFG_get_pir_relay() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.pir_alarm.checked=%s;\n",
                           CFG_get_pir_alarm() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.sensor_type.value=\"%d\";\n",
                           CFG_get_sensor_type());
        size += sprintf(js + size, "document.CONTROL.sensor_flow.value=\"%d\";\n",
                           CFG_get_sensor_flow());
        size += sprintf(js + size, "document.CONTROL.sensor_limit.value=\"%d\";\n",
                           CFG_get_sensor_limit());
        size += sprintf(js + size, "document.CONTROL.sensor_relay.checked=%s;\n",
                           CFG_get_sensor_relay() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.sensor_alarm.checked=%s;\n",
                           CFG_get_sensor_alarm() ? "true" : "false");
#endif
#if defined(CONFIG__MLI_1WRP_TYPE__)
        size += sprintf(js + size, "document.CONTROL.pow_vol_cal.value=\"%d\";\n",
                           CFG_get_pow_voltage_cal());
        size += sprintf(js + size, "document.CONTROL.pow_vol_upper.value=\"%d\";\n",
                           CFG_get_pow_voltage_upper());
        size += sprintf(js + size, "document.CONTROL.pow_vol_lower.value=\"%d\";\n",
                           CFG_get_pow_voltage_lower());
        size += sprintf(js + size, "document.CONTROL.pow_cur_cal.value=\"%d\";\n",
                           CFG_get_pow_current_cal());
        size += sprintf(js + size, "document.CONTROL.pow_cur_upper.value=\"%d\";\n",
                           CFG_get_pow_current_upper());
        size += sprintf(js + size, "document.CONTROL.pow_cur_lower.value=\"%d\";\n",
                           CFG_get_pow_current_lower());
        size += sprintf(js + size, "document.CONTROL.pow_pwr_upper.value=\"%d\";\n",
                           CFG_get_pow_power_upper());
        size += sprintf(js + size, "document.CONTROL.pow_pwr_lower.value=\"%d\";\n",
                           CFG_get_pow_power_lower());
        size += sprintf(js + size, "document.CONTROL.pow_relay.checked=%s;\n",
                           CFG_get_pow_relay() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.pow_alarm_time.value=\"%d\";\n",
                           CFG_get_pow_alarm_time());
        size += sprintf(js + size, "document.CONTROL.pow_relay_timo.value=\"%d\";\n",
                           CFG_get_pow_relay_timeout());
        size += sprintf(js + size, "document.CONTROL.pow_relay_ext.checked=%s;\n",
                           CFG_get_pow_relay_ext() ? "true" : "false");
        size += sprintf(js + size, "document.CONTROL.pow_interval.value=\"%d\";\n",
                           CFG_get_pow_interval());
        size += sprintf(js + size, "document.CONTROL.pow_day.value=\"%d\";\n",
                           CFG_get_pow_day());
        size += sprintf(js + size, "document.CONTROL.daily_limit.value=\"%d\";\n",
                           CFG_get_energy_daily_limit());
        size += sprintf(js + size, "document.CONTROL.monthly_limit.value=\"%d\";\n",
                           CFG_get_energy_monthly_limit());
        size += sprintf(js + size, "document.CONTROL.total_limit.value=\"%d\";\n",
                           CFG_get_energy_total_limit());
        size += sprintf(js + size, "document.CONTROL.pow_nrg_cal.value=\"%d\";\n",
                           CFG_get_pow_energy_cal());
#endif
        size += sprintf(js + size, "%s", "document.getElementById('acc').style.backgroundColor='#0075be';\n");
    } else if (menuopt == MENU_HTTP) {
        size += sprintf(js + size, "document.HTTP.server.value=\"%s\";\n",
                           CFG_get_server_ip() ? CFG_get_server_ip() : "");
        size += sprintf(js + size, "document.HTTP.port.value=\"%d\";\n",
                           CFG_get_server_port());
        size += sprintf(js + size, "document.HTTP.user.value=\"%s\";\n",
                           CFG_get_server_user() ? CFG_get_server_user() : "");
        size += sprintf(js + size, "document.HTTP.pass.value=\"%s\";\n",
                           CFG_get_server_passwd() ? CFG_get_server_passwd() : "");
        size += sprintf(js + size, "document.HTTP.url.value=\"%s\";\n",
                           CFG_get_server_url() ? CFG_get_server_url() : "");
        size += sprintf(js + size, "document.HTTP.retries.value=\"%d\";\n",
                           CFG_get_server_retries());
        size += sprintf(js + size, "document.HTTP.auth.checked=%s;\n",
                           CFG_get_user_auth() ? "true" : "false");
        size += sprintf(js + size, "%s", "document.getElementById('http').style.backgroundColor='#0075be';\n");
    } else if (menuopt == MENU_USER) {
        if (subopt == -1) subopt = account_db_get_first();
        acc = account_db_get_index(subopt);
        if (acc) {
            size += sprintf(js + size, "document.USER.level.value=\"%d\";\n",
                               account_get_level(acc));
            size += sprintf(js + size, "document.USER.name.value=\"%s\";\n",
                               account_get_name(acc) ? account_get_name(acc) : "");
            size += sprintf(js + size, "document.USER.user.value=\"%s\";\n",
                               account_get_user(acc) ? account_get_user(acc) : "");
            size += sprintf(js + size, "document.USER.pass.value=\"%s\";\n",
                               account_get_password(acc) ? account_get_password(acc) : "");
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
            size += sprintf(js + size, "document.USER.card.value=\"%s\";\n",
                               account_get_card(acc) ? account_get_card(acc) : "");
#endif
#if defined(CONFIG__MLI_1WQ_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRQ_TYPE__)
            size += sprintf(js + size, "document.USER.code.value=\"%s\";\n",
                               account_get_code(acc) ? account_get_code(acc) : "");
            if (qrcode_get_dynamic()) {
                size += sprintf(js + size, "document.USER.code.readOnly=true;\n");
                size += sprintf(js + size, "document.USER.code.style.backgroundColor='#dddddd';\n");
            }
#endif
#if defined(CONFIG__MLI_1WF_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRF_TYPE__)
            size += sprintf(js + size, "document.USER.rfcode.value=\"%s\";\n",
                               account_get_rfcode(acc) ? account_get_rfcode(acc) : "");
#endif
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
            if (account_get_fingerprint(acc)) {
                // base64Encode(ACCOUNT_FINGERPRINT_SIZE, account_get_fingerprint(acc),
                //              sizeof(fingerprint), fingerprint);
                size_t olen;
                mbedtls_base64_encode((unsigned char *)fingerprint, sizeof(fingerprint), &olen,
                                      account_get_fingerprint(acc), ACCOUNT_FINGERPRINT_SIZE);
                size += sprintf(js + size, "document.USER.fingerprint.value=\"%s\";\n",
                                   fingerprint);
            } else {
                size += sprintf(js + size, "document.USER.fingerprint.value=\"\";\n");
            }
            ESP_LOGI("CGI", "fingerprint: %s of %s", account_get_finger(acc) ? account_get_finger(acc) : "", account_get_name(acc) ? account_get_name(acc) : "");
            size += sprintf(js + size, "document.USER.finger.value=\"%s\";\n",
                               account_get_finger(acc) ? account_get_finger(acc) : "");
#endif
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
            size += sprintf(js + size, "document.USER.lifecount.value=\"%d\";\n",
                               account_get_lifecount(acc));
            size += sprintf(js + size, "document.USER.accessibility.checked=%s;\n",
                               account_get_accessibility(acc) ? "true" : "false");
            size += sprintf(js + size, "document.USER.panic.checked=%s;\n",
                               account_get_panic(acc) ? "true" : "false");
            size += sprintf(js + size, "document.USER.visitor.checked=%s;\n",
                               account_get_visitor(acc) ? "true" : "false");
            size += sprintf(js + size, "document.USER.key.value=\"%s\";\n",
                               account_get_key(acc) ? account_get_key(acc) : "");
            perm = account_get_permission(acc);
            if (perm) {
                for (i = 0; i < 2; i++) {
                    if (*perm[i] == '\0') break;
                    strcpy(date, perm[i]);
                    if (!strchr(date, '/')) {
                        /* Week */
                        p = strtok_r(date, " ", &t);
                        p = strtok_r(p, "-", &l);
                        w1 = strtol(p, NULL, 10);
                        p = strtok_r(NULL, "-", &l);
                        if (!p) continue;
                        w2 = strtol(p, NULL, 10);
                        /* Hour */
                        p = strtok_r(NULL, " ", &t);
                        if (!p) continue;
                        p = strtok_r(p, "-", &t);
                        p = strtok_r(p, ":", &l);
                        h1 = strtol(p, NULL, 10);
                        p = strtok_r(NULL, ":", &l);
                        if (!p) continue;
                        m1 = strtol(p, NULL, 10);
                        p = strtok_r(NULL, "-", &t);
                        p = strtok_r(p, ":", &l);
                        h2 = strtol(p, NULL, 10);
                        p = strtok_r(NULL, ":", &l);
                        if (!p) continue;
                        m2 = strtol(p, NULL, 10);
                        size += sprintf(js + size, "document.USER.w%di.value=\"%d\";\n",
                                           i + 1, w1);
                        size += sprintf(js + size, "document.USER.w%de.value=\"%d\";\n",
                                           i + 1, w2);
                        size += sprintf(js + size, "document.USER.h%di.value=\"%d\";\n",
                                           i + 1, h1);
                        size += sprintf(js + size, "document.USER.m%di.value=\"%d\";\n",
                                           i + 1, m1);
                        size += sprintf(js + size, "document.USER.h%de.value=\"%d\";\n",
                                           i + 1, h2);
                        size += sprintf(js + size, "document.USER.m%de.value=\"%d\";\n",
                                           i + 1, m2);
                    } else {
                        p = strtok_r(date, " ", &t);
                        /* 
                         * Date range 
                         */
                        p = strtok_r(p, "-", &l);
                        p = strtok(p, "/");
                        y1 = strtol(p, NULL, 10);
                        p = strtok(NULL, "/");
                        if (!p) continue;
                        t1 = strtol(p, NULL, 10);
                        p = strtok(NULL, "/");
                        if (!p) continue;
                        d1 = strtol(p, NULL, 10);
                        p = strtok_r(NULL, "-", &l);
                        if (!p) continue;
                        p = strtok(p, "/");
                        y2 = strtol(p, NULL, 10);
                        p = strtok(NULL, "/");
                        if (!p) continue;
                        t2 = strtol(p, NULL, 10);
                        p = strtok(NULL, "/");
                        if (!p) continue;
                        d2 = strtol(p, NULL, 10);
                        /* 
                         * Hour range
                         */
                        p = strtok_r(NULL, " ", &t);
                        if (!p) continue;
                        p = strtok_r(p, "-", &t);
                        p = strtok_r(p, ":", &l);
                        h1 = strtol(p, NULL, 10);
                        p = strtok_r(NULL, ":", &l);
                        if (!p) continue;
                        m1 = strtol(p, NULL, 10);
                        p = strtok_r(NULL, "-", &t);
                        if (!p) continue;
                        p = strtok_r(p, ":", &l);
                        h2 = strtol(p, NULL, 10);
                        p = strtok_r(NULL, ":", &l);
                        if (!p) continue;
                        m2 = strtol(p, NULL, 10);
                        size += sprintf(js + size, "document.USER.d%di.value=\"%d\";\n",
                                           i + 1, d1);
                        size += sprintf(js + size, "document.USER.t%di.value=\"%d\";\n",
                                           i + 1, t1);
                        size += sprintf(js + size, "document.USER.y%di.value=\"%d\";\n",
                                           i + 1, y1);
                        size += sprintf(js + size, "document.USER.d%de.value=\"%d\";\n",
                                           i + 1, d2);
                        size += sprintf(js + size, "document.USER.t%de.value=\"%d\";\n",
                                           i + 1, t2);
                        size += sprintf(js + size, "document.USER.y%de.value=\"%d\";\n",
                                           i + 1, y2);
                        size += sprintf(js + size, "document.USER.h%di.value=\"%d\";\n",
                                           i + 1, h1);
                        size += sprintf(js + size, "document.USER.m%di.value=\"%d\";\n",
                                           i + 1, m1);
                        size += sprintf(js + size, "document.USER.h%de.value=\"%d\";\n",
                                           i + 1, h2);
                        size += sprintf(js + size, "document.USER.m%de.value=\"%d\";\n",
                                           i + 1, m2);
                    }
                }
            }
#endif
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
            size += sprintf(js + size, "%s", "document.USER.user_Finger.style.display='inline';\n");
            if (ulip_core_capture_finger_status()) {
                if (fpm_get_enroll() == subopt) {
                    size += sprintf(js + size, "%s", "document.USER.user_Finger.style.backgroundColor='#dc3545';\n");
                    size += sprintf(js + size, "%s", "document.USER.user_Finger.style.borderColor='#dc3545';\n");
                }
            }
            size += sprintf(js + size, "%s", "document.USER.user_Update.style.display='inline';\n");
#endif
            if (subopt == account_db_get_first())
                size += sprintf(js + size, "%s", "document.USER.user_Prev.disabled=true;\n");
            if (subopt == account_db_get_last())
                size += sprintf(js + size, "%s", "document.USER.user_Next.disabled=true;\n");
            account_destroy(acc);
        }
        size += sprintf(js + size, "document.USER.subopt.value=\"%d\";\n", subopt);
        size += sprintf(js + size, "document.getElementById('total').innerHTML='<BR><b>Total de Usu&aacute;rios: %d</b>';\n",
                           account_db_get_size());
        if (ulip_core_probe_status()) {
            size += sprintf(js + size, "%s", "document.USER.user_Det.style.backgroundColor='#dc3545';\n");
            size += sprintf(js + size, "%s", "document.USER.user_Det.style.borderColor='#dc3545';\n");
        }
        if (ulip_core_erase_status()) {
            size += sprintf(js + size, "%s", "document.USER.user_Erase.style.backgroundColor='#dc3545';\n");
            size += sprintf(js + size, "%s", "document.USER.user_Erase.style.borderColor='#dc3545';\n");
        }
        size += sprintf(js + size, "%s", "document.getElementById('user').style.backgroundColor='#0075be';\n");
    } else if (menuopt == MENU_LOG) {
        size += sprintf(js + size, "%s", "document.getElementById('log').style.backgroundColor='#0075be';\n");
    } else if (menuopt == MENU_STATUS) {
#if !defined(CONFIG__MLI_1WRP_TYPE__)
        if (ctl_alarm_status() == CTL_ALARM_ON) {
            size += sprintf(js + size, "%s", "document.STATUS.alarm.value=\"Habilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.alarm.classList.add('danger');\n");
        } else {
            size += sprintf(js + size, "%s", "document.STATUS.alarm.value=\"Desabilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.alarm.classList.add('success');\n");
        }
        if (ctl_panic_status() == CTL_PANIC_ON) {
            size += sprintf(js + size, "%s", "document.STATUS.panic.value=\"Habilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.panic.classList.add('danger');\n");
        } else {
            size += sprintf(js + size, "%s", "document.STATUS.panic.value=\"Desabilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.panic.classList.add('success');\n");
        }
#endif
        if (ctl_relay_status() == CTL_RELAY_ON) {
            size += sprintf(js + size, "%s", "document.STATUS.relay.value=\"Habilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.relay.classList.add('danger');\n");
        } else {
            size += sprintf(js + size, "%s", "document.STATUS.relay.value=\"Desabilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.relay.classList.add('success');\n");
        }
        if (ctl_sensor_status() == CTL_SENSOR_ON) {
            size += sprintf(js + size, "%s", "document.STATUS.sensor.value=\"Habilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.sensor.classList.add('danger');\n");
        } else {
            size += sprintf(js + size, "%s", "document.STATUS.sensor.value=\"Desabilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.sensor.classList.add('success');\n");
        }
#if defined(CONFIG__MLI_1WRP_TYPE__)
        if (ctl_relay_ext_status() == CTL_RELAY_ON) {
            size += sprintf(js + size, "%s", "document.STATUS.relayaux.value=\"Habilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.relayaux.classList.add('danger');\n");
        } else {
            size += sprintf(js + size, "%s", "document.STATUS.relayaux.value=\"Desabilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.relayaux.classList.add('success');\n");
        }
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || \
    defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
        size += sprintf(js + size, "document.STATUS.temp.value=\"%s \\272C\";\n",
                           dht_get_str_temperature());
        if (dht_get_temperature_alarm())
            size += sprintf(js + size, "%s", "document.STATUS.temp.classList.add('danger');\n");
        else
            size += sprintf(js + size, "%s", "document.STATUS.temp.classList.add('success');\n");
        size += sprintf(js + size, "document.STATUS.rh.value=\"%s%%\";\n", dht_get_str_humidity());
        if (dht_get_humidity_alarm())
            size += sprintf(js + size, "%s", "document.STATUS.rh.classList.add('danger');\n");
        else
            size += sprintf(js + size, "%s", "document.STATUS.rh.classList.add('success');\n");
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__)
        size += sprintf(js + size, "document.STATUS.lux.value=\"%d lx\";\n", temt_get_lux());
        if (temt_get_alarm())
            size += sprintf(js + size, "%s", "document.STATUS.lux.classList.add('danger');\n");
        else
            size += sprintf(js + size, "%s", "document.STATUS.lux.classList.add('success');\n");
#endif
#if defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WLG_TYPE__)
        size += sprintf(js + size, "document.STATUS.gas.value=\"%d\";\n", mq2_get_gas());
        if (mq2_get_alarm())
            size += sprintf(js + size, "%s", "document.STATUS.gas.classList.add('danger');\n");
        else
            size += sprintf(js + size, "%s", "document.STATUS.gas.classList.add('success');\n");
#endif
#if defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
        size += sprintf(js + size, "document.STATUS.loop.value=\"%d\";\n", cli_get_value());
        if (cli_get_alarm())
            size += sprintf(js + size, "%s", "document.STATUS.loop.classList.add('danger');\n");
        else
            size += sprintf(js + size, "%s", "document.STATUS.loop.classList.add('success');\n");
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || \
    defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WRG_TYPE__) || \
    defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
        if (pir_get_status()) {
            size += sprintf(js + size, "%s", "document.STATUS.pir.value=\"Alarme\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.pir.classList.add('danger');\n");
        } else {
            size += sprintf(js + size, "%s", "document.STATUS.pir.value=\"Normal\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.pir.classList.add('success');\n");
        }
        if (CFG_get_sensor_type() == CFG_SENSOR_NORMAL) {
            size += sprintf(js + size, "%s", "document.STATUS.level.value=\"Desabilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.level.classList.add('default');\n");
            size += sprintf(js + size, "%s", "document.STATUS.volume.value=\"Desabilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.volume.classList.add('default');\n");
        } else if (CFG_get_sensor_type() == CFG_SENSOR_LEVEL) {
            if (ctl_sensor_status() == CTL_SENSOR_ON) {
                size += sprintf(js + size, "%s", "document.STATUS.level.value=\"Alarme\";\n");
                size += sprintf(js + size, "%s", "document.STATUS.level.classList.add('danger');\n");
            } else {
                size += sprintf(js + size, "%s", "document.STATUS.level.value=\"Normal\";\n");
                size += sprintf(js + size, "%s", "document.STATUS.level.classList.add('success');\n");
            }
            size += sprintf(js + size, "%s", "document.STATUS.volume.value=\"Desabilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.volume.classList.add('default');\n");
        } else {
            bool volumealarm = false;
            if (CFG_get_sensor_limit()) {
                if (CFG_get_sensor_volume() >= CFG_get_sensor_limit())
                    volumealarm = true;
            }
            size += sprintf(js + size, "document.STATUS.volume.value=\"%s L\";\n",
                               CFG_get_sensor_str_volume());
            size += sprintf(js + size, "document.STATUS.volume.classList.add('%s');\n",
                               volumealarm ? "danger" : "success");
            size += sprintf(js + size, "%s", "document.STATUS.level.value=\"Desabilitado\";\n");
            size += sprintf(js + size, "%s", "document.STATUS.level.classList.add('default');\n");
        }
#endif
#if defined(CONFIG__MLI_1WRP_TYPE__)
        size += sprintf(js + size, "document.STATUS.vol.value=\"%s V\";\n",
                           pow_get_voltage_str());
        if (pow_get_voltage_alarm())
            size += sprintf(js + size, "%s", "document.STATUS.vol.classList.add('danger');\n");
        else
            size += sprintf(js + size, "%s", "document.STATUS.vol.classList.add('success');\n");
        size += sprintf(js + size, "document.STATUS.cur.value=\"%s A\";\n",
                           pow_get_current_str());
        if (pow_get_current_alarm())
            size += sprintf(js + size, "%s", "document.STATUS.cur.classList.add('danger');\n");
        else
            size += sprintf(js + size, "%s", "document.STATUS.cur.classList.add('success');\n");
        size += sprintf(js + size, "document.STATUS.act_power.value=\"%s W\";\n",
                           pow_get_active_power_str());
        if (pow_get_power_alarm())
            size += sprintf(js + size, "%s", "document.STATUS.act_power.classList.add('danger');\n");
        else
            size += sprintf(js + size, "%s", "document.STATUS.act_power.classList.add('success');\n");

        size += sprintf(js + size, "document.STATUS.app_power.value=\"%s VA\";\n",
                           pow_get_apparent_power_str());
        if (pow_get_power_alarm())
            size += sprintf(js + size, "%s", "document.STATUS.app_power.classList.add('danger');\n");
        else
            size += sprintf(js + size, "%s", "document.STATUS.app_power.classList.add('success');\n");
        size += sprintf(js + size, "document.STATUS.pf.value=\"%s\";\n",
                           pow_get_power_factor_str());
        size += sprintf(js + size, "document.STATUS.eng_daily.value=\"%s kWh\";\n",
                           CFG_get_energy_daily_str());
        size += sprintf(js + size, "document.STATUS.eng_daily_last.value=\"%s kWh\";\n",
                           CFG_get_energy_daily_last_str());
        size += sprintf(js + size, "document.STATUS.eng_monthly.value=\"%s kWh\";\n",
                           CFG_get_energy_monthly_str());
        size += sprintf(js + size, "document.STATUS.eng_monthly_last.value=\"%s kWh\";\n",
                           CFG_get_energy_monthly_last_str());
        size += sprintf(js + size, "document.STATUS.eng_total.value=\"%s kWh\";\n",
                           CFG_get_energy_total_str());
        if (pow_get_energy_alarm()) {
            size += sprintf(js + size, "%s", "document.STATUS.eng_daily.classList.add('danger');\n");
            size += sprintf(js + size, "%s", "document.STATUS.eng_monthly.classList.add('danger');\n");
            size += sprintf(js + size, "%s", "document.STATUS.eng_total.classList.add('danger');\n");
        } else {
            size += sprintf(js + size, "%s", "document.STATUS.eng_daily.classList.add('success');\n");
            size += sprintf(js + size, "%s", "document.STATUS.eng_monthly.classList.add('success');\n");
            size += sprintf(js + size, "%s", "document.STATUS.eng_total.classList.add('success');\n");
        }
        for (i = 0; i < 12; i++) {
            size += sprintf(js + size, "document.STATUS.eng_mon%d.value=\"%s\";\n",
                               i, CFG_get_energy_month_str(i));
        }
#endif
        size += sprintf(js + size, "%s", "document.getElementById('stat').style.backgroundColor='#0075be';\n");
    } else if (menuopt == MENU_ADMIN) {
        switch (subopt) {
            default:
            case MENU_ADMIN_TAB_UPDATE:
                size += sprintf(js + size, "document.INDEXADMIN.uri.value='%s';\n",
                                   CFG_get_ota_url() ? CFG_get_ota_url() : "");
                size += sprintf(js + size, "document.getElementById('tabUpdate').className=\"btn btn-primary\";\n");
                break;
            case MENU_ADMIN_TAB_RESET:
                size += sprintf(js + size, "document.getElementById('tabReset').className=\"btn btn-primary\";\n");
                break;
            case MENU_ADMIN_TAB_SENHA:
                size += sprintf(js + size, "document.INDEXADMIN.user.value=\"%s\";\n",
                               CFG_get_web_user() ? CFG_get_web_user() : "");
                size += sprintf(js + size, "document.INDEXADMIN.npasswd.value=\"%s\";\n",
                               CFG_get_web_passwd() ? CFG_get_web_passwd() : "");
                size += sprintf(js + size, "document.INDEXADMIN.repasswd.value=\"%s\";\n",
                               CFG_get_web_passwd() ? CFG_get_web_passwd() : "");
                size += sprintf(js + size, "document.getElementById('tabPasswd').className=\"btn btn-primary\";\n");
                break;
            case MENU_ADMIN_TAB_TIMEZONE:
                size += sprintf(js + size, "document.INDEXADMIN.dst_enable.checked=%s;\n",
                               CFG_get_dst() ? "true" : "false");
                size += sprintf(js + size, "document.getElementById('timeZone').selectedIndex=\"%d\";\n",
                                CFG_get_timezone()+12);
                strcpy(date, CFG_get_dst_date());
                p = strtok_r(date, " ", &t);            
                p = strtok(p, "/");
                i = 0;
                while (p != NULL)
                {
                    if (i == 0)
                        size += sprintf(js + size, "document.getElementById('startMonth').selectedIndex=\"%ld\";\n",
                                           strtol(p, NULL, 10) - 1); 
                    else if (i == 1)
                        size += sprintf(js + size, "document.getElementById('startWeek').selectedIndex=\"%ld\";\n",
                                           strtol(p, NULL, 10) - 1);
                    else if (i == 2)
                        size += sprintf(js + size, "document.getElementById('startDay').selectedIndex=\"%ld\";\n",
                                           strtol(p, NULL, 10));
                    p = strtok(NULL, "/");
                    i++;
                }
                p = strtok_r(NULL, " ", &t);
                p = strtok(p, "/");
                i = 0;
                while (p != NULL)
                {
                    if (i == 0)
                        size += sprintf(js + size, "document.getElementById('endMonth').selectedIndex=\"%ld\";\n",
                                           strtol(p, NULL, 10) - 1); 
                    else if (i == 1)
                        size += sprintf(js + size, "document.getElementById('endWeek').selectedIndex=\"%ld\";\n",
                                           strtol(p, NULL, 10) - 1);
                    else if (i == 2)
                        size += sprintf(js + size, "document.getElementById('endDay').selectedIndex=\"%ld\";\n",
                                           strtol(p, NULL, 10));
                    p = strtok(NULL, "/");
                    i++;
                }
                size += sprintf(js + size, "document.getElementById('tabTimezone').className=\"btn btn-primary\";\n");
                break;
            case MENU_ADMIN_TAB_LOCATION:
                size += sprintf(js + size, "document.INDEXADMIN.latitude.value='%s';\n",
                                   CFG_get_latitude() ? CFG_get_latitude() : "");
                size += sprintf(js + size, "document.INDEXADMIN.longitude.value='%s';\n",
                                   CFG_get_longitude() ? CFG_get_longitude() : "");
                size += sprintf(js + size, "document.getElementById('tabLocation').className=\"btn btn-primary\";\n");
                break;
            case MENU_ADMIN_TAB_SYSTEM:
                size += sprintf(js + size, "document.INDEXADMIN.serial.value='%s';\n",
                                   CFG_get_serialnum());
                ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_ETH));
                size += sprintf(js + size, "document.INDEXADMIN.mac.value='"MACSTR"';\n",
                                   MAC2STR(mac));
                time_t t = time(NULL);
                tm = localtime(&t);
                sprintf(date, "%02d/%02d/%04d %02d:%02d:%02d",
                           tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
                           tm->tm_hour, tm->tm_min, tm->tm_sec);
                size += sprintf(js + size, "document.INDEXADMIN.date.value='%s';\n",
                                   date);
                size += sprintf(js + size, "document.INDEXADMIN.release.value='%s';\n",
                                   CFG_get_release());
                time_value = esp_timer_get_time()/1000000; 
                day = time_value / 86400;
                time_value -= day * 86400;
                hour = time_value / 3600;
                time_value -= hour * 3600;
                min = time_value / 60;
                sec = time_value - min * 60;
                if (day) {
                    sprintf(date, "%d dias, %d horas, %d minutos, %d segundos",
                               day, hour, min, sec);
                } else if (hour) {
                    sprintf(date, "%d horas, %d minutos, %d segundos",
                               hour, min, sec);
                } else if (min) {
                    sprintf(date, "%d minutos, %d segundos",
                               min, sec);
                } else {
                    sprintf(date, "%d segundos", sec);
                }
                size += sprintf(js + size, "document.INDEXADMIN.uptime.value='%s';\n",
                                   date);
                size += sprintf(js + size, "document.getElementById('tabSystem').className = \"btn btn-primary\";\n");
                break;
            case MENU_ADMIN_TAB_DEBUG:
                CFG_get_debug(&mode, &level, &host, &port);
                size += sprintf(js + size, "document.INDEXADMIN.debug_enable.checked=%s;\n",
                                   mode != ESP_LOG_NONE ? "true" : "false");
                size += sprintf(js + size, "document.INDEXADMIN.ip.value=\"%s\";\n",
                                   host ? host : "");
                size += sprintf(js + size, "document.INDEXADMIN.port.value=\"%d\";\n",
                                   port);
                size += sprintf(js + size, "document.INDEXADMIN.debug_level.value=\"%d\";\n",
                                   level);
                size += sprintf(js + size, "document.getElementById('tabDebug').className = \"btn btn-primary\";\n");
                break;
            case MENU_ADMIN_TAB_BACKUP:
                size += sprintf(js + size, "document.getElementById('tabBackup').className=\"btn btn-primary\";\n");
                break;
            case MENU_ADMIN_TAB_WIFI:
                memset(&scanConf, 0, sizeof(scanConf));
                scanConf.show_hidden = 1;
                scanConf.scan_type = WIFI_SCAN_TYPE_ACTIVE;
                wifi_station_scan(&scanConf, ulip_cgi_scan_callback);
                if (scan_html)
                    size += sprintf(js + size, "document.getElementById('wifi').innerHTML=\"%s\";\n",
                                       scan_html);
                else
                    size += sprintf(js + size, "document.getElementById('wifi').innerHTML=\"" \
                                       "<tr><td colspan=\\\"4\\\">Carregando...</td></tr>\";\n");
                size += sprintf(js + size, "document.getElementById('tabWifi').className = \"btn btn-primary\";\n");
                break;
            case MENU_ADMIN_TAB_WATCHDOG:
                size += sprintf(js + size, "document.INDEXADMIN.shutdown.value=\"%d\";\n",
                                   CFG_get_rtc_shutdown() != -1 ? CFG_get_rtc_shutdown() : 0);
                if (CFG_get_rtc_shutdown() && CFG_get_rtc_shutdown() != -1) {
                    // time_value = rtc_get_shutdown();
                    time_value = esp_timer_get_time()/1000;
                    day = time_value / 86400;
                    time_value -= day * 86400;
                    hour = time_value / 3600;
                    time_value -= hour * 3600;
                    min = time_value / 60;
                    sec = time_value - min * 60;
                    if (day) {
                        sprintf(date, "%d dia(s), %d hora(s), %d minuto(s), %d segundo(s)",
                                   day, hour, min, sec);
                    } else if (hour) {
                        sprintf(date, "%d hora(s), %d minuto(s), %d segundo(s)",
                                   hour, min, sec);
                    } else if (min) {
                        sprintf(date, "%d minuto(s), %d segundo(s)",
                                   min, sec);
                    } else {
                        sprintf(date, "%d segundo(s)", sec);
                    }
                    size += sprintf(js + size, "document.getElementById('shutdown').innerHTML = \"" \
                                       "<small>O equipamento ser&aacute; reiniciado em: %s</small>\";\n", date);
                    size += sprintf(js + size, "document.getElementById('shutdown').style.display = \"block\";\n");
                }
                size += sprintf(js + size, "document.getElementById('tabWatchdog').className = \"btn btn-primary\";\n");
                break;
        }
        size += sprintf(js + size, "%s", "document.getElementById('admin').style.backgroundColor='#0075be';\n");
    } else if (menuopt == MENU_PROG) {
        for (i = 0; i < CFG_SCHEDULE; i++) {
            period = CFG_get_schedule(i);
            if (!period) continue;
            strcpy(date, period);
            if (!strchr(date, '/')) {
                /* Week */
                p = strtok_r(date, " ", &t);
                p = strtok_r(p, "-", &l);
                w1 = strtol(p, NULL, 10);
                p = strtok_r(NULL, "-", &l);
                if (!p) continue;
                w2 = strtol(p, NULL, 10);
                /* Hour */
                p = strtok_r(NULL, " ", &t);
                if (!p) continue;
                p = strtok_r(p, "-", &t);
                p = strtok_r(p, ":", &l);
                h1 = strtol(p, NULL, 10);
                p = strtok_r(NULL, ":", &l);
                if (!p) continue;
                m1 = strtol(p, NULL, 10);
                p = strtok_r(NULL, "-", &t);
                p = strtok_r(p, ":", &l);
                h2 = strtol(p, NULL, 10);
                p = strtok_r(NULL, ":", &l);
                if (!p) continue;
                m2 = strtol(p, NULL, 10);
                size += sprintf(js + size, "document.PROG.w%di.value=\"%d\";\n",
                                   i + 1, w1);
                size += sprintf(js + size, "document.PROG.w%de.value=\"%d\";\n",
                                   i + 1, w2);
                size += sprintf(js + size, "document.PROG.h%di.value=\"%d\";\n",
                                   i + 1, h1);
                size += sprintf(js + size, "document.PROG.m%di.value=\"%d\";\n",
                                   i + 1, m1);
                size += sprintf(js + size, "document.PROG.h%de.value=\"%d\";\n",
                                   i + 1, h2);
                size += sprintf(js + size, "document.PROG.m%de.value=\"%d\";\n",
                                   i + 1, m2);
            } else {
                p = strtok_r(date, " ", &t);
                /*
                 * Date range
                 */
                p = strtok_r(p, "-", &l);
                p = strtok(p, "/");
                y1 = strtol(p, NULL, 10);
                p = strtok(NULL, "/");
                if (!p) continue;
                t1 = strtol(p, NULL, 10);
                p = strtok(NULL, "/");
                if (!p) continue;
                d1 = strtol(p, NULL, 10);
                p = strtok_r(NULL, "-", &l);
                if (!p) continue;
                p = strtok(p, "/");
                y2 = strtol(p, NULL, 10);
                p = strtok(NULL, "/");
                if (!p) continue;
                t2 = strtol(p, NULL, 10);
                p = strtok(NULL, "/");
                if (!p) continue;
                d2 = strtol(p, NULL, 10);
                /*
                 * Hour range
                 */
                p = strtok_r(NULL, " ", &t);
                if (!p) continue;
                p = strtok_r(p, "-", &t);
                p = strtok_r(p, ":", &l);
                h1 = strtol(p, NULL, 10);
                p = strtok_r(NULL, ":", &l);
                if (!p) continue;
                m1 = strtol(p, NULL, 10);
                p = strtok_r(NULL, "-", &t);
                if (!p) continue;
                p = strtok_r(p, ":", &l);
                h2 = strtol(p, NULL, 10);
                p = strtok_r(NULL, ":", &l);
                if (!p) continue;
                m2 = strtol(p, NULL, 10);
                size += sprintf(js + size, "document.PROG.d%di.value=\"%d\";\n",
                                   i + 1, d1);
                size += sprintf(js + size, "document.PROG.t%di.value=\"%d\";\n",
                                   i + 1, t1);
                size += sprintf(js + size, "document.PROG.y%di.value=\"%d\";\n",
                                   i + 1, y1);
                size += sprintf(js + size, "document.PROG.d%de.value=\"%d\";\n",
                                   i + 1, d2);
                size += sprintf(js + size, "document.PROG.t%de.value=\"%d\";\n",
                                   i + 1, t2);
                size += sprintf(js + size, "document.PROG.y%de.value=\"%d\";\n",
                                   i + 1, y2);
                size += sprintf(js + size, "document.PROG.h%di.value=\"%d\";\n",
                                   i + 1, h1);
                size += sprintf(js + size, "document.PROG.m%di.value=\"%d\";\n",
                                   i + 1, m1);
                size += sprintf(js + size, "document.PROG.h%de.value=\"%d\";\n",
                                   i + 1, h2);
                size += sprintf(js + size, "document.PROG.m%de.value=\"%d\";\n",
                                   i + 1, m2);
            }
        }
        size += sprintf(js + size, "%s", "document.getElementById('prog').style.backgroundColor='#0075be';\n");
    }
    size += sprintf(js + size, "%s", "}\n");
    ulip_cgi_send_data(pInstance, connData, (uint8_t *)js, size);

    free(js);
}


static int ulip_cgi_get_handler(HttpdInstance *pInstance, HttpdConnData *connData)
{
    int menuopt = 1;
    int subopt = -1;
    uint32_t etag = 0;
    char buf[512];
    char *html;
    int len;
    int rc = HTTPD_CGI_DONE;
    // ESP_LOGI("CGI", "GET %s isCss %d", connData->url, strcmp(connData->url, "/css/style.css") == 0);
    /* Get menu and sub menu */
    if (connData->getArgs) {
        if (httpdFindArg(connData->getArgs, "menuopt", buf, sizeof(buf)) != -1)
            menuopt = strtol(buf, NULL, 10);
        if (httpdFindArg(connData->getArgs, "subopt", buf, sizeof(buf)) != -1)
            subopt = strtol(buf, NULL, 10);
    }
    if (strcmp(connData->url, "/")) {
        /* Javascript */
        if (strcmp(connData->url, "/js/init.js") == 0) {
            ulip_cgi_response(connData, 200, 0, "application/javascript", -1);
            ulip_cgi_init_js(pInstance, connData, menuopt, subopt);
            return HTTPD_CGI_DONE;
        }
        /* CSS */
        if (strcmp(connData->url, "/css/style.css") == 0) { 
            // ESP_LOGI("CGI", "CSS");
            if (!connData->cgiData) {
                /* Cache control */
                if (!sntp_getservername(0)) {
                    if (ulip_cgi_cache_lookup(connData->url, *((uint32_t *)connData->remote_ip),
                                              &etag)) {
                        ulip_cgi_response(connData, 304, etag, NULL, 0);
                        return HTTPD_CGI_DONE;
                    }
                    ulip_cgi_cache_insert(connData->url, *((uint32_t *)connData->remote_ip),
                                          &etag);
                }
                ulip_cgi_response(connData, 200, etag, "text/css", sizeof(STYLE) - 1);
            }
            // ESP_LOGI("CGI", "Sending CSS");
            return ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)STYLE, sizeof(STYLE) - 1);
        }
        /* Images */
#if 0
        if (strcmp(connData->url, "/imgs/logo_utech.png") == 0) {
            if (!connData->cgiData) {
                /* Cache control */
                if (!sntp_getservername(0)) {
                    if (ulip_cgi_cache_lookup(connData->url, *((uint32_t *)connData->remote_ip),
                                              &etag)) {
                        ulip_cgi_response(connData, 304, etag, NULL, 0);
                        return HTTPD_CGI_DONE;
                    }
                    ulip_cgi_cache_insert(connData->url, *((uint32_t *)connData->remote_ip),
                                          &etag);
                }
                ulip_cgi_response(connData, 200, etag, "image/png", sizeof(LOGO));
            }
            return ulip_cgi_send_data_from_flash(pInstance, connData, LOGO, sizeof(LOGO));
        }
#endif
        ulip_cgi_response(connData, 404, 0, NULL, 0);
        return HTTPD_CGI_DONE;
    }

    if (!connData->cgiData) {
        ulip_cgi_response(connData, 200, 0, NULL, -1);
        connData->cgiData = (void *)CGI_PAGE_TOP;
    }
    if ((uint32_t)connData->cgiData & CGI_PAGE_TOP) {
        // ESP_LOGI("CGI", "CGI_PAGE_TOP");
        rc = ulip_cgi_send_data_from_flash(pInstance,connData, (uint8_t *)PAGE_TOP, sizeof(PAGE_TOP) - 1);
        if (rc == HTTPD_CGI_DONE)
            connData->cgiData = (void *)CGI_PAGE_BODY;
        return HTTPD_CGI_MORE;
    }

    switch (menuopt) {
        case MENU_NETWORK:
            if ((uint32_t)connData->cgiData & CGI_PAGE_BODY) {
                rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXREDE,
                                                   sizeof(INDEXREDE) - 1);
                if (rc == HTTPD_CGI_DONE)
                    connData->cgiData = (void *)CGI_PAGE_BOTTOM;
                return HTTPD_CGI_MORE;
            }
            ulip_cgi_include_script(pInstance, connData, menuopt, subopt);
            break;
        case MENU_CONTROL: 
            if ((uint32_t)connData->cgiData & CGI_PAGE_BODY) {
                rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXACIONAMENTO,
                                                   sizeof(INDEXACIONAMENTO) - 1);
                if (rc == HTTPD_CGI_DONE)
                    connData->cgiData = (void *)CGI_PAGE_BOTTOM;
                return HTTPD_CGI_MORE;
            }
            ulip_cgi_include_script(pInstance, connData, menuopt, subopt);
            break;
        case MENU_HTTP:
            if ((uint32_t)connData->cgiData & CGI_PAGE_BODY) {
                rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXHTTP,
                                                   sizeof(INDEXHTTP) - 1);
                if (rc == HTTPD_CGI_DONE)
                    connData->cgiData = (void *)CGI_PAGE_BOTTOM;
                return HTTPD_CGI_MORE;
            }
            ulip_cgi_include_script(pInstance, connData, menuopt, subopt);
            break;
        case MENU_USER:
            if ((uint32_t)connData->cgiData & CGI_PAGE_BODY) {
                rc = ulip_cgi_send_data_from_flash(pInstance, connData,(uint8_t *) INDEXUSER,
                                                   sizeof(INDEXUSER) - 1);
                if (rc == HTTPD_CGI_DONE)
                    connData->cgiData = (void *)CGI_PAGE_BOTTOM;
                return HTTPD_CGI_MORE;
            }
            ulip_cgi_include_script(pInstance, connData, menuopt, subopt);
            break;
        case MENU_LOG:
            if ((uint32_t)connData->cgiData & CGI_PAGE_BODY) {
                rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXLOG,
                                                   sizeof(INDEXLOG) - 1);
                if (rc == HTTPD_CGI_DONE)
                    connData->cgiData = (void *)CGI_PAGE_BOTTOM;
                return HTTPD_CGI_MORE;
            }
            html = (char *)malloc(2048);
            if (html) {
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
                len = ulip_core_log2html(html, 2048);
#else
                len = ulip_core_telemetry2html(html, 2048);
#endif
                ulip_cgi_send_data(pInstance, connData, (uint8_t *)html, len);
                free(html);
            }
            ulip_cgi_include_script(pInstance, connData, menuopt, subopt);
            break;
        case MENU_STATUS:
            if ((uint32_t)connData->cgiData & CGI_PAGE_BODY) {
                rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXSTATUS,
                                                   sizeof(INDEXSTATUS) - 1);
                if (rc == HTTPD_CGI_DONE)
                    connData->cgiData = (void *)CGI_PAGE_BOTTOM;
                return HTTPD_CGI_MORE;
            }
            ulip_cgi_include_script(pInstance, connData, menuopt, subopt);
            break;
        case MENU_ADMIN:
            if ((uint32_t)connData->cgiData & CGI_PAGE_BODY) {
                switch (subopt) {
                    case MENU_ADMIN_TAB_UPDATE:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_UPDATE, sizeof(INDEXADMIN_UPDATE) - 1);
                        break;
                    case MENU_ADMIN_TAB_RESET:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_RESET, sizeof(INDEXADMIN_RESET) - 1);
                        break;
                    case MENU_ADMIN_TAB_SENHA:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_SENHA, sizeof(INDEXADMIN_SENHA) - 1);
                        break;
                    case MENU_ADMIN_TAB_TIMEZONE:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_TIMEZONE, sizeof(INDEXADMIN_TIMEZONE) - 1);
                        break;
                    case MENU_ADMIN_TAB_LOCATION:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_LOCATION, sizeof(INDEXADMIN_LOCATION) - 1);
                        break;
                    case MENU_ADMIN_TAB_SYSTEM:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_SYSTEM, sizeof(INDEXADMIN_SYSTEM) - 1);
                        break;
                    case MENU_ADMIN_TAB_DEBUG:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_DEBUG, sizeof(INDEXADMIN_DEBUG) - 1);
                        break;
                    case MENU_ADMIN_TAB_BACKUP:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_BACKUP, sizeof(INDEXADMIN_BACKUP) - 1);
                        break;
                    case MENU_ADMIN_TAB_WIFI:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_WIFI, sizeof(INDEXADMIN_WIFI) - 1);
                        break;
                    case MENU_ADMIN_TAB_WATCHDOG:
                        rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXADMIN_WATCHDOG, sizeof(INDEXADMIN_WATCHDOG) - 1);
                        break;
                }
                if (rc == HTTPD_CGI_DONE)
                    connData->cgiData = (void *)CGI_PAGE_BOTTOM;
                return HTTPD_CGI_MORE;
            }
            ulip_cgi_include_script(pInstance, connData, menuopt, subopt);
            break;
        case MENU_PROG:
            if ((uint32_t)connData->cgiData & CGI_PAGE_BODY) {
                rc = ulip_cgi_send_data_from_flash(pInstance, connData, (uint8_t *)INDEXPROG,
                                                   sizeof(INDEXPROG) - 1);
                if (rc == HTTPD_CGI_DONE)
                    connData->cgiData = (void *)CGI_PAGE_BOTTOM;
                return HTTPD_CGI_MORE;
            }
            ulip_cgi_include_script(pInstance, connData, menuopt, subopt);
            break;
    }

    ulip_cgi_send_data(pInstance, connData, (uint8_t *)PAGE_BOTTOM, sizeof(PAGE_BOTTOM) - 1);

    connData->cgiData = NULL;

    //All done.
    return HTTPD_CGI_DONE;
}


static int ulip_cgi_post_handler(HttpdConnData *connData)
{
    int menuopt = 1;
    int subopt = -1;
    char buf[128] = "";
    char name[64] = "";
    char user[20] = "";
    char pass[20] = "";
    char card[32] = "";
    char code[128]= "";
    char rfcode[16] = "";
    char fingerprint[700] = "";
    char finger[2] = "";
    uint16_t lifecount = 0;
    uint8_t accessibility = false;
    uint8_t panic = false;
    uint8_t visitor = false;
    char pkey[16] = "";
    acc_permission_t perm[2];
    int size = 0;
    char key[16];
    account_t *acc;
    struct tm *tm;
    uint32_t now;
    int index;
    char date[32];
    int mode;
    int level = 0;
    char host[16];
    int port = 80;
    int w1;
    int w2;
    int h1;
    int m1;
    int h2;
    int m2;
    int y1;
    int t1;
    int d1;
    int y2;
    int t2;
    int d2;
    int i;
    
    /* Get menu and sub menu */
    if (httpdFindArg(connData->post.buff, "menuopt", buf, sizeof(buf)) != -1)
        menuopt = strtol(buf, NULL, 10);
    if (httpdFindArg(connData->post.buff, "subopt", buf, sizeof(buf)) != -1)
        subopt = strtol(buf, NULL, 10);

    if (httpdFindArg(connData->post.buff, "save", buf, sizeof(buf)) != -1) {
        switch (menuopt) {
            case MENU_NETWORK:
                if (httpdFindArg(connData->post.buff, "hotspot", buf, sizeof(buf)) != -1)
                    CFG_set_hotspot(true);
                else
                    CFG_set_hotspot(false);
                if (httpdFindArg(connData->post.buff, "apmode", buf, sizeof(buf)) != -1)
                    CFG_set_ap_mode(true);
                else
                    CFG_set_ap_mode(false);
                if (httpdFindArg(connData->post.buff, "ssid", buf, sizeof(buf)) != -1)
                    CFG_set_wifi_ssid(buf);
                if (httpdFindArg(connData->post.buff, "passwd", buf, sizeof(buf)) != -1)
                    CFG_set_wifi_passwd(buf);
                if (httpdFindArg(connData->post.buff, "channel", buf, sizeof(buf)) != -1)
                    CFG_set_wifi_channel(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "beacon", buf, sizeof(buf)) != -1)
                    CFG_set_wifi_beacon_interval(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "hidessid", buf, sizeof(buf)) != -1)
                    CFG_set_ssid_hidden(true);
                else
                    CFG_set_ssid_hidden(false);
                if (httpdFindArg(connData->post.buff, "wifi_enable", buf, sizeof(buf)) != -1) {
                    CFG_set_wifi_disable(false);
                } else{
                    CFG_set_wifi_disable(true);
                }
                if (httpdFindArg(connData->post.buff, "dhcp", buf, sizeof(buf)) != -1) {
                    CFG_set_dhcp(true);
                } else{
                    CFG_set_dhcp(false);
                }
                if (httpdFindArg(connData->post.buff, "ip", buf, sizeof(buf)) != -1)
                    CFG_set_ip_address(buf);
                if (httpdFindArg(connData->post.buff, "netmask", buf, sizeof(buf)) != -1)
                    CFG_set_netmask(buf);
                if (httpdFindArg(connData->post.buff, "gateway", buf, sizeof(buf)) != -1)
                    CFG_set_gateway(buf);

                if (httpdFindArg(connData->post.buff, "eth_enable", buf, sizeof(buf)) != -1) {
                    CFG_set_eth_enable(true);
                } else{
                    CFG_set_eth_enable(false);
                }
                if (httpdFindArg(connData->post.buff, "eth_dhcp", buf, sizeof(buf)) != -1) {
                    CFG_set_eth_dhcp(true);
                } else{
                    CFG_set_eth_dhcp(false);
                }
                if (httpdFindArg(connData->post.buff, "eth_ip", buf, sizeof(buf)) != -1)
                    CFG_set_eth_ip_address(buf);
                if (httpdFindArg(connData->post.buff, "eth_netmask", buf, sizeof(buf)) != -1)
                    CFG_set_eth_netmask(buf);
                if (httpdFindArg(connData->post.buff, "eth_gateway", buf, sizeof(buf)) != -1)
                    CFG_set_eth_gateway(buf);
                if (httpdFindArg(connData->post.buff, "dns", buf, sizeof(buf)) != -1)
                    CFG_set_dns(buf);
                if (httpdFindArg(connData->post.buff, "ntp", buf, sizeof(buf)) != -1)
                    CFG_set_ntp(buf);
                if (httpdFindArg(connData->post.buff, "hostname", buf, sizeof(buf)) != -1)
                    CFG_set_hostname(buf);
                if (httpdFindArg(connData->post.buff, "ddns", buf, sizeof(buf)) != -1)
                    CFG_set_ddns(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "ddns_domain", buf, sizeof(buf)) != -1)
                    CFG_set_ddns_domain(buf);
                if (httpdFindArg(connData->post.buff, "ddns_user", buf, sizeof(buf)) != -1)
                    CFG_set_ddns_user(buf);
                if (httpdFindArg(connData->post.buff, "ddns_passwd", buf, sizeof(buf)) != -1)
                    CFG_set_ddns_passwd(buf);
                /* Save configuration */
                CFG_Save();
                break;
            case MENU_CONTROL:
                if (httpdFindArg(connData->post.buff, "desc", buf, sizeof(buf)) != -1)
                    CFG_set_control_description(buf);
#if !defined(CONFIG__MLI_1WRP_TYPE__)
                if (httpdFindArg(connData->post.buff, "standalone", buf, sizeof(buf)) != -1)
                    CFG_set_standalone(true);
                else
                    CFG_set_standalone(false);
                if (httpdFindArg(connData->post.buff, "external", buf, sizeof(buf)) != -1)
                    CFG_set_control_external(true);
                else
                    CFG_set_control_external(false);
                if (httpdFindArg(connData->post.buff, "external_url", buf, sizeof(buf)) != -1)
                    CFG_set_control_url(buf);
                if (httpdFindArg(connData->post.buff, "mode", buf, sizeof(buf)) != -1)
                    CFG_set_control_mode(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "timeout", buf, sizeof(buf)) != -1)
                    CFG_set_control_timeout(strtol(buf, NULL, 10));
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRC_TYPE__) && !defined(CONFIG__MLI_1WLC_TYPE__)
                if (httpdFindArg(connData->post.buff, "acc_timeout", buf, sizeof(buf)) != -1)
                    CFG_set_control_acc_timeout(strtol(buf, NULL, 10));
#endif
                if (httpdFindArg(connData->post.buff, "breakin", buf, sizeof(buf)) != -1)
                    CFG_set_breakin_alarm(true);
                else
                    CFG_set_breakin_alarm(false);
                if (httpdFindArg(connData->post.buff, "button_enable", buf, sizeof(buf)) != -1)
                    CFG_set_button_enable(true);
                else
                    CFG_set_button_enable(false);
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRC_TYPE__) && !defined(CONFIG__MLI_1WLC_TYPE__)
                if (httpdFindArg(connData->post.buff, "doublepass_timeout", buf, sizeof(buf)) != -1)
                    CFG_set_control_doublepass_timeout(strtol(buf, NULL, 10));
#endif
#endif
#if defined(CONFIG__MLI_1WRP_TYPE__)
                if (httpdFindArg(connData->post.buff, "relay_status", buf, sizeof(buf)) != -1)
                    CFG_set_relay_status(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "button_enable", buf, sizeof(buf)) != -1)
                    CFG_set_button_enable(true);
                else
                    CFG_set_button_enable(false);
                if (httpdFindArg(connData->post.buff, "timeout", buf, sizeof(buf)) != -1)
                    CFG_set_control_timeout(strtol(buf, NULL, 10));
#endif
#if !defined(CONFIG__MLI_1WF_TYPE__) && !defined(CONFIG__MLI_1WQF_TYPE__) && !defined(CONFIG__MLI_1WRF_TYPE__)
                if (httpdFindArg(connData->post.buff, "rfid", buf, sizeof(buf)) != -1)
                    CFG_set_rfid_enable(true);
                else
                    CFG_set_rfid_enable(false);
                if (httpdFindArg(connData->post.buff, "rfid_timo", buf, sizeof(buf)) != -1)
                    CFG_set_rfid_timeout(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "rfid_retries", buf, sizeof(buf)) != -1)
                    CFG_set_rfid_retries(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "rfid_nfc", buf, sizeof(buf)) != -1)
                    CFG_set_rfid_nfc(true);
                else
                    CFG_set_rfid_nfc(false);
                if (httpdFindArg(connData->post.buff, "rfid_panic_timo", buf, sizeof(buf)) != -1)
                    CFG_set_rfid_panic_timeout(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "rfid_format", buf, sizeof(buf)) != -1)
                    CFG_set_rfid_format(strtol(buf, NULL, 10));
#endif
#if defined(CONFIG__MLI_1WQ_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRQ_TYPE__)
                if (httpdFindArg(connData->post.buff, "qrcode", buf, sizeof(buf)) != -1)
                    CFG_set_qrcode_enable(true);
                else
                    CFG_set_qrcode_enable(false);
                if (httpdFindArg(connData->post.buff, "qrcode_timo", buf, sizeof(buf)) != -1)
                    CFG_set_qrcode_timeout(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "qrcode_panic_timo", buf, sizeof(buf)) != -1)
                    CFG_set_qrcode_panic_timeout(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "qrcode_config", buf, sizeof(buf)) != -1)
                    CFG_set_qrcode_config(true);
                else
                    CFG_set_qrcode_config(false);
                if (httpdFindArg(connData->post.buff, "qrcode_led", buf, sizeof(buf)) != -1)
                    CFG_set_qrcode_led(true);
                else
                    CFG_set_qrcode_led(false);
                if (httpdFindArg(connData->post.buff, "qrcode_dynamic", buf, sizeof(buf)) != -1)
                    CFG_set_qrcode_dynamic(true);
                else
                    CFG_set_qrcode_dynamic(false);
                if (httpdFindArg(connData->post.buff, "qrcode_validity", buf, sizeof(buf)) != -1)
                    CFG_set_qrcode_validity(strtol(buf, NULL, 10));
#endif
#if defined(CONFIG__MLI_1WF_TYPE__) || defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRF_TYPE__)
                if (httpdFindArg(connData->post.buff, "rf433", buf, sizeof(buf)) != -1)
                    CFG_set_rf433_enable(true);
                else
                    CFG_set_rf433_enable(false);
                if (httpdFindArg(connData->post.buff, "rf433_rc", buf, sizeof(buf)) != -1)
                    CFG_set_rf433_rc(true);
                else
                    CFG_set_rf433_rc(false);
                if (httpdFindArg(connData->post.buff, "rf433_hc", buf, sizeof(buf)) != -1)
                    CFG_set_rf433_hc(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "rf433_alarm", buf, sizeof(buf)) != -1)
                    CFG_set_rf433_alarm(true);
                else
                    CFG_set_rf433_alarm(false);
                if (httpdFindArg(connData->post.buff, "rf433_bc", buf, sizeof(buf)) != -1)
                    CFG_set_rf433_bc(true);
                else
                    CFG_set_rf433_bc(false);
                if (httpdFindArg(connData->post.buff, "rf433_bp", buf, sizeof(buf)) != -1)
                    CFG_set_rf433_bp(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "rf433_panic_timo", buf, sizeof(buf)) != -1)
                    CFG_set_rf433_panic_timeout(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "rf433_ba", buf, sizeof(buf)) != -1)
                    CFG_set_rf433_ba(strtol(buf, NULL, 10));
#endif
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
                if (httpdFindArg(connData->post.buff, "fpm", buf, sizeof(buf)) != -1)
                    CFG_set_fingerprint_enable(true);
                else
                    CFG_set_fingerprint_enable(false);
                if (httpdFindArg(connData->post.buff, "fpm_timo", buf, sizeof(buf)) != -1)
                    CFG_set_fingerprint_timeout(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "fpm_sec", buf, sizeof(buf)) != -1)
                    CFG_set_fingerprint_security(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "fpm_id", buf, sizeof(buf)) != -1)
                    CFG_set_fingerprint_identify_retries(strtol(buf, NULL, 10));
#endif
#if defined(CONFIG__MLI_1WRQ_TYPE__) || defined(CONFIG__MLI_1WR_TYPE__) || defined(CONFIG__MLI_1WRF_TYPE__) || \
    defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WRP_TYPE__) || defined(CONFIG__MLI_1WRC_TYPE__)
                if (httpdFindArg(connData->post.buff, "rs485", buf, sizeof(buf)) != -1)
                    CFG_set_rs485_enable(true);
                else
                    CFG_set_rs485_enable(false);
                if (httpdFindArg(connData->post.buff, "rs485_addr", buf, sizeof(buf)) != -1)
                    CFG_set_rs485_hwaddr(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "rs485_server_addr", buf, sizeof(buf)) != -1)
                    CFG_set_rs485_server_hwaddr(strtol(buf, NULL, 10));
#endif
#if defined(CONFIG__MLI_1WLS_TYPE__)
                if (httpdFindArg(connData->post.buff, "lora", buf, sizeof(buf)) != -1)
                    CFG_set_lora_enable(true);
                else
                    CFG_set_lora_enable(false);
                if (httpdFindArg(connData->post.buff, "lora_channel", buf, sizeof(buf)) != -1)
                    CFG_set_lora_channel(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "lora_baudrate", buf, sizeof(buf)) != -1)
                    CFG_set_lora_baudrate(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "lora_addr", buf, sizeof(buf)) != -1)
                    CFG_set_lora_address(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "lora_server_addr", buf, sizeof(buf)) != -1)
                    CFG_set_lora_server_address(strtol(buf, NULL, 10));
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || \
    defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
                if (httpdFindArg(connData->post.buff, "dht_enable", buf, sizeof(buf)) != -1)
                    CFG_set_dht_enable(true);
                else
                    CFG_set_dht_enable(false);
                if (httpdFindArg(connData->post.buff, "dht_timo", buf, sizeof(buf)) != -1)
                    CFG_set_dht_timeout(strtol(buf, NULL, 10) * 1000);
                if (httpdFindArg(connData->post.buff, "dht_temp_upper", buf, sizeof(buf)) != -1)
                    CFG_set_dht_temp_upper(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "dht_temp_lower", buf, sizeof(buf)) != -1)
                    CFG_set_dht_temp_lower(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "dht_rh_upper", buf, sizeof(buf)) != -1)
                    CFG_set_dht_rh_upper(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "dht_rh_lower", buf, sizeof(buf)) != -1)
                    CFG_set_dht_rh_lower(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "dht_relay", buf, sizeof(buf)) != -1)
                    CFG_set_dht_relay(true);
                else
                    CFG_set_dht_relay(false);
                if (httpdFindArg(connData->post.buff, "dht_alarm", buf, sizeof(buf)) != -1)
                    CFG_set_dht_alarm(true);
                else
                    CFG_set_dht_alarm(false);
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__)
                if (httpdFindArg(connData->post.buff, "temt_enable", buf, sizeof(buf)) != -1)
                    CFG_set_temt_enable(true);
                else
                    CFG_set_temt_enable(false);
                if (httpdFindArg(connData->post.buff, "temt_timo", buf, sizeof(buf)) != -1)
                    CFG_set_temt_timeout(strtol(buf, NULL, 10) * 1000);
                if (httpdFindArg(connData->post.buff, "temt_upper", buf, sizeof(buf)) != -1)
                    CFG_set_temt_upper(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "temt_lower", buf, sizeof(buf)) != -1)
                    CFG_set_temt_lower(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "temt_relay", buf, sizeof(buf)) != -1)
                    CFG_set_temt_relay(true);
                else
                    CFG_set_temt_relay(false);
                if (httpdFindArg(connData->post.buff, "temt_alarm", buf, sizeof(buf)) != -1)
                    CFG_set_temt_alarm(true);
                else
                    CFG_set_temt_alarm(false);
#endif
#if defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WLG_TYPE__)
                if (httpdFindArg(connData->post.buff, "mq2_enable", buf, sizeof(buf)) != -1)
                    CFG_set_mq2_enable(true);
                else
                    CFG_set_mq2_enable(false);
                if (httpdFindArg(connData->post.buff, "mq2_timo", buf, sizeof(buf)) != -1)
                    CFG_set_mq2_timeout(strtol(buf, NULL, 10) * 1000);
                if (httpdFindArg(connData->post.buff, "mq2_limit", buf, sizeof(buf)) != -1)
                    CFG_set_mq2_limit(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "mq2_relay", buf, sizeof(buf)) != -1)
                    CFG_set_mq2_relay(true);
                else
                    CFG_set_mq2_relay(false);
                if (httpdFindArg(connData->post.buff, "mq2_alarm", buf, sizeof(buf)) != -1)
                    CFG_set_mq2_alarm(true);
                else
                    CFG_set_mq2_alarm(false);
#endif
#if defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
                if (httpdFindArg(connData->post.buff, "cli_enable", buf, sizeof(buf)) != -1)
                    CFG_set_cli_enable(true);
                else
                    CFG_set_cli_enable(false);
                if (httpdFindArg(connData->post.buff, "cli_timo", buf, sizeof(buf)) != -1)
                    CFG_set_cli_timeout(strtol(buf, NULL, 10) * 1000);
                if (httpdFindArg(connData->post.buff, "cli_range", buf, sizeof(buf)) != -1)
                    CFG_set_cli_range(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "cli_upper", buf, sizeof(buf)) != -1)
                    CFG_set_cli_upper(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "cli_lower", buf, sizeof(buf)) != -1)
                    CFG_set_cli_lower(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "cli_relay", buf, sizeof(buf)) != -1)
                    CFG_set_cli_relay(true);
                else
                    CFG_set_cli_relay(false);
                if (httpdFindArg(connData->post.buff, "cli_alarm", buf, sizeof(buf)) != -1)
                    CFG_set_cli_alarm(true);
                else
                    CFG_set_cli_alarm(false);
#endif
#if defined(CONFIG__MLI_1WRS_TYPE__) || defined(CONFIG__MLI_1WLS_TYPE__) || \
    defined(CONFIG__MLI_1WRG_TYPE__) || defined(CONFIG__MLI_1WLG_TYPE__) || \
    defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
                if (httpdFindArg(connData->post.buff, "pir_enable", buf, sizeof(buf)) != -1)
                    CFG_set_pir_enable(true);
                else
                    CFG_set_pir_enable(false);
                if (httpdFindArg(connData->post.buff, "pir_timo", buf, sizeof(buf)) != -1)
                    CFG_set_pir_timeout(strtol(buf, NULL, 10) * 1000);
                if (httpdFindArg(connData->post.buff, "pir_chime", buf, sizeof(buf)) != -1)
                    CFG_set_pir_chime(true);
                else
                    CFG_set_pir_chime(false);
                if (httpdFindArg(connData->post.buff, "pir_relay", buf, sizeof(buf)) != -1)
                    CFG_set_pir_relay(true);
                else
                    CFG_set_pir_relay(false);
                if (httpdFindArg(connData->post.buff, "pir_alarm", buf, sizeof(buf)) != -1)
                    CFG_set_pir_alarm(true);
                else
                    CFG_set_pir_alarm(false);
                if (httpdFindArg(connData->post.buff, "sensor_type", buf, sizeof(buf)) != -1)
                    CFG_set_sensor_type(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "sensor_flow", buf, sizeof(buf)) != -1)
                    CFG_set_sensor_flow(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "sensor_limit", buf, sizeof(buf)) != -1)
                    CFG_set_sensor_limit(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "sensor_relay", buf, sizeof(buf)) != -1)
                    CFG_set_sensor_relay(true);
                else
                    CFG_set_sensor_relay(false);
                if (httpdFindArg(connData->post.buff, "sensor_alarm", buf, sizeof(buf)) != -1)
                    CFG_set_sensor_alarm(true);
                else
                    CFG_set_sensor_alarm(false);
#endif
#if defined(CONFIG__MLI_1WRP_TYPE__)
                if (httpdFindArg(connData->post.buff, "pow_vol_cal", buf, sizeof(buf)) != -1)
                    CFG_set_pow_voltage_cal(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_vol_upper", buf, sizeof(buf)) != -1)
                    CFG_set_pow_voltage_upper(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_vol_lower", buf, sizeof(buf)) != -1)
                    CFG_set_pow_voltage_lower(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_cur_cal", buf, sizeof(buf)) != -1)
                    CFG_set_pow_current_cal(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_cur_upper", buf, sizeof(buf)) != -1)
                    CFG_set_pow_current_upper(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_cur_lower", buf, sizeof(buf)) != -1)
                    CFG_set_pow_current_lower(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_pwr_upper", buf, sizeof(buf)) != -1)
                    CFG_set_pow_power_upper(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_pwr_lower", buf, sizeof(buf)) != -1)
                    CFG_set_pow_power_lower(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_relay", buf, sizeof(buf)) != -1)
                    CFG_set_pow_relay(true);
                else
                    CFG_set_pow_relay(false);
                if (httpdFindArg(connData->post.buff, "pow_alarm_time", buf, sizeof(buf)) != -1)
                    CFG_set_pow_alarm_time(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_relay_timo", buf, sizeof(buf)) != -1)
                    CFG_set_pow_relay_timeout(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_relay_ext", buf, sizeof(buf)) != -1)
                    CFG_set_pow_relay_ext(true);
                else
                    CFG_set_pow_relay_ext(false);
                if (httpdFindArg(connData->post.buff, "pow_interval", buf, sizeof(buf)) != -1)
                    CFG_set_pow_interval(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_day", buf, sizeof(buf)) != -1)
                    CFG_set_pow_day(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "daily_limit", buf, sizeof(buf)) != -1)
                    CFG_set_energy_daily_limit(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "monthly_limit", buf, sizeof(buf)) != -1)
                    CFG_set_energy_monthly_limit(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "total_limit", buf, sizeof(buf)) != -1)
                    CFG_set_energy_total_limit(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "pow_nrg_cal", buf, sizeof(buf)) != -1)
                    CFG_set_pow_energy_cal(strtol(buf, NULL, 10));
#endif
                /* Save configuration */
                CFG_Save();
                break;
            case MENU_HTTP:
                if (httpdFindArg(connData->post.buff, "server", buf, sizeof(buf)) != -1)
                    CFG_set_server_ip(buf);
                if (httpdFindArg(connData->post.buff, "port", buf, sizeof(buf)) != -1)
                    CFG_set_server_port(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "user", buf, sizeof(buf)) != -1)
                    CFG_set_server_user(buf);
                if (httpdFindArg(connData->post.buff, "pass", buf, sizeof(buf)) != -1)
                    CFG_set_server_passwd(buf);
                if (httpdFindArg(connData->post.buff, "url", buf, sizeof(buf)) != -1)
                    CFG_set_server_url(buf);
                if (httpdFindArg(connData->post.buff, "retries", buf, sizeof(buf)) != -1)
                    CFG_set_server_retries(strtol(buf, NULL, 10));
                if (httpdFindArg(connData->post.buff, "auth", buf, sizeof(buf)) != -1)
                    CFG_set_user_auth(true);
                else
                    CFG_set_user_auth(false);
                /* Save configuration */
                CFG_Save();
                break;
            case MENU_USER:
                if (httpdFindArg(connData->post.buff, "level", buf, sizeof(buf)) != -1)
                    level = strtol(buf, NULL, 10);
                httpdFindArg(connData->post.buff, "name", name, sizeof(name));
                httpdFindArg(connData->post.buff, "user", user, sizeof(user));
                httpdFindArg(connData->post.buff, "pass", pass, sizeof(pass));
                httpdFindArg(connData->post.buff, "card", card, sizeof(card));
                httpdFindArg(connData->post.buff, "code", code, sizeof(code));
                httpdFindArg(connData->post.buff, "rfcode", rfcode, sizeof(rfcode));
                httpdFindArg(connData->post.buff, "fingerprint", fingerprint,
                             sizeof(fingerprint));
                httpdFindArg(connData->post.buff, "finger", finger, sizeof(finger));
                if (httpdFindArg(connData->post.buff, "lifecount", buf, sizeof(buf)) != -1)
                    lifecount = strtol(buf, NULL, 10);
                if (httpdFindArg(connData->post.buff, "accessibility", buf, sizeof(buf)) != -1)
                    accessibility = true;
                if (httpdFindArg(connData->post.buff, "panic", buf, sizeof(buf)) != -1)
                    panic = true;
                if (httpdFindArg(connData->post.buff, "visitor", buf, sizeof(buf)) != -1)
                    visitor = true;
                httpdFindArg(connData->post.buff, "key", pkey, sizeof(pkey));
                for (i = 0; i < 2; i++) {
                    w1 = -1;
                    w2 = -1;
                    h1 = -1;
                    m1 = -1;
                    h2 = -1;
                    m2 = -1;
                    y1 = -1;
                    t1 = -1;
                    d1 = -1;
                    y2 = -1;
                    t2 = -1;
                    d2 = -1;
                    sprintf(key, "w%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            w1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "w%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            w2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "d%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            d1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "t%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            t1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "y%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            y1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "d%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            d2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "t%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            t2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "y%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            y2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "h%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            h1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "m%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            m1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "h%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            h2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "m%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            m2 = strtol(buf, NULL, 10);
                    }
                    if (w1 != -1 && w2 != -1 && h1 != -1 && m1 != -1 &&
                        h2 != -1 && m2 != -1) {
                        sprintf(perm[size++], "%d-%d %d:%d-%d:%d", w1, w2,
                                   h1, m1, h2, m2);
                    } else if (y1 != -1 && t1 != -1 && d1 != -1 && y2 != -1 &&
                               t2 != -1 && d2 != -1 && h1 != -1 && m1 != -1 &&
                               h2 != -1 && m2 != -1) {
                        sprintf(perm[size++], "%d/%d/%d-%d/%d/%d %d:%d-%d:%d",
                                   y1, t1, d1, y2, t2, d2, h1, m1, h2, m2);
                    }
                }
                if (*user == '\0' && *card == '\0' && *code == '\0' &&
                    *rfcode == '\0' && *fingerprint == '\0') {
                    ESP_LOGI("CGI", "Invalid account parameters");
                    sprintf(buf, "/?menuopt=%d", menuopt);
                    httpdRedirect(connData, buf);
                    return HTTPD_CGI_DONE;
                }
                if (*fingerprint != '\0'){
                    unsigned int olen;
                    mbedtls_base64_decode((uint8_t *)fingerprint, strlen(fingerprint),
                                          &olen, (uint8_t *)fingerprint, strlen(fingerprint));
                    // base64Decode(strlen(fingerprint), fingerprint,
                    //              sizeof(fingerprint), fingerprint);
                }
                index = account_db_find(NULL, user, card, code, rfcode,
                                        (uint8_t *)fingerprint, NULL);
                if (index == -1)
                    acc = account_new();
                else
                    acc = account_db_get_index(index);
                if (acc) {
                    account_set_level(acc, level);
                    account_set_name(acc, name);
                    account_set_user(acc, user);
                    account_set_password(acc, pass);
                    account_set_card(acc, card);
#if defined(CONFIG__MLI_1WQ_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__) || \
    defined(CONFIG__MLI_1WQF_TYPE__) || defined(CONFIG__MLI_1WRQ_TYPE__)
                    if (!qrcode_get_dynamic()) {
                        account_set_code(acc, code);
                    } else {
                        /* Create code from timestamp */
                        if (!account_get_code(acc)) {
                            now = time(NULL);
                            tm = localtime(&now);
                            snprintf(code, sizeof(code), "%u", now);
                            account_set_code(acc, code);
                        }
                    }
#else
                    account_set_code(acc, code);
#endif
                    account_set_rfcode(acc, rfcode);
                    if (*fingerprint == 0)
                        memset(fingerprint, 0, sizeof(fingerprint));
                    account_set_fingerprint(acc, (uint8_t *)fingerprint);
                    account_set_finger(acc, finger);
                    account_set_lifecount(acc, lifecount);
                    account_set_accessibility(acc, accessibility);
                    account_set_panic(acc, panic);
                    account_set_visitor(acc, visitor);
                    account_set_key(acc, pkey);
                    if (size)
                        account_set_permission(acc, perm, size);
                    index = account_db_insert(acc);
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
                    if (*fingerprint == 0)
                        fpm_delete_template(index);
                    else
                        fpm_set_template(index, (unsigned char *)fingerprint);
#endif
                    account_destroy(acc);
                    ESP_LOGI("CGI", "Save account [%d]", index);
                    sprintf(buf, "/?menuopt=%d&subopt=%d", menuopt, index);
                    httpdRedirect(connData, buf);
                    return HTTPD_CGI_DONE;
                }
                break;
            case MENU_ADMIN:
                switch(subopt) {
                    case MENU_ADMIN_TAB_UPDATE:
                        if (httpdFindArg(connData->post.buff, "proto", buf, sizeof(buf)) != -1) {
                            if (!strcmp(buf, "HTTP")) {
                                if (httpdFindArg(connData->post.buff, "uri", buf, sizeof(buf)) != -1) {
                                    CFG_set_ota_url(buf);
                                    CFG_Save();
                                    ulip_cgi_response(connData, 200, 0, NULL, 0);
                                    /* Schedule system update */
                                    // timer_setfn(&update_timer,
                                    //                (timer_func_t *)ulip_core_system_update,
                                    //                (void *)CFG_get_ota_url());
                                    // timer_arm(&update_timer, 1000, false);
                                    esp_timer_create_args_t update_timer_args = {
                                        .callback = (esp_timer_cb_t)ulip_core_system_update,
                                        .arg = (void *)CFG_get_ota_url(),
                                    };
                                    esp_timer_create(&update_timer_args, &update_timer);
                                    esp_timer_start_once(update_timer, 1000);

                                    return HTTPD_CGI_DONE;
                                }
                            } else if (!strcmp(buf, "TFTP")) {
                                if (httpdFindArg(connData->post.buff, "uri", buf, sizeof(buf)) != -1) {
                                }
                            }
                        }
                        break;
                    case MENU_ADMIN_TAB_RESET:
                        if (httpdFindArg(connData->post.buff, "reboot_mode", buf, sizeof(buf)) != -1){
                            if (strcmp(buf, "0") == 0) {
                               esp_restart();
                            } else if (strcmp(buf, "1") == 0) {
                                ulip_core_restore_config(true);
                            }
                        }
                        break;
                    case MENU_ADMIN_TAB_SENHA:
                        if (httpdFindArg(connData->post.buff, "user", buf, sizeof(buf)) != -1){
                            CFG_set_web_user(buf);
                        } 
                        if (httpdFindArg(connData->post.buff, "npasswd", buf, sizeof(buf)) != -1){
                            CFG_set_web_passwd(buf);         
                        }
                        break;
                    case MENU_ADMIN_TAB_TIMEZONE:
                        if (httpdFindArg(connData->post.buff, "settimezone", buf, sizeof(buf)) != -1){
                               CFG_set_timezone(strtol(buf, NULL, 10)-12); 
                        }
                        if (httpdFindArg(connData->post.buff, "dst_enable", buf, sizeof(buf)) != -1)
                            CFG_set_dst(true);
                        else
                            CFG_set_dst(false);   
                        if (httpdFindArg(connData->post.buff, "start_mon", buf, sizeof(buf)) != -1){
                            sprintf(date, buf);
                            strcat(date, "/"); 
                        } else{
                            break;
                        }
                        if (httpdFindArg(connData->post.buff, "start_week", buf, sizeof(buf)) != -1){
                            strcat(date, buf);
                            strcat(date, "/"); 
                        } else{
                            break;
                        }
                        if (httpdFindArg(connData->post.buff, "start_day", buf, sizeof(buf)) != -1){
                            strcat(date, buf);
                            strcat(date, " "); 
                        } else{
                            break;
                        }
                        if (httpdFindArg(connData->post.buff, "end_mon", buf, sizeof(buf)) != -1){
                            strcat(date, buf);
                            strcat(date, "/"); 
                        } else{
                            break;
                        }
                        if (httpdFindArg(connData->post.buff, "end_week", buf, sizeof(buf)) != -1){
                            strcat(date, buf);
                            strcat(date, "/"); 
                        } else{
                            break;
                        }
                        if (httpdFindArg(connData->post.buff, "end_day", buf, sizeof(buf)) != -1){
                            strcat(date, buf); 
                        } else{
                            break;
                        }
                        CFG_set_dst_date(date);
                        break;
                    case MENU_ADMIN_TAB_LOCATION:
                        if (httpdFindArg(connData->post.buff, "latitude", buf, sizeof(buf)) != -1)
                            CFG_set_latitude(buf);
                        if (httpdFindArg(connData->post.buff, "longitude", buf, sizeof(buf)) != -1)
                            CFG_set_longitude(buf);
                        break;
                    case MENU_ADMIN_TAB_SYSTEM:
                        break;
                    case MENU_ADMIN_TAB_DEBUG:
                        if (httpdFindArg(connData->post.buff, "debug_enable", buf, sizeof(buf)) != -1)
                            mode = ESP_LOG_INFO;
                        else
                            mode = ESP_LOG_NONE;
                        if (httpdFindArg(connData->post.buff, "ip", buf, sizeof(buf)) != -1)
                            strncpy(host, buf, sizeof(host) - 1);
                        if (httpdFindArg(connData->post.buff, "port", buf, sizeof(buf)) != -1)
                            port = strtol(buf, NULL, 10);
                        if (httpdFindArg(connData->post.buff, "debug_level", buf, sizeof(buf)) != -1)
                            level = strtol(buf, NULL, 10);
                        CFG_set_debug(mode, level, host, port);
                        if (mode) {
                            esp_log_level_set("*", level);
                            udp_logging_init(host, port, udp_logging_vprintf);
                            
                        } else {
                            esp_log_level_set("*", ESP_LOG_NONE);
                        }
                        break;
                    case MENU_ADMIN_TAB_BACKUP:
                        if (httpdFindArg(connData->post.buff, "backup", buf, sizeof(buf)) != -1) {
                        }
                        if (httpdFindArg(connData->post.buff, "restore", buf, sizeof(buf)) != -1) {
                        }
                        break;
                    case MENU_ADMIN_TAB_WATCHDOG:
                        if (httpdFindArg(connData->post.buff, "shutdown", buf, sizeof(buf)) != -1) {
                            //TODO: #3 shutdown
                            CFG_set_rtc_shutdown(strtol(buf, NULL, 10));
                            // rtc_set_shutdown(CFG_get_rtc_shutdown() * 3600);
                        }
                        break;
                }
                CFG_Save();
                break;
            case MENU_PROG:
                for (i = 0; i < CFG_SCHEDULE; i++) {
                    w1 = -1;
                    w2 = -1;
                    h1 = -1;
                    m1 = -1;
                    h2 = -1;
                    m2 = -1;
                    y1 = -1;
                    t1 = -1;
                    d1 = -1;
                    y2 = -1;
                    t2 = -1;
                    d2 = -1;
                    sprintf(key, "w%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            w1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "w%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            w2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "d%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            d1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "t%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            t1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "y%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            y1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "d%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            d2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "t%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            t2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "y%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            y2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "h%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            h1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "m%di", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            m1 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "h%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            h2 = strtol(buf, NULL, 10);
                    }
                    sprintf(key, "m%de", i + 1);
                    if (httpdFindArg(connData->post.buff, key, buf, sizeof(buf)) != -1) {
                        if (*buf != '\0')
                            m2 = strtol(buf, NULL, 10);
                    }
                    if (w1 != -1 && w2 != -1 && h1 != -1 && m1 != -1 &&
                        h2 != -1 && m2 != -1) {
                        sprintf(buf, "%d-%d %d:%d-%d:%d", w1, w2,
                                   h1, m1, h2, m2);
                        CFG_set_schedule(i, buf);
                    } else if (y1 != -1 && t1 != -1 && d1 != -1 && y2 != -1 &&
                               t2 != -1 && d2 != -1 && h1 != -1 && m1 != -1 &&
                               h2 != -1 && m2 != -1) {
                        sprintf(buf, "%d/%d/%d-%d/%d/%d %d:%d-%d:%d",
                                   y1, t1, d1, y2, t2, d2, h1, m1, h2, m2);
                        CFG_set_schedule(i, buf);
                    } else {
                        CFG_set_schedule(i, NULL);
                    }
                }
                CFG_Save();
                break;
        }
    }

    switch (menuopt) {
        case MENU_STATUS:
#if !defined(CONFIG__MLI_1WRP_TYPE__)
            if (httpdFindArg(connData->post.buff, "stat_Alarm", buf, sizeof(buf)) != -1) {
                if (ctl_alarm_status() == CTL_ALARM_ON)
                    ctl_alarm_off();
                else
                    ctl_alarm_on();
            } else if (httpdFindArg(connData->post.buff, "stat_Panic", buf, sizeof(buf)) != -1) {
                if (ctl_panic_status() == CTL_PANIC_ON)
                    ctl_panic_off();
                else
                    ctl_panic_on();
            } else if (httpdFindArg(connData->post.buff, "stat_Open", buf, sizeof(buf)) != -1) {
                ctl_relay_on(CFG_get_control_timeout());
            } else if (httpdFindArg(connData->post.buff, "stat_Close", buf, sizeof(buf)) != -1) {
                ctl_relay_off();
            }
#else
            if (httpdFindArg(connData->post.buff, "stat_Open", buf, sizeof(buf)) != -1) {
                ctl_relay_on(0);
            } else if (httpdFindArg(connData->post.buff, "stat_Close", buf, sizeof(buf)) != -1) {
                ctl_relay_off();
            } else if (httpdFindArg(connData->post.buff, "stat_AuxOpen", buf, sizeof(buf)) != -1) {
                ctl_relay_ext_on(CFG_get_control_timeout());
            } else if (httpdFindArg(connData->post.buff, "stat_AuxClose", buf, sizeof(buf)) != -1) {
                ctl_relay_ext_off();
            } else if (httpdFindArg(connData->post.buff, "stat_Reset", buf, sizeof(buf)) != -1) {
                CFG_set_energy_daily(0);
                CFG_set_energy_daily_last(0);
                CFG_set_energy_monthly(0);
                CFG_set_energy_monthly_last(0);
                CFG_set_energy_total(0);
                for (i = 0; i < 12; i++)
                    CFG_set_energy_month(i, 0);
                CFG_Save();
            }
#endif
            break;
        case MENU_USER:
            if (httpdFindArg(connData->post.buff, "user_Add", buf, sizeof(buf)) != -1) {
                subopt = account_db_get_empty();
                sprintf(buf, "/?menuopt=%d&subopt=%d", menuopt, subopt);
                httpdRedirect(connData, buf);
            } else if (httpdFindArg(connData->post.buff, "user_Det", buf, sizeof(buf)) != -1) {
                ulip_core_probe_user(!ulip_core_probe_status());
            } else if (httpdFindArg(connData->post.buff, "user_Erase", buf, sizeof(buf)) != -1) {
                ulip_core_erase_user(!ulip_core_erase_status());
            } else if (httpdFindArg(connData->post.buff, "user_Clean", buf, sizeof(buf)) != -1) {
                account_db_remove_all();
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
                fpm_delete_all();
#endif
            } else if (httpdFindArg(connData->post.buff, "user_Search", buf, sizeof(buf)) != -1) {
                httpdFindArg(connData->post.buff, "filter", buf, sizeof(buf));
                index = account_db_find(buf, buf, buf, buf, buf, NULL, NULL);
                if (index != -1) {
                    ESP_LOGI("CGI", "Found account [%d]", index);
                    sprintf(buf, "/?menuopt=%d&subopt=%d", menuopt, index);
                    httpdRedirect(connData, buf);
                    return HTTPD_CGI_DONE;
                }
            } else if (httpdFindArg(connData->post.buff, "user_First", buf, sizeof(buf)) != -1) {
                /* Previous */
                subopt = account_db_get_first();
                ESP_LOGD("CGI", "First account [%d]", subopt);
                sprintf(buf, "/?menuopt=%d&subopt=%d", menuopt, subopt);
                httpdRedirect(connData, buf);
            } else if (httpdFindArg(connData->post.buff, "user_Prev", buf, sizeof(buf)) != -1) {
                /* First */
                subopt = account_db_get_previous(subopt);
                ESP_LOGD("CGI", "Previous account [%d]", subopt);
                sprintf(buf, "/?menuopt=%d&subopt=%d", menuopt, subopt);
                httpdRedirect(connData, buf);
            } else if (httpdFindArg(connData->post.buff, "user_Next", buf, sizeof(buf)) != -1) {
                /* Next */
                subopt = account_db_get_next(subopt);
                ESP_LOGD("CGI", "Next account [%d]", subopt);
                sprintf(buf, "/?menuopt=%d&subopt=%d", menuopt, subopt);
                httpdRedirect(connData, buf);
                return HTTPD_CGI_DONE;
            } else if (httpdFindArg(connData->post.buff, "user_Last", buf, sizeof(buf)) != -1) {
                /* Last */
                subopt = account_db_get_last();
                ESP_LOGD("CGI", "Last account [%d]", subopt);
                sprintf(buf, "/?menuopt=%d&subopt=%d", menuopt, subopt);
                httpdRedirect(connData, buf);
            } else if (httpdFindArg(connData->post.buff, "user_Del", buf, sizeof(buf)) != -1) {
                if (subopt != -1) {
                    ESP_LOGI("CGI", "Delete account [%d]", subopt);
                    account_db_delete(subopt);
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
                    fpm_delete_template(subopt);
#endif
                    /* First */
                    subopt = account_db_get_first(); 
                }
            } else if (httpdFindArg(connData->post.buff, "user_Finger", buf, sizeof(buf)) != -1) {
#if defined(CONFIG__MLI_1WB_TYPE__) || defined(CONFIG__MLI_1WQB_TYPE__)
                if (subopt != -1) {
                    ESP_LOGI("CGI", "Account [%d] capture finger", subopt);
                    ulip_core_capture_finger(!ulip_core_capture_finger_status(), subopt);
                }
#endif
            }
            break;
        case MENU_LOG:
            if (httpdFindArg(connData->post.buff, "log_Remove", buf, sizeof(buf)) != -1) {
                ESP_LOGI("CGI", "Remove access log");
#if !defined(CONFIG__MLI_1WRS_TYPE__) && !defined(CONFIG__MLI_1WLS_TYPE__) && \
    !defined(CONFIG__MLI_1WRG_TYPE__) && !defined(CONFIG__MLI_1WLG_TYPE__) && \
    !defined(CONFIG__MLI_1WRP_TYPE__) && !defined(CONFIG__MLI_1WRC_TYPE__) && \
    !defined(CONFIG__MLI_1WLC_TYPE__)
                ulip_core_log_remove();
#else
                ulip_core_telemetry_remove();
#endif
            }
            break;
        case MENU_CONTROL:
#if defined(CONFIG__MLI_1WRC_TYPE__) || defined(CONFIG__MLI_1WLC_TYPE__)
            if (httpdFindArg(connData->post.buff, "cli_cal", buf, sizeof(buf)) != -1) {
                uint16_t value = cli_set_calibration();
                if (value && value != CFG_get_cli_cal()) {
                    CFG_set_cli_cal(value);
                    CFG_Save();
                }
            } else if (httpdFindArg(connData->post.buff, "cli_reset", buf, sizeof(buf)) != -1) {
                if (CFG_get_cli_cal()) {
                    cli_reset_calibration();
                    CFG_set_cli_cal(0);
                    CFG_Save();
                }
            }
#endif
            break;
    }

    if (subopt < 0)
        sprintf(buf, "/?menuopt=%d", menuopt);
    else
        sprintf(buf, "/?menuopt=%d&subopt=%d", menuopt, subopt);
       
    httpdRedirect(connData, buf);

    //All done.
    return HTTPD_CGI_DONE;
}


int ulip_cgi_process(HttpdInstance *pInstance, HttpdConnData *connData)
{
    int rc = HTTPD_CGI_DONE;

    if (connData->requestType == HTTPD_METHOD_GET) {
        ESP_LOGD("CGI", "GET handler");
        rc = ulip_cgi_get_handler(pInstance, connData);
    } else if (connData->requestType == HTTPD_METHOD_POST) {
        ESP_LOGD("CGI", "POST handler");
        rc = ulip_cgi_post_handler(connData);
    }

    return rc;
}
