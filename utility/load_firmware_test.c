/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Routines for verifying a firmware image's signature.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmap.h"
#include "gbb_header.h"
#include "host_common.h"
#include "host_misc.h"
#include "load_firmware_fw.h"


typedef struct _CallerInternal {
  struct {
    uint8_t* fw;
    uint64_t size;
  } firmware[2];
} CallerInternal;

static char* progname = NULL;
static char* image_path = NULL;


/* wrapper of FmapAreaIndex; print error when not found */
int FmapAreaIndexOrError(const FmapHeader* fh, const FmapAreaHeader* ah,
    const char* name);
/* return NULL on error */
const char* status_string(int status);

int GetFirmwareBody(LoadFirmwareParams* params, uint64_t firmware_index) {
  CallerInternal* ci = (CallerInternal*) params->caller_internal;

  if (firmware_index != 0 && firmware_index != 1)
    return 1;

  UpdateFirmwareBodyHash(params,
                         ci->firmware[firmware_index].fw,
                         ci->firmware[firmware_index].size);

  return 0;
}

/* Get GBB
 *
 * Return pointer to GBB from firmware image, or NULL if not found.
 *
 * [base_of_rom] pointer to firmware image
 * [fmap] pointer to Flash Map of firmware image
 * [gbb_size] GBB size will be stored here if GBB is found
 */
void* GetFirmwareGBB(const void* base_of_rom, const void* fmap,
                     uint64_t* gbb_size) {
  const FmapHeader* fh = (const FmapHeader*) fmap;
  const FmapAreaHeader* ah = (const FmapAreaHeader*)
    (fmap + sizeof(FmapHeader));
  int i = FmapAreaIndexOrError(fh, ah, "GBB");

  if (i < 0)
    return NULL;

  *gbb_size = ah[i].area_size;
  return (void*)(base_of_rom + ah[i].area_offset);
}

/* Get verification block
 *
 * Return zero if succeed, or non-zero if failed
 *
 * [base_of_rom] pointer to firmware image
 * [fmap] pointer to Flash Map of firmware image
 * [index] index of verification block
 * [verification_block_ptr] pointer to storing the found verification block
 * [verification_size_ptr] pointer to store the found verification block size
 */
int GetVerificationBlock(const void* base_of_rom, const void* fmap, int index,
    void** verification_block_ptr, uint64_t* verification_size_ptr) {
  const char* key_area_name[2] = {
    "VBLOCK_A",
    "VBLOCK_B"
  };
  const FmapHeader* fh = (const FmapHeader*) fmap;
  const FmapAreaHeader* ah = (const FmapAreaHeader*)
    (fmap + sizeof(FmapHeader));
  int i = FmapAreaIndexOrError(fh, ah, key_area_name[index]);
  const void* kb;
  const VbKeyBlockHeader* kbh;
  const VbFirmwarePreambleHeader* fph;

  if (i < 0)
    return 1;

  kb = base_of_rom + ah[i].area_offset;
  *verification_block_ptr = (void*) kb;

  kbh = (const VbKeyBlockHeader*) kb;
  fph = (const VbFirmwarePreambleHeader*) (kb + kbh->key_block_size);

  *verification_size_ptr = kbh->key_block_size + fph->preamble_size;

  return 0;
}

/* Return non-zero if not found */
int GetFirmwareData(const void* base_of_rom, const void* fmap, int index,
    void *verification_block, uint8_t** body_ptr, uint64_t *size_ptr) {
  const char* data_area_name[2] = {
    "FW_MAIN_A",
    "FW_MAIN_B"
  };
  const FmapHeader* fh = (const FmapHeader*) fmap;
  const FmapAreaHeader* ah = (const FmapAreaHeader*)
    (fmap + sizeof(FmapHeader));
  const VbKeyBlockHeader* kbh = (const VbKeyBlockHeader*) verification_block;
  const VbFirmwarePreambleHeader* fph = (const VbFirmwarePreambleHeader*)
    (verification_block + kbh->key_block_size);
  int i = FmapAreaIndexOrError(fh, ah, data_area_name[index]);

  if (i < 0)
    return 1;

  *body_ptr = (uint8_t*) (base_of_rom + ah[i].area_offset);
  *size_ptr = (uint64_t) fph->body_signature.data_size;
  return 0;
}

/* Verify firmware image [base_of_rom] using [fmap] for looking up areas.
 * Return zero on success, non-zero on error
 *
 * [base_of_rom] pointer to start of firmware image
 * [fmap] pointer to start of Flash Map of firmware image
 */
int DriveLoadFirmware(const void* base_of_rom, const void* fmap,
    const uint64_t boot_flags) {
  LoadFirmwareParams lfp;
  CallerInternal ci;

  const char* status_str;
  int index, status;

  void** vblock_ptr[2] = {
    &lfp.verification_block_0, &lfp.verification_block_1
  };
  uint64_t* vsize_ptr[2] = {
    &lfp.verification_size_0, &lfp.verification_size_1
  };

  /* Initialize LoadFirmwareParams lfp */

  lfp.caller_internal = &ci;
  lfp.gbb_data = GetFirmwareGBB(base_of_rom, fmap, &lfp.gbb_size);
  if (!lfp.gbb_data) {
    printf("ERROR: cannot get firmware GBB\n");
    return 1;
  }

  printf("firmware GBB at 0x%08" PRIx64 "\n",
      (uint64_t) (lfp.gbb_data - base_of_rom));

  /* Loop to initialize firmware key and data A / B */
  for (index = 0; index < 2; ++index) {
    if (GetVerificationBlock(base_of_rom, fmap, index,
          vblock_ptr[index], vsize_ptr[index])) {
      printf("ERROR: cannot get key block %d\n", index);
      return 1;
    }

    printf("verification block %d at 0x%08" PRIx64 "\n", index,
        (uint64_t) (*vblock_ptr[index] - base_of_rom));
    printf("verification block %d size is 0x%08" PRIx64 "\n", index,
        *vsize_ptr[index]);

    if (GetFirmwareData(base_of_rom, fmap, index, *vblock_ptr[index],
          &(ci.firmware[index].fw), &(ci.firmware[index].size))) {
      printf("ERROR: cannot get firmware body %d\n", index);
      return 1;
    }

    printf("firmware %c at 0x%08" PRIx64 "\n", "AB"[index],
        (uint64_t) ((void*) ci.firmware[index].fw - base_of_rom));
    printf("firmware %c size is 0x%08" PRIx64 "\n", "AB"[index],
        ci.firmware[index].size);
  }

  lfp.shared_data_blob = malloc(VB_SHARED_DATA_MIN_SIZE);
  lfp.shared_data_size = VB_SHARED_DATA_MIN_SIZE;
  printf("shared data size 0x%08" PRIx64 "\n", lfp.shared_data_size);

  lfp.boot_flags = boot_flags;
  printf("boot flags is 0x%08" PRIx64 "\n", lfp.boot_flags);

  status = LoadFirmware(&lfp);
  status_str = status_string(status);
  if (status_str)
    printf("LoadFirmware returns %s\n", status_str);
  else
    printf("LoadFirmware returns unknown status code: %d\n", status);
  if (status == LOAD_FIRMWARE_SUCCESS)
    printf("firmwiare index is %" PRIu64 "\n", lfp.firmware_index);

  free(lfp.shared_data_blob);

  return 0;
}

/* wrap FmapAreaIndex; print error when not found */
int FmapAreaIndexOrError(const FmapHeader* fh, const FmapAreaHeader* ah,
    const char* name) {
  int i = FmapAreaIndex(fh, ah, name);
  if (i < 0)
    fprintf(stderr, "%s: can't find %s in firmware image\n", progname, name);
  return i;
}

/* Convert status returned by LoadFirmware to string. Return NULL on error. */
const char* status_string(int status) {
  switch (status) {
    case LOAD_FIRMWARE_SUCCESS:
      return "LOAD_FIRMWARE_SUCCESS";
    case LOAD_FIRMWARE_RECOVERY:
      return "LOAD_FIRMWARE_RECOVERY";
    case LOAD_FIRMWARE_REBOOT:
      return "LOAD_FIRMWARE_REBOOT";
    default:
      return NULL;
  }
}

int main(int argc, char* argv[]) {
  int i;
  int retval = 0;
  const void* base_of_rom;
  const void* fmap;
  uint64_t boot_flags = 0, rom_size;

  progname = argv[0];

  if (argc < 2) {
    fprintf(stderr, "usage: %s [-b NUM] <firmware_image>\n", progname);
    exit(1);
  }

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-b") && i < argc - 1)
      boot_flags = strtoull(argv[++i], NULL, 0);
    else
      image_path = argv[i];
  }

  base_of_rom = ReadFile(image_path, &rom_size);
  if (base_of_rom == NULL) {
    fprintf(stderr, "%s: can not open %s\n", progname, image_path);
    exit(1);
  }

  printf("opened %s\n", image_path);

  fmap = FmapFind((char*) base_of_rom, rom_size);

  retval = DriveLoadFirmware(base_of_rom, fmap, boot_flags);

  free((void*) base_of_rom);

  return retval;
}
