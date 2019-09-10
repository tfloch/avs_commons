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

#include <avs_commons_config.h>

#include "global.h"
#include "net_impl.h"

#include <avsystem/commons/init_once.h>

VISIBILITY_SOURCE_BEGIN

static avs_init_once_handle_t g_net_init_handle;

static int initialize_global(void *err_ptr_) {
    avs_error_t *err_ptr = (avs_error_t *) err_ptr_;
    *err_ptr = _avs_net_initialize_global_compat_state();
    if (avs_is_ok(*err_ptr)) {
        *err_ptr = _avs_net_initialize_global_ssl_state();
        if (avs_is_err(*err_ptr)) {
            _avs_net_cleanup_global_compat_state();
        }
    }
    return avs_is_ok(*err_ptr) ? 0 : -1;
}

void _avs_net_cleanup_global_state(void) {
    _avs_net_cleanup_global_ssl_state();
    _avs_net_cleanup_global_compat_state();
    g_net_init_handle = NULL;
}

avs_error_t _avs_net_ensure_global_state(void) {
    AVS_STATIC_ASSERT(sizeof(int) >= 4, int_is_at_least_32bit);
    avs_error_t err = avs_errno(AVS_UNKNOWN_ERROR);
    if (avs_init_once(&g_net_init_handle, initialize_global, &err)) {
        assert(avs_is_err(err));
        return err;
    } else {
        return AVS_OK;
    }
}
