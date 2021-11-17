#include "tty.h"
#include "esp_timer.h"
#include "rfid.h"
#include "esp_log.h"
#include "string.h"
#undef DEBUG
//constants

// PN532 header
#define PN532_PREAMBLE (0x00)
#define PN532_STARTCODE1 (0x00)
#define PN532_STARTCODE2 (0xFF)
#define PN532_POSTAMBLE (0x00)
#define PN532_HOSTTOPN532 (0xD4)
#define PN532_PN532TOHOST (0xD5)

// PN532 Frames
#define PN532_ACK (0x00FF)
#define PN532_NACK (0xFF00)

// PN532 Commands
#define PN532_COMMAND_DIAGNOSE (0x00)
#define PN532_COMMAND_GETFIRMWAREVERSION (0x02)
#define PN532_COMMAND_GETGENERALSTATUS (0x04)
#define PN532_COMMAND_READREGISTER (0x06)
#define PN532_COMMAND_WRITEREGISTER (0x08)
#define PN532_COMMAND_READGPIO (0x0C)
#define PN532_COMMAND_WRITEGPIO (0x0E)
#define PN532_COMMAND_SETSERIALBAUDRATE (0x10)
#define PN532_COMMAND_SETPARAMETERS (0x12)
#define PN532_COMMAND_SAMCONFIGURATION (0x14)
#define PN532_COMMAND_POWERDOWN (0x16)
#define PN532_COMMAND_RFCONFIGURATION (0x32)
#define PN532_COMMAND_RFREGULATIONTEST (0x58)
#define PN532_COMMAND_INJUMPFORDEP (0x56)
#define PN532_COMMAND_INJUMPFORPSL (0x46)
#define PN532_COMMAND_INLISTPASSIVETARGET (0x4A)
#define PN532_COMMAND_INATR (0x50)
#define PN532_COMMAND_INPSL (0x4E)
#define PN532_COMMAND_INDATAEXCHANGE (0x40)
#define PN532_COMMAND_INCOMMUNICATETHRU (0x42)
#define PN532_COMMAND_INDESELECT (0x44)
#define PN532_COMMAND_INRELEASE (0x52)
#define PN532_COMMAND_INSELECT (0x54)
#define PN532_COMMAND_INAUTOPOLL (0x60)
#define PN532_COMMAND_TGINITASTARGET (0x8C)
#define PN532_COMMAND_TGSETGENERALBYTES (0x92)
#define PN532_COMMAND_TGGETDATA (0x86)
#define PN532_COMMAND_TGSETDATA (0x8E)
#define PN532_COMMAND_TGSETMETADATA (0x94)
#define PN532_COMMAND_TGGETINITIATORCOMMAND (0x88)
#define PN532_COMMAND_TGRESPONSETOINITIATOR (0x90)
#define PN532_COMMAND_TGGETTARGETSTATUS (0x8A)

#define PN532_MIFARE_ISO14443A (0x00)

// Mifare Commands
#define MIFARE_CMD_AUTH_A (0x60)
#define MIFARE_CMD_AUTH_B (0x61)
#define MIFARE_CMD_READ (0x30)
#define MIFARE_CMD_WRITE (0xA0)
#define MIFARE_CMD_TRANSFER (0xB0)
#define MIFARE_CMD_DECREMENT (0xC0)
#define MIFARE_CMD_INCREMENT (0xC1)
#define MIFARE_CMD_STORE (0xC2)

// Prefixes for NDEF Records (to identify record type)
#define NDEF_URIPREFIX_NONE (0x00)
#define NDEF_URIPREFIX_HTTP_WWWDOT (0x01)
#define NDEF_URIPREFIX_HTTPS_WWWDOT (0x02)
#define NDEF_URIPREFIX_HTTP (0x03)
#define NDEF_URIPREFIX_HTTPS (0x04)
#define NDEF_URIPREFIX_TEL (0x05)
#define NDEF_URIPREFIX_MAILTO (0x06)
#define NDEF_URIPREFIX_FTP_ANONAT (0x07)
#define NDEF_URIPREFIX_FTP_FTPDOT (0x08)
#define NDEF_URIPREFIX_FTPS (0x09)
#define NDEF_URIPREFIX_SFTP (0x0A)
#define NDEF_URIPREFIX_SMB (0x0B)
#define NDEF_URIPREFIX_NFS (0x0C)
#define NDEF_URIPREFIX_FTP (0x0D)
#define NDEF_URIPREFIX_DAV (0x0E)
#define NDEF_URIPREFIX_NEWS (0x0F)
#define NDEF_URIPREFIX_TELNET (0x10)
#define NDEF_URIPREFIX_IMAP (0x11)
#define NDEF_URIPREFIX_RTSP (0x12)
#define NDEF_URIPREFIX_URN (0x13)
#define NDEF_URIPREFIX_POP (0x14)
#define NDEF_URIPREFIX_SIP (0x15)
#define NDEF_URIPREFIX_SIPS (0x16)
#define NDEF_URIPREFIX_TFTP (0x17)
#define NDEF_URIPREFIX_BTSPP (0x18)
#define NDEF_URIPREFIX_BTL2CAP (0x19)
#define NDEF_URIPREFIX_BTGOEP (0x1A)
#define NDEF_URIPREFIX_TCPOBEX (0x1B)
#define NDEF_URIPREFIX_IRDAOBEX (0x1C)
#define NDEF_URIPREFIX_FILE (0x1D)
#define NDEF_URIPREFIX_URN_EPC_ID (0x1E)
#define NDEF_URIPREFIX_URN_EPC_TAG (0x1F)
#define NDEF_URIPREFIX_URN_EPC_PAT (0x20)
#define NDEF_URIPREFIX_URN_EPC_RAW (0x21)
#define NDEF_URIPREFIX_URN_EPC (0x22)
#define NDEF_URIPREFIX_URN_NFC (0x23)

#define PN532_GPIO_VALIDATIONBIT (0x80)
#define PN532_GPIO_P30 (0)
#define PN532_GPIO_P31 (1)
#define PN532_GPIO_P32 (2)
#define PN532_GPIO_P33 (3)
#define PN532_GPIO_P34 (4)
#define PN532_GPIO_P35 (5)

#define LLCP_DEFAULT_DSAP 0x04
#define LLCP_DEFAULT_SSAP 0x20

// LLCP PDU Type Values
#define LLCP_PDU_SYMM 0x00
#define LLCP_PDU_PAX 0x01
#define LLCP_PDU_CONNECT 0x04
#define LLCP_PDU_DISC 0x05
#define LLCP_PDU_CC 0x06
#define LLCP_PDU_DM 0x07
#define LLCP_PDU_I 0x0c
#define LLCP_PDU_RR 0x0d

// SNEP
#define SNEP_DEFAULT_VERSION 0x10 // Major: 1, Minor: 0
#define SNEP_REQUEST_PUT 0x02
#define SNEP_REQUEST_GET 0x01
#define SNEP_RESPONSE_SUCCESS 0x81
#define SNEP_RESPONSE_REJECT 0xFF

// LLCP state
#define LLCP_DISCONNECTED 0
#define LLCP_ACTIVATE 1
#define LLCP_WAIT_CONNECTION 2
#define LLCP_CONNECTED 3

// LLCP timeout
#define LLCP_TIMEOUT 5000

#define RFID_TTY 0
#define RFID_TIMEOUT 1000 /* msec */
#define RFID_BFSIZE 128
#define RFID_CARDSIZE 64
#define RFID_NUM_STATUS 3


typedef union
{
    unsigned char *b;
    unsigned short *w;
    unsigned long *dw;
} pgen_t;

static bool rfid_configured = false;
static int rfid_init_stage = 0;
static esp_timer_handle_t rfid_timer;
static rfid_handler_t rfid_func = NULL;
static void *rfid_user_data = NULL;
static uint8_t rfid_buf[RFID_BFSIZE];
static int rfid_buflen = 0;
static uint8_t rfid_card_baudrate = PN532_MIFARE_ISO14443A;
static int rfid_timeout = RFID_TIMEOUT;
static uint8_t rfid_retries = 0;
static bool rfid_nfc = false;
static int rfid_panic_timeout = 0;
static int rfid_format = RFID_BE_FORMAT;
static char rfid_card[RFID_CARDSIZE];
static int64_t rfid_timestamp;
static int64_t rfid_panic_timestamp = 0;
static int rfid_status = 0;

/* LLCP */
static int llcp_state = LLCP_DISCONNECTED;
static int llcp_timeout = 0;

static void rfid_module_initialize(int stage);

static int rfid_send_command(const uint8_t *buf, int len);

static uint8_t llcp_getPType(const uint8_t *buf)
{
    return ((buf[0] & 0x3) << 2) | ((buf[1] & 0xC0) >> 6);
}

static uint8_t llcp_getSSAP(const uint8_t *buf)
{
    return buf[1] & 0x3f;
}

static uint8_t llcp_getDSAP(const uint8_t *buf)
{
    return buf[0] >> 2;
}

static int llcp_get_data(void)
{
    uint8_t cmd = PN532_COMMAND_TGGETDATA;
    int rc;

    rc = rfid_send_command(&cmd, 1);
    if (rc < 0)
        return -1;

    return 0;
}

static int llcp_activate(void)
{
    const uint8_t cmd[] = {
        PN532_COMMAND_TGINITASTARGET,
        0,
        0x00, 0x00,       // SENS_RES
        0x12, 0x34, 0x56, // NFCID1
        0x40,             // SEL_RES

        0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, // POL_RES
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF,

        0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, 0x00, 0x00, // NFCID3t: Change this to desired value

        0x06, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x10, 0x00 // LLCP magic number and version parameter
    };
    int rc;

    ESP_LOGI("RFID", "LLCP activate");

    rc = rfid_send_command(cmd, sizeof(cmd));
    if (rc < 0)
        return -1;

    llcp_state = LLCP_ACTIVATE;
    llcp_timeout = LLCP_TIMEOUT << 1;

    return 0;
}

static int llcp_wait_connection(void)
{
    int rc;

    ESP_LOGI("RFID", "LLCP wait connection");

    rc = llcp_get_data();
    if (rc)
        return -1;

    llcp_state = LLCP_WAIT_CONNECTION;
    llcp_timeout = LLCP_TIMEOUT;

    return 0;
}

static int llcp_symmetry(uint8_t *data, int len)
{
    uint8_t cmd[] = {PN532_COMMAND_TGSETDATA, 0, 0};
    int ssap;
    int dsap;
    int rc;


    ESP_LOGI("RFID", "LLCP symmetry");


    /* Send SYMM */
    rc = rfid_send_command(cmd, 3);
    if (rc < 0)
        return -1;

    llcp_timeout = LLCP_TIMEOUT;

    return 0;
}

static int llcp_connect(uint8_t *data, int len)
{
    uint8_t cmd[8];
    uint8_t ssap;
    uint8_t dsap;
    int rc;

    /* Invalid PDU length */
    if (len < 2)
        return -1;

    ESP_LOGI("RFID", "LLCP connected");

    ssap = llcp_getDSAP(data);
    dsap = llcp_getSSAP(data);
    cmd[0] = PN532_COMMAND_TGSETDATA;
    cmd[1] = (dsap << 2) | ((LLCP_PDU_CC >> 2) & 0x3);
    cmd[2] = ((LLCP_PDU_CC & 0x3) << 6) | ssap;
    rc = rfid_send_command(cmd, 3);
    if (rc < 0)
        return -1;

    llcp_state = LLCP_CONNECTED;
    llcp_timeout = LLCP_TIMEOUT;

    return 0;
}

static int llcp_read(uint8_t *data, int len)
{
    uint8_t ssap;
    uint8_t dsap;
    unsigned char cmd[8];
    uint32_t size;
    uint8_t tnf_byte;
    int mb;
    int me;
    int cf;
    int sr;
    int il;
    uint8_t tnf;
    int typeLength;
    uint32_t type = 0;
    int payloadLength;
    int idLength;
    uint8_t status;
    char lang[32];
    char key[256];
    int rc;
    int i;

    /* Invalid PDU length */
    if (len < 3)
        return -1;

    ssap = llcp_getDSAP(data);
    dsap = llcp_getSSAP(data);
    cmd[0] = PN532_COMMAND_TGSETDATA;
    cmd[1] = (dsap << 2) + (LLCP_PDU_RR >> 2);
    cmd[2] = ((LLCP_PDU_RR & 0x3) << 6) + ssap;
    cmd[3] = (data[2] >> 4) + 1;
    rc = rfid_send_command(cmd, 4);
    if (rc < 0)
        return -1;
    data += 3;
    len -= 3;
    /* Invalid SNEP length */
    if (len < 6)
        return -1;

        /* SNEP Message */

    ESP_LOGD("RFID", "SNEP message len [%d]", len);


    if (data[0] != SNEP_DEFAULT_VERSION)
    {
        ESP_LOGW("RFID", "SNEP invalid version!");
        return -1;
    }
    if (data[1] != SNEP_REQUEST_PUT)
    {
        ESP_LOGW("RFID", "SNEP invalid request!");
        return -1;
    }
    size = (data[2] << 24) + (data[3] << 16) + (data[4] << 8) + data[5];
    // Length should not be more than 244 (header + body < 255, header = 6 + 3 + 2)
    if (size > len - 6)
    {
        ESP_LOGW("RFID", "SNEP message is too large");
        return -1;
    }
    data += 6;
    len -= 6;

    // NDEF Message
    // Decode TNF - first byte is TNF with bit flags
    // see the NFDEF spec for more info
    i = 0;
    tnf_byte = data[i++];
    mb = (tnf_byte & 0x80) != 0;
    me = (tnf_byte & 0x40) != 0;
    cf = (tnf_byte & 0x20) != 0;
    sr = (tnf_byte & 0x10) != 0;
    il = (tnf_byte & 0x8) != 0;
    tnf = (tnf_byte & 0x7);
    typeLength = data[i++];
    payloadLength = 0;
    if (sr)
        payloadLength = data[i++];
    else {
        payloadLength = ((0xFF & data[i]) << 24) |
                        ((0xFF & data[i+1]) << 16) |
                        ((0xFF & data[i+2]) << 8) |
                        (0xFF & data[i+3]);
        i+=4;
    }
    idLength = 0;
    if (il)
        idLength = data[i++];
    /* Type */
    memcpy(&type, &data[i], typeLength);
    i += typeLength;
    if (il)
    {
        i += idLength;
    }
    /* Payload */
    data += i;
    len = payloadLength;

    ESP_LOGI("RFID", "SNEP TNF [%d] RTD [%d] payload size [%d]",
            tnf, type, len);

    rfid_timestamp = esp_timer_get_time();
    /* RTD Text */
    rc = -1;
    if (type == 'T')
    {
        /* Status */
        status = data[0];
        i = status & 0x3F;
        /* Language */
        memcpy(lang, &data[1], i);
        lang[i++] = '\0';
        data += i;
        len -= i;
        if (len < sizeof(key))
        {
            memcpy(key, data, len);
            key[len] = '\0';
            if (rfid_func)
                rc = rfid_func(RFID_EVT_NFC, key, len,
                               rfid_user_data);
        }
        else
        {
            ESP_LOGD("RFID", "SNEP invalid RTD size!");
        }
    }
    else
    {
        ESP_LOGD("RFID", "SNEP invalid RTD!");
    }

    /* Response SNEP message */
    cmd[0] = PN532_COMMAND_TGSETDATA;
    cmd[1] = SNEP_DEFAULT_VERSION;
    cmd[2] = !rc ? SNEP_RESPONSE_SUCCESS : SNEP_RESPONSE_REJECT;
    cmd[3] = 0;
    cmd[4] = 0;
    cmd[5] = 0;
    cmd[6] = 0;
    rfid_send_command(cmd, 7);

    return 0;
}

static void rfid_event(int tty, const char *event,
                       int len, void *user_data)
{
    char card[RFID_CARDSIZE];
    int64_t now;
    uint64_t serial;
    uint16_t cmd;
    uint8_t size;
    uint8_t crc;
    int64_t d;
    uint8_t *data;
    uint8_t type;
    uint8_t status;
    uint8_t error;
    pgen_t p;
    int rc;
    int i;

    ESP_LOGI("RFID", "RFID read [%d] bytes len [%d]",
             len, rfid_buflen);


    if (rfid_buflen + len > RFID_BFSIZE)
    {
        ESP_LOGW("RFID", "RFID buffer overflow");

        rfid_buflen = 0;
        return;
    }
    memcpy(rfid_buf + rfid_buflen, event, len);
    rfid_buflen += len;

    /* Check packet size */
    if (rfid_buflen < 6)
        return;

    p.b = rfid_buf;
    while (rfid_buflen >= 6)
    {
        /* Check header */
        if (p.b[0] != PN532_PREAMBLE ||
            p.b[1] != PN532_STARTCODE1 ||
            p.b[2] != PN532_STARTCODE2)
        {
            ESP_LOGW("RFID", "Invalid header!");

            rfid_buflen = 0;
            break;
        }
        p.b += 3;
        size = *p.b++;
        crc = *p.b++;
        /* Check if ACK / NACK */
        cmd = size << 8 | crc;
        /* Checksum */
        crc += size;
        if (cmd == PN532_ACK)
        {
            if (*p.b++ == PN532_POSTAMBLE)
            {
                ESP_LOGD("RFID", "ACK frame");

                rfid_status = 0;
                rfid_buflen -= 6;
                continue;
            }
            else
            {
                ESP_LOGW("RFID", "Invalid frame!");

                rfid_buflen = 0;
                break;
            }
        }
        else if (cmd == PN532_NACK)
        {
            if (*p.b++ == PN532_POSTAMBLE)
            {
                ESP_LOGD("RFID", "NACK frame");

                rfid_buflen -= 6;
                continue;
            }
            else
            {
                ESP_LOGW("RFID", "Invalid frame!");

                rfid_buflen = 0;
                break;
            }
        }
        else
        {
            if (crc == 0)
            {
                if (rfid_buflen < (size + 7))
                {
                    p.b -= 5;
                    break;
                }
                if (p.b[0] != PN532_PN532TOHOST)
                {
                    ESP_LOGW("RFID", "Invalid command!");

                    rfid_buflen = 0;
                    break;
                }
                /* Checksum */
                crc = 0;
                for (i = 0; i < size; i++)
                    crc += p.b[i];
                crc += p.b[size];
                if (crc != 0)
                {
                    ESP_LOGW("RFID", "Checksum error!");

                    rfid_buflen = 0;
                    break;
                }
                /* Postamble */
                if (p.b[size + 1] != 0)
                {
                    ESP_LOGW("RFID", "Invalid frame!");
                    rfid_buflen = 0;
                    break;
                }
                cmd = p.b[1];
                len = size - 2;
                cmd &= 0xfe;
                switch (cmd)
                {
                case PN532_COMMAND_SAMCONFIGURATION:
                    ESP_LOGI("RFID", "SAM configuration response");
                    rfid_module_initialize(++rfid_init_stage);
                    break;
                case PN532_COMMAND_RFCONFIGURATION:
                    ESP_LOGI("RFID", "RF configuration response");
                    rfid_module_initialize(++rfid_init_stage);
                    break;
                /* MIFARE Classic */
                case PN532_COMMAND_INLISTPASSIVETARGET:
                    if (len < 10)
                    {
                        /* Timeout */
                        break;
                    }
                    len = p.b[7];
                    data = p.b + 8;
                    serial = 0;
                    if (rfid_format == RFID_BE_FORMAT)
                    {
                        for (i = 0; i < len; i++)
                        {
                            serial <<= 8;
                            serial |= data[i];
                        }
                    }
                    else
                    {
                        for (i = len - 1; i >= 0; i--)
                        {
                            serial <<= 8;
                            serial |= data[i];
                        }
                    }
                    sprintf(card, "%llu", serial);
                    /* Debounce */
                    now = esp_timer_get_time();
                    if (rfid_timestamp)
                    {
                        if (!strcmp(rfid_card, card))
                        {
                            d = (now - rfid_timestamp) / 1000;
                            if (d <= (rfid_timeout << 1))
                            {
                                ESP_LOGI("RFID", "Debounce card [%s]", card);
                                /* Panic */
                                if (rfid_panic_timestamp > 0)
                                {
                                    rfid_panic_timestamp -= d;
                                    if (rfid_panic_timestamp <= 0)
                                    {
                                        rc = rfid_func(RFID_EVT_CHECK, card, len,
                                                       rfid_user_data);
                                        if (!rc)
                                        {
                                            ESP_LOGI("RFID", "Panic card [%s]", card);
                                            rc = rfid_func(RFID_EVT_PANIC, card, len,
                                                           rfid_user_data);
                                        }
                                        rfid_panic_timestamp = 0;
                                    }
                                }
                                rfid_timestamp = now;
                                break;
                            }
                        }
                    }
                    strncpy(rfid_card, card, sizeof(rfid_card) - 1);
                    rfid_timestamp = now;
                    rfid_panic_timestamp = rfid_panic_timeout;
                    if (rfid_func)
                    {
                        if (rfid_nfc)
                        {
                            rc = rfid_func(RFID_EVT_CHECK, card, len,
                                           rfid_user_data);
                            if (rc)
                            {
                                /* LLCP */
                                llcp_activate();
                                break;
                            }
                        }
                        rc = rfid_func(RFID_EVT_MIFARE, card, len,
                                       rfid_user_data);
                    }
                    break;
                /* LLCP */
                case PN532_COMMAND_TGINITASTARGET:
                    llcp_wait_connection();
                    break;
                case PN532_COMMAND_TGGETDATA:
                    data = p.b + 2;
                    status = data[0];
                    error = status & 0x3F;
                    if (!error)
                    {
                        data += 1;
                        len -= 1;
                        type = llcp_getPType(data);
                        switch (type)
                        {
                        case LLCP_PDU_SYMM:
                            llcp_symmetry(data, len);
                            break;
                        case LLCP_PDU_CONNECT:
                            llcp_connect(data, len);
                            break;
                        case LLCP_PDU_CC:
                            break;
                        case LLCP_PDU_I:
                            llcp_read(data, len);
                            llcp_state = LLCP_DISCONNECTED;
                            llcp_timeout = 0;
                            /* Initialize module */
                            rfid_module_initialize(0);
                            break;
                        }
                    }
                    else
                    {
                        /* Error */
                        ESP_LOGD("RFID", "LLCP error [0x%02X]", error);
                        if (error == 0x25)
                        {
                            /* Invalid device state */
                            llcp_get_data();
                        }
                        else if (error == 0x29)
                        {
                            /* Released by its initiator */
                            switch (llcp_state)
                            {
                            case LLCP_WAIT_CONNECTION:
                            case LLCP_CONNECTED:
                                llcp_activate();
                                break;
                            default:
                                llcp_timeout = 0;
                                break;
                            }
                        }
                    }
                    break;
                case PN532_COMMAND_TGSETDATA:
                    switch (llcp_state)
                    {
                    case LLCP_WAIT_CONNECTION:
                    case LLCP_CONNECTED:
                        llcp_get_data();
                        break;
                    }
                    break;
                default:
                    ESP_LOGW("RFID", "Invalid command [0x%02X]", cmd);
                    break;
                }
                rfid_buflen -= size + 7;
                p.b += size + 1;
            }
            else
            {
                ESP_LOGW("RFID", "Invalid frame length!");
                rfid_buflen = 0;
            }
            break;
        }
    }
    if (rfid_buflen > 0)
    {
        if (p.b != rfid_buf)
            memcpy(rfid_buf, p.b, rfid_buflen);
    }
}

static int rfid_send_command(const uint8_t *buf, int len)
{
    uint8_t cmd[256];
    uint8_t size;
    uint8_t crc;
    pgen_t p;
    int i;

    ESP_LOGD("RFID", "Send command [%d] bytes", len);

    p.b = cmd;
    *p.b++ = PN532_PREAMBLE;
    *p.b++ = PN532_STARTCODE1;
    *p.b++ = PN532_STARTCODE2;
    size = len + 1; // Length of data field: TFI + DATA
    *p.b++ = size;
    *p.b++ = ~size + 1; // Checksum of length
    *p.b++ = PN532_HOSTTOPN532;
    crc = PN532_HOSTTOPN532;
    for (i = 0; i < len; i++)
    {
        *p.b++ = buf[i];
        crc += buf[i];
    }
    crc = ~crc + 1; // Checksum of TFI + DATA
    *p.b++ = crc;
    *p.b++ = PN532_POSTAMBLE;

    return tty_write(RFID_TTY, cmd, p.b - cmd);
}

static void rfid_sam_configuration(void)
{
    uint8_t cmd[] = {PN532_COMMAND_SAMCONFIGURATION, 0x01, 0x14, 0x01};

    rfid_send_command(cmd, sizeof(cmd));
}

static void rfid_rf_configuration(void)
{
    uint8_t cmd[] = {PN532_COMMAND_RFCONFIGURATION, 0x05, 0xFF, 0x01, rfid_retries};

    rfid_send_command(cmd, sizeof(cmd));
}

static void rfid_read_passive_card(uint8_t cardbaudrate)
{
    uint8_t cmd[] = {PN532_COMMAND_INLISTPASSIVETARGET, 0x01, cardbaudrate};

    rfid_send_command(cmd, sizeof(cmd));
}

static void rfid_module_initialize(int stage)
{
    uint8_t cmd[] = {0x55, 0x55, 0x00, 0x00, 0x00};

    ESP_LOGI("RFID", "RFID init stage [%d]", stage);

    switch (stage)
    {
    case 0:
        rfid_configured = false;
        rfid_status = 0;

        /* Wakeup */
        tty_write(RFID_TTY, cmd, sizeof(cmd));

        /* SAM configuration */
        rfid_sam_configuration();
        break;
    case 1:
        /* RF configuration */
        rfid_rf_configuration();
        break;
    default:
        rfid_configured = true;
        break;
    }
    rfid_init_stage = stage;
}

static void rfid_polling_timeout(void *data)
{
    int rc;

    if (llcp_state == LLCP_DISCONNECTED)
    {
        /* MIRAFE Classic */
        if (!rfid_configured)
        {
            rfid_module_initialize(0);
        }
        else
        {
            if (rfid_status++ >= RFID_NUM_STATUS)
            {
                ESP_LOGW("RFID", "RFID module restart");
                /* Initialize module */
                rfid_module_initialize(0);
                return;
            }
            rfid_read_passive_card(rfid_card_baudrate);
        }
    }
    else
    {
        /* LLCP */
        llcp_timeout -= rfid_timeout;
        if (llcp_timeout <= 0)
        {
            ESP_LOGI("RFID", "LLCP timeout");
            llcp_state = LLCP_DISCONNECTED;
            llcp_timeout = 0;
            /* Initialize module */
            rfid_module_initialize(0);
            if (rfid_func)
                rc = rfid_func(RFID_EVT_MIFARE, rfid_card,
                               strlen(rfid_card), rfid_user_data);
        }
    }
}

int rfid_init(int timeout, int retries, bool nfc, int panic_timeout,
              int format, rfid_handler_t func, void *user_data)
{
    ESP_LOGI("RFID", "Initialize RFID");

    /* Open tty */
    if (tty_open(RFID_TTY, rfid_event, NULL))
    {
        ESP_LOGW("RFID", "Failed to initialize RFID!");
        return -1;
    }
    rfid_func = func;
    rfid_user_data = user_data;
    rfid_timeout = timeout;
    rfid_retries = retries;
    rfid_nfc = nfc;
    rfid_panic_timeout = panic_timeout;
    rfid_format = format;
    memset(rfid_card, 0, sizeof(rfid_card));
    rfid_timestamp = 0;

    /* Initialize module */
    rfid_module_initialize(0);

    /* Start polling */
    if (!rfid_timeout)
        rfid_timeout = RFID_TIMEOUT;
    // timer_setfn(&rfid_timer, (timer_func_t *)rfid_polling_timeout, NULL);
    // timer_arm(&rfid_timer, rfid_timeout, true);
    const esp_timer_create_args_t rfid_timer_args = {
        .callback = &rfid_polling_timeout,
        /* name is optional, but may help identify the timer when debugging */
        .name = "rfid"};
    ESP_ERROR_CHECK(esp_timer_create(&rfid_timer_args, &rfid_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(rfid_timer, rfid_timeout*1000));
    return 0;
}

void rfid_release(void)
{
    tty_close(RFID_TTY);

    ESP_ERROR_CHECK(esp_timer_stop(rfid_timer));
    ESP_ERROR_CHECK(esp_timer_delete(rfid_timer));
    // timer_disarm(&rfid_timer);
    rfid_func = NULL;
    rfid_user_data = NULL;
    rfid_timestamp = 0;
}
