/*
 * SPDX-FileCopyrightText: 2020-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ESP_TLS_CRYPTO_H
#define _ESP_TLS_CRYPTO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate sha1 sum
 * esp-tls abstraction for crypto sha1 API, calculates the sha1 sum(digest) of
 * the data provided in input which is of ilen size and returns
 * a 20 char sha1 sum
 * @param[in]   input   Input array
 * @param[in]   ilen    Length of Input array
 * @param[out]  output  calculated sha1 sum
 *
 * @return
 * mbedtls stack:-
 *              - MBEDTLS_ERR_SHA1_BAD_INPUT_DATA   on BAD INPUT.
 *              -  0 on success.
 * wolfssl stack:-
 *              - -1    on failure.
 *              -  0    on success.
 */
int esp_crypto_sha1(const unsigned char *input,
                    size_t ilen,
                    unsigned char output[20]);

/**
 * @brief Do Base64 encode of the src data
 *
 * @param[in]   dst   destination buffer
 * @param[in]   dlen  length of destination buffer
 * @param[out]  olen  number of bytes written
 * @param[in]   src   src buffer to be encoded
 * @param[in]   slen  src buffer len
 *
 * @return
 * mbedtls stack:-
 *               - MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL  if buffer is of insufficient size.
 *               -  0   if successful.
 * wolfssl stack:-
 *               - <0   on failure.
 *               -  0   if succcessful.
 */
int esp_crypto_base64_encode(unsigned char *dst, size_t dlen,
                             size_t *olen, const unsigned char *src,
                             size_t slen);

#ifdef __cplusplus
}
#endif

/*
 * SPDX-FileCopyrightText: 2020-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_tls_crypto.h"
#include "esp_log.h"
#include "esp_err.h"
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"
#define _esp_crypto_sha1 esp_crypto_sha1_mbedtls
#define _esp_crypto_base64_encode esp_crypto_bas64_encode_mbedtls
#elif  CONFIG_ESP_TLS_USING_WOLFSSL
#include "wolfssl/ssl.h" /* SHA functions are listed in wolfssl/ssl.h */
#include "wolfssl/wolfcrypt/coding.h"
#define _esp_crypto_sha1 esp_crypto_sha1_wolfSSL
#define _esp_crypto_base64_encode esp_crypto_base64_encode_woflSSL
#endif

#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
static int esp_crypto_sha1_mbedtls( const unsigned char *input,
                                    size_t ilen,
                                    unsigned char output[20])
{
    int ret = mbedtls_sha1_ret(input, ilen, output);
    if (ret != 0) {
        ESP_LOGE("esp_crypto", "Error in calculating sha1 sum , Returned 0x%02X", ret);
    }
    return ret;
}

static int esp_crypto_bas64_encode_mbedtls( unsigned char *dst, size_t dlen,
        size_t *olen, const unsigned char *src,
        size_t slen)
{
    return mbedtls_base64_encode(dst, dlen, olen, src, slen);
}

#elif CONFIG_ESP_TLS_USING_WOLFSSL
static int esp_crypto_sha1_wolfSSL( const unsigned char *input,
                                    size_t ilen,
                                    unsigned char output[20])
{
    unsigned char *ret = wolfSSL_SHA1(input, ilen, output);
    if (ret == NULL) {
        ESP_LOGE("esp_crypto", "Error in calculating sha1 sum");
        return -1;
    }
    return 0;
}

static int esp_crypto_base64_encode_woflSSL(unsigned char *dst, size_t dlen, size_t *olen,
        const unsigned char *src, size_t slen)
{
    *olen = dlen;
    return Base64_Encode((const byte *) src, (word32) slen, (byte *) dst, (word32 *) olen);
}

#else
#error "No TLS/SSL Stack selected"
#endif

int esp_crypto_sha1( const unsigned char *input,
                     size_t ilen,
                     unsigned char output[20])
{
    return _esp_crypto_sha1(input, ilen, output);
}

int esp_crypto_base64_encode(unsigned char *dst, size_t dlen, size_t *olen,
                             const unsigned char *src, size_t slen )
{
    return _esp_crypto_base64_encode(dst, dlen, olen, src, slen);
}

#endif /* _ESP_TLS_CRYPTO_H */
