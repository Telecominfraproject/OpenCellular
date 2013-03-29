/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdint.h>
#include <stdio.h>

#define _STUB_IMPLEMENTATION_

#include "cryptolib.h"
#include "file_keys.h"
#include "rsa_padding_test.h"
#include "test_common.h"
#include "utility.h"
#include "vboot_api.h"


/* Data for mock functions */
static int mock_rsaverify_retval;

/* Mock functions */
uint8_t* DigestBuf(const uint8_t* buf, uint64_t len, int sig_algorithm) {
  /* Just need to return something; it's only passed to the mock RSAVerify() */
  return VbExMalloc(4);
}

int RSAVerify(const RSAPublicKey *key,
              const uint8_t* sig,
              const uint32_t sig_len,
              const uint8_t sig_type,
              const uint8_t* hash) {
  return mock_rsaverify_retval;
}

static void ResetMocks(void) {
  mock_rsaverify_retval = 1;
}

/* Test RSA utility funcs */
static void TestUtils(void) {
  RSAPublicKey* key;
  uint64_t u;

  /* Processed key size */
  TEST_EQ(RSAProcessedKeySize(0, &u), 1, "Processed key size 0");
  TEST_EQ(u, RSA1024NUMBYTES * 2 + sizeof(uint32_t) * 2,
          "Processed key size 0 size");
  TEST_EQ(RSAProcessedKeySize(3, &u), 1, "Processed key size 3");
  TEST_EQ(u, RSA2048NUMBYTES * 2 + sizeof(uint32_t) * 2,
          "Processed key size 3 size");
  TEST_EQ(RSAProcessedKeySize(7, &u), 1, "Processed key size 7");
  TEST_EQ(u, RSA4096NUMBYTES * 2 + sizeof(uint32_t) * 2,
          "Processed key size 7 size");
  TEST_EQ(RSAProcessedKeySize(11, &u), 1, "Processed key size 11");
  TEST_EQ(u, RSA8192NUMBYTES * 2 + sizeof(uint32_t) * 2,
          "Processed key size 11 size");
  TEST_EQ(RSAProcessedKeySize(kNumAlgorithms, &u), 0,
          "Processed key size invalid algorithm");

  /* Alloc key */
  key = RSAPublicKeyNew();
  TEST_EQ(key == NULL, 0, "New key not null");
  /* New key fields */
  TEST_PTR_EQ(key->n, NULL, "New key no n");
  TEST_PTR_EQ(key->rr, NULL, "New key no rr");
  TEST_EQ(key->len, 0, "New key len");
  TEST_EQ(key->algorithm, kNumAlgorithms, "New key no algorithm");
  /* Free key */
  RSAPublicKeyFree(key);
  /* Freeing null key shouldn't implode */
  RSAPublicKeyFree(NULL);
}

/* Test creating key from buffer */
static void TestKeyFromBuffer(void) {
  RSAPublicKey* key;
  uint8_t* buf;
  uint32_t* buf_key_len;
  int i;

  buf = malloc(8 + 2 * RSA8192NUMBYTES);
  buf_key_len = (uint32_t*)buf;

  for (i = 0; i < 4; i++) {
    uint32_t key_len = RSA1024NUMBYTES << i;
    Memset(buf, 0xAB, sizeof(buf));
    *buf_key_len = key_len / sizeof(uint32_t);
    *(buf_key_len + 1) = 0xF00D2345;  /* n0inv */
    buf[8] = 100;
    buf[8 + key_len - 1] = 101;
    buf[8 + key_len] = 120;
    buf[8 + key_len * 2 - 1] = 121;

    /* Correct length */
    key = RSAPublicKeyFromBuf(buf, 8 + key_len * 2);
    TEST_PTR_NEQ(key, NULL, "RSAPublicKeyFromBuf() ptr");
    TEST_EQ(key->len, *buf_key_len, "RSAPublicKeyFromBuf() len");
    TEST_EQ(key->n0inv, 0xF00D2345, "RSAPublicKeyFromBuf() n0inv");
    TEST_PTR_NEQ(key->n, NULL, "RSAPublicKeyFromBuf() n ptr");
    TEST_EQ(((uint8_t*)key->n)[0], 100, "RSAPublicKeyFromBuf() n start");
    TEST_EQ(((uint8_t*)key->n)[key_len - 1], 101,
            "RSAPublicKeyFromBuf() n end");
    TEST_PTR_NEQ(key->rr, NULL, "RSAPublicKeyFromBuf() rr ptr");
    TEST_EQ(((uint8_t*)key->rr)[0], 120, "RSAPublicKeyFromBuf() rr start");
    TEST_EQ(((uint8_t*)key->rr)[key_len - 1], 121,
            "RSAPublicKeyFromBuf() rr end");
    RSAPublicKeyFree(key);

    /* Underflow and overflow */
    TEST_PTR_EQ(RSAPublicKeyFromBuf(buf, 8 + key_len * 2 - 1), NULL,
                "RSAPublicKeyFromBuf() underflow");
    TEST_PTR_EQ(RSAPublicKeyFromBuf(buf, 8 + key_len * 2 + 1), NULL,
                "RSAPublicKeyFromBuf() overflow");

    /* Invalid key length in buffer */
    *buf_key_len = key_len / sizeof(uint32_t) + 1;
    TEST_PTR_EQ(RSAPublicKeyFromBuf(buf, 8 + key_len * 2 + 1), NULL,
                "RSAPublicKeyFromBuf() invalid key length");

    /* Valid key length in buffer, but for some other length key */
    *buf_key_len = (RSA1024NUMBYTES << ((i + 1) & 3)) / sizeof(uint32_t);
    TEST_PTR_EQ(RSAPublicKeyFromBuf(buf, 8 + key_len * 2 + 1), NULL,
                "RSAPublicKeyFromBuf() key length for wrong key");
  }
  free(buf);
}

/* Test verifying binary */
static void TestVerifyBinary(void) {
  RSAPublicKey key;
  uint8_t keybuf[8 + 2 * RSA1024NUMBYTES];
  uint32_t* keybuf_len = (uint32_t*)keybuf;
  uint8_t buf[120];
  uint8_t sig[4];

  *keybuf_len = RSA1024NUMBYTES / sizeof(uint32_t);

  /* Successful verification */
  ResetMocks();
  TEST_EQ(RSAVerifyBinary_f(NULL, &key, buf, sizeof(buf), sig, 0),
          1, "RSAVerifyBinary_f() success");
  /* Successful verification using key blob */
  TEST_EQ(RSAVerifyBinary_f(keybuf, NULL, buf, sizeof(buf), sig, 0),
          1, "RSAVerifyBinary_f() success with keyblob");

  /* Invalid algorithm */
  ResetMocks();
  TEST_EQ(RSAVerifyBinary_f(NULL, &key, buf, sizeof(buf), sig, kNumAlgorithms),
          0, "RSAVerifyBinary_f() invalid algorithm");
  /* Must have either a key or a key blob */
  ResetMocks();
  TEST_EQ(RSAVerifyBinary_f(NULL, NULL, buf, sizeof(buf), sig, kNumAlgorithms),
          0, "RSAVerifyBinary_f() no key or key_blob");
  /* Wrong algorithm for key buffer (so key buffer is wrong size) */
  ResetMocks();
  TEST_EQ(RSAVerifyBinary_f(keybuf, NULL, buf, sizeof(buf), sig, 3),
          0, "RSAVerifyBinary_f() wrong alg for key blob");

  /* Simulate failed verification */
  ResetMocks();
  mock_rsaverify_retval = 0;
  TEST_EQ(RSAVerifyBinary_f(NULL, &key, buf, sizeof(buf), sig, 0),
          0, "RSAVerifyBinary_f() bad verify");
}

/* Test verifying binary with digest */
static void TestVerifyBinaryWithDigest(void) {
  RSAPublicKey key;
  uint8_t keybuf[8 + 2 * RSA1024NUMBYTES];
  uint32_t* keybuf_len = (uint32_t*)keybuf;
  uint8_t digest[120];
  uint8_t sig[4];

  *keybuf_len = RSA1024NUMBYTES / sizeof(uint32_t);

  /* Successful verification */
  ResetMocks();
  TEST_EQ(RSAVerifyBinaryWithDigest_f(NULL, &key, digest, sig, 0),
          1, "RSAVerifyBinaryWithDigest_f() success");
  /* Successful verification using key blob */
  TEST_EQ(RSAVerifyBinaryWithDigest_f(keybuf, NULL, digest, sig, 0),
          1, "RSAVerifyBinaryWithDigest_f() success with keyblob");

  /* Invalid algorithm */
  ResetMocks();
  TEST_EQ(RSAVerifyBinaryWithDigest_f(NULL, &key, digest, sig, kNumAlgorithms),
          0, "RSAVerifyBinaryWithDigest_f() invalid algorithm");
  /* Must have either a key or a key blob */
  ResetMocks();
  TEST_EQ(RSAVerifyBinaryWithDigest_f(NULL, NULL, digest, sig, kNumAlgorithms),
          0, "RSAVerifyBinaryWithDigest_f() no key or key_blob");
  /* Wrong algorithm for key buffer (so key buffer is wrong size) */
  ResetMocks();
  TEST_EQ(RSAVerifyBinaryWithDigest_f(keybuf, NULL, digest, sig, 3),
          0, "RSAVerifyBinaryWithDigest_f() wrong alg for key blob");

  /* Simulate failed verification */
  ResetMocks();
  mock_rsaverify_retval = 0;
  TEST_EQ(RSAVerifyBinaryWithDigest_f(NULL, &key, digest, sig, 0),
          0, "RSAVerifyBinaryWithDigest_f() bad verify");
}

int main(int argc, char* argv[]) {
  int error_code = 0;

  /* Run tests */
  TestUtils();
  TestKeyFromBuffer();
  TestVerifyBinary();
  TestVerifyBinaryWithDigest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
