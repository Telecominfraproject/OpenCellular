/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Exports the kernel commandline from a given partition/image.
 */

#include "dump_kernel_config.h"

#include <stdio.h>
#include <sys/mman.h>

#include "host_common.h"
#include "kernel_blob.h"

uint8_t* find_kernel_config(uint8_t* blob, uint64_t blob_size,
                            uint64_t kernel_body_load_address) {

  VbKeyBlockHeader* key_block;
  VbKernelPreambleHeader* preamble;
  struct linux_kernel_params *params;
  uint32_t now = 0;
  uint32_t offset = 0;

  /* Skip the key block */
  key_block = (VbKeyBlockHeader*)blob;
  now += key_block->key_block_size;
  if (now + blob > blob + blob_size) {
    VbExError("key_block_size advances past the end of the blob\n");
    return NULL;
  }

  /* Open up the preamble */
  preamble = (VbKernelPreambleHeader*)(blob + now);
  now += preamble->preamble_size;
  if (now + blob > blob + blob_size) {
    VbExError("preamble_size advances past the end of the blob\n");
    return NULL;
  }

  /* The x86 kernels have a pointer to the kernel commandline in the zeropage
   * table, but that's irrelevant for ARM. Both types keep the config blob in
   * the same place, so just go find it. */
  offset = preamble->bootloader_address -
    (kernel_body_load_address + CROS_PARAMS_SIZE +
     CROS_CONFIG_SIZE) + now;
  if (offset > blob_size) {
    VbExError("params are outside of the memory blob: %x\n", offset);
    return NULL;
  }
  return blob + offset;
}

void* MapFile(const char* filename, size_t *size) {
  FILE* f;
  uint8_t* buf;
  long file_size = 0;

  f = fopen(filename, "rb");
  if (!f) {
    VBDEBUG(("Unable to open file %s\n", filename));
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  file_size = ftell(f);
  rewind(f);

  if (file_size <= 0) {
    fclose(f);
    return NULL;
  }
  *size = (size_t) file_size;

  /* Uses a host primitive as this is not meant for firmware use. */
  buf = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fileno(f), 0);
  if (buf == MAP_FAILED) {
    VbExError("Failed to mmap the file %s\n", filename);
    fclose(f);
    return NULL;
  }

  fclose(f);
  return buf;
}
