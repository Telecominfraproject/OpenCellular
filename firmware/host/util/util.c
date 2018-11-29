/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* stdlib includes */
#include <stdio.h>
#include <util.h>

#define UTIL_NUMBER_ZERO 0

void hexdisp(const unsigned char *buf, int buflen)
{
    int c = UTIL_NUMBER_ZERO, i = UTIL_NUMBER_ZERO;
    for (c = UTIL_NUMBER_ZERO; c < buflen; c++) {
        printf("0x%02x ", buf[c]);
        if ((c + 1) % 8 == UTIL_NUMBER_ZERO) {
            printf("\t");
            for (i = (c + 1) - 8; i <= (c + 1); i++)
                printf("%c", (buf[i] > 0x20 && buf[i] < 0x7F) ? buf[i] : '.');
            printf("\n");
        }
    }
    printf("\n");
}
