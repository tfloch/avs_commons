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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <avsystem/commons/unit/test.h>

static char TEMPLATE[] = "/tmp/test_stream_file-XXXXXX";

static int make_temporary(char *out_filename) {
    char template[sizeof(TEMPLATE)];
    int result;
    memcpy(template, TEMPLATE, sizeof(TEMPLATE));
    result = mkstemp(template);
    if (result != -1) {
        close(result);
    }
    memcpy(out_filename, template, sizeof(TEMPLATE));
    return result < 0 ? -1 : 0;
}

AVS_UNIT_TEST(stream_file, init) {
    char filename[sizeof(TEMPLATE)];
    avs_stream_abstract_t *stream;
    AVS_UNIT_ASSERT_SUCCESS(make_temporary(filename));
    AVS_UNIT_ASSERT_NOT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_READ)));
    avs_stream_cleanup(&stream);

    AVS_UNIT_ASSERT_NOT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_WRITE)));
    avs_stream_cleanup(&stream);

    AVS_UNIT_ASSERT_NULL(avs_stream_file_create(filename, 0xff));
    unlink(filename);
}

AVS_UNIT_TEST(stream_file, write_mode_creates_file) {
    char filename[sizeof(TEMPLATE)];
    avs_stream_abstract_t *stream;
    AVS_UNIT_ASSERT_SUCCESS(make_temporary(filename));
    unlink(filename);
    AVS_UNIT_ASSERT_NOT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_WRITE)));
    avs_stream_cleanup(&stream);
}

AVS_UNIT_TEST(stream_file, write_and_read) {
    char filename[sizeof(TEMPLATE)];
    char data[] = "TEST";
    size_t bytes_read;
    char end_of_msg;
    /* let the valgrind check for eventual overflows */
    char *buf = malloc(sizeof(data));

    avs_stream_abstract_t *stream;

    AVS_UNIT_ASSERT_NOT_NULL(buf);
    AVS_UNIT_ASSERT_SUCCESS(make_temporary(filename));
    AVS_UNIT_ASSERT_NOT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_WRITE | AVS_STREAM_FILE_READ)));

    AVS_UNIT_ASSERT_SUCCESS(avs_stream_write(stream, data, sizeof(data)));
    AVS_UNIT_ASSERT_SUCCESS(avs_stream_write(stream, NULL, 0));

    AVS_UNIT_ASSERT_SUCCESS(avs_stream_reset(stream));
    AVS_UNIT_ASSERT_SUCCESS(avs_stream_read(stream, &bytes_read, &end_of_msg, buf, 2*sizeof(buf)));
    AVS_UNIT_ASSERT_EQUAL(bytes_read, sizeof(data));
    AVS_UNIT_ASSERT_EQUAL(end_of_msg, 1);
    AVS_UNIT_ASSERT_EQUAL_BYTES_SIZED(buf, data, sizeof(data));
    avs_stream_cleanup(&stream);

    AVS_UNIT_ASSERT_NOT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_READ)));
    AVS_UNIT_ASSERT_FAILED(avs_stream_write(stream, data, sizeof(data)));
    AVS_UNIT_ASSERT_EQUAL(avs_stream_errno(stream), EBADF);
    avs_stream_cleanup(&stream);
    unlink(filename);
    free(buf);
}

AVS_UNIT_TEST(stream_file, seek_peek_and_read) {
    char filename[sizeof(TEMPLATE)];
    char buf[128];
    size_t bytes_read;
    char end_of_msg;
    avs_stream_abstract_t *stream;

    AVS_UNIT_ASSERT_SUCCESS(make_temporary(filename));
    unlink(filename);
    AVS_UNIT_ASSERT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_READ)));
    AVS_UNIT_ASSERT_SUCCESS(make_temporary(filename));
    AVS_UNIT_ASSERT_NOT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_READ)));

    AVS_UNIT_ASSERT_SUCCESS(avs_stream_read(stream, &bytes_read, &end_of_msg, buf, 0));
    AVS_UNIT_ASSERT_EQUAL(end_of_msg, 0);
    AVS_UNIT_ASSERT_SUCCESS(avs_stream_read(stream, &bytes_read, &end_of_msg, buf, 1));
    AVS_UNIT_ASSERT_EQUAL(end_of_msg, 1);
    AVS_UNIT_ASSERT_EQUAL(bytes_read, 0);
    AVS_UNIT_ASSERT_EQUAL(avs_stream_peek(stream, 0), EOF);
    AVS_UNIT_ASSERT_EQUAL(avs_stream_peek(stream, 9001), EOF);
    avs_stream_cleanup(&stream);

    AVS_UNIT_ASSERT_NOT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_READ)));
    AVS_UNIT_ASSERT_EQUAL(avs_stream_peek(stream, 9001), EOF);
    AVS_UNIT_ASSERT_EQUAL(avs_stream_peek(stream, 0), EOF);
    avs_stream_cleanup(&stream);
    unlink(filename);
}

AVS_UNIT_TEST(stream_file, extensions_seek) {
    char filename[sizeof(TEMPLATE)];
    char data[] = "TEST";
    avs_stream_abstract_t *stream;
    AVS_UNIT_ASSERT_SUCCESS(make_temporary(filename));
    AVS_UNIT_ASSERT_NOT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_READ | AVS_STREAM_FILE_WRITE)));
    AVS_UNIT_ASSERT_SUCCESS(avs_stream_file_seek(stream, 9001));
    AVS_UNIT_ASSERT_EQUAL(avs_stream_peek(stream, 0), EOF);
    AVS_UNIT_ASSERT_SUCCESS(avs_stream_reset(stream));
    AVS_UNIT_ASSERT_SUCCESS(avs_stream_write(stream, data, sizeof(data)));
    avs_stream_cleanup(&stream);
    unlink(filename);
}

AVS_UNIT_TEST(stream_file, extensions_length) {
    char filename[sizeof(TEMPLATE)];
    char data[] = "TEST";
    avs_off_t length;
    avs_stream_abstract_t *stream;
    AVS_UNIT_ASSERT_SUCCESS(make_temporary(filename));
    AVS_UNIT_ASSERT_NOT_NULL((stream = avs_stream_file_create(filename, AVS_STREAM_FILE_WRITE)));
    AVS_UNIT_ASSERT_SUCCESS(avs_stream_write(stream, data, sizeof(data)));
    AVS_UNIT_ASSERT_SUCCESS(avs_stream_write(stream, data, sizeof(data)));
    AVS_UNIT_ASSERT_SUCCESS(avs_stream_file_length(stream, &length));
    AVS_UNIT_ASSERT_EQUAL(length, 2*sizeof(data));
    avs_stream_cleanup(&stream);
    unlink(filename);
}
