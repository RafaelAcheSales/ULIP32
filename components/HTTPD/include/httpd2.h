#pragma once
void start_httpd(void);

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


static const char PAGE_TOP[] = {
"<!DOCTYPE html>\
<html>\
<head>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
<meta http-equiv=\"Content-Type\" content=\"text/html;charset=iso-8859-1\" >\
<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\"/>"
#ifdef __MLI_1W_TYPE__
"<TITLE>MLI-1W - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WQ_TYPE__)
"<TITLE>MLI-1WQ - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WQB_TYPE__)
"<TITLE>MLI-1WQB - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WQF_TYPE__)
"<TITLE>MLI-1WQF - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WB_TYPE__)
"<TITLE>MLI-1WB - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WF_TYPE__)
"<TITLE>MLI-1WF - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WRQ_TYPE__)
"<TITLE>MLI-1WRQ - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WR_TYPE__)
"<TITLE>MLI-1WR - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WRF_TYPE__)
"<TITLE>MLI-1WRF - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WRS_TYPE__)
"<TITLE>MLI-1WRS - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WLS_TYPE__)
"<TITLE>MLI-1WLS - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WRG_TYPE__)
"<TITLE>MLI-1WRG - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WLG_TYPE__)
"<TITLE>MLI-1WLG - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WRP_TYPE__)
"<TITLE>MLI-1WRP - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WRC_TYPE__)
"<TITLE>MLI-1WRC - &micro;Tech Tecnologia</TITLE>"
#elif defined(__MLI_1WLC_TYPE__)
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
#ifdef __MLI_1W_TYPE__
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1W - Leitor IP</b></a>"
#elif defined(__MLI_1WQ_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WQ - Leitor IP</b></a>"
#elif defined(__MLI_1WQB_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WQB - Leitor IP</b></a>"
#elif defined(__MLI_1WQF_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WQF - Leitor IP</b></a>"
#elif defined(__MLI_1WB_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WB - Leitor IP</b></a>"
#elif defined(__MLI_1WF_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WF - Leitor IP</b></a>"
#elif defined(__MLI_1WRQ_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRQ - Leitor IP</b></a>"
#elif defined(__MLI_1WR_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WR - Leitor IP</b></a>"
#elif defined(__MLI_1WRF_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRF - Leitor IP</b></a>"
#elif defined(__MLI_1WRS_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRS - Leitor IP</b></a>"
#elif defined(__MLI_1WLS_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WLS - Leitor IP</b></a>"
#elif defined(__MLI_1WRG_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRG - Leitor IP</b></a>"
#elif defined(__MLI_1WLG_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WLG - Leitor IP</b></a>"
#elif defined(__MLI_1WRP_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRP - Leitor IP</b></a>"
#elif defined(__MLI_1WRC_TYPE__)
"<a style=\"color: #696969;padding-left:20px;\" href=\"http://www.utech.com.br/\"><b>MLI-1WRC - Leitor IP</b></a>"
#elif defined(__MLI_1WLC_TYPE__)
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
#if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
    !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && \
    !defined(__MLI_1WRP_TYPE__) && !defined(__MLI_1WRC_TYPE__) && \
    !defined(__MLI_1WLC_TYPE__)
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

 
static const char INDEXREDE[] = {
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
<td colspan=\"2\" class=\"alert-info\"><b>Rede</b></td> \
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
<td colspan=\"2\">&nbsp;</td> \
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

 
static const char INDEXACIONAMENTO[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div class=\"panel panel-primary\" style=\"margin: 0 auto;\">\
<div class=\"panel-heading\">\
<h3 class=\"panel-title\"><b>Configura&ccedil;&otilde;es Gerais</b></h3>\
</div>\
<div class=\"panel-body\">\
<FORM name=\"CONTROL\" action=\"save\" method=\"POST\">\
<table border=\"0\" align=\"left\" width=\"98%\" height=\"100%\">"
#if defined(__MLI_1WRQ_TYPE__) || defined(__MLI_1WR_TYPE__) || \
    defined(__MLI_1WRF_TYPE__) || defined(__MLI_1WRS_TYPE__) || \
    defined(__MLI_1WLS_TYPE__) || defined(__MLI_1WRP_TYPE__) || \
    defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
"<script type=\"text/javascript\">\
function select_address(ff)\
{\
for(var i=0;i<30;i++)\
ff.options[i]=new Option(i+1,i+1);\
}\n\
</script>"
#endif
#if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__) || \
    defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
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
#if defined(__MLI_1WRP_TYPE__)
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
#if !defined(__MLI_1WRP_TYPE__)
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
#if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
    !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && \
    !defined(__MLI_1WRC_TYPE__) && !defined(__MLI_1WLC_TYPE__)
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
#if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
    !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && \
    !defined(__MLI_1WRC_TYPE__) && !defined(__MLI_1WLC_TYPE__)
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
#if !defined(__MLI_1WF_TYPE__) && !defined(__MLI_1WQF_TYPE__) && !defined(__MLI_1WRF_TYPE__) && \
    !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && !defined(__MLI_1WRG_TYPE__) && \
    !defined(__MLI_1WLG_TYPE__) && !defined(__MLI_1WRP_TYPE__) && !defined(__MLI_1WRC_TYPE__) && \
    !defined(__MLI_1WLC_TYPE__)
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
#if defined(__MLI_1WQ_TYPE__) || defined(__MLI_1WQB_TYPE__) || defined(__MLI_1WQF_TYPE__) || defined(__MLI_1WRQ_TYPE__)
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
#if defined(__MLI_1WF_TYPE__) || defined(__MLI_1WQF_TYPE__) || defined(__MLI_1WRF_TYPE__)
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
#if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
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
#if defined(__MLI_1WRQ_TYPE__) || defined(__MLI_1WR_TYPE__) || \
    defined(__MLI_1WRF_TYPE__) || defined(__MLI_1WRS_TYPE__) || \
    defined(__MLI_1WRP_TYPE__) || defined(__MLI_1WRC_TYPE__)
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
#if defined(__MLI_1WLS_TYPE__)
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
#if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__) || \
    defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
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
#if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__)
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
#if defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WLG_TYPE__)
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
#if defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
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
#if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__) || \
    defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WLG_TYPE__) || \
    defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
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
#if defined(__MLI_1WRP_TYPE__)
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

 
static const char INDEXHTTP[] = {
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

 
static const char INDEXUSER[] = {
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
#if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
    !defined(__MLI_1WRC_TYPE__) && !defined(__MLI_1WLC_TYPE__)
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
#if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
    !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && \
    !defined(__MLI_1WRP_TYPE__) && !defined(__MLI_1WRC_TYPE__) && \
    !defined(__MLI_1WLC_TYPE__)
"<tr>\
<td><b>Cart&atilde;o</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"card\" maxlength=\"32\"></td>\
</tr>"
#endif
#if defined(__MLI_1WQ_TYPE__) || defined(__MLI_1WQB_TYPE__) || defined(__MLI_1WQF_TYPE__) || defined(__MLI_1WRQ_TYPE__)
"<tr>\
<td><b>C&oacute;digo QR</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"code\" maxlength=\"128\"></td>\
</tr>"
#endif
#if defined(__MLI_1WF_TYPE__) || defined(__MLI_1WQF_TYPE__) || defined(__MLI_1WRF_TYPE__)
"<tr>\
<td><b>C&oacute;digo RF</b></td>\
<td><INPUT type=\"text\" class=\"form-control\" name=\"rfcode\" maxlength=\"16\"></td>\
</tr>"
#endif
#if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
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
#if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
    !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && \
    !defined(__MLI_1WRP_TYPE__) && !defined(__MLI_1WRC_TYPE__) && \
    !defined(__MLI_1WLC_TYPE__)
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

#if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
    !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && \
    !defined(__MLI_1WRP_TYPE__) && !defined(__MLI_1WRC_TYPE__) && \
    !defined(__MLI_1WLC_TYPE__)
 
static const char INDEXLOG[] = {
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
 
static const char INDEXLOG[] = {
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
#if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__)
"<td class=\"alert-info\" align=\"center\"><b>Hor&aacute;rio</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Temperatura</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Humidade</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Luminosidade</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Movimento</b></td>\
<td class=\"alert-info\" align=\"center\"><b>N&iacute;vel</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Volume</b></td>\
</tr>"
#elif defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WLG_TYPE__)
"<td class=\"alert-info\" align=\"center\"><b>Hor&aacute;rio</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Fuma&ccedil;a e G&aacute;s</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Movimento</b></td>\
<td class=\"alert-info\" align=\"center\"><b>N&iacute;vel</b></td>\
<td class=\"alert-info\" align=\"center\"><b>Volume</b></td>\
</tr>"
#elif defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
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

 
static const char INDEXSTATUS[] = {
"<tr valign=\"top\" height=\"100%\">\
<td valign=\"top\">\
<div class=\"panel panel-primary\" style=\"marign: 0 auto;\">\
<div class=\"panel-heading\">\
<h3 class=\"panel-title\"><b>Status</b></h3>\
</div>\
<div class=\"panel-body\">\
<FORM name=\"STATUS\" action=\"status\" method=\"POST\">\
<table border=\"0\" align=\"left\" width=\"98%\" height=\"100%\">"
#if !defined(__MLI_1WRP_TYPE__)
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
#if !defined(__MLI_1WRP_TYPE__)
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
#if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__) || \
    defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WRG_TYPE__) || \
    defined(__MLI_1WRP_TYPE__) || defined(__MLI_1WRC_TYPE__) || \
    defined(__MLI_1WLC_TYPE__)
"<tr> \
<td colspan=\"3\">&nbsp;</td>\
</tr><tr>\
<td colspan=\"3\">\
<table width=\"100%\">"
#if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__)
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
#elif defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WLG_TYPE__)
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
#elif defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
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
#elif defined(__MLI_1WRP_TYPE__)
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
#if defined(__MLI_1WRP_TYPE__)
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

 
static const char INDEXADMIN_UPDATE[] = {
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

 
static const char INDEXADMIN_RESET[] = {
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

 
static const char INDEXADMIN_SENHA[] = {
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

 
static const char INDEXADMIN_TIMEZONE[] = {
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

 
static const char INDEXADMIN_LOCATION[] = {
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

 
static const char INDEXADMIN_SYSTEM[] = {
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

 
static const char INDEXADMIN_DEBUG[] = {
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

 
static const char INDEXADMIN_BACKUP[] = {
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

 
static const char INDEXADMIN_WIFI[] = {
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


static const char INDEXADMIN_WATCHDOG[] = {
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


static const char INDEXPROG[] = {
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
