//
// Copyright (c) 2023 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//
#include <stdio.h>
#include <string.h>

#include "zenoh.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#else
#include <unistd.h>
#endif

int main(int argc, char **argv) {
    char *keyexpr = "demo/example/zenoh-c-pub";
    char *value = "Pub from C!";

    if (argc > 1) keyexpr = argv[1];
    if (argc > 2) value = argv[2];

    z_owned_config_t config = z_config_default();
    if (argc > 3) {
        if (zc_config_insert_json(z_loan(config), Z_CONFIG_CONNECT_KEY, argv[3]) < 0) {
            printf(
                "Couldn't insert value `%s` in configuration at `%s`. This is likely because `%s` expects a "
                "JSON-serialized list of strings\n",
                argv[3], Z_CONFIG_CONNECT_KEY, Z_CONFIG_CONNECT_KEY);
            exit(-1);
        }
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    printf("Declaring Publisher on '%s'...\n", keyexpr);
    z_owned_publisher_t pub = z_declare_publisher(z_loan(s), z_keyexpr(keyexpr), NULL);
    if (!z_check(pub)) {
        printf("Unable to declare Publisher for key expression!\n");
        exit(-1);
    }

    z_publisher_put_options_t options = z_publisher_put_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);

    // allocate attachment map
    z_owned_bytes_map_t map = z_bytes_map_new();

    // set it as an attachment
    options.attachment = z_bytes_map_as_attachment(&map);

    // add some value
    z_bytes_map_insert_by_alias(&map, z_bytes_from_str("source"), z_bytes_from_str("C"));

    char buf[256];
    char buf_ind[16];
    for (int idx = 0; 1; ++idx) {
        sleep(1);

        // add some other attachment value
        sprintf(buf_ind, "%d", idx);
        z_bytes_map_insert_by_alias(&map, z_bytes_from_str("index"), z_bytes_from_str(buf_ind));

        sprintf(buf, "[%4d] %s", idx, value);
        printf("Putting Data ('%s': '%s')...\n", keyexpr, buf);
        z_publisher_put(z_loan(pub), (const uint8_t *)buf, strlen(buf), &options);
    }

    z_undeclare_publisher(z_move(pub));

    z_close(z_move(s));
    z_drop(z_move(map));

    return 0;
}
