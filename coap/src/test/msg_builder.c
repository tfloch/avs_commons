/*
 * Copyright 2017 AVSystem <avsystem@avsystem.com>
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

#include <config.h>

#include <sys/types.h>
#include <alloca.h>

#include <avsystem/commons/coap/content_format.h>
#include <avsystem/commons/unit/test.h>

#define RANDOM_MSGID ((uint16_t)4)
#define RANDOM_MSGID_NBO ((uint16_t){htons(RANDOM_MSGID)})
#define RANDOM_MSGID_INIT { ((const uint8_t*)&RANDOM_MSGID_NBO)[0], \
                            ((const uint8_t*)&RANDOM_MSGID_NBO)[1] }

#define VTTL(version, type, token_length) \
    ((((version) & 0x03) << 6) | (((type) & 0x03) << 4) | ((token_length) & 0x07))

static anjay_coap_msg_t *make_msg_template(void *buffer,
                                           size_t buffer_size) {
    assert(buffer_size >= offsetof(anjay_coap_msg_t, content));

    anjay_coap_msg_t *msg = (anjay_coap_msg_t *)buffer;

    memset(buffer, 0, buffer_size);
    msg->length = (uint32_t)(buffer_size - sizeof(msg->length));
    msg->header = (anjay_coap_msg_header_t) {
        .version_type_token_length = VTTL(1, AVS_COAP_MSG_CONFIRMABLE, 0),
        .code = AVS_COAP_CODE_CONTENT,
        .message_id = RANDOM_MSGID_INIT
    };

    return msg;
}

static anjay_coap_msg_t *make_msg_template_with_data(void *buffer,
                                                     const void *data,
                                                     size_t data_size) {
    anjay_coap_msg_t *msg =
            make_msg_template(buffer,
                              offsetof(anjay_coap_msg_t, content) + data_size);

    memcpy(msg->content, data, data_size);
    return msg;
}

#define DECLARE_MSG_TEMPLATE(VarName, SizeVarName, DataSize) \
    const size_t SizeVarName = \
            offsetof(anjay_coap_msg_t, content) + (DataSize); \
    anjay_coap_msg_t *VarName = make_msg_template(alloca(SizeVarName), \
                                                  SizeVarName)

#define DECLARE_MSG_TEMPLATE_WITH_DATA(VarName, SizeVarName, Data) \
    const size_t SizeVarName = \
            offsetof(anjay_coap_msg_t, content) + sizeof(Data) - 1; \
    anjay_coap_msg_t *VarName = make_msg_template_with_data( \
            alloca(SizeVarName), (Data), sizeof(Data) - 1)

#define INFO_WITH_DUMMY_HEADER \
    avs_coap_msg_info_init(); \
    info.type = AVS_COAP_MSG_CONFIRMABLE; \
    info.code = AVS_COAP_CODE_CONTENT; \
    info.identity.msg_id = 0

#define INFO_WITH_HEADER(HeaderPtr) \
    avs_coap_msg_info_init(); \
    info.type = avs_coap_msg_header_get_type(HeaderPtr); \
    info.code = (HeaderPtr)->code; \
    info.identity.msg_id = extract_u16((HeaderPtr)->message_id)

AVS_UNIT_TEST(coap_builder, header_only) {
    DECLARE_MSG_TEMPLATE(msg_tpl, msg_tpl_size, 0);
    anjay_coap_msg_info_t info =
            INFO_WITH_HEADER(&msg_tpl->header);

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    const anjay_coap_msg_t *msg = avs_coap_msg_build_without_payload(
            avs_coap_ensure_aligned_buffer(storage), storage_size,
            &info);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);
    free(storage);
}

AVS_UNIT_TEST(coap_info, token) {
    anjay_coap_token_t TOKEN = {"A Token"};
    const size_t msg_tpl_size =
            offsetof(anjay_coap_msg_t, content) + AVS_COAP_MAX_TOKEN_LENGTH;
    anjay_coap_msg_t *msg_tpl = make_msg_template_with_data(
            alloca(msg_tpl_size), &TOKEN, AVS_COAP_MAX_TOKEN_LENGTH);
    _anjay_coap_msg_header_set_token_length(&msg_tpl->header, sizeof(TOKEN));

    anjay_coap_msg_info_t info =
            INFO_WITH_HEADER(&msg_tpl->header);
    info.identity.token = TOKEN;
    info.identity.token_size = sizeof(TOKEN);

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    const anjay_coap_msg_t *msg = avs_coap_msg_build_without_payload(
            avs_coap_ensure_aligned_buffer(storage), storage_size,
            &info);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, sizeof(msg_tpl->length) + msg_tpl->length);
    free(storage);
}

AVS_UNIT_TEST(coap_builder, option_empty) {
    DECLARE_MSG_TEMPLATE_WITH_DATA(msg_tpl, msg_tpl_size, "\x00");
    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_empty(&info, 0));

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    const anjay_coap_msg_t *msg = avs_coap_msg_build_without_payload(
            avs_coap_ensure_aligned_buffer(storage), storage_size,
            &info);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);

    avs_coap_msg_info_reset(&info);
    free(storage);
}

AVS_UNIT_TEST(coap_builder, option_opaque) {
    DECLARE_MSG_TEMPLATE_WITH_DATA(msg_tpl, msg_tpl_size, "\x00" "foo");
    _anjay_coap_opt_set_short_length((anjay_coap_opt_t *)msg_tpl->content, 3);

    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_opaque(&info, 0, "foo", sizeof("foo") - 1));

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    const anjay_coap_msg_t *msg = avs_coap_msg_build_without_payload(
            avs_coap_ensure_aligned_buffer(storage), storage_size,
            &info);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);

    avs_coap_msg_info_reset(&info);
    free(storage);
}

AVS_UNIT_TEST(coap_builder, option_multiple_ints) {
    uint32_t content_length = sizeof(uint8_t) + sizeof(uint8_t)
                            + sizeof(uint8_t) + sizeof(uint16_t)
                            + sizeof(uint8_t) + sizeof(uint32_t)
                            + sizeof(uint8_t) + sizeof(uint64_t)
                            + sizeof(uint8_t) + sizeof(uint8_t)
                            + sizeof(uint8_t);
    DECLARE_MSG_TEMPLATE(msg_tpl, msg_tpl_size, content_length);

    _anjay_coap_opt_set_short_length((anjay_coap_opt_t *)&msg_tpl->content[0], 1);
    msg_tpl->content[1] = 0x10;
    _anjay_coap_opt_set_short_length((anjay_coap_opt_t *)&msg_tpl->content[2], 2);
    msg_tpl->content[3] = 0x21;
    msg_tpl->content[4] = 0x20;
    _anjay_coap_opt_set_short_length((anjay_coap_opt_t *)&msg_tpl->content[5], 4);
    msg_tpl->content[6] = 0x43;
    msg_tpl->content[7] = 0x42;
    msg_tpl->content[8] = 0x41;
    msg_tpl->content[9] = 0x40;
    _anjay_coap_opt_set_short_length((anjay_coap_opt_t *)&msg_tpl->content[10], 8);
    msg_tpl->content[11] = 0x87;
    msg_tpl->content[12] = 0x86;
    msg_tpl->content[13] = 0x85;
    msg_tpl->content[14] = 0x84;
    msg_tpl->content[15] = 0x83;
    msg_tpl->content[16] = 0x82;
    msg_tpl->content[17] = 0x81;
    msg_tpl->content[18] = 0x80;
    _anjay_coap_opt_set_short_length((anjay_coap_opt_t *)&msg_tpl->content[19], 1);
    msg_tpl->content[20] = 0xFF;
    _anjay_coap_opt_set_short_length((anjay_coap_opt_t *)&msg_tpl->content[21], 0);

    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_uint(&info, 0, &(uint8_t)  { 0x10 },               1));
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_uint(&info, 0, &(uint16_t) { 0x2120 },             2));
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_uint(&info, 0, &(uint32_t) { 0x43424140 },         4));
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_uint(&info, 0, &(uint64_t) { 0x8786858483828180 }, 8));
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_uint(&info, 0, &(uint64_t) { 0xFF },               8));
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_uint(&info, 0, &(uint64_t) { 0 },                  8));

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    const anjay_coap_msg_t *msg = avs_coap_msg_build_without_payload(
            avs_coap_ensure_aligned_buffer(storage), storage_size,
            &info);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);

    avs_coap_msg_info_reset(&info);
    free(storage);
}

AVS_UNIT_TEST(coap_builder, option_content_format) {
    uint32_t content_length = sizeof(uint8_t) + sizeof(uint16_t);
    DECLARE_MSG_TEMPLATE(msg_tpl, msg_tpl_size, content_length);
    _anjay_coap_opt_set_short_length((anjay_coap_opt_t *)&msg_tpl->content[0], 2);
    _anjay_coap_opt_set_short_delta((anjay_coap_opt_t *)&msg_tpl->content[0], AVS_COAP_OPT_CONTENT_FORMAT);
    memcpy(&msg_tpl->content[1], &(uint16_t){htons(AVS_COAP_FORMAT_TLV)}, 2);

    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_content_format(&info, AVS_COAP_FORMAT_TLV));

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    const anjay_coap_msg_t *msg = avs_coap_msg_build_without_payload(
            avs_coap_ensure_aligned_buffer(storage), storage_size,
            &info);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);

    avs_coap_msg_info_reset(&info);
    free(storage);
}

#define PAYLOAD "trololo"

AVS_UNIT_TEST(coap_builder, payload_only) {
    DECLARE_MSG_TEMPLATE_WITH_DATA(msg_tpl, msg_tpl_size, "\xFF" PAYLOAD);
    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info)
                          + sizeof(AVS_COAP_PAYLOAD_MARKER)
                          + sizeof(PAYLOAD) - 1;
    void *storage = malloc(storage_size);

    anjay_coap_msg_builder_t builder;
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_builder_init(
                &builder,
                avs_coap_ensure_aligned_buffer(storage),
                storage_size, &info));

    AVS_UNIT_ASSERT_EQUAL(sizeof(PAYLOAD) - 1, avs_coap_msg_builder_payload(&builder, PAYLOAD, sizeof(PAYLOAD) - 1));

    const anjay_coap_msg_t *msg = avs_coap_msg_builder_get_msg(&builder);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);
    free(storage);
}

#undef PAYLOAD

#define PAYLOAD1 "I can haz "
#define PAYLOAD2 "payload"
#define PAYLOAD_SIZE (sizeof(PAYLOAD1 PAYLOAD2) - 1)

AVS_UNIT_TEST(coap_builder, incremental_payload) {
    DECLARE_MSG_TEMPLATE_WITH_DATA(msg_tpl, msg_tpl_size,
                                   "\xFF" PAYLOAD1 PAYLOAD2);

    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info)
                          + sizeof(AVS_COAP_PAYLOAD_MARKER)
                          + PAYLOAD_SIZE;
    void *storage = malloc(storage_size);

    anjay_coap_msg_builder_t builder;
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_builder_init(
                &builder,
                avs_coap_ensure_aligned_buffer(storage),
                storage_size, &info));

    AVS_UNIT_ASSERT_EQUAL(sizeof(PAYLOAD1) - 1, avs_coap_msg_builder_payload(&builder, PAYLOAD1, sizeof(PAYLOAD1) - 1));
    AVS_UNIT_ASSERT_EQUAL(sizeof(PAYLOAD2) - 1, avs_coap_msg_builder_payload(&builder, PAYLOAD2, sizeof(PAYLOAD2) - 1));

    const anjay_coap_msg_t *msg = avs_coap_msg_builder_get_msg(&builder);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);
    free(storage);
}

#undef PAYLOAD1
#undef PAYLOAD2
#undef PAYLOAD_SIZE

#define OPT_EXT_DELTA1 "\xD0\x00"
#define OPT_EXT_DELTA2 "\xE0\x00\x00"

AVS_UNIT_TEST(coap_builder, option_ext_number) {
    DECLARE_MSG_TEMPLATE_WITH_DATA(msg_tpl, msg_tpl_size,
                                   OPT_EXT_DELTA1 OPT_EXT_DELTA2);
    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);

    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_empty(&info, 13));
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_empty(&info, 13 + 269));

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    const anjay_coap_msg_t *msg = avs_coap_msg_build_without_payload(
            avs_coap_ensure_aligned_buffer(storage), storage_size,
            &info);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);

    avs_coap_msg_info_reset(&info);
    free(storage);
}

#undef OPT_EXT_DELTA1
#undef OPT_EXT_DELTA2

#define ZEROS_8 "\x00\x00\x00\x00\x00\x00\x00\x00"
#define ZEROS_64 ZEROS_8 ZEROS_8 ZEROS_8 ZEROS_8 ZEROS_8 ZEROS_8 ZEROS_8 ZEROS_8
#define ZEROS_256 ZEROS_64 ZEROS_64 ZEROS_64 ZEROS_64

#define ZEROS_13 ZEROS_8 "\x00\x00\x00\x00\x00"
#define ZEROS_269 ZEROS_256 ZEROS_13

#define OPT_EXT_LENGTH1 "\x0D\x00"
#define OPT_EXT_LENGTH2 "\x0E\x00\x00"

AVS_UNIT_TEST(coap_builder, option_ext_length) {
    DECLARE_MSG_TEMPLATE_WITH_DATA(msg_tpl, msg_tpl_size,
                                   OPT_EXT_LENGTH1 ZEROS_13 OPT_EXT_LENGTH2 ZEROS_269);
    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);

    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_opaque(&info, 0, ZEROS_13, sizeof(ZEROS_13) - 1));
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_opaque(&info, 0, ZEROS_269, sizeof(ZEROS_269) - 1));

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    const anjay_coap_msg_t *msg = avs_coap_msg_build_without_payload(
            avs_coap_ensure_aligned_buffer(storage), storage_size,
            &info);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);

    avs_coap_msg_info_reset(&info);
    free(storage);
}

#undef OPT_EXT_LENGTH1
#undef OPT_EXT_LENGTH2

#define STRING "SomeString"

AVS_UNIT_TEST(coap_builder, opt_string) {
    DECLARE_MSG_TEMPLATE_WITH_DATA(msg_tpl, msg_tpl_size, "\x0A" STRING);
    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);

    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_info_opt_string(&info, 0, STRING));

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    const anjay_coap_msg_t *msg = avs_coap_msg_build_without_payload(
            avs_coap_ensure_aligned_buffer(storage), storage_size,
            &info);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);

    avs_coap_msg_info_reset(&info);
    free(storage);
}

#undef STRING

#define DATA_16 "0123456789abcdef"
#define DATA_256 DATA_16 DATA_16 DATA_16 DATA_16 DATA_16 DATA_16 DATA_16 DATA_16 \
                 DATA_16 DATA_16 DATA_16 DATA_16 DATA_16 DATA_16 DATA_16 DATA_16
#define DATA_8192 DATA_256 DATA_256 DATA_256 DATA_256 DATA_256 DATA_256 DATA_256 DATA_256 \
                  DATA_256 DATA_256 DATA_256 DATA_256 DATA_256 DATA_256 DATA_256 DATA_256
#define DATA_65536 DATA_8192 DATA_8192 DATA_8192 DATA_8192 DATA_8192 DATA_8192 DATA_8192 DATA_8192 \
                   DATA_8192 DATA_8192 DATA_8192 DATA_8192 DATA_8192 DATA_8192 DATA_8192 DATA_8192

AVS_UNIT_TEST(coap_builder, opt_string_too_long) {
    anjay_coap_msg_info_t info = INFO_WITH_DUMMY_HEADER;
    AVS_UNIT_ASSERT_FAILED(avs_coap_msg_info_opt_string(&info, 0, DATA_65536));
}

#undef DATA_16
#undef DATA_256
#undef DATA_8192
#undef DATA_65536

AVS_UNIT_TEST(coap_builder, payload_call_with_zero_size) {
    DECLARE_MSG_TEMPLATE(msg_tpl, msg_tpl_size, 0);
    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info);
    void *storage = malloc(storage_size);

    anjay_coap_msg_builder_t builder;
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_builder_init(
                &builder,
                avs_coap_ensure_aligned_buffer(storage),
                storage_size, &info));

    AVS_UNIT_ASSERT_EQUAL(0, avs_coap_msg_builder_payload(&builder, "", 0));
    const anjay_coap_msg_t *msg = avs_coap_msg_builder_get_msg(&builder);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);
    free(storage);
}

#define PAYLOAD "And IiiiiiiiiiiiiiiIIIiiiii will alllwayyyyyys crash youuuuUUUUuuu"

AVS_UNIT_TEST(coap_builder, payload_call_with_zero_size_then_nonzero) {
    DECLARE_MSG_TEMPLATE_WITH_DATA(msg_tpl, msg_tpl_size, "\xFF" PAYLOAD);

    anjay_coap_msg_info_t info = INFO_WITH_HEADER(&msg_tpl->header);

    size_t storage_size = avs_coap_msg_info_get_storage_size(&info)
                          + sizeof(AVS_COAP_PAYLOAD_MARKER)
                          + sizeof(PAYLOAD);
    void *storage = malloc(storage_size);

    anjay_coap_msg_builder_t builder;
    AVS_UNIT_ASSERT_SUCCESS(avs_coap_msg_builder_init(
                &builder,
                avs_coap_ensure_aligned_buffer(storage),
                storage_size, &info));

    AVS_UNIT_ASSERT_EQUAL(0, avs_coap_msg_builder_payload(&builder, "", 0));
    AVS_UNIT_ASSERT_EQUAL(sizeof(PAYLOAD) - 1, avs_coap_msg_builder_payload(&builder, PAYLOAD, sizeof(PAYLOAD) - 1));

    const anjay_coap_msg_t *msg = avs_coap_msg_builder_get_msg(&builder);

    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(msg, msg_tpl, msg_tpl_size);
    free(storage);
}

#undef PAYLOAD
