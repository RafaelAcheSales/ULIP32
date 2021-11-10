alias python=python3
$IDF_PATH/install.sh
. /home/rafael/esp/esp-idf/export.sh
# idf.py menuconfig
# idf.py openocd
idf.py create-component -C components DEBUG

