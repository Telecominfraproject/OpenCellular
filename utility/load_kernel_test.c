/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
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
#include <unistd.h>

#include "gbb_header.h"
#include "host_common.h"
#include "load_firmware_fw.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "vboot_common.h"
#include "vboot_kernel.h"

#define LBA_BYTES 512
#define KERNEL_BUFFER_SIZE 0xA00000

/* Global variables for stub functions */
static LoadKernelParams lkp;
static VbCommonParams cparams;
static VbNvContext vnc;
static FILE *image_file = NULL;


/* Boot device stub implementations to read from the image file */
VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
                       uint64_t lba_count, void *buffer) {
  printf("Read(%" PRIu64 ", %" PRIu64 ")\n", lba_start, lba_count);

  if (lba_start >= lkp.streaming_lba_count ||
      lba_start + lba_count > lkp.streaming_lba_count) {
    fprintf(stderr, "Read overrun: %" PRIu64 " + %" PRIu64 " > %" PRIu64 "\n",
            lba_start, lba_count, lkp.streaming_lba_count);
    return 1;
  }

  fseek(image_file, lba_start * lkp.bytes_per_lba, SEEK_SET);
  if (1 != fread(buffer, lba_count * lkp.bytes_per_lba, 1, image_file)) {
    fprintf(stderr, "Read error.");
    return 1;
  }
  return VBERROR_SUCCESS;
}


VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
                        uint64_t lba_count, const void *buffer) {
  printf("Write(%" PRIu64 ", %" PRIu64 ")\n", lba_start, lba_count);

  if (lba_start >= lkp.streaming_lba_count ||
      lba_start + lba_count > lkp.streaming_lba_count) {
    fprintf(stderr, "Read overrun: %" PRIu64 " + %" PRIu64 " > %" PRIu64 "\n",
            lba_start, lba_count, lkp.streaming_lba_count);
    return 1;
  }

  /* TODO: enable writes, once we're sure it won't trash our example file */
  return VBERROR_SUCCESS;

  fseek(image_file, lba_start * lkp.bytes_per_lba, SEEK_SET);
  if (1 != fwrite(buffer, lba_count * lkp.bytes_per_lba, 1, image_file)) {
    fprintf(stderr, "Read error.");
    return 1;
  }
  return VBERROR_SUCCESS;
}


/* Main routine */
int main(int argc, char* argv[]) {

  const char* image_name;
  uint64_t key_size;
  uint8_t* key_blob = NULL;
  VbSharedDataHeader* shared;
  GoogleBinaryBlockHeader* gbb;
  VbError_t rv;
  int c, argsleft;
  int errorcnt = 0;
  char *e = 0;

  Memset(&lkp, 0, sizeof(LoadKernelParams));
  lkp.bytes_per_lba = LBA_BYTES;
  lkp.boot_flags = BOOT_FLAG_RECOVERY;
  Memset(&vnc, 0, sizeof(VbNvContext));
  VbNvSetup(&vnc);
  lkp.nv_context = &vnc;
  Memset(&cparams, 0, sizeof(VbCommonParams));

  /* Parse options */
  opterr = 0;
  while ((c=getopt(argc, argv, ":b:")) != -1)
  {
    switch (c)
    {
    case 'b':
      lkp.boot_flags = strtoull(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        fprintf(stderr, "Invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case '?':
      fprintf(stderr, "Unrecognized switch: -%c\n", optopt);
      errorcnt++;
      break;
    case ':':
      fprintf(stderr, "Missing argument to -%c\n", optopt);
      errorcnt++;
      break;
    default:
      errorcnt++;
      break;
    }
  }

  /* Update argc */
  argsleft = argc - optind;

  if (errorcnt || !argsleft)
  {
    fprintf(stderr, "usage: %s [options] <drive_image> [<sign_key>]\n",
            argv[0]);
    fprintf(stderr, "\noptions:\n");
    /* These cases are because uint64_t isn't necessarily the same as ULL. */
    fprintf(stderr, "  -b NUM     boot flag bits (default %" PRIu64 "):\n",
            (uint64_t)BOOT_FLAG_RECOVERY);
    fprintf(stderr, "               %" PRIu64 " = developer mode on\n",
            (uint64_t)BOOT_FLAG_DEVELOPER);
    fprintf(stderr, "               %" PRIu64 " = recovery mode on\n",
            (uint64_t)BOOT_FLAG_RECOVERY);
    return 1;
  }

  image_name = argv[optind];

  /* Read header signing key blob */
  if (argsleft > 1) {
    key_blob = ReadFile(argv[optind+1], &key_size);
    if (!key_blob) {
      fprintf(stderr, "Unable to read key file %s\n", argv[optind+1]);
      return 1;
    }
    printf("Read %" PRIu64 " bytes of key from %s\n", key_size, argv[optind+1]);
  }

  /* Initialize the GBB */
  lkp.gbb_size = sizeof(GoogleBinaryBlockHeader) + key_size;
  lkp.gbb_data = (void*)malloc(lkp.gbb_size);
  gbb = (GoogleBinaryBlockHeader*)lkp.gbb_data;
  cparams.gbb = gbb;
  Memset(gbb, 0, lkp.gbb_size);
  Memcpy(gbb->signature, GBB_SIGNATURE, GBB_SIGNATURE_SIZE);
  gbb->major_version = GBB_MAJOR_VER;
  gbb->minor_version = GBB_MINOR_VER;
  gbb->header_size = sizeof(GoogleBinaryBlockHeader);
  /* Fill in the given key, if any, for both root and recovery */
  if (key_blob) {
    gbb->rootkey_offset = gbb->header_size;
    gbb->rootkey_size = key_size;
    Memcpy((uint8_t*)gbb + gbb->rootkey_offset, key_blob, key_size);

    gbb->recovery_key_offset = gbb->rootkey_offset;
    gbb->recovery_key_size = key_size;
  }

  /* Initialize the shared data area */
  lkp.shared_data_blob = malloc(VB_SHARED_DATA_REC_SIZE);
  lkp.shared_data_size = VB_SHARED_DATA_REC_SIZE;
  shared = (VbSharedDataHeader*)lkp.shared_data_blob;
  if (0 != VbSharedDataInit(shared, lkp.shared_data_size)) {
    fprintf(stderr, "Unable to init shared data\n");
    return 1;
  }
  /* Copy in the key blob, if any */
  if (key_blob) {
    if (0 != VbSharedDataSetKernelKey(shared, (VbPublicKey*)key_blob)) {
      fprintf(stderr, "Unable to set key in shared data\n");
      return 1;
    }
  }

  /* Free the key blob, now that we're done with it */
  free(key_blob);

  printf("bootflags = %" PRIu64 "\n", lkp.boot_flags);

  /* Get image size */
  printf("Reading from image: %s\n", image_name);
  image_file = fopen(image_name, "rb");
  if (!image_file) {
    fprintf(stderr, "Unable to open image file %s\n", image_name);
    return 1;
  }
  fseek(image_file, 0, SEEK_END);
  lkp.streaming_lba_count = (ftell(image_file) / LBA_BYTES);
  lkp.gpt_lba_count = lkp.streaming_lba_count;
  rewind(image_file);
  printf("Streaming LBA count: %" PRIu64 "\n", lkp.streaming_lba_count);

  /* Allocate a buffer for the kernel */
  lkp.kernel_buffer = malloc(KERNEL_BUFFER_SIZE);
  if(!lkp.kernel_buffer) {
    fprintf(stderr, "Unable to allocate kernel buffer.\n");
    return 1;
  }
  lkp.kernel_buffer_size = KERNEL_BUFFER_SIZE;

  /* Call LoadKernel() */
  rv = LoadKernel(&lkp, &cparams);
  printf("LoadKernel() returned %d\n", rv);

  if (VBERROR_SUCCESS == rv) {
    printf("Partition number:   %" PRIu64 "\n", lkp.partition_number);
    printf("Bootloader address: %" PRIu64 "\n", lkp.bootloader_address);
    printf("Bootloader size:    %" PRIu64 "\n", lkp.bootloader_size);
    printf("Partition guid:     "
           "%02x%02x%02x%02x-%02x%02x-%02x%02x"
           "-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
           lkp.partition_guid[3],
           lkp.partition_guid[2],
           lkp.partition_guid[1],
           lkp.partition_guid[0],
           lkp.partition_guid[5],
           lkp.partition_guid[4],
           lkp.partition_guid[7],
           lkp.partition_guid[6],
           lkp.partition_guid[8],
           lkp.partition_guid[9],
           lkp.partition_guid[10],
           lkp.partition_guid[11],
           lkp.partition_guid[12],
           lkp.partition_guid[13],
           lkp.partition_guid[14],
           lkp.partition_guid[15]);
  }

  fclose(image_file);
  free(lkp.kernel_buffer);
  return rv != VBERROR_SUCCESS;
}
