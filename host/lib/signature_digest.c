/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <openssl/pem.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cryptolib.h"
#include "host_common.h"
#include "signature_digest.h"


uint8_t* PrependDigestInfo(unsigned int algorithm, uint8_t* digest) {
  const int digest_size = hash_size_map[algorithm];
  const int digestinfo_size = digestinfo_size_map[algorithm];
  const uint8_t* digestinfo = hash_digestinfo_map[algorithm];
  uint8_t* p = malloc(digestinfo_size + digest_size);
  Memcpy(p, digestinfo, digestinfo_size);
  Memcpy(p + digestinfo_size, digest, digest_size);
  return p;
}

uint8_t* SignatureDigest(const uint8_t* buf, uint64_t len,
                         unsigned int algorithm) {
  uint8_t* info_digest  = NULL;
  uint8_t* digest = NULL;

  if (algorithm >= kNumAlgorithms) {
    VBDEBUG(("SignatureDigest() called with invalid algorithm!\n"));
  } else if ((digest = DigestBuf(buf, len, algorithm))) {
    info_digest = PrependDigestInfo(algorithm, digest);
  }
  free(digest);
  return info_digest;
}

uint8_t* SignatureBuf(const uint8_t* buf, uint64_t len, const char* key_file,
                      unsigned int algorithm) {
  FILE* key_fp = NULL;
  RSA* key = NULL;
  uint8_t* signature = NULL;
  uint8_t* signature_digest = SignatureDigest(buf, len, algorithm);
  int signature_digest_len = (hash_size_map[algorithm] +
                              digestinfo_size_map[algorithm]);
  key_fp  = fopen(key_file, "r");
  if (!key_fp) {
    VBDEBUG(("SignatureBuf(): Couldn't open key file: %s\n", key_file));
    free(signature_digest);
    return NULL;
  }
  if ((key = PEM_read_RSAPrivateKey(key_fp, NULL, NULL, NULL)))
    signature = (uint8_t*) malloc(siglen_map[algorithm]);
  else
    VBDEBUG(("SignatureBuf(): Couldn't read private key from file: %s\n",
             key_file));
  if (signature) {
    if (-1 == RSA_private_encrypt(signature_digest_len,  /* Input length. */
                                  signature_digest,  /* Input data. */
                                  signature,  /* Output signature. */
                                  key,  /* Key to use. */
                                  RSA_PKCS1_PADDING))  /* Padding to use. */
      VBDEBUG(("SignatureBuf(): RSA_private_encrypt() failed.\n"));
  }
  fclose(key_fp);
  if (key)
    RSA_free(key);
  free(signature_digest);
  return signature;
}
