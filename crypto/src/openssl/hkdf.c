/*
 * Copyright 2017-2019 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// NOTE: OpenSSL headers sometimes (depending on a version) contain some of the
// symbols poisoned via inclusion of avs_commons_config.h. Therefore they must
// be included first.
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/opensslv.h>

#include <avs_commons_config.h>

#include <avsystem/commons/hkdf.h>

#include <assert.h>

VISIBILITY_SOURCE_BEGIN

#if OPENSSL_VERSION_NUMBER >= 0x1000000f

// Adapted from:
// https://www.openssl.org/docs/man1.1.1/man3/EVP_PKEY_CTX_set1_hkdf_key.html
int avs_crypto_hkdf_sha_256(const unsigned char *salt, size_t salt_len,
                            const unsigned char *ikm, size_t ikm_len,
                            const unsigned char *info, size_t info_len,
                            unsigned char *out_okm, size_t *inout_okm_len) {
    assert(!salt_len || salt);
    assert(ikm && ikm_len);
    assert(!info_len || info);
    assert(out_okm && inout_okm_len);

    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, NULL);

    if (EVP_PKEY_derive_init(pctx) <= 0
            || EVP_PKEY_CTX_set_hkdf_md(pctx, EVP_sha256()) <= 0
            || EVP_PKEY_CTX_set1_hkdf_key(pctx, ikm, (int) ikm_len) <= 0) {
        return -1;
    }

    if (salt_len
            && EVP_PKEY_CTX_set1_hkdf_salt(pctx, salt, (int) salt_len) <= 0) {
        return -1;
    }

    if (info_len
            && EVP_PKEY_CTX_add1_hkdf_info(pctx, info, (int) info_len) <= 0) {
        return -1;
    }

    if (EVP_PKEY_derive(pctx, out_okm, inout_okm_len) <= 0) {
        return -1;
    }

    EVP_PKEY_CTX_free(pctx);
    return 0;
}

#else

#error "HKDF is not supported if OpenSSL version < 1.1.0"

#endif
