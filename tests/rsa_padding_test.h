/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Signature tests vectors for checking RSA PKCS #1 v1.5 padding
 * implementations. These check an RSA signature verification implementation
 * against Daniel Bleichhenbacher's RSA signature padding attack.
 *
 * Test vectors are due to Daniel Bleichenbacher (bleichen@google.com).
 */

#ifndef VBOOT_REFERENCE_RSA_PADDING_TEST_H_
#define VBOOT_REFERENCE_RSA_PADDING_TEST_H_

#include "cryptolib.h"

/* The modulus of the public key (RSA-1024). */
static const uint8_t pubkey_n[] = {
210, 136, 105, 162, 117, 171, 114, 146, 81, 242, 21, 222, 87, 60, 122, 176, 245,
57, 213, 159, 196, 165, 40, 75, 146, 35, 114, 118, 25, 196, 150, 38, 40, 195,
56, 109, 145, 47, 171, 117, 57, 220, 176, 186, 70, 175, 222, 65, 60, 65, 31, 88,
93, 220, 27, 74, 73, 43, 112, 6, 242, 78, 38, 229, 58, 206, 200, 4, 196, 67, 8,
141, 21, 141, 79, 214, 221, 179, 184, 64, 92, 8, 222, 68, 16, 97, 101, 68, 210,
155, 217, 238, 78, 207, 202, 43, 16, 241, 194, 78, 28, 172, 124, 44, 111, 181,
187, 71, 57, 190, 50, 109, 254, 170, 103, 126, 124, 156, 169, 36, 164, 186, 136,
108, 66, 44, 147, 137};

/* The exponent of the public key (65535) */
static const uint8_t pubkey_e[] = {1, 0, 1};

/* The message for the test. */
static const uint8_t test_message[] = {0, 0, 0, 0};

/* The SHA1 digest of the message. i.e.
 * '9069ca78e7450a285173431b3e52c5c25299e473'. */
static const uint8_t test_message_sha1_hash[] = {
144, 105, 202, 120, 231, 69, 10, 40, 81, 115, 67, 27, 62, 82, 197, 194, 82, 153,
228, 115};

/* The test signatures (RSA-1024/SHA-1). signature[0] is correct,
 * all other signatures are incorrect.
 *
 * TODO(gauravsh): Add similar test vectors for other string
 */
static const uint8_t signatures[][RSA1024NUMBYTES] = {
#include "testcases/padding_test_vectors.inc"
};

#endif  /* VBOOT_REFERENCE_RSA_PADDING_TEST_H_ */
