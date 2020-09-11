/*
 * Copyright 2017-2020 AVSystem <avsystem@avsystem.com>
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
#ifndef NET_OPENSSL_DATA_LOADER_H
#define NET_OPENSSL_DATA_LOADER_H

#include <avsystem/commons/avs_crypto_pki.h>

#include "../avs_crypto_utils.h"

VISIBILITY_PRIVATE_HEADER_BEGIN

// NOTE: <openssl/ssl.h> could not be included here. See the note about OpenSSL
// in avs_openssl_data_loader.c for details (and think about circular
// dependencies with VISIBILITY_PRIVATE_HEADER_BEGIN macros).

avs_error_t
_avs_crypto_openssl_load_ca_certs(X509_STORE *store,
                                  const avs_crypto_trusted_cert_info_t *info);

avs_error_t _avs_crypto_openssl_load_crls(
        X509_STORE *store, const avs_crypto_cert_revocation_list_info_t *info);

avs_error_t
_avs_crypto_openssl_load_client_key(EVP_PKEY **out_key,
                                    const avs_crypto_client_key_info_t *info);

typedef avs_error_t avs_crypto_openssl_load_certs_cb_t(void *arg, X509 *cert);

avs_error_t
_avs_crypto_openssl_load_client_cert(const avs_crypto_client_cert_info_t *info,
                                     avs_crypto_openssl_load_certs_cb_t *cb,
                                     void *cb_arg);

VISIBILITY_PRIVATE_HEADER_END
#endif // NET_OPENSSL_DATA_LOADER_H
