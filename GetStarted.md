= Ambiente de desenvolvimento ESP32 ULET para Linux =

Link para tutorial oficial da espressif [[https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#|aqui]]

=== Instruções iniciais ===

É recomendado utilizar uma IDE para desenvolver com ESP32 (Eclipse ou VsCode), pórem tambem é possivel apenas instalar as ferramentas e fazer o build e flash via idf.py, esptool e Cmake.

== Passo 1: Pré-requisitos ==

Alguns pacotes são necessarios para o ambiente de desenvolvimento funcionar, para instalá-los rode este comando:

Ubuntu e Debian:

'''sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 '''

CentOS:

'''sudo yum -y update && sudo yum install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache dfu-util libusbx'''

== Passo 2: Instale ESP-IDF ==

A instalação deve ser feita pela sequência de comandos explicitados abaixo:

'''mkdir -p ~/esp

cd ~/esp

git clone --recursive https://github.com/RafaelAcheSales/esp-idf.git

cd esp-idf

git switch -c test origin/rafa-version'''

== Passo 3: Baixe o projeto ULET ==

Baixe o projeto utilizando os seguintes comandos:

'''cd ~/esp

git clone https://github.com/RafaelAcheSales/ULIP32'''


== Passo 4: Instale a biblioteca esphttpd ==

Execute estes comandos:

 '''cd ~/esp/esp-idf/components'''
 
 '''git clone https://github.com/RafaelAcheSales/libesphttpd.git'''

 '''cd libesphttpd'''
 
 '''git switch -c test origin/rafa-version'''

== Passo 5: Instale ferramentas necessárias ==

Rode este comando para instalar as ferramentas utilizadas por ESP-IDF como compilador, debugger, etc

'''cd ~/esp/esp-idf

./install.sh'''

== Passo 6: Variáveis de ambiente ==

Antes de rodar qualquer comando é necessario exportar as variáveis de ambiente utilizando o comando abaixo:

'''. $HOME/esp/esp-idf/export.sh'''

Atenção! Este comando deve ser utilizado toda vez que um novo terminal for utilizar os comandos da espressif (idf.py, esptool, make, etc). Caso contrário irá surgir erros.

== Build ==

Agora é apenas apenas entrar na pasta do projeto ULIP32 e fazer o build via 

'''cd ~/esp/ULIP32'''

'''idf.py build''' 

== Flash ==

Entre na pasta do projeto

'''cd ~/esp/ULIP32'''

Inicie o flash utilizando:

'''idf.py -p PORT -b BAUD flash'''

Onde PORT é o endereço da porta USB que o ESP32 está conectado, por exemplo: /dev/ttyUSB0

BAUD é o valor do baudrate para realizar o flash (valor default: 115200)

Ou apenas copie e cole o comando que é disponibilizado ao final do '''idf.py build''' quando é bem sucedido

== Monitor ==

Para receber via UART os Logs é necessario rodar o seguinte comando:

'''idf.py -p PORT monitor'''


== Possíveis erros ==

 * Não conseguir rodar comandos de build ou flash pois o Passo 4 não foi respeitado
 * Esp32 não se comunica pois o sinal DTR está oscilando, para resolver bastar rodar o comando com o argumento '''--before no_reset'''
 

