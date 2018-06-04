// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "cgpt.h"
#include "cgpt_params.h"
#include "cgptlib_internal.h"
#include "vboot_host.h"

int CgptGetBootPartitionNumber(CgptBootParams *params) {
  struct drive drive;
  int gpt_retval= 0;
  int retval;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDONLY,
                           params->drive_size))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    retval = CGPT_FAILED;
    goto done;
  }

  if (CGPT_OK != ReadPMBR(&drive)) {
    Error("Unable to read PMBR\n");
    retval = CGPT_FAILED;
    goto done;
  }

  char buf[GUID_STRLEN];
  GuidToStr(&drive.pmbr.boot_guid, buf, sizeof(buf));

  int numEntries = GetNumberOfEntries(&drive);
  int i;
  for(i = 0; i < numEntries; i++) {
      GptEntry *entry = GetEntry(&drive.gpt, ANY_VALID, i);

      if (GuidEqual(&entry->unique, &drive.pmbr.boot_guid)) {
        params->partition = i + 1;
        retval = CGPT_OK;
        goto done;
      }
  }

  Error("Didn't find any boot partition\n");
  params->partition = 0;
  retval = CGPT_FAILED;

done:
  (void) DriveClose(&drive, 1);
  return retval;
}


int CgptBoot(CgptBootParams *params) {
  struct drive drive;
  int retval = 1;
  int gpt_retval= 0;
  int mode = O_RDONLY;

  if (params == NULL)
    return CGPT_FAILED;

  if (params->create_pmbr || params->partition || params->bootfile)
    mode = O_RDWR;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, mode,
                           params->drive_size)) {
    return CGPT_FAILED;
  }

  if (CGPT_OK != ReadPMBR(&drive)) {
    Error("Unable to read PMBR\n");
    goto done;
  }

  if (params->create_pmbr) {
    drive.pmbr.magic[0] = 0x1d;
    drive.pmbr.magic[1] = 0x9a;
    drive.pmbr.sig[0] = 0x55;
    drive.pmbr.sig[1] = 0xaa;
    memset(&drive.pmbr.part, 0, sizeof(drive.pmbr.part));
    drive.pmbr.part[0].f_head = 0x00;
    drive.pmbr.part[0].f_sect = 0x02;
    drive.pmbr.part[0].f_cyl = 0x00;
    drive.pmbr.part[0].type = 0xee;
    drive.pmbr.part[0].l_head = 0xff;
    drive.pmbr.part[0].l_sect = 0xff;
    drive.pmbr.part[0].l_cyl = 0xff;
    drive.pmbr.part[0].f_lba = htole32(1);
    uint32_t max = 0xffffffff;
    if (drive.gpt.streaming_drive_sectors < 0xffffffff)
      max = drive.gpt.streaming_drive_sectors - 1;
    drive.pmbr.part[0].num_sect = htole32(max);
  }

  if (params->partition) {
    if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
      Error("GptSanityCheck() returned %d: %s\n",
            gpt_retval, GptError(gpt_retval));
      goto done;
    }

    if (params->partition > GetNumberOfEntries(&drive)) {
      Error("invalid partition number: %d\n", params->partition);
      goto done;
    }

    uint32_t index = params->partition - 1;
    GptEntry *entry = GetEntry(&drive.gpt, ANY_VALID, index);
    memcpy(&drive.pmbr.boot_guid, &entry->unique, sizeof(Guid));
  }

  if (params->bootfile) {
    int fd = open(params->bootfile, O_RDONLY);
    if (fd < 0) {
      Error("Can't read %s: %s\n", params->bootfile, strerror(errno));
      goto done;
    }

    int n = read(fd, drive.pmbr.bootcode, sizeof(drive.pmbr.bootcode));
    if (n < 1) {
      Error("problem reading %s: %s\n", params->bootfile, strerror(errno));
      close(fd);
      goto done;
    }

    close(fd);
  }

  char buf[GUID_STRLEN];
  GuidToStr(&drive.pmbr.boot_guid, buf, sizeof(buf));
  printf("%s\n", buf);

  // Write it all out, if needed.
  if (mode == O_RDONLY || CGPT_OK == WritePMBR(&drive))
    retval = 0;

done:
  (void) DriveClose(&drive, 1);
  return retval;
}
