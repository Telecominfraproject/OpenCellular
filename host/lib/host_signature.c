/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for signature generation.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#define OPENSSL_NO_SHA
#include <openssl/engine.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "utility.h"
#include "vboot_common.h"
#include "host_common.h"


VbSignature* SignatureAlloc(uint64_t sig_size, uint64_t data_size) {
  VbSignature* sig = (VbSignature*)Malloc(sizeof(VbSignature) + sig_size);
  if (!sig)
    return NULL;

  sig->sig_offset = sizeof(VbSignature);
  sig->sig_size = sig_size;
  sig->data_size = data_size;
  return sig;
}


void SignatureInit(VbSignature* sig, uint8_t* sig_data,
                          uint64_t sig_size, uint64_t data_size) {
  sig->sig_offset = OffsetOf(sig, sig_data);
  sig->sig_size = sig_size;
  sig->data_size = data_size;
}


int SignatureCopy(VbSignature* dest, const VbSignature* src) {
  if (dest->sig_size < src->sig_size)
    return 1;
  dest->sig_size = src->sig_size;
  dest->data_size = src->data_size;
  Memcpy(GetSignatureData(dest), GetSignatureDataC(src), src->sig_size);
  return 0;
}


VbSignature* CalculateChecksum(const uint8_t* data, uint64_t size) {

  uint8_t* header_checksum;
  VbSignature* sig;

  header_checksum = DigestBuf(data, size, SHA512_DIGEST_ALGORITHM);
  if (!header_checksum)
    return NULL;

  sig = SignatureAlloc(SHA512_DIGEST_SIZE, 0);
  if (!sig) {
    Free(header_checksum);
    return NULL;
  }
  sig->sig_offset = sizeof(VbSignature);
  sig->sig_size = SHA512_DIGEST_SIZE;
  sig->data_size = size;

  /* Signature data immediately follows the header */
  Memcpy(GetSignatureData(sig), header_checksum, SHA512_DIGEST_SIZE);
  Free(header_checksum);
  return sig;
}


VbSignature* CalculateSignature(const uint8_t* data, uint64_t size,
                                const VbPrivateKey* key) {

  uint8_t* digest;
  int digest_size = hash_size_map[key->algorithm];

  const uint8_t* digestinfo = hash_digestinfo_map[key->algorithm];
  int digestinfo_size = digestinfo_size_map[key->algorithm];

  uint8_t* signature_digest;
  int signature_digest_len = digest_size + digestinfo_size;

  VbSignature* sig;
  int rv;

  /* Calculate the digest */
  /* TODO: rename param 3 of DigestBuf to hash_type */
  digest = DigestBuf(data, size, hash_type_map[key->algorithm]);
  if (!digest)
    return NULL;

  /* Prepend the digest info to the digest */
  signature_digest = Malloc(signature_digest_len);
  if (!signature_digest) {
    Free(digest);
    return NULL;
  }
  Memcpy(signature_digest, digestinfo, digestinfo_size);
  Memcpy(signature_digest + digestinfo_size, digest, digest_size);
  Free(digest);

  /* Allocate output signature */
  sig = SignatureAlloc(siglen_map[key->algorithm], size);
  if (!sig) {
    Free(signature_digest);
    return NULL;
  }

  /* Sign the signature_digest into our output buffer */
  rv = RSA_private_encrypt(signature_digest_len,   /* Input length */
                           signature_digest,       /* Input data */
                           GetSignatureData(sig),  /* Output sig */
                           key->rsa_private_key,   /* Key to use */
                           RSA_PKCS1_PADDING);     /* Padding to use */
  Free(signature_digest);

  if (-1 == rv) {
    VBDEBUG(("SignatureBuf(): RSA_private_encrypt() failed.\n"));
    Free(sig);
    return NULL;
  }

  /* Return the signature */
  return sig;
}
