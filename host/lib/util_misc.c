/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Miscellaneous functions for userspace vboot utilities.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cryptolib.h"
#include "host_common.h"
#include "util_misc.h"
#include "vboot_common.h"

void PrintPubKeySha1Sum(VbPublicKey* key) {
  uint8_t* buf = ((uint8_t *)key) + key->key_offset;
  uint64_t buflen = key->key_size;
  uint8_t* digest = DigestBuf(buf, buflen, SHA1_DIGEST_ALGORITHM);
  int i;
  for (i=0; i<SHA1_DIGEST_SIZE; i++)
    printf("%02x", digest[i]);
  free(digest);
}
