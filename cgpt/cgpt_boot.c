// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "cgptlib_internal.h"
#include "endian.h"
#include "cgpt_params.h"

int cgpt_boot(CgptBootParams *params) {
  struct drive drive;
  int retval = 1;
  int gpt_retval= 0;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->driveName, &drive))
    return CGPT_FAILED;

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
    if (drive.gpt.drive_sectors < 0xffffffff)
      max = drive.gpt.drive_sectors - 1;
    drive.pmbr.part[0].num_sect = htole32(max);
  }

  if (params->partition) {
    if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
      Error("GptSanityCheck() returned %d: %s\n",
            gpt_retval, GptError(gpt_retval));
      goto done;
    }

    if (params->partition > GetNumberOfEntries(&drive.gpt)) {
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

  // Write it all out
  if (CGPT_OK == WritePMBR(&drive))
    retval = 0;

done:
  (void) DriveClose(&drive, 1);
  return retval;
}
