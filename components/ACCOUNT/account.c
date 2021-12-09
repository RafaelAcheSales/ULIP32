#include <time.h>
#include "account.h"
#include "stdbool.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "string.h"
#include "esp_log.h"
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/* DATABASE sector */
#define DB_START_SECTOR         0
#define DB_NUM_SECTOR           500
#define DB_SECTOR_SIZE          4096

/* DATABASE node */
#define DB_NODE_SIZE            1024
#define DB_NUM_NODE             2000
#define DB_NODE_PER_SECTOR      4
#define SHA1_DIGEST_LENGTH      64

#define DB_SECTOR(i) \
    (DB_START_SECTOR + (i / DB_NODE_PER_SECTOR))
#define DB_SECTOR_ADDR(i) \
    (DB_SECTOR(i) * DB_SECTOR_SIZE)
#define DB_NODE_ADDR(i) \
    ((DB_START_SECTOR * DB_SECTOR_SIZE) + (DB_NODE_SIZE * i))
#define DB_NODE_OFFSET(i) \
    ((i % DB_NODE_PER_SECTOR) * DB_NODE_SIZE)

/* DATABASE log sector */
#define DB_LOG_START_SECTOR     500
#define DB_LOG_NUM_SECTOR       32

/* DATABASE log node */
#define DB_LOG_NODE_SIZE        128
#define DB_LOG_NUM_NODE         1000
#define DB_LOG_NODE_PER_SECTOR  32

#define DB_LOG_SECTOR(i) \
    (DB_LOG_START_SECTOR + (i / DB_LOG_NODE_PER_SECTOR))
#define DB_LOG_SECTOR_ADDR(i) \
    (DB_LOG_SECTOR(i) * DB_SECTOR_SIZE)
#define DB_LOG_NODE_ADDR(i) \
    ((DB_LOG_START_SECTOR * DB_SECTOR_SIZE) + (DB_LOG_NODE_SIZE * i))
#define DB_LOG_NODE_OFFSET(i) \
    ((i % DB_LOG_NODE_PER_SECTOR) * DB_LOG_NODE_SIZE)

#define DB_NODE_FLAG            0x55
static const char * TAG = "ACCOUNT"; 
union account_u
{
    struct {
        uint8_t flag;
        uint8_t level;
        char name[64];
        char user[20];
        char password[20];
        char card[32];
        char code[128];
        acc_permission_t perm[ACCOUNT_PERMISSIONS];
        unsigned char fingerprint[ACCOUNT_FINGERPRINT_SIZE];
        char rfcode[16];
        uint16_t lifecount;
        uint8_t accessibility;
        uint8_t panic;
        char key[17];
        uint16_t rfsync;
        uint8_t visitor;
        char finger[2];
    } a;
    uint8_t u[DB_NODE_SIZE];
};

struct account_log_s {
    char date[32];
    char name[64];
    char code[64];
    uint8_t type;
    bool granted;
};

typedef union account_log_data_u
{
    struct {
        uint32_t timestamp;
        char name[64];
        char data[58];
        uint8_t type;
        uint8_t granted;
    } a;
    uint8_t u[DB_LOG_NODE_SIZE];
} account_log_data_t;

typedef struct account_db_s  {
    esp_timer_handle_t timer;
    uint16_t node_empty;
    int16_t node_index_empty;
    uint16_t node_full;
    int16_t node_index_full;
    uint16_t node_log_index;
} account_db_t;

static account_db_t account_db = {
    .node_empty = 0,
    .node_index_empty = DB_NUM_NODE,
    .node_full = 0,
    .node_index_full = DB_NUM_NODE,
    .node_log_index = DB_LOG_NUM_NODE,
};


static void account_db_cleanup(void *arg)
{
    account_t *acc;
    char period[32];
    struct tm *tm;
    time_t rawtime;
    char date[32];
    char now[32];
    int index;
    int year;
    int mon;
    int mday;
    int hour;
    int min;
    char *p;
    char *t;
    bool rv;
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

    if (!account_db.node_full) return;

    time(&rawtime);
    tm = localtime(&rawtime);
    year = tm->tm_year;
    mon = tm->tm_mon + 1;
    mday = tm->tm_mday;
    hour = tm->tm_hour;
    min = tm->tm_min; 
    sprintf(now, "%04d/%02d/%02d %02d:%02d",
               year, mon, mday, hour, min); 
    for (index = account_db_get_first(); ;
         index = account_db_get_next(index)) {
        acc = account_db_get_index(index);
        if (acc) {
            if (acc->a.visitor) {
                rv = true;
                for (i = 0; *acc->a.perm[i] != '\0' && i < ACCOUNT_PERMISSIONS; i++) {
                    strcpy(period, acc->a.perm[i]);
                    if (!strchr(period, '/')) {
                        rv = false;
                        break;
                    } else {
                        p = strtok_r(period, " ", &t);
                        p = strtok(p, "-");
                        p = strtok(NULL, "-");
                        if (!p) continue;
                        p = strtok(p, "/");
                        y2 = strtol(p, NULL, 10);
                        p = strtok(NULL, "/");
                        if (!p) continue;
                        t2 = strtol(p, NULL, 10);
                        p = strtok(NULL, "/");
                        if (!p) continue;
                        d2 = strtol(p, NULL, 10);
                        p = strtok_r(NULL, " ", &t);
                        if (!p) continue;
                        p = strtok(p, "-");
                        p = strtok(NULL, "-");
                        if (!p) continue;
                        p = strtok(p, ":");
                        h2 = strtol(p, NULL, 10);
                        p = strtok(NULL, ":");
                        if (!p) continue;
                        m2 = strtol(p, NULL, 10);
                        sprintf(date, "%04d/%02d/%02d %02d:%02d",
                                   y2, t2, d2, h2, m2);
                        if (strcmp(now, date) < 0) {
                            rv = false;
                            break;
                        }
                    }
                }
                if (rv)
                    account_db_delete(index);
            }
            free(acc);
        }
        if (index == account_db.node_index_full)
            break;
    }
}


int account_init(void)
{
    account_t *acc;
    account_log_data_t *log;
    uint32_t timestamp;
    uint8_t *data;
    uint32_t addr;
    int i;
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    
    data = (uint8_t *)malloc(DB_SECTOR_SIZE);
    if (!data) return -1;

    /* Read database */
    addr = DB_SECTOR_ADDR(0);
    acc = (account_t *)data;
    for (i = 0; i < DB_NUM_NODE; i++) {
        /* Read sector boundary */
        if (!(i % DB_NODE_PER_SECTOR)) {
            acc = (account_t *)data;
            // ESP_LOGE("account", "%x", addr);
            esp_err_t error = esp_partition_read(partition, addr, (void *) data, DB_SECTOR_SIZE);
            // ets_intr_lock();
            // spi_flash_read(addr, (uint32 *)data,
            //                DB_SECTOR_SIZE);
            // ets_intr_unlock();
        }
        /* Check node */
        if (acc->a.flag != DB_NODE_FLAG) {
            account_db.node_empty++;
            if (account_db.node_index_empty == DB_NUM_NODE)
                account_db.node_index_empty = i;
        } else {
            account_db.node_full++;
            account_db.node_index_full = i;
        }
        addr += DB_NODE_SIZE;
        acc++;
        /* Watchdog */
        // system_soft_wdt_feed();
    }

    /* Read log database */
    addr = DB_LOG_SECTOR_ADDR(0);
    log = (account_log_data_t *)data;
    timestamp = 0;
    for (i = 0; i < DB_LOG_NUM_NODE; i++) {
        /* Read sector boundary */
        if (!(i % DB_LOG_NODE_PER_SECTOR)) {
            log = (account_log_data_t *)data;
            ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) data, DB_SECTOR_SIZE));
            // ets_intr_lock();
            // spi_flash_read(addr, (uint32 *)data,
            //                DB_SECTOR_SIZE);
            // ets_intr_unlock();
        }
        /* Check node */
        if (log->a.timestamp == -1)
            break;
        if (timestamp <= log->a.timestamp) {
            timestamp = log->a.timestamp;
            account_db.node_log_index = i;
        }
        addr += DB_LOG_NODE_SIZE;
        log++;
        /* Watchdog */
        // system_soft_wdt_feed();
    }

    free(data);

    /* Visitor */
    const esp_timer_create_args_t timer_args = {
            .callback = &account_db_cleanup,
            /* name is optional, but may help identify the timer when ESP_LOGDging */
            .name = "polling timeout"
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &account_db.timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(account_db.timer, 300000000));
    // timer_setfn(&account_db.timer,
    //                (timer_func_t *)account_db_cleanup, NULL);
    // timer_arm(&account_db.timer, 300000, true);

    return 0;
}


void account_release(void)
{
    ESP_ERROR_CHECK(esp_timer_stop(account_db.timer));
    // timer_disarm(&account_db.timer);
    account_db.node_empty = 0;
    account_db.node_index_empty = DB_NUM_NODE;
    account_db.node_full = 0;
    account_db.node_index_full = DB_NUM_NODE;
    account_db.node_log_index = DB_LOG_NUM_NODE;
}


account_t *account_new(void)
{
    account_t *acc;

    // acc = (account_t *)zalloc(sizeof(account_t));
    acc = (account_t *)calloc(1, sizeof(account_t));
    if (!acc) return NULL;

    return acc;
}


void account_destroy(account_t *acc)
{
    if (acc)
        free(acc);
}


void account_set_level(account_t *acc,
                       uint8_t level)
{
    if (!acc) return;

    acc->a.level = level;
}


void account_set_name(account_t *acc,
                      const char *name)
{
    if (!acc) return;

    memset(acc->a.name, 0, sizeof(acc->a.name));
    if (name)
        strncpy(acc->a.name, name,
                   sizeof(acc->a.name) - 1);
}


void account_set_user(account_t *acc,
                      const char *user)
{
    if (!acc) return;

    memset(acc->a.user, 0, sizeof(acc->a.user));
    if (user)
        strncpy(acc->a.user, user,
                   sizeof(acc->a.user) - 1);
}


void account_set_password(account_t *acc,
                          const char *password)
{
    if (!acc) return;

    memset(acc->a.password, 0, sizeof(acc->a.password));
    if (password)
        strncpy(acc->a.password, password,
                   sizeof(acc->a.password) - 1);
}


void account_set_card(account_t *acc,
                      const char *card)
{
    if (!acc) return;

    memset(acc->a.card, 0, sizeof(acc->a.card));
    if (card)
        strncpy(acc->a.card, card, sizeof(acc->a.card) - 1);
}


void account_set_code(account_t *acc,
                      const char *code)
{
    if (!acc) return;

    memset(acc->a.code, 0, sizeof(acc->a.code));
    if (code)
        strncpy(acc->a.code, code, sizeof(acc->a.code) - 1);
}


void account_set_fingerprint(account_t *acc, uint8_t *fingerprint)
{
    if (!acc) return;

    memset(acc->a.fingerprint, 0, sizeof(acc->a.fingerprint));
    if (fingerprint)
        memcpy(acc->a.fingerprint, fingerprint,
                  sizeof(acc->a.fingerprint));
}


void account_set_permission(account_t *acc,
                            acc_permission_t *perm,
                            int len)
{
    int size = 0;
    int i;

    if (!acc) return;

    memset(acc->a.perm, 0, sizeof(acc->a.perm));
    if (perm) {
        for (i = 0; i < len && i < ACCOUNT_PERMISSIONS; i++) {
            if (*perm[i] != '\0')
                strcpy(acc->a.perm[size++], perm[i]);
        }
    }
}


void account_set_rfcode(account_t *acc,
                        const char *rfcode)
{
    if (!acc) return;

    memset(acc->a.rfcode, 0, sizeof(acc->a.rfcode));
    if (rfcode)
        strncpy(acc->a.rfcode, rfcode, sizeof(acc->a.rfcode) - 1);
}


void account_set_lifecount(account_t *acc,
                           uint16_t lifecount)
{
    if (!acc) return;

    acc->a.lifecount = lifecount;
}


void account_set_accessibility(account_t *acc,
                               uint16_t accessibility)
{
    if (!acc) return;

    acc->a.accessibility = accessibility;
}


void account_set_panic(account_t *acc,
                       uint8_t panic)
{
    if (!acc) return;

    acc->a.panic = panic;
}


void account_set_key(account_t *acc,
                     const char *key)
{
    if (!acc) return;

    memset(acc->a.key, 0, sizeof(acc->a.key));
    if (key)
        strncpy(acc->a.key, key, sizeof(acc->a.key) - 1);
}


void account_set_rfsync(account_t *acc, uint16_t rfsync)
{
    if (!acc) return;

    acc->a.rfsync = rfsync;
}


void account_set_visitor(account_t *acc, uint8_t visitor)
{
    if (!acc) return;

    acc->a.visitor = visitor;
}


void account_set_finger(account_t *acc, const char *finger)
{
    if (!acc) return;

    memset(acc->a.finger, 0, sizeof(acc->a.finger));
    if (finger)
        strncpy(acc->a.finger, finger,
                   sizeof(acc->a.finger) - 1);
}


uint8_t account_get_level(account_t *acc)
{
    if (!acc) return 0;

    return acc->a.level;
}


const char *account_get_name(account_t *acc)
{
    if (!acc) return NULL;

    return *acc->a.name != '\0' ? acc->a.name : NULL;
}


const char *account_get_user(account_t *acc)
{
    if (!acc) return NULL;

    return *acc->a.user != '\0' ? acc->a.user : NULL;
}


const char *account_get_password(account_t *acc)
{
    if (!acc) return NULL;

    return *acc->a.password != '\0' ? acc->a.password : NULL;
}


const char *account_get_card(account_t *acc)
{
    if (!acc) return NULL;

    return *acc->a.card != '\0' ? acc->a.card : NULL;
}


const char *account_get_code(account_t *acc)
{
    if (!acc) return NULL;

    return *acc->a.code != '\0' ? acc->a.code : NULL;
}


acc_permission_t *account_get_permission(account_t *acc)
{
    if (!acc) return NULL;

    if (**acc->a.perm == '\0') return NULL;

    return (acc_permission_t *)acc->a.perm;
}


const char *account_get_rfcode(account_t *acc)
{
    if (!acc) return NULL;

    return *acc->a.rfcode != '\0' ? acc->a.rfcode : NULL;
}


uint8_t *account_get_fingerprint(account_t *acc)
{
    if (!acc) return NULL;

    return *acc->a.fingerprint != 0 ? acc->a.fingerprint : NULL;
}


uint16_t account_get_lifecount(account_t *acc)
{
    if (!acc) return 0;

    return acc->a.lifecount;
}


uint8_t account_get_accessibility(account_t *acc)
{
    if (!acc) return false;

    return acc->a.accessibility;
}


uint8_t account_get_panic(account_t *acc)
{
    if (!acc) return false;

    return acc->a.panic;
}


const char *account_get_key(account_t *acc)
{
    if (!acc) return NULL;

    return *acc->a.key != '\0' ? acc->a.key : NULL;
}


uint16_t account_get_rfsync(account_t *acc)
{
    if (!acc) return 0;

    return acc->a.rfsync;
}


uint8_t account_get_visitor(account_t *acc)
{
    if (!acc) return 0;

    return acc->a.visitor;
}


const char *account_get_finger(account_t *acc)
{
    if (!acc) return NULL;

    return *acc->a.finger ? acc->a.finger : NULL;
}


bool account_check_permission(account_t *acc)
{
    time_t rawtime;
    char period[34];
    struct tm *tm;
    char date[32];
    char now[32];
    int year;
    int mon;
    int mday;
    int hour;
    int min;
    int wday;
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
    int s1;
    int s2;
    int i;
    int k;
    int j;

    if (!acc) return false;

    if (*acc->a.perm[0] == '\0')
        return true;
    time(&rawtime);
    tm = localtime(&rawtime);
    year = tm->tm_year;
    mon = tm->tm_mon + 1;
    mday = tm->tm_mday;
    hour = tm->tm_hour;
    min = tm->tm_min; 
    wday = tm->tm_wday;
    sprintf(now, "%04d/%02d/%02d %02d:%02d",
               year, mon, mday, hour, min); 
    for (j = 0; *acc->a.perm[j] != '\0' && j < ACCOUNT_PERMISSIONS; j++) {
        strcpy(period, acc->a.perm[j]);
        if (!strchr(period, '/')) {
            /*
             * Period format: weekday-weekday hour:minute-hour:minute
             * For weekdays, the range is close
             * For hours, the range is open
             */

            /* Week */
            p = strtok_r(period, " ", &t);
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
            /*
             * Weekday range
             */
            i = w2 - w1;
            if (i < 0) i += 7;
            k = wday - w1;
            if (k < 0) k += 7;
            if (k > i) {
                continue;
            }
            /*
             * Hour range
             */
            s1 = 60 * (60 * h1 + m1);
            s2 = 60 * (60 * h2 + m2);
            i = s2 - s1;
            if (i < 0) i += 86400;
            k = 60 * (hour * 60 + min) - s1;
            if (k < 0) k += 86400;
            if (k > i) {
                continue;
            }
            return true;
        } else {
            /*
             * Period format: year/month/day-year/month/day hour:minute-hour:minute
             */

            p = strtok_r(period, " ", &t);
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
            sprintf(date, "%04d/%02d/%02d %02d:%02d",
                       y1, t1, d1, h1, m1);

            if (strcmp(now, date) < 0)
                continue;
            sprintf(date, "%04d/%02d/%02d %02d:%02d",
                       y2, t2, d2, h2, m2);
            if (strcmp(now, date) > 0)
                continue;
            return true;
        }
    }

    return false;
}


int account_db_insert(account_t *acc)
{
    uint8_t *data;
    int index;
    int sector;
    uint32_t addr;
    int off;
    int i;
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    if (!acc) return -1;

    if (*acc->a.user == '\0' && *acc->a.card == '\0' &&
        *acc->a.code == '\0' && *acc->a.rfcode == '\0' &&
        *acc->a.fingerprint == 0)
        return -1;

    /* Update database */
    index = account_db_find(NULL, acc->a.user, acc->a.card,
                            acc->a.code, acc->a.rfcode,
                            acc->a.fingerprint, NULL);
    if (index >= 0) {
        data = (uint8_t *)malloc(DB_SECTOR_SIZE);
        if (!data) return -1;
        sector = DB_SECTOR(index);
        addr = sector * DB_SECTOR_SIZE;
        ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) data, DB_SECTOR_SIZE));
        // ets_intr_lock();
        // spi_flash_read(addr, (uint32 *)data, DB_SECTOR_SIZE);
        // ets_intr_unlock();
        off = DB_NODE_OFFSET(index);
        /* Update database node */
        acc->a.flag = DB_NODE_FLAG;
        memcpy(data + off, acc, sizeof(account_t));
        ESP_ERROR_CHECK(esp_partition_erase_range(partition, addr, DB_SECTOR_SIZE));
        ESP_ERROR_CHECK(esp_partition_write(partition, addr, (void *)data, DB_SECTOR_SIZE));
        // ets_intr_lock();
        // spi_flash_erase_sector(sector);
        // spi_flash_write(addr, (uint32 *)data, DB_SECTOR_SIZE);
        // ets_intr_unlock();
        /* Watchdog */
        // system_soft_wdt_feed();
        free(data);
        ESP_LOGD("DB", "Database update node [%d] empty [%d %d] full [%d %d]",
                 index, account_db.node_empty, account_db.node_index_empty,
                 account_db.node_full, account_db.node_index_full);
        return index;
    }

    index = account_db.node_index_empty;
    /* Full database */
    if (index == DB_NUM_NODE)
        return -1;

    data = (uint8_t *)malloc(DB_SECTOR_SIZE);
    if (!data) return -1;
  
    sector = DB_SECTOR(index);
    addr = sector * DB_SECTOR_SIZE;
    ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) data, DB_SECTOR_SIZE));
    // ets_intr_lock();
    // spi_flash_read(addr, (uint32 *)data, DB_SECTOR_SIZE);
    // ets_intr_unlock();
    off = DB_NODE_OFFSET(index);
    /* Update database node */
    acc->a.flag = DB_NODE_FLAG;
    memcpy(data + off, acc, sizeof(account_t));
    ESP_ERROR_CHECK(esp_partition_erase_range(partition, addr, DB_SECTOR_SIZE));
    ESP_ERROR_CHECK(esp_partition_write(partition, addr, (void *)data, DB_SECTOR_SIZE));
    // ets_intr_lock();
    // spi_flash_erase_sector(sector);
    // spi_flash_write(addr, (uint32 *)data, DB_SECTOR_SIZE);
    // ets_intr_unlock();
    /* Watchdog */
    // system_soft_wdt_feed();

    /* Update database information */
    account_db.node_empty--;
    account_db.node_index_empty++;
    off += DB_NODE_SIZE;
    addr += off;
    acc = (account_t *)(data + off);
    for (i = account_db.node_index_empty; i < DB_NUM_NODE; i++) {
        /* Read sector boundary */
        if (!(i % DB_NODE_PER_SECTOR)) {
            acc = (account_t *)data;
            // ets_intr_lock();
            ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) data, DB_SECTOR_SIZE));
            // ets_intr_unlock();
        }
        if (acc->a.flag != DB_NODE_FLAG) {
            account_db.node_index_empty = i;
            break;
        }
        addr += DB_NODE_SIZE;
        acc++;
        /* Watchdog */
        // system_soft_wdt_feed();
    }
    account_db.node_full++;
    if (account_db.node_index_full == DB_NUM_NODE ||
        account_db.node_index_full < account_db.node_index_empty)
        account_db.node_index_full = account_db.node_index_empty - 1;

    free(data);

    ESP_LOGD("DB", "Database insert node [%d] empty [%d %d] full [%d %d]",
             index, account_db.node_empty, account_db.node_index_empty,
             account_db.node_full, account_db.node_index_full);

    return index;
}


int account_db_delete(int index)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    account_t *acc;
    uint8_t *data;
    int sector;
    uint32_t addr;
    int off;
    int i;

    if (index > DB_NUM_NODE) return -1;

    data = (uint8_t *)malloc(DB_SECTOR_SIZE);
    if (!data) return -1;

    sector = DB_SECTOR(index);
    addr = sector * DB_SECTOR_SIZE;
    off = DB_NODE_OFFSET(index);
    ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) data, DB_SECTOR_SIZE));
    // ets_intr_lock();
    // spi_flash_read(addr, (uint32 *)data, DB_SECTOR_SIZE);
    // ets_intr_unlock();
    acc = (account_t *)(data + off);
    if (acc->a.flag != DB_NODE_FLAG) {
        free(data);
        return -1;
    }
    memset(data + off, 0, DB_NODE_SIZE);
    /* Update database node */
    ESP_ERROR_CHECK(esp_partition_erase_range(partition, addr, DB_SECTOR_SIZE));
    ESP_ERROR_CHECK(esp_partition_write(partition, addr, (void *)data, DB_SECTOR_SIZE));
    // ets_intr_lock();
    // spi_flash_erase_sector(sector);
    // spi_flash_write(addr, (uint32 *)data, DB_SECTOR_SIZE);
    // ets_intr_unlock();
    /* Watchdog */
    // system_soft_wdt_feed();

    /* Update database information */
    if (index == account_db.node_index_full) {
        account_db.node_index_full--;
        if (account_db.node_index_full >= 0) {
            off -= DB_NODE_SIZE;
            addr += off;
            acc = (account_t *)(data + off);
            for (i = account_db.node_index_full; i >= 0; i--) {
                /* Read sector boundary */
                if (!(i % (DB_NODE_PER_SECTOR - 1))) {
                    ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) data, DB_SECTOR_SIZE));
                    // ets_intr_lock();
                    // spi_flash_read(addr, (uint32 *)data, DB_SECTOR_SIZE);
                    // ets_intr_unlock();
                    acc = (account_t *)(data + DB_SECTOR_SIZE);
                    acc--;
                }
                if (acc->a.flag == DB_NODE_FLAG) {
                    account_db.node_index_full = i;
                    break;
                }
                addr -= DB_NODE_SIZE;
                acc--;
                /* Watchdog */
                // system_soft_wdt_feed();
            }
        } else {
            account_db.node_index_empty = 0; 
            account_db.node_index_full = DB_NUM_NODE;
        }
    }
    account_db.node_full--;
    account_db.node_empty++;
    if (account_db.node_index_empty > index) {
        account_db.node_index_empty = index;
    }

    free(data);

    ESP_LOGD("DB", "Database remove node [%d] empty [%d %d] full [%d %d]",
             index, account_db.node_empty, account_db.node_index_empty,
             account_db.node_full, account_db.node_index_full);
    return 0;
}


int account_db_find(const char *name,
                    const char *user,
                    const char *card,
                    const char *code,
                    const char *rfcode,
                    uint8_t *fingerprint,
                    const char *key)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    uint32_t addr;
    account_t acc;
    int i;

    if (!name && !user && !card && !code && !rfcode &&
        !fingerprint && !key)
        return -1;

    /* Empty database */
    if (!account_db.node_full) return -1;

    addr = DB_SECTOR_ADDR(0);
    for (i = 0; i <= account_db.node_index_full; i++) {
        ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) &acc, DB_NODE_SIZE));
        // ets_intr_lock();
        // spi_flash_read(addr, (uint32 *)&acc, DB_NODE_SIZE);
        // ets_intr_unlock();
        if (acc.a.flag == DB_NODE_FLAG) {
            if ((name && *name && strcasestr(acc.a.name, name)) ||
                (user && *user && !strcmp(user, acc.a.user)) ||
                (card && *card && !strcmp(card, acc.a.card)) ||
                (code && *code && !strcmp(code, acc.a.code)) ||
                (rfcode && *rfcode && !strcmp(rfcode, acc.a.rfcode)) ||
                (fingerprint && *fingerprint &&
                 !memcmp(fingerprint, acc.a.fingerprint, ACCOUNT_FINGERPRINT_SIZE)) ||
                (key && *key && !strcmp(key, acc.a.key)))
                return i;
        }
        addr += DB_NODE_SIZE;
        /* Watchdog */
        // system_soft_wdt_feed();
    }

    return -1;
}


int account_db_find_hash(const char *key, int validity)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    uint8_t hash[SHA1_DIGEST_LENGTH];
    uint32_t addr;
    account_t acc;
    struct tm *tm;
    time_t rawtime;
    uint32_t now;
    mbedtls_sha1_context ctx;
    // SHA1_CTX ctx;
    char stmp[64];
    long unsigned int d;
    int i;
    int k;
    int j;

    if (!key || !validity) return -1;

    /* Empty database */
    if (!account_db.node_full) return -1;

    if (strlen(key) < (SHA1_DIGEST_LENGTH << 1))
        return -1;
    time(&rawtime);
    tm = localtime(&rawtime);
    // tm = rtc_localtime();
    now = rawtime;
    addr = DB_SECTOR_ADDR(0);
    for (i = 0; i <= account_db.node_index_full; i++) {
        ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) &acc, DB_NODE_SIZE));
        // ets_intr_lock();
        // spi_flash_read(addr, (uint32 *)&acc, DB_NODE_SIZE);
        // ets_intr_unlock();
        if (acc.a.flag == DB_NODE_FLAG) {
            if (*acc.a.code == '\0' || *acc.a.key == '\0')
                continue;
            d = (now - (uint32_t)strtol(acc.a.code, NULL, 10)) / validity;
            for (k = 0; k < 3; k++) {
                snprintf(stmp, sizeof(stmp), "%s:%lu",
                            acc.a.key, d + k - 1);
                mbedtls_sha1_init(&ctx);
                mbedtls_sha1_update_ret(&ctx, (const unsigned char *)stmp, strlen(stmp));
                mbedtls_sha1_finish_ret(&ctx, hash);
                // SHA1_Init(&ctx);
                // SHA1_Update(&ctx, stmp, strlen(stmp));
                // SHA1_Final(hash, &ctx);
                for (j = 0; j < SHA1_DIGEST_LENGTH; j++)
                    snprintf(stmp + (j << 1), sizeof(stmp) - (j << 1),
                                "%02x", hash[j]);
                if (!strncasecmp(stmp, key, SHA1_DIGEST_LENGTH << 1))
                    return i;
            }
        }
        addr += DB_NODE_SIZE;
        /* Watchdog */
        // system_soft_wdt_feed();
    }

    return -1;
}


int account_db_remove_all(void)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    uint32_t addr;
    account_t acc;
    uint32_t sector;
    int i;

    /* Empty database */
    if (!account_db.node_full) return 0;

    addr = DB_SECTOR_ADDR(0);
    for (i = 0; i <= account_db.node_index_full; ) {
        ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) &acc, DB_NODE_SIZE));
        // ets_intr_lock();
        // spi_flash_read(addr, (uint32 *)&acc, DB_NODE_SIZE);
        // ets_intr_unlock();
        if (acc.a.flag == DB_NODE_FLAG) {
            /* Erase sector */
            sector = DB_SECTOR_ADDR(i);
            ESP_ERROR_CHECK(esp_partition_erase_range(partition, sector, DB_SECTOR_SIZE));
            // ets_intr_lock();
            // spi_flash_erase_sector(sector);
            // ets_intr_unlock();
            i += (DB_NODE_PER_SECTOR - (i % DB_NODE_PER_SECTOR));
            addr = DB_NODE_ADDR(i);
            /* Watchdog */
            // system_soft_wdt_feed();
            continue;
        }
        addr += DB_NODE_SIZE;
        /* Watchdog */
        // system_soft_wdt_feed();
        i++;
    }

    account_db.node_empty = DB_NUM_NODE;
    account_db.node_index_empty = 0;
    account_db.node_full = 0;
    account_db.node_index_full = DB_NUM_NODE;

    return 0;
}


int account_db_get_size(void)
{
    return account_db.node_full;
}


int account_db_get_first(void)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    uint32_t addr;
    account_t acc;
    int i;

    /* Empty database */
    if (!account_db.node_full) return 0;

    addr = DB_NODE_ADDR(0);
    for (i = 0; i <= account_db.node_index_full; i++) {
        ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) &acc, DB_NODE_SIZE));
        // ets_intr_lock();
        // spi_flash_read(addr, (uint32 *)&acc, DB_NODE_SIZE);
        // ets_intr_unlock(); 
        if (acc.a.flag == DB_NODE_FLAG)
            return i;
        addr += DB_NODE_SIZE;
        /* Watchdog */
        // system_soft_wdt_feed();
    }

    return account_db.node_index_full;
}


int account_db_get_last(void)
{
    return account_db.node_index_full;
}


int account_db_get_next(int index)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    uint32_t addr;
    account_t acc;
    int i;

    if (!account_db.node_full) return 0;

    /* Last */
    if (++index >= account_db.node_index_full)
        return account_db.node_index_full;

    addr = DB_NODE_ADDR(index);
    for (i = index; i <= account_db.node_index_full; i++) {
        ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) &acc, DB_NODE_SIZE));

        // ets_intr_lock(); 
        // spi_flash_read(addr, (uint32 *)&acc, DB_NODE_SIZE);
        // ets_intr_unlock(); 
        if (acc.a.flag == DB_NODE_FLAG)
            return i;
        addr += DB_NODE_SIZE;
        /* Watchdog */
        // system_soft_wdt_feed();
    }

    return account_db.node_index_full;
}


int account_db_get_previous(int index)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    uint32_t addr;
    account_t acc;
    int i;

    if (!account_db.node_full) return 0;

    /* First */
    if (--index <= 0) return 0;

    addr = DB_NODE_ADDR(index);
    for (i = index; i >= 0; i--) {
        ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) &acc, DB_NODE_SIZE));

        // ets_intr_lock(); 
        // spi_flash_read(addr, (uint32 *)&acc, DB_NODE_SIZE);
        // ets_intr_unlock(); 
        if (acc.a.flag == DB_NODE_FLAG)
            return i;
        addr -= DB_NODE_SIZE;
        /* Watchdog */
        // system_soft_wdt_feed();
    }

    return 0;
}


int account_db_get_empty(void)
{
    return account_db.node_index_empty;
}


account_t *account_db_get_index(int index)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    uint32_t addr;
    // static portMUX_TYPE my_mutex = portMUX_INITIALIZER_UNLOCKED;
    account_t *acc;

    if (index < 0 || index > account_db.node_index_full)
        return NULL;

    acc = (account_t *)malloc(sizeof(account_t));
    // ESP_LOGI(TAG, "malloced");
    if (!acc) return NULL;

    addr = DB_NODE_ADDR(index);
    // ESP_LOGI(TAG, "addr of node: %x", addr);
    // vTaskDelay(100);
    // taskENTER_CRITICAL(&my_mutex);
    esp_err_t error = esp_partition_read(partition, addr, (void *) acc, DB_NODE_SIZE);
    // taskEXIT_CRITICAL(&my_mutex);
    // ESP_LOGI(TAG, "read partition and error: %x", error);
    // ets_intr_lock(); 
    // spi_flash_read(addr, (uint32 *)acc, DB_NODE_SIZE);
    // ets_intr_unlock(); 
    if (acc->a.flag != DB_NODE_FLAG) {
        free(acc);
        ESP_LOGI(TAG, "flag != 0x55");
        return NULL;
    }

    return acc;
}


int account_db_json(int index, char *json, int len)
{
    account_t *acc;
    char temp[700];
    int size;
    int i;

    if (index > DB_NUM_NODE || !json) return -1;

    acc = account_db_get_index(index);
    if (!acc) return -1;
   
    if (*acc->a.fingerprint)
        mbedtls_base64_encode((unsigned char *)temp, sizeof(temp), NULL, acc->a.fingerprint, sizeof(acc->a.fingerprint));
        // base64Encode(sizeof(acc->a.fingerprint), acc->a.fingerprint,
        //              sizeof(temp), temp);
    else
        temp[0] = '\0';
    size = snprintf(json, len, "{\"id\":\"%d\",\"name\":\"%s\",\"user\":\"%s\"," \
                                  "\"password\":\"%s\",\"card\":\"%s\",\"qrcode\":\"%s\"," \
                                  "\"rfcode\":\"%s\",\"fingerprint\":\"%s\",\"lifecount\":\"%d\"," \
                                  "\"accessibility\":\"%s\",\"panic\":\"%s\",\"key\":\"%s\"," \
                                  "\"administrator\":\"%s\",\"visitor\":\"%s\",\"finger\":\"%s\"",
                       index, *acc->a.name ? acc->a.name : "",
                       *acc->a.user ? acc->a.user : "",
                       *acc->a.password ? acc->a.password : "",
                       *acc->a.card ? acc->a.card : "",
                       *acc->a.code ? acc->a.code : "",
                       *acc->a.rfcode ? acc->a.rfcode : "",
                       *temp ? temp : "", acc->a.lifecount,
                       acc->a.accessibility ? "true" : "false", 
                       acc->a.panic ? "true" : "false",
                       *acc->a.key ? acc->a.key : "", //11
                       acc->a.level ? "true" : "false",
                       acc->a.visitor ? "true" : "false",
                       *acc->a.finger ? acc->a.finger : "");
    for (i = 0; i < ACCOUNT_PERMISSIONS; i++)
        size += snprintf(json + size, len - size, ",\"perm%d\":\"%s\"",
                            i + 1, acc->a.perm[i]);
    if (size + 2 > len) {
        free(acc);
        return -1;
    }
    json[size++] = '}';
    json[size] = '\0';

    free(acc);

    return size;
}


int account_db_json_summary(char *json, int len)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    int accounts = 0;
    int user = 0;
    int card = 0;
    int code = 0;
    int rfcode = 0;
    int fingerprint = 0;
    account_t *acc;
    uint8_t *data;
    uint32_t addr;
    int i;

    if (!json) return -1;

    if (account_db.node_full) {
        data = (uint8_t *)malloc(DB_SECTOR_SIZE);
        if (!data) return -1;
        /* Read database */
        addr = DB_SECTOR_ADDR(0);
        acc = (account_t *)data;
        for (i = 0; i <= account_db.node_index_full; i++) {
            /* Read sector boundary */
            if (!(i % DB_NODE_PER_SECTOR)) {
                acc = (account_t *)data;
                ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) data, DB_SECTOR_SIZE));
                // ets_intr_lock();
                // spi_flash_read(addr, (uint32 *)data,
                //                DB_SECTOR_SIZE);
                // ets_intr_unlock();
            }
            /* Check node */
            if (acc->a.flag == DB_NODE_FLAG) {
                if (*acc->a.user)
                    user++;
                if (*acc->a.card)
                    card++;
                if (*acc->a.code)
                    code++;
                if (*acc->a.rfcode)
                    rfcode++;
                if (*acc->a.fingerprint)
                    fingerprint++;
            }
            addr += DB_NODE_SIZE;
            acc++;
            /* Watchdog */
            // system_soft_wdt_feed();
        }
        free(data);
    }

    len = snprintf(json, len, "{\"accounts\":\"%d\",\"user\":\"%d\"," \
                      "\"card\":\"%d\",\"qrcode\":\"%d\"," \
                      "\"rfcode\":\"%d\",\"fingerprint\":\"%d\"}",
                      accounts, user, card, code, rfcode, fingerprint);

    return len;
}


int account_db_string(int index, char *str, int len)
{
    account_t *acc;
    char temp[700];
    int size;
    int i;

    if (index > DB_NUM_NODE || !str) return -1;

    acc = account_db_get_index(index);
    if (!acc) return -1;
   
    if (*acc->a.fingerprint)
        mbedtls_base64_encode((unsigned char *)temp, sizeof(temp), NULL, acc->a.fingerprint, sizeof(acc->a.fingerprint));
        // base64Encode(sizeof(acc->a.fingerprint), acc->a.fingerprint,
        //              sizeof(temp), temp);
    else
        temp[0] = '\0';
    size = snprintf(str, len, "%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%s,%d,%d,%c",
                       *acc->a.name ? acc->a.name : "",
                       *acc->a.user ? acc->a.user : "",
                       *acc->a.password ? acc->a.password : "",
                       *acc->a.card ? acc->a.card : "",
                       *acc->a.code ? acc->a.code : "",
                       *acc->a.rfcode ? acc->a.rfcode : "",
                       *temp ? temp : "", acc->a.lifecount,
                       acc->a.accessibility, acc->a.panic,
                       *acc->a.key ? acc->a.key : "",
                       acc->a.level, acc->a.visitor,
                       acc->a.finger[0]);
    for (i = 0; i < ACCOUNT_PERMISSIONS; i++)
        size += snprintf(str + size, len - size, ",%s",
                            acc->a.perm[i]);
    if (size + 3 > len) {
        free(acc);
        return -1;
    }
    str[size++] = '\r';
    str[size++] = '\n';
    str[size] = '\0';

    free(acc);

    return size;
}


account_log_t *account_log_new(void)
{
    account_log_t *p;

    p = (account_log_t *)calloc(1, sizeof(account_log_t));

    return p;
}


void account_log_destroy(account_log_t *log)
{
    if (!log) return;

    free(log);
}


void account_log_set_date(account_log_t *log,
                          const char *date)
{
    if (!log) return;

    memset(log->date, 0, sizeof(log->date));
    if (date)
        strncpy(log->date, date, sizeof(log->date) - 1);
}


const char *account_log_get_date(account_log_t *log)
{
    if (!log) return NULL;

    if (*log->date == '\0')
        return NULL;

    return log->date;
}


void account_log_set_name(account_log_t *log,
                          const char *name)
{
    if (!log) return;

    memset(log->name, 0, sizeof(log->name));
    if (name)
        strncpy(log->name, name, sizeof(log->name) - 1);
}


const char *account_log_get_name(account_log_t *log)
{
    if (!log) return NULL;

    if (*log->name == '\0')
        return NULL;

    return log->name;
}


void account_log_set_code(account_log_t *log,
                          const char *code)
{
    if (!log) return;

    memset(log->code, 0, sizeof(log->code));
    if (code)
        strncpy(log->code, code, sizeof(log->code) - 1);
}


const char *account_log_get_code(account_log_t *log)
{
    if (!log) return NULL;

    if (*log->code == '\0')
        return NULL;

    return log->code;
}


void account_log_set_type(account_log_t *log,
                          uint8_t type)
{
    if (!log) return;

    log->type = type;
}


uint8_t account_log_get_type(account_log_t *log)
{
    if (!log) return false;

    return log->type;
}


void account_log_set_granted(account_log_t *log,
                             bool granted)
{
    if (!log) return;

    log->granted = granted;
}


bool account_log_get_granted(account_log_t *log)
{
    if (!log) return false;

    return log->granted;
}


int account_db_log_insert(account_log_t *log)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    account_log_data_t d;
    uint8_t *data;
    uint16_t index;
    int sector;
    time_t rawtime;
    uint32_t addr;
    int off;

    if (!log) return -1;

    data = (uint8_t *)malloc(DB_SECTOR_SIZE);
    if (!data) return -1;

    /* Get next node */
    if (account_db.node_log_index == DB_LOG_NUM_NODE) {
        index = 0;
    } else {
        index = (account_db.node_log_index + 1) % DB_LOG_NUM_NODE;
    }
    sector = DB_LOG_SECTOR(index);
    addr = sector * DB_SECTOR_SIZE;
    ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *) data, DB_SECTOR_SIZE));
    // ets_intr_lock();
    // spi_flash_read(addr, (uint32 *)data, DB_SECTOR_SIZE);
    // ets_intr_unlock();
    off = DB_LOG_NODE_OFFSET(index);
    /* Update log database node */
    memset(&d, 0, sizeof(account_log_data_t));
    time(&rawtime);
    d.a.timestamp = rawtime;
    strncpy(d.a.name, log->name, sizeof(d.a.name) - 1);
    strncpy(d.a.data, log->code, sizeof(d.a.data) - 1);
    d.a.type = log->type;
    d.a.granted = log->granted;
    memcpy(data + off, &d, sizeof(account_log_data_t));
    ESP_ERROR_CHECK(esp_partition_erase_range(partition, addr, DB_SECTOR_SIZE));
    ESP_ERROR_CHECK(esp_partition_write(partition, addr, (void *) data, DB_SECTOR_SIZE));
    // ets_intr_lock();
    // spi_flash_erase_sector(sector);
    // spi_flash_write(addr, (uint32 *)data, DB_SECTOR_SIZE);
    // ets_intr_unlock();
    // /* Watchdog */
    // system_soft_wdt_feed();
    /* Update log database information */
    account_db.node_log_index = index;

    free(data);

    ESP_LOGD("DB", "Database log write node [%d]", index);

    return index;
}


int account_db_log_remove_all(void)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    account_log_data_t data;
    uint32_t addr;
    uint32_t sector;
    int i;

    /* Empty log database */
    if (account_db.node_log_index == DB_LOG_NUM_NODE)
        return 0;

    addr = DB_LOG_SECTOR_ADDR(0);
    for (i = 0; i < DB_LOG_NUM_NODE; ) {
        ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *)&data, DB_LOG_NODE_SIZE));
        // ets_intr_lock();
        // spi_flash_read(addr, (uint32 *)&data, DB_LOG_NODE_SIZE);
        // ets_intr_unlock();
        if (data.a.timestamp != -1) {
            /* Erase sector */
            sector = DB_LOG_SECTOR_ADDR(i);
            ESP_ERROR_CHECK(esp_partition_erase_range(partition, sector, DB_SECTOR_SIZE));
            // ets_intr_lock();
            // spi_flash_erase_sector(sector);
            // ets_intr_unlock();
            i += (DB_LOG_NODE_PER_SECTOR - (i % DB_LOG_NODE_PER_SECTOR));
            addr = DB_LOG_NODE_ADDR(i);
            /* Watchdog */
            // system_soft_wdt_feed();
            continue;
        }
        addr += DB_LOG_NODE_SIZE;
        /* Watchdog */
        // system_soft_wdt_feed();
        i++;
    }

    return 0;
}


account_log_t *account_db_log_get_index(uint16_t index)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "accounts");
    assert(partition != NULL);
    account_log_t *log;
    account_log_data_t data;
    // account_log_data_t *dataptr;
    uint32_t addr;
    struct tm *tm;

    if (index >= DB_LOG_NUM_NODE) return NULL;

    /* Read log node */
    addr = DB_LOG_NODE_ADDR(index);
    ESP_LOGI(TAG, "before reading log addr: %x", addr);
    // dataptr = (account_log_data_t *)calloc(1, sizeof(account_log_data_t));
    // if (!dataptr) return NULL;
    ESP_ERROR_CHECK(esp_partition_read(partition, addr, (void *)&data, DB_LOG_NODE_SIZE));
    ESP_LOGI(TAG, "after reading log");
    // data = *dataptr;
    // ets_intr_lock();
    // spi_flash_read(addr, (uint32 *)&data, DB_LOG_NODE_SIZE);
    // ets_intr_unlock();
    if (data.a.timestamp == -1)
        return NULL;

    log = (account_log_t *)calloc(1, sizeof(account_log_t));
    if (!log) return NULL;
    tm = localtime((time_t *)&data.a.timestamp);
    sprintf(log->date, "%04d-%02d-%02d %02d:%02d:%02d",
               tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
               tm->tm_hour, tm->tm_min, tm->tm_sec);
    strncpy(log->name, data.a.name, sizeof(log->name) - 1);
    strncpy(log->code, data.a.data, sizeof(log->code) - 1);
    log->type = data.a.type;
    log->granted = data.a.granted;

    return log;
}


int account_db_log_string(int index, char *str, int len)
{
    account_log_t *log;
    int size;

    if (index >= DB_LOG_NUM_NODE || !str) return -1;

    log = account_db_log_get_index(index);
    if (!log) return -1;
   
    size = snprintf(str, len, "%s,%s,%s,%s",
                       log->date, log->name, log->code,
                       log->granted ? "Liberado" : "Bloqueado");
    if (size + 3 > len) {
        free(log);
        return -1;
    }
    str[size++] = '\r';
    str[size++] = '\n';
    str[size] = '\0';

    free(log);

    return size;
}


int account_db_log_get_first(void)
{
    /* Empty log database */
    if (account_db.node_log_index == DB_LOG_NUM_NODE)
        return 0;

    return account_db.node_log_index;
}


int account_db_log_get_previous(uint16_t index)
{
    /* Empty log database */
    if (account_db.node_log_index == DB_LOG_NUM_NODE)
        return 0;

    index = (index + DB_LOG_NUM_NODE - 1) % DB_LOG_NUM_NODE;

    return index;
}


int account_db_log_json(int index, char *json, int len)
{
    account_log_t *log;
    char *user = NULL;
    char *card = NULL;
    char *code = NULL;
    char *rfcode = NULL;
    char *fingerprint = NULL;
    int size;

    if (index >= DB_LOG_NUM_NODE || !json) return -1;

    log = account_db_log_get_index(index);
    if (!log) return -1;
  
    switch (log->type) {
        case ACCOUNT_LOG_USER:
            user = log->code;
            break;
        case ACCOUNT_LOG_CARD:
            card = log->code;
            break;
        case ACCOUNT_LOG_CODE:
            code = log->code;
            break;
        case ACCOUNT_LOG_RFCODE:
            rfcode = log->code;
            break;
        case ACCOUNT_LOG_FINGERPRINT:
            fingerprint = log->code;
            break;
    }
    size = snprintf(json, len, "{\"name\":\"%s\",\"user\":\"%s\",\"card\":\"%s\"," \
                       "\"qrcode\":\"%s\",\"rfcode\":\"%s\",\"fingerprint\":\"%s\"," \
                       "\"time\":\"%s\",\"granted\":\"%s\"}",
                       log->name, user ? user : "", card ? card : "",
                       code ? code : "", rfcode ? rfcode : "",
                       fingerprint ? fingerprint : "", log->date,
                       log->granted ? "true" : "false");

    free(log);

    return size;
}
