// #ifndef __ACCOUNT_H__
// #define __ACCOUNT_H__

// #define ACCOUNT_LEVEL_USER          0
// #define ACCOUNT_LEVEL_ADMIN         1

// #define ACCOUNT_DATA_SIZE           1024
// #define ACCOUNT_FINGERPRINT_SIZE    498
// #define ACCOUNT_PERMISSIONS         5
// #define ACCOUNT_PERMISSION_SIZE     34

// #define ACCOUNT_LOG_USER            0
// #define ACCOUNT_LOG_CARD            1
// #define ACCOUNT_LOG_CODE            2
// #define ACCOUNT_LOG_RFCODE          3
// #define ACCOUNT_LOG_FINGERPRINT     4

// typedef union account_u account_t;
// typedef struct account_log_s account_log_t;

// typedef typeof(char [ACCOUNT_PERMISSION_SIZE]) acc_permission_t;


// int account_init(void);

// void account_release(void);


// account_t *account_new(void);

// void account_destroy(account_t *acc);


// void account_set_level(account_t *acc,
//                        uint8_t level);

// void account_set_name(account_t *acc,
//                       const char *name);

// void account_set_user(account_t *acc,
//                       const char *user);

// void account_set_password(account_t *acc,
//                           const char *password);

// void account_set_card(account_t *acc,
//                       const char *card);

// void account_set_code(account_t *acc,
//                       const char *code);

// void account_set_permission(account_t *acc,
//                             acc_permission_t *perm,
//                             int len);

// void account_set_rfcode(account_t *acc,
//                         const char *rfcode);

// void account_set_fingerprint(account_t *acc,
//                              uint8_t *fingerprint);

// void account_set_lifecount(account_t *acc,
//                            uint16_t lifecount);

// void account_set_accessibility(account_t *acc,
//                                uint16_t accessibility);

// void account_set_panic(account_t *acc, uint8_t panic);

// void account_set_key(account_t *acc,
//                      const char *key);

// void account_set_rfsync(account_t *acc, uint16_t rfsync);

// void account_set_visitor(account_t *acc, uint8_t visitor);

// void account_set_finger(account_t *acc, const char *finger);


// uint8_t account_get_level(account_t *acc);

// const char *account_get_name(account_t *acc);

// const char *account_get_user(account_t *acc);

// const char *account_get_password(account_t *acc);

// const char *account_get_card(account_t *acc);

// const char *account_get_code(account_t *acc);

// acc_permission_t *account_get_permission(account_t *acc);

// const char *account_get_rfcode(account_t *acc);

// uint8_t *account_get_fingerprint(account_t *acc);

// uint16_t account_get_lifecount(account_t *acc);

// uint8_t account_get_accessibility(account_t *acc);

// uint8_t account_get_panic(account_t *acc);

// const char *account_get_key(account_t *acc);

// uint16_t account_get_rfsync(account_t *acc);

// uint8_t account_get_visitor(account_t *acc);

// const char *account_get_finger(account_t *acc);


// bool account_check_permission(account_t *acc);


// int account_db_insert(account_t *acc);

// int account_db_delete(int index);

// int account_db_remove_all(void);

// int account_db_find(const char *name,
//                     const char *user,
//                     const char *card,
//                     const char *code,
//                     const char *rfcode,
//                     uint8_t *fingerprint,
//                     const char *key);

// int account_db_get_size(void);

// int account_db_get_first(void);

// int account_db_get_last(void);

// int account_db_get_next(int index);

// int account_db_get_previous(int index);

// int account_db_get_empty(void);

// account_t *account_db_get_index(int index);

// int account_db_json(int index, char *json, int len);

// int account_db_json_summary(char *json, int len);

// int account_db_string(int index, char *str, int len);


// account_log_t *account_log_new();

// void account_log_destroy(account_log_t *log);

// void account_log_set_date(account_log_t *log,
//                           const char *date);

// const char *account_log_get_date(account_log_t *log);

// void account_log_set_name(account_log_t *log,
//                           const char *name);

// const char *account_log_get_name(account_log_t *log);

// void account_log_set_code(account_log_t *log,
//                           const char *code);

// const char *account_log_get_code(account_log_t *log);

// void account_log_set_type(account_log_t *log,
//                           uint8_t type);

// uint8_t account_log_get_type(account_log_t *log);

// void account_log_set_granted(account_log_t *log,
//                              bool granted);

// bool account_log_get_granted(account_log_t *log);


// int account_db_log_insert(account_log_t *log);

// int account_db_log_remove_all(void);

// int account_db_log_get_first(void);

// int account_db_log_get_previous(uint16_t index);

// account_log_t *account_db_log_get_index(uint16_t index);

// #endif  /* __ACCOUNT_H__ */
