/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Routines for verifying a file's signature. Useful in testing the core
 * RSA verification implementation.
 */

#include <inttypes.h>  /* For PRIu64 macro */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "load_kernel_fw.h"
#include "boot_device.h"
#include "host_common.h"
#include "rollback_index.h"
#include "utility.h"
#include "vboot_kernel.h"

#define LBA_BYTES 512
#define KERNEL_BUFFER_SIZE 0xA00000

/* Global variables for stub functions */
static LoadKernelParams lkp;
static FILE *image_file = NULL;


/* Boot device stub implementations to read from the image file */
int BootDeviceReadLBA(uint64_t lba_start, uint64_t lba_count, void *buffer) {
  printf("Read(%" PRIu64 ", %" PRIu64 ")\n", lba_start, lba_count);

  if (lba_start > lkp.ending_lba ||
      lba_start + lba_count - 1 > lkp.ending_lba) {
    fprintf(stderr, "Read overrun: %" PRIu64 " + %" PRIu64 " > %" PRIu64 "\n",
            lba_start, lba_count, lkp.ending_lba);
    return 1;
  }

  fseek(image_file, lba_start * lkp.bytes_per_lba, SEEK_SET);
  if (1 != fread(buffer, lba_count * lkp.bytes_per_lba, 1, image_file)) {
    fprintf(stderr, "Read error.");
    return 1;
  }
  return 0;
}


int BootDeviceWriteLBA(uint64_t lba_start, uint64_t lba_count,
                       const void *buffer) {
  printf("Write(%" PRIu64 ", %" PRIu64 ")\n", lba_start, lba_count);

  if (lba_start > lkp.ending_lba ||
      lba_start + lba_count - 1 > lkp.ending_lba) {
    fprintf(stderr, "Read overrun: %" PRIu64 " + %" PRIu64 " > %" PRIu64 "\n",
            lba_start, lba_count, lkp.ending_lba);
    return 1;
  }

  /* TODO: enable writes, once we're sure it won't trash our example file */
  return 0;

  fseek(image_file, lba_start * lkp.bytes_per_lba, SEEK_SET);
  if (1 != fwrite(buffer, lba_count * lkp.bytes_per_lba, 1, image_file)) {
    fprintf(stderr, "Read error.");
    return 1;
  }
  return 0;
}


/* Main routine */
int main(int argc, char* argv[]) {

  const char* image_name;
  const char* keyfile_name;
  int rv;

  Memset(&lkp, 0, sizeof(LoadKernelParams));
  lkp.bytes_per_lba = LBA_BYTES;

  /* Read command line parameters */
  if (3 > argc) {
    fprintf(stderr, "usage: %s <drive_image> <sign_key> [boot flag]\n", argv[0]);
    return 1;
  }
  image_name = argv[1];
  keyfile_name = argv[2];

  /* Read header signing key blob */
  {
    uint64_t key_size;
    lkp.header_sign_key_blob = ReadFile(keyfile_name, &key_size);
    if (!lkp.header_sign_key_blob) {
      fprintf(stderr, "Unable to read key file %s\n", keyfile_name);
      return 1;
    }
  }

  /* Get image size */
  printf("Reading from image: %s\n", image_name);
  image_file = fopen(image_name, "rb");
  if (!image_file) {
    fprintf(stderr, "Unable to open image file %s\n", image_name);
    return 1;
  }
  fseek(image_file, 0, SEEK_END);
  lkp.ending_lba = (ftell(image_file) / LBA_BYTES) - 1;
  rewind(image_file);
  printf("Ending LBA: %" PRIu64 "\n", lkp.ending_lba);

  /* Allocate a buffer for the kernel */
  lkp.kernel_buffer = Malloc(KERNEL_BUFFER_SIZE);
  if(!lkp.kernel_buffer) {
    fprintf(stderr, "Unable to allocate kernel buffer.\n");
    return 1;
  }

  /* Need to skip the address check, since we're putting it somewhere on the
   * heap instead of its actual target address in the firmware. */
  if (argc == 4) {
    lkp.boot_flags = atoi(argv[3]) | BOOT_FLAG_SKIP_ADDR_CHECK;
  } else {
    /* Default to recovery. */
    lkp.boot_flags = BOOT_FLAG_SKIP_ADDR_CHECK | BOOT_FLAG_RECOVERY;
  }
  /* Call LoadKernel() */
  rv = LoadKernel(&lkp);
  printf("LoadKernel() returned %d\n", rv);

  if (LOAD_KERNEL_SUCCESS == rv) {
    printf("Partition number:   %" PRIu64 "\n", lkp.partition_number);
    printf("Bootloader address: %" PRIu64 "\n", lkp.bootloader_address);
    printf("Bootloader size:    %" PRIu64 "\n", lkp.bootloader_size);
  }

  fclose(image_file);
  Free(lkp.kernel_buffer);
  return rv != LOAD_KERNEL_SUCCESS;
}
