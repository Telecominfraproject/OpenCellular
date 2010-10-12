// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "cgptlib_internal.h"
#include "endian.h"

static void Usage(void)
{
  printf("\nUsage: %s boot [OPTIONS] DRIVE\n\n"
         "Edit the PMBR sector for legacy BIOSes\n\n"
         "Options:\n"
         "  -i NUM       Set bootable partition\n"
         "  -b FILE      Install bootloader code in the PMBR\n"
         "  -p           Create legacy PMBR partition table\n"
         "\n"
         "With no options, it will just print the PMBR boot guid\n"
         "\n", progname);
}


int cmd_boot(int argc, char *argv[]) {
  struct drive drive;
  uint32_t partition = 0;
  char *bootfile = 0;
  int create_pmbr = 0;
  int retval = 1;
  int gpt_retval;

  int c;
  int errorcnt = 0;
  char *e = 0;

  opterr = 0;                     // quiet, you
  while ((c=getopt(argc, argv, ":hi:b:p")) != -1)
  {
    switch (c)
    {
    case 'i':
      partition = (uint32_t)strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e))
      {
        Error("invalid argument to -%c: \"%s\"\n", c, optarg);
        errorcnt++;
      }
      break;
    case 'b':
      bootfile = optarg;
      break;
    case 'p':
      create_pmbr = 1;
      break;

    case 'h':
      Usage();
      return CGPT_OK;
    case '?':
      Error("unrecognized option: -%c\n", optopt);
      errorcnt++;
      break;
    case ':':
      Error("missing argument to -%c\n", optopt);
      errorcnt++;
      break;
    default:
      errorcnt++;
      break;
    }
  }
  if (errorcnt)
  {
    Usage();
    return CGPT_FAILED;
  }

  if (optind >= argc) {
    Error("missing drive argument\n");
    return CGPT_FAILED;
  }

  if (CGPT_OK != DriveOpen(argv[optind], &drive))
    return CGPT_FAILED;

  if (CGPT_OK != ReadPMBR(&drive)) {
    Error("Unable to read PMBR\n");
    goto done;
  }

  if (create_pmbr) {
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

  if (partition) {
    if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
      Error("GptSanityCheck() returned %d: %s\n",
            gpt_retval, GptError(gpt_retval));
      goto done;
    }

    if (partition > GetNumberOfEntries(&drive.gpt)) {
      Error("invalid partition number: %d\n", partition);
      goto done;
    }

    uint32_t index = partition - 1;
    GptEntry *entry = GetEntry(&drive.gpt, PRIMARY, index);
    memcpy(&drive.pmbr.boot_guid, &entry->unique, sizeof(Guid));
  }

  if (bootfile) {
    int fd = open(bootfile, O_RDONLY);
    if (fd < 0) {
      Error("Can't read %s: %s\n", bootfile, strerror(errno));
      goto done;
    }

    int n = read(fd, drive.pmbr.bootcode, sizeof(drive.pmbr.bootcode));
    if (n < 1) {
      Error("problem reading %s: %s\n", bootfile, strerror(errno));
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
