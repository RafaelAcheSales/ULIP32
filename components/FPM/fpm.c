

#include <sys/time.h>
#include <string.h>
#include "tty.h"
#include "ctl.h"
#include "stdbool.h"
#include "fpm.h"
#include "esp_timer.h"
#include "esp_log.h"

// #undef DEBUG
#define DEBUG 1

#define FPM_TTY                     1
#define FPM_BFSIZE                  1024
#define FPM_TIMEOUT                 1000000  /* usec */
#define FPM_SECURITY_LEVEL          3
#define FPM_TEMPSIZE                498


#define FPM5210_CMD_HEAD            0xAA55
#define FPM5210_DATA_HEAD           0xA55A
#define FPM5210_DEVICE_ID           0x0001
#define FPM5210_CMD_LEN             12
#define FPM5210_DATA_LEN            504

#define FPM5210_ACK                 0x30
#define FPM5210_NACK                0x31

#define NACK_TIMEOUT                0x1001  /* Obsolete, capture timeout */
#define NACK_INVALID_BAUDRATE       0x1002  /* Obsolete, Invalid serial baud rate */
#define NACK_INVALID_POS            0x1003  /* The specified ID is not between 0~Max */
#define NACK_IS_NOT_USED            0x1004  /* The specified ID is not used */
#define NACK_IS_ALREADY_USED        0x1005  /* The specified ID is already used */
#define NACK_COMM_ERR               0x1006  /* Communication Error */
#define NACK_VERIFY_FAILED          0x1007  /* 1:1 Verification Failure */
#define NACK_IDENTIFY_FAILED        0x1008  /* 1:N Identification Failure */
#define NACK_DB_IS_FULL             0x1009  /* The database is full */
#define NACK_DB_IS_EMPTY            0x100A  /* The database is empty */
#define NACK_TURN_ERR               0x100B  /* Obsolete, Invalid order of the enrollment 
                                               (The order was not as: EnrollStart ->
                                                Enroll1 -> Enroll2 -> Enroll3) */
#define NACK_BAD_FINGER             0x100C  /* Too bad fingerprint */
#define NACK_ENROLL_FAILED          0x100D  /* Enrollment Failure */
#define NACK_IS_NOT_SUPPORTED       0x100E  /* The specified command is not supported */
#define NACK_DEV_ERR                0x100F  /* Device Error, especially if Crypto-Chip is trouble */
#define NACK_CAPTURE_CANCELED       0x1010  /* Obsolete, The capturing is canceled */
#define NACK_INVALID_PARAM          0x1011  /* Invalid parameter */
#define NACK_FINGER_IS_NOT_PRESSED  0x1012  /* Finger is not pressed */
#define NACK_RAM_ERROR              0x1013  /* Memory setting fail */
#define NACK_TEMPLATE_CAPACITY_FULL 0X1014  /* Template capacity is full */
#define NAKC_COMMAND_NO_SUPPORT     0X1015  /* Function no support */

#define FPM5210_Open                0x0001  /* Open / Initialization */
#define FPM5210_Close               0x0002  /* Close / Termination */
#define FPM5210_UsbInternalCheck    0x0003  /* UsbInternalCheck / Check if the connected USB device is valid */
#define FPM5210_ChangeBaudrate      0x0004  /* ChangeBaudrate / Change UART baud rate */
#define FPM5210_CmosLed             0x0012  /* CmosLed / Control CMOS LED */
#define FPM5210_GetEnrollCount      0x0020  /* GetEnrollCount / Get enrolled fingerprint count */
#define FPM5210_CheckEnrolled       0x0021  /* CheckEnrolled / Check whether the specified ID is already enrolled */
#define FPM5210_EnrollStart         0x0022  /* EnrollStart / Start an enrollment */
#define FPM5210_Enroll1             0x0023  /* Enroll1 / Make 1st template for an enrollment */
#define FPM5210_Enroll2             0x0024  /* Enroll2 / Make 2nd template for an enrollment */
#define FPM5210_Enroll3             0x0025  /* Enroll3 / Make 3rd template for an enrollment, merge
                                               three templates into one template, save
                                               merged template to the database */
#define FPM5210_IsPressFinger       0x0026  /* IsPressFinger / Check if a finger is placed on the sensor */
#define FPM5210_DeleteID            0x0040  /* DeleteID / Delete the fingerprint with the specified ID */
#define FPM5210_DeleteAll           0x0041  /* DeleteAll / Delete all fingerprints from the database */
#define FPM5210_Verify              0x0050  /* Verify 1:1 / Verification of the capture fingerprint image with the specified ID */
#define FPM5210_Identify            0x0051  /* Identify 1:N / Identification of the capture fingerprint image with the database */
#define FPM5210_VerifyTemplate      0x0052  /* VerifyTemplate / 1:1 Verification of a fingerprint template with the specified ID */
#define FPM5210_IdentifyTemplate    0x0053  /* IdentifyTemplate / 1:N Identification of a fingerprint template with the database */
#define FPM5210_CaptureFinger       0x0060  /* CaptureFinger / Capture a fingerprint image(256x256) from the sensor  */
#define FPM5210_MakeTemplate        0x0061  /* MakeTemplate / Make template for transmission */
#define FPM5210_GetImage            0x0062  /* GetImage / Download the captured fingerprint image(256x256) */
#define FPM5210_GetRawImage         0x0063  /* GetRawImage / Capture & Download raw fingerprint image(320x240) */
#define FPM5210_GetTemplate         0x0070  /* GetTemplate / Download the template of the specified ID */
#define FPM5210_SetTemplate         0x0071  /* SetTemplate / Upload the template of the specified ID */
#define FPM5210_GetDatabaseStart    0x0072  /* GetDatabaseStart / Start database download, obsolete */
#define FPM5210_GetDatabaseEnd      0x0073  /* GetDatabaseEnd / End database download, obsolete */
#define FPM5210_SetSecurityLevel    0x00F0  /* SetSecurityLevel / Set Security Level */
#define FPM5210_GetSecurityLevel    0x00F1  /* GetSecurityLevel / Get Security Level */
#define FPM5210_Identify_Template2  0x00F4  /* Identify_Template2 / Identify of the capture fingerprint image with
                                               the specified template */
#define FPM5210_EnterStandbyMode    0x00F9  /* EnterStandbyMode / Enter Standby Mode (Low power mode) */

#define FPM_ENROLL_NONE             0
#define FPM_ENROLL_STAGE1           1
#define FPM_ENROLL_TAKEOFF1         2
#define FPM_ENROLL_STAGE2           3
#define FPM_ENROLL_TAKEOFF2         4 
#define FPM_ENROLL_STAGE3           5

#define FPM_COMMAND_TIMEOUT         3000
#define FPM_COMMAND_RETRIES         2
#define FPM_ENROLL_TIMEOUT          60000
#define FPM_IDENTIFY_RETRIES        2

#define FPM_CMD_BSIZE               32

typedef union {
    unsigned char *b;
    unsigned short *w;
    unsigned long *dw;
} pgen_t;

typedef struct {
    uint8_t cmd;
    uint32_t param;
    uint8_t *data;
    uint8_t respCmd;
    uint8_t respData;
    uint32_t timeout;
    uint8_t retries;
} fpm_cmd_t;

static bool fpm_configured = false;
static int fpm_init_stage = 0;
static fpm_handler_t fpm_func = NULL;
static void *fpm_user_data = NULL;
static uint8_t fpm_buf[FPM_BFSIZE];
static int fpm_buflen = 0;
static int fpm_timeout = FPM_TIMEOUT;
static int fpm_security_level = FPM_SECURITY_LEVEL;
static int fpm_identify_retries = FPM_IDENTIFY_RETRIES;
static esp_timer_handle_t fpm_timer;
static uint16_t fpm_identify_id = -1;
static uint32_t fpm_timestamp = 0;
static int fpm_enroll_stage = FPM_ENROLL_NONE;
static uint16_t fpm_enroll_id = -1;
static uint32_t fpm_enroll_timeout = 0;
static bool fpm_identify_finger = false;
static int fpm_identify_counter = 0;
static int fpm_led_status = 0;

static fpm_cmd_t fpm_cmd_buf[FPM_CMD_BSIZE];
static uint8_t fpm_cmd_head = 0;
static uint8_t fpm_cmd_tail = 0;


static uint16_t crc16(uint8_t *buf, int len)
{
    uint16_t crc = 0;

    while (len-- > 0)
        crc += *buf++;
    return crc;
}

static int fpm_write_command(uint16_t cmd, uint32_t param)
{
    uint8_t buf[16];
    uint16_t crc;
    pgen_t p;

    p.b = buf;
    *p.w++ = FPM5210_CMD_HEAD;
    *p.w++ = FPM5210_DEVICE_ID;
    *p.dw++ = param;
    *p.w++ = cmd;
    crc = crc16(buf, p.b - buf);
    *p.w++ = crc;

    return tty_write(FPM_TTY, buf, p.b - buf);
}

static int fpm_write_data(uint8_t *data)
{
    uint8_t buf[512];
    uint16_t crc;
    pgen_t p;

    p.b = buf;
    *p.w++ = FPM5210_DATA_HEAD;
    *p.w++ = FPM5210_DEVICE_ID;
    memcpy(p.b, data, FPM_TEMPSIZE);
    p.b += FPM_TEMPSIZE;
    crc = crc16(buf, p.b - buf);
    *p.w++ = crc;

    return tty_write(FPM_TTY, buf, p.b - buf);
}

static int fpm_send_command(uint16_t cmd, uint32_t param,
                            uint8_t *data)
{
    fpm_cmd_t *c;
    int tail;

#ifdef DEBUG
    ESP_LOGD("FPM", "Send command [0x%02x] parameter [0x%08x]",
             cmd, param);
#endif

    tail = (fpm_cmd_tail + 1) & (FPM_CMD_BSIZE - 1);
    if (tail == fpm_cmd_head) {
        ESP_LOGW("FPM", "Command buffer overflow!");
        return -1;
    }

    /* Insert command into queue */
    c = &fpm_cmd_buf[fpm_cmd_tail];
    c->cmd = cmd;
    c->param = param;
    if (data) {
        c->data = (uint8_t *)malloc(FPM_TEMPSIZE);
        if (!c->data) {
            return -1;
        }
        memcpy(c->data, data, FPM_TEMPSIZE);
    } else {
        c->data = NULL;
    }
    c->respCmd = 0;
    c->respData = 0;
    c->timeout = 0;

    if (fpm_cmd_head == fpm_cmd_tail) {
        fpm_write_command(cmd, param);
        c->timeout = FPM_COMMAND_TIMEOUT;
        c->retries = FPM_COMMAND_RETRIES;
    }
    fpm_cmd_tail = tail;

    return 0;
}

static void fpm_module_initialize(int stage)
{
    ESP_LOGI("FPM", "FPM init stage [%d]", stage);
    
    if (fpm_cmd_head != fpm_cmd_tail)
        return;

    switch (stage) {
        case 0:
            /* Initialize */
            fpm_send_command(FPM5210_Open, 0, NULL);
            break;
        case 1:
            /* Set security level */
            fpm_send_command(FPM5210_SetSecurityLevel,
                             fpm_security_level, NULL);
            break;
        case 2:
            /* Disable LED */
            fpm_send_command(FPM5210_CmosLed, 0, NULL);
            break;
        case 3:
        default:
            fpm_configured = 1;
            break;
    }
}

static void fpm_module_restart(void)
{
    ESP_LOGI("FPM", "FPM module restart");

    fpm_timestamp = 0;
    memset(fpm_cmd_buf, 0, sizeof(fpm_cmd_buf));
    fpm_cmd_head = fpm_cmd_tail = 0;
    fpm_buflen = 0;
    fpm_led_status = 0;
    fpm_identify_id = -1;
    fpm_enroll_stage = FPM_ENROLL_NONE;
    fpm_enroll_id = -1;
    fpm_enroll_timeout = 0;
    fpm_identify_finger = false;
    fpm_identify_counter = 0;
    fpm_configured = false;
    fpm_init_stage = 0;
    fpm_module_initialize(0);
}

static void fpm_event(int tty, const char *event,
                      int len, void *user_data)
{
    uint8_t temp[FPM_TEMPSIZE];
    uint32_t now;
    struct timeval tv_now;
    uint16_t head;
    uint16_t devid;
    uint32_t param;
    uint16_t ack;
    uint16_t crc;
    fpm_cmd_t *c;
    uint32_t d;
    int error;
    pgen_t p;
    int i;
   
#ifdef DEBUG
    ESP_LOGD("FPM", "FPM read [%d] bytes len [%d]",
             len, fpm_buflen);
#endif

    if (fpm_buflen + len > FPM_BFSIZE) {
        ESP_LOGW("FPM", "FPM buffer overflow");
        fpm_buflen = 0;
        return;
    }
    memcpy(fpm_buf + fpm_buflen, event, len);
    fpm_buflen += len;

    /* Check packet size */
    if (fpm_buflen < FPM5210_CMD_LEN) return;

    p.b = fpm_buf;
    while (fpm_buflen >= FPM5210_CMD_LEN) {
        /* Check header */
        head = p.b[0] | (p.b[1] << 8);
        if (head == FPM5210_CMD_HEAD) {
            /* Command */
            c = &fpm_cmd_buf[fpm_cmd_head];
            /* Device ID */
            devid = (p.b[2] | (p.b[3] << 8));
            /* Parameter */
            param = p.b[4];
            param |= ((uint32_t)p.b[5] << 8);
            param |= ((uint32_t)p.b[6] << 16);
            param |= ((uint32_t)p.b[7] << 24);
            /* Response */
            ack = p.b[8];
            ack |= (p.b[9] << 8);
            /* CRC check */
            crc = crc16(p.b, FPM5210_CMD_LEN - 2);
            p.b += FPM5210_CMD_LEN - 2;
            crc -= (p.b[0] | (p.b[1] << 8));
            if (crc) {
                ESP_LOGW("FPM", "CRC error!");
                fpm_buflen = 0;
                /* Retry write command */
                fpm_write_command(c->cmd, c->param);
                c->timeout = FPM_COMMAND_TIMEOUT;
                break;
            }
            p.b += 2;
            /* Check command response */
            if (c->data) {
                if (!c->respCmd) {
                    c->respCmd = ack;
                    if (ack == FPM5210_ACK) {
                        fpm_write_data(c->data);
                        fpm_buflen -= FPM5210_CMD_LEN;
                        break;
                    }
                } else {
                    c->respData = ack;
                }
                free(c->data);
                c->data = NULL;
            } else {
                c->respCmd = ack;
            }
            /* Send next command */
            ++fpm_cmd_head;
            fpm_cmd_head =  fpm_cmd_head & (FPM_CMD_BSIZE - 1);
            if (fpm_cmd_head != fpm_cmd_tail) {
                fpm_write_command(fpm_cmd_buf[fpm_cmd_head].cmd,
                                  fpm_cmd_buf[fpm_cmd_head].param);
                fpm_cmd_buf[fpm_cmd_head].timeout = FPM_COMMAND_TIMEOUT;
            }
            switch (ack) {
                case FPM5210_ACK:
#ifdef DEBUG
                    ESP_LOGD("FPM", "Command response ACK");
#endif
                    if (!fpm_configured) {
                        fpm_module_initialize(++fpm_init_stage);
                        break;
                    }
                    switch (c->cmd) {
                        case FPM5210_CmosLed:
                            ESP_LOGD("FPM", "FPM5210_CmosLed");
                            fpm_led_status = c->param;
                            break;
                        case FPM5210_IsPressFinger:
                            if (!param) {
                                /* Pressed */
                                switch (fpm_enroll_stage) {
                                    case FPM_ENROLL_NONE:
                                        break;
                                    case FPM_ENROLL_STAGE1:
                                    case FPM_ENROLL_STAGE2:
                                    case FPM_ENROLL_STAGE3:
                                        fpm_send_command(FPM5210_CaptureFinger,
                                                         1, NULL);
                                        break;
                                    case FPM_ENROLL_TAKEOFF1:
                                    case FPM_ENROLL_TAKEOFF2:
                                        fpm_send_command(FPM5210_IsPressFinger,
                                                         0, NULL);
                                        break;
                                }
                            } else {
                                /* Not pressed */    
                                switch (fpm_enroll_stage) {
                                    case FPM_ENROLL_NONE:
                                        /* Do nothing */
                                        break;
                                    case FPM_ENROLL_STAGE1:
                                    case FPM_ENROLL_STAGE2:
                                    case FPM_ENROLL_STAGE3:
                                        fpm_send_command(FPM5210_IsPressFinger,
                                                         0, NULL);
                                        break;
                                    case FPM_ENROLL_TAKEOFF1:
                                        fpm_enroll_stage = FPM_ENROLL_STAGE2;
                                        fpm_send_command(FPM5210_IsPressFinger,
                                                         0, NULL);
                                        ctl_beep(1);
                                        break;
                                    case FPM_ENROLL_TAKEOFF2:
                                        fpm_enroll_stage = FPM_ENROLL_STAGE3;
                                        fpm_send_command(FPM5210_IsPressFinger,
                                                         0, NULL);
                                        ctl_beep(1);
                                        break;
                                }
                            }
                            break;
                        case FPM5210_CaptureFinger:
                            switch (fpm_enroll_stage) {
                                case FPM_ENROLL_NONE:
                                    /* Check fingerprint */
                                    fpm_send_command(FPM5210_Identify,
                                                     0, NULL);
                                    break;
                                case FPM_ENROLL_STAGE1:
                                    fpm_send_command(FPM5210_Enroll1,
                                                     0, NULL);
                                    break;
                                case FPM_ENROLL_STAGE2:
                                    fpm_send_command(FPM5210_Enroll2,
                                                     0, NULL);
                                    break;
                                case FPM_ENROLL_STAGE3:
                                    fpm_send_command(FPM5210_Enroll3,
                                                     0, NULL);
                                    break;
                            }
                            break;
                        case FPM5210_Identify:
                            fpm_identify_finger = false;
                            fpm_identify_counter = 0;
                            /* Debounce */
                            gettimeofday(&tv_now, NULL);
                            now = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
                            if (fpm_timestamp) {
                                if (fpm_identify_id == param) {
                                    d = (now - fpm_timestamp) / 1000;
                                    if (d <= ((fpm_timeout << 1) + 1000)) {
                                        ESP_LOGD("FPM", "Debounce fingerprint");
                                        fpm_timestamp = now;
                                        break;
                                    }
                                }
                            }
                            fpm_identify_id = param; 
                            fpm_timestamp = now;
                            if (fpm_func)
                                fpm_func(FPM_EVT_GRANTED, fpm_identify_id,
                                         NULL, 0, FPM_ERR_NONE, fpm_user_data);
                            break;
                        case FPM5210_SetTemplate:
                            ESP_LOGD("FPM", "FPM5210_SetTemplate");
                            break;
                        case FPM5210_EnrollStart:
                            ESP_LOGI("FPM", "EnrollStart");
                            fpm_send_command(FPM5210_IsPressFinger,
                                             0, NULL);
                            ctl_beep(1);
                            break;
                        case FPM5210_Enroll1:
                            ESP_LOGI("FPM", "Enroll1");
                            fpm_enroll_stage = FPM_ENROLL_TAKEOFF1;
                            fpm_send_command(FPM5210_IsPressFinger,
                                             0, NULL);
                            ctl_beep(2);
                            break;
                        case FPM5210_Enroll2:
                            ESP_LOGI("FPM", "Enroll2");
                            fpm_enroll_stage = FPM_ENROLL_TAKEOFF2;
                            fpm_send_command(FPM5210_IsPressFinger,
                                             0, NULL);
                            ctl_beep(2);
                            break;
                        case FPM5210_Enroll3:
                            ESP_LOGI("FPM", "Enroll3");
                            fpm_enroll_stage = FPM_ENROLL_STAGE3;
                            /* Check template */
                            fpm_send_command(FPM5210_GetTemplate,
                                             fpm_enroll_id, NULL);
                            break;
                        case FPM5210_DeleteID:
                            ESP_LOGD("FPM", "DeleteID");
                            if (fpm_enroll_stage == FPM_ENROLL_STAGE1) {
                                /* Restart enrollment */
                                fpm_enroll_timeout = FPM_ENROLL_TIMEOUT;
                                param = fpm_enroll_id;
                                fpm_send_command(FPM5210_EnrollStart,
                                                 param, NULL);
                            }
                            break;
                        case FPM5210_DeleteAll:
                            ESP_LOGD("FPM", "DeleteAll");
                            break;
                    }
                    break;
                case FPM5210_NACK:
                    ESP_LOGD("FPM", "Command response NACK cmd [0x%02X] error [0x%04X]",
                             c->cmd, param);
                    switch (c->cmd) {
                        case FPM5210_CaptureFinger:
                            switch (fpm_enroll_stage) {
                                case FPM_ENROLL_NONE:
                                    fpm_identify_finger = false;
                                    fpm_identify_counter = 0;
                                    break;
                                case FPM_ENROLL_STAGE1:
                                case FPM_ENROLL_STAGE2:
                                case FPM_ENROLL_STAGE3:
                                    fpm_send_command(FPM5210_IsPressFinger,
                                                     0, NULL);
                                    break;
                            }
                            break;
                        case FPM5210_Identify:
                            fpm_identify_finger = false;
                            /* Check identify retries */
                            if (++fpm_identify_counter >= fpm_identify_retries) {
                                /* Debounce */
                                gettimeofday(&tv_now, NULL);
                                now = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
                                if (fpm_timestamp) {
                                    d = (now - fpm_timestamp) / 1000;
                                    if (d <= ((fpm_timeout << 1) + 1000)) {
                                        ESP_LOGD("FPM", "Debounce fingerprint");
                                        fpm_timestamp = now;
                                        break;
                                    }
                                }
                                fpm_timestamp = now;
                                /* Blocked */
                                if (fpm_func)
                                    fpm_func(FPM_EVT_BLOCKED, -1, NULL, 0,
                                             FPM_ERR_IDENTIFY, fpm_user_data);
                            }
                            break;
                        case FPM5210_EnrollStart:
                            if (param == NACK_IS_ALREADY_USED) {
                                fpm_send_command(FPM5210_DeleteID,
                                                 fpm_enroll_id, NULL);
                                break;
                            }
                        case FPM5210_Enroll1:
                        case FPM5210_Enroll2:
                        case FPM5210_Enroll3:
                            /* Enroll error */
                            fpm_enroll_stage = FPM_ENROLL_NONE;
                            fpm_enroll_id = -1;
                            fpm_enroll_timeout = 0;
                            /* Check enrollment error */
                            if (param < NACK_TIMEOUT)
                                error = FPM_ERR_DUPLICATED;
                            else
                                error = FPM_ERR_ENROLL;
                            if (fpm_func)
                                fpm_func(FPM_EVT_ENROLL, -1, NULL, 0,
                                         error, fpm_user_data);
                            break;
                    }
                    break;
                default:
                    break;
            }
            fpm_buflen -= FPM5210_CMD_LEN;
        } else if (head == FPM5210_DATA_HEAD) {
            if (fpm_buflen < FPM5210_DATA_LEN)
                break;
            crc = crc16(p.b, FPM5210_DATA_LEN - 2);
            memcpy(temp, p.b + 4, FPM_TEMPSIZE);
            p.b += FPM5210_DATA_LEN - 2;
            crc -= (p.b[0] | (p.b[1] << 8));
            p.b += 2;
            if (crc) {
                ESP_LOGW("FPM", "CRC error!");
                fpm_buflen = 0;
                if (fpm_func)
                    fpm_func(FPM_EVT_ENROLL, -1,
                             NULL, 0, FPM_ERR_ENROLL,
                             fpm_user_data);
            } else {
                fpm_buflen -= FPM5210_DATA_LEN;
                if (fpm_func)
                    fpm_func(FPM_EVT_ENROLL, fpm_enroll_id,
                             temp, FPM_TEMPSIZE, FPM_ERR_NONE,
                             fpm_user_data);
            }
            fpm_enroll_stage = FPM_ENROLL_NONE;
            fpm_enroll_id = -1;
            fpm_enroll_timeout = 0;
        } else {
            ESP_LOGW("FPM", "Invalid header [0x%04X]!", head);
            fpm_buflen = 0;
            /* Retry write command */
            if (fpm_cmd_head != fpm_cmd_tail) {
                c = &fpm_cmd_buf[fpm_cmd_head];
                fpm_write_command(c->cmd, c->param);
                c->timeout = FPM_COMMAND_TIMEOUT;
            }
            break;
        }
    }
    if (fpm_buflen > 0) {
        if (p.b != fpm_buf)
            memmove(fpm_buf, p.b, fpm_buflen);
    }
}

static void fpm_polling_timeout(void *data)
{
    static int stage = 0;
    int touch;

    /* Command timeout */
    if (fpm_cmd_head != fpm_cmd_tail) {
        fpm_cmd_t *c = &fpm_cmd_buf[fpm_cmd_head];
        if (c->timeout) {
            c->timeout -= fpm_timeout;
            if (c->timeout <= 0) {
                ESP_LOGW("FPM", "Command [0x%04X] timeout!", c->cmd);
                if (c->retries-- > 0) {
                    /* Retry write command */
                    fpm_write_command(c->cmd, c->param);
                    c->timeout = FPM_COMMAND_TIMEOUT;
                } else {
                    /* Module not ready */
                    fpm_module_restart();
                    return;
                }
            }
        }
    }

    if (!fpm_configured) {
        if (fpm_init_stage == stage)
            fpm_module_initialize(stage);
        stage = fpm_init_stage;
    } else {
        if (fpm_enroll_stage == FPM_ENROLL_NONE) {
            /* Check LED status */
            touch = ctl_fpm_get();
            if (touch) {
                /* Enable LED */
                if (fpm_led_status == 0) {
                    fpm_send_command(FPM5210_CmosLed, 1, NULL);
                    fpm_led_status = 1;
                }
                /* Identify fingerprint */
                if (!fpm_identify_finger) {
                    fpm_send_command(FPM5210_CaptureFinger,
                                     0, NULL);
                    fpm_identify_finger = true;
                }
            } else {
                /* Disable LED */
                if (fpm_led_status == 1) {
                    fpm_send_command(FPM5210_CmosLed, 0, NULL);
                    fpm_led_status = 0;
                }
                fpm_identify_finger = false;
                fpm_identify_counter = 0;
            }
        } else {
            /* Enroll */
            fpm_enroll_timeout -= fpm_timeout;
            if (fpm_enroll_timeout <= 0) {
                fpm_enroll_stage = FPM_ENROLL_NONE;
                fpm_enroll_id = -1;
                fpm_enroll_timeout = 0;
                if (fpm_func)
                    fpm_func(FPM_EVT_ENROLL, -1,
                             NULL, 0, FPM_ERR_TIMEOUT,
                             fpm_user_data);
            }
        }
    }
}

int fpm_init(int timeout, int security, int identify_retries,
             fpm_handler_t func, void *user_data)
{
    ESP_LOGI("FPM", "Initialize FPM");
    
    /* Open tty */
    if (tty_open(FPM_TTY, fpm_event, NULL)) {
        ESP_LOGW("FPM", "Failed to initialize FPM!");
        return -1;
    }
    fpm_timeout = timeout;
    fpm_security_level = security ?
                         security : FPM_SECURITY_LEVEL;
    fpm_identify_retries = identify_retries ?
                           identify_retries : FPM_IDENTIFY_RETRIES;
    fpm_func = func;
    fpm_user_data = user_data;
    fpm_timestamp = 0;
    memset(fpm_cmd_buf, 0, sizeof(fpm_cmd_buf));
    fpm_cmd_head = fpm_cmd_tail = 0;
    fpm_led_status = 0;

    /* Initialize module */
    fpm_module_initialize(0);

    /* Start polling */
    if (!fpm_timeout)
        fpm_timeout = FPM_TIMEOUT;

    const esp_timer_create_args_t fpm_timer_args = {
            .callback = &fpm_polling_timeout,
            /* name is optional, but may help identify the timer when debugging */
            .name = "fpm polling timeout"
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&fpm_timer_args, &fpm_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(fpm_timer, fpm_timeout));
    // os_timer_setfn(&fpm_timer, (os_timer_func_t *)fpm_polling_timeout, NULL);
    // os_timer_arm(&fpm_timer, fpm_timeout, true);

    return 0;
}

void fpm_release(void)
{
    tty_close(FPM_TTY);
    ESP_ERROR_CHECK(esp_timer_stop(fpm_timer));
    fpm_timeout = FPM_TIMEOUT;
    fpm_security_level = FPM_SECURITY_LEVEL;
    fpm_func = NULL;
    fpm_user_data = NULL;
    fpm_timestamp = 0;
    fpm_configured = false;
    fpm_init_stage = 0;
}

int fpm_set_enroll(uint16_t index)
{
    uint32_t param;

    /* Not configured */
    if (!fpm_configured) return -1;

    if (fpm_enroll_stage != FPM_ENROLL_NONE)
        return -1;

    ESP_LOGI("FPM", "Set enroll index [%d]", index);
    
    fpm_enroll_stage = FPM_ENROLL_STAGE1;
    fpm_enroll_id = index;
    fpm_enroll_timeout = FPM_ENROLL_TIMEOUT;
    /* Enable LED */
    if (fpm_led_status == 0)
        fpm_send_command(FPM5210_CmosLed, 1, NULL);
    param = index;
    fpm_send_command(FPM5210_EnrollStart, param, NULL);

    return 0;
}

int fpm_get_enroll(void)
{
    /* Not configured */
    if (!fpm_configured) return -1;

    if (fpm_enroll_stage == FPM_ENROLL_NONE)
        return -1;

    return fpm_enroll_id;
}

int fpm_cancel_enroll(void)
{
    /* Not configured */
    if (!fpm_configured) return -1;

    if (fpm_enroll_stage == FPM_ENROLL_NONE)
        return -1;

    fpm_enroll_stage = FPM_ENROLL_NONE;
    fpm_enroll_id = -1;
    fpm_enroll_timeout = 0;

    return 0;
}

int fpm_set_template(uint16_t index, uint8_t *data)
{
    int rc;

    /* Not configured */
    if (!fpm_configured) return -1;

    if (!data) return -1;

    rc = fpm_send_command(FPM5210_SetTemplate, index, data);
    
    return rc;
}

int fpm_delete_template(uint16_t index)
{
    /* Not configured */
    if (!fpm_configured) return -1;

    fpm_send_command(FPM5210_DeleteID, index, NULL);

    return 0;
}

int fpm_delete_all(void)
{
    /* Not configured */
    if (!fpm_configured) return -1;

    fpm_send_command(FPM5210_DeleteAll, 0, NULL);

    return 0;
}

bool fpm_is_busy(void)
{
    /* Not configured */
    if (!fpm_configured) return false;

    if (fpm_cmd_head != fpm_cmd_tail)
        return true;

    return false;
}
