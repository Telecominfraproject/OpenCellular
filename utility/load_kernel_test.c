/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Routines for verifying a file's signature. Useful in testing the core
 * RSA verification implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "load_kernel_fw.h"
#include "boot_device.h"
#include "rollback_index.h"
#include "utility.h"

/* ANSI Color coding sequences. */
#define COL_GREEN "\e[1;32m"
#define COL_RED "\e[0;31m"
#define COL_STOP "\e[m"


#define LBA_BYTES 512
#define KERNEL_BUFFER_SIZE 0x600000

/* Global variables for stub functions */
static LoadKernelParams lkp;
static FILE *image_file = NULL;


/* Boot device stub implementations to read from the image file */
int BootDeviceReadLBA(uint64_t lba_start, uint64_t lba_count, void *buffer) {
  printf("Read(%ld, %ld)\n", lba_start, lba_count);

  if (lba_start > lkp.ending_lba ||
      lba_start + lba_count - 1 > lkp.ending_lba) {
    fprintf(stderr, "Read overrun: %ld + %ld > %ld\n",
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
  printf("Write(%ld, %ld)\n", lba_start, lba_count);

  if (lba_start > lkp.ending_lba ||
      lba_start + lba_count - 1 > lkp.ending_lba) {
    fprintf(stderr, "Read overrun: %ld + %ld > %ld\n",
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

  const char *image_name;
  int rv;

  Memset(&lkp, 0, sizeof(LoadKernelParams));
  lkp.bytes_per_lba = LBA_BYTES;

  /* Read command line parameters */
  if (2 > argc) {
    fprintf(stderr, "usage: %s <drive_image>\n", argv[0]);
    return 1;
  }
  image_name = argv[1];
  printf("Reading from image: %s\n", image_name);

  /* TODO: Read header signing key blob */
  lkp.header_sign_key_blob = NULL;

  /* Get image size */
  image_file = fopen(image_name, "rb");
  if (!image_file) {
    fprintf(stderr, "Unable to open image file %s\n", image_name);
    return 1;
  }
  fseek(image_file, 0, SEEK_END);
  lkp.ending_lba = (ftell(image_file) / LBA_BYTES) - 1;
  rewind(image_file);
  printf("Ending LBA: %ld\n", lkp.ending_lba);

  /* Allocate a buffer for the kernel */
  lkp.kernel_buffer = Malloc(KERNEL_BUFFER_SIZE);
  if(!lkp.kernel_buffer) {
    fprintf(stderr, "Unable to allocate kernel buffer.\n");
    return 1;
  }

  /* TODO: Option for boot mode */
  lkp.boot_mode = BOOT_MODE_NORMAL;

  /* Call LoadKernel() */
  rv = LoadKernel(&lkp);
  printf("LoadKernel() returned %d\n", rv);

  fclose(image_file);
  Free(lkp.kernel_buffer);
  return 0;
}
