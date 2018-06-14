/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <openssl/hmac.h>

#include "2sha.h"
#include "2hmac.h"
#include "test_common.h"

const char short_key[] = "key";
const char message[] = "The quick brown fox jumps over the lazy dog";
/* This is supposed to be longer than the supported block sizes */
const char long_key[] =
	"loooooooooooooooooooooooooooooooooooooooooooonooooooooooooooooooo"
	"ooooooooooooooooooooooooooooooooooooooooooooonooooooooooooog key";

static void test_hmac_by_openssl(enum vb2_hash_algorithm alg,
				 const void *key, uint32_t key_size,
				 const void *msg, uint32_t msg_size)
{
	uint8_t mac[VB2_MAX_DIGEST_SIZE];
	uint32_t mac_size = sizeof(mac);
	uint8_t md[VB2_MAX_DIGEST_SIZE];
	uint32_t md_size = sizeof(md);
	char test_name[256];

	switch (alg) {
	case VB2_HASH_SHA1:
		HMAC(EVP_sha1(), key, key_size, msg, msg_size, md, &md_size);
		break;
	case VB2_HASH_SHA256:
		HMAC(EVP_sha256(), key, key_size, msg, msg_size, md, &md_size);
		break;
	case VB2_HASH_SHA512:
		HMAC(EVP_sha512(), key, key_size, msg, msg_size, md, &md_size);
		break;
	default:
		TEST_SUCC(-1, "Unsupported hash algorithm");
	}
	sprintf(test_name, "%s: HMAC-%s (key_size=%d)",
		__func__, vb2_get_hash_algorithm_name(alg), key_size);
	TEST_SUCC(hmac(alg, key, key_size, msg, msg_size, mac, mac_size),
		  test_name);
	TEST_SUCC(memcmp(mac, md, md_size), "HMAC digests match");
}

static void test_hmac_error(void)
{
	uint8_t mac[VB2_MAX_DIGEST_SIZE];
	enum vb2_hash_algorithm alg;

	alg = VB2_HASH_SHA1;
	TEST_TRUE(hmac(alg, NULL, 0,
		       message, strlen(message), mac, sizeof(mac)),
		  "key = NULL");
	TEST_TRUE(hmac(alg, short_key, strlen(short_key),
		       NULL, 0, mac, sizeof(mac)),
		  "msg = NULL");
	TEST_TRUE(hmac(alg, short_key, strlen(short_key),
		       message, strlen(message), NULL, 0),
		  "mac = NULL");
	TEST_TRUE(hmac(alg, short_key, strlen(short_key),
		       message, strlen(message), mac, 0),
		  "Buffer too small");
	alg = -1;
	TEST_TRUE(hmac(alg, short_key, strlen(short_key),
		       message, strlen(message), mac, sizeof(mac)),
		  "Invalid algorithm");
}

static void test_hmac(void)
{
	int alg;

	for (alg = 1; alg < VB2_HASH_ALG_COUNT; alg++) {
		/* Try short key */
		test_hmac_by_openssl(alg, short_key, strlen(short_key),
				     message, strlen(message));
		/* Try key longer than a block size */
		test_hmac_by_openssl(alg, long_key, strlen(long_key),
				     message, strlen(message));
		/* Try empty key and message */
		test_hmac_by_openssl(alg, "", 0, "", 0);
	}
}

int main(void)
{
	test_hmac();
	test_hmac_error();

	return gTestSuccess ? 0 : 255;
}
