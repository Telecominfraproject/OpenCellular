/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This tests for the presence of functions used by vboot_reference utilities.
 */

#include <stdio.h>

#include "host_common.h"
#include "file_keys.h"
#include "signature_digest.h"

int main(void)
{
  /* host_key.h */
  PrivateKeyReadPem(0, 0);
  PrivateKeyFree(0);
  PublicKeyAlloc(0, 0, 0);
  PublicKeyRead(0);
  PublicKeyReadKeyb(0, 0, 0);
  PublicKeyWrite(0, 0);

  /* host_keyblock.h */
  KeyBlockCreate(0, 0, 0);
  KeyBlockRead(0);
  KeyBlockWrite(0, 0);

  /* host_misc.h */
  ReadFile(0, 0);
  WriteFile(0, 0, 0);

  /* host_signature.h */
  SignatureInit(0, 0, 0, 0);
  SignatureAlloc(0, 0);
  SignatureCopy(0, 0);
  CalculateChecksum(0, 0);
  CalculateSignature(0, 0, 0);

  /* host_common.h */
  CreateFirmwarePreamble(0, 0, 0, 0, 0);
  CreateKernelPreamble(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  /* file_keys.h */
  BufferFromFile(0, 0);
  RSAPublicKeyFromFile(0);
  DigestFile(0, 0);

  /* signature_digest.h */
  PrependDigestInfo(0, 0);
  SignatureDigest(0, 0, 0);
  SignatureBuf(0, 0, 0, 0);

  return 0;
}
