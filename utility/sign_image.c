/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility for signing boot firmware images.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "file_keys.h"
#include "utility.h"
#include "host_key.h"
#include "host_signature.h"
#include "host_common.h"

static void usage()
{
  static char* help_mesg =
      "Usage: sign_image <fw_version> <fw_key_block> <signing_key> "
      "<kernel_public_key> <firmware_file> <output_file>\n";
  printf("%s", help_mesg);
}

int SignAndWriteImage(uint64_t fw_version, VbKeyBlockHeader* wrapper_kb,
                      VbPrivateKey* signing_key,
                      VbPublicKey* nested_pubkey,
                      uint8_t* image, uint64_t image_size,
                      FILE* out_file)
{
  VbFirmwarePreambleHeader* fw_preamble = NULL;
  int rv = 1;
  do { /* to be able to bail out anywhere */
    VbSignature* firmware_sig;

    /* sign the firmware first */
    firmware_sig = CalculateSignature(image, image_size, signing_key);

    /* write the original keyblock */
    if (fwrite(wrapper_kb, wrapper_kb->key_block_size, 1, out_file) != 1) {
      debug("failed writing key block\n");
      break;
    }

    fw_preamble = CreateFirmwarePreamble(fw_version, nested_pubkey,
                                         firmware_sig, signing_key);

    if (!fw_preamble) {
      debug("failed creating preamble\n");
      break;
    }

    /* write the preamble */
    if (fwrite(fw_preamble, fw_preamble->preamble_size, 1, out_file) != 1) {
      debug("failed writing fw preamble\n");
      break;
    }

    /* write the image */
    if (fwrite(image, image_size, 1, out_file) != 1) {
      debug("failed writing image\n");
      break;
    }
    rv = 0;
  } while(0);

  if (fw_preamble) {
    Free(fw_preamble);
  }

  return rv;
}

int main(int argc, char* argv[]) {
  VbKeyBlockHeader* firmware_kb;
  VbPublicKey* kernel_pubk;
  uint8_t* firmware;
  uint64_t fw_size;
  uint64_t version;
  VbPrivateKey* signing_key = NULL;
  FILE* out_file;
  int rv;

  if (argc != 7) {
    usage();
    exit(1);
  }

  version = strtoul(argv[1], 0, 0);
  firmware_kb = KeyBlockRead(argv[2]);
  kernel_pubk = PublicKeyRead(argv[4]);
  firmware = BufferFromFile(argv[5], &fw_size);
  if (firmware_kb) {
    signing_key = PrivateKeyRead(argv[3], firmware_kb->data_key.algorithm);
  }
  if (!firmware_kb || !kernel_pubk || !firmware || ! signing_key) {
    return 1;
  }

  out_file = fopen(argv[6], "wb");
  if (!out_file) {
    debug("could not open %s for writing\n");
    return 1;
  }

  rv = SignAndWriteImage(version, firmware_kb, signing_key,
                         kernel_pubk, firmware, fw_size, out_file);

  fclose(out_file);
  if (rv) {
    unlink(argv[6]);
  }
  return rv;
}
