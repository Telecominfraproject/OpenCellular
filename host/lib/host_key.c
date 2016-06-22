/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for keys.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#include <openssl/pem.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cryptolib.h"
#include "host_common.h"
#include "host_key.h"
#include "host_misc.h"
#include "vb2_common.h"
#include "vboot_common.h"

/* Allocate a new public key with space for a [key_size] byte key. */
VbPublicKey* PublicKeyAlloc(uint64_t key_size, uint64_t algorithm,
                            uint64_t version) {
  VbPublicKey* key = (VbPublicKey*)malloc(sizeof(VbPublicKey) + key_size);
  if (!key)
    return NULL;

  key->algorithm = algorithm;
  key->key_version = version;
  key->key_size = key_size;
  key->key_offset = sizeof(VbPublicKey);
  return key;
}

VbPublicKey* PublicKeyReadKeyb(const char* filename, uint64_t algorithm,
                               uint64_t version) {
  VbPublicKey* key;
  uint8_t* key_data;
  uint64_t key_size;
  uint64_t expected_key_size;

  if (algorithm >= kNumAlgorithms) {
    VBDEBUG(("PublicKeyReadKeyb() called with invalid algorithm!\n"));
    return NULL;
  }
  if (version > 0xFFFF) {
    /* Currently, TPM only supports 16-bit version */
    VBDEBUG(("PublicKeyReadKeyb() called with invalid version!\n"));
    return NULL;
  }

  key_data = ReadFile(filename, &key_size);
  if (!key_data)
    return NULL;

  if (!RSAProcessedKeySize(algorithm, &expected_key_size) ||
      expected_key_size != key_size) {
    VBDEBUG(("PublicKeyReadKeyb() wrong key size for algorithm\n"));
    free(key_data);
    return NULL;
  }

  key = PublicKeyAlloc(key_size, algorithm, version);
  if (!key) {
    free(key_data);
    return NULL;
  }
  Memcpy(GetPublicKeyData(key), key_data, key_size);

  free(key_data);
  return key;
}

int packed_key_looks_ok(const struct vb2_packed_key *key, uint32_t size)
{
	uint64_t key_size;

	if (size < sizeof(*key))
		return 0;

	/* Sanity-check key data */
	if (0 != VerifyPublicKeyInside(key, size, (VbPublicKey *)key)) {
		VBDEBUG(("PublicKeyRead() not a VbPublicKey\n"));
		return 0;
	}
	if (key->algorithm >= kNumAlgorithms) {
		VBDEBUG(("PublicKeyRead() invalid algorithm\n"));
		return 0;
	}
	if (key->key_version > 0xFFFF) {
		VBDEBUG(("PublicKeyRead() invalid version\n"));
		return 0;  /* Currently, TPM only supports 16-bit version */
	}
	if (!RSAProcessedKeySize(key->algorithm, &key_size) ||
	    key_size != key->key_size) {
		VBDEBUG(("PublicKeyRead() wrong key size for algorithm\n"));
		return 0;
	}

	/* Success */
	return 1;
}

VbPublicKey* PublicKeyRead(const char* filename) {
	struct vb2_packed_key *key;
	uint64_t file_size;

	key = (struct vb2_packed_key *)ReadFile(filename, &file_size);
	if (!key)
		return NULL;

	if (packed_key_looks_ok(key, file_size))
		return (VbPublicKey *)key;

	/* Error */
	free(key);
	return NULL;
}

int PublicKeyWrite(const char* filename, const VbPublicKey* key) {
  VbPublicKey* kcopy;
  int rv;

  /* Copy the key, so its data is contiguous with the header */
  kcopy = PublicKeyAlloc(key->key_size, 0, 0);
  if (!kcopy)
    return 1;
  if (0 != PublicKeyCopy(kcopy, key)) {
    free(kcopy);
    return 1;
  }

  /* Write the copy, then free it */
  rv = WriteFile(filename, kcopy, kcopy->key_offset + kcopy->key_size);
  free(kcopy);
  return rv;
}
