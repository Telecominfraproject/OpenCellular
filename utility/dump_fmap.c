/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fmap.h"

/* global variables */
static int opt_extract = 0;
static char* progname;
static void* base_of_rom;


/* Return 0 if successful */
static int dump_fmap(const void* ptr) {
  int i,retval = 0;
  char buf[80];                         // DWR: magic number
  const FmapHeader* fmh = (const FmapHeader*) ptr;
  const FmapAreaHeader* ah = (const FmapAreaHeader*) (ptr + sizeof(FmapHeader));

  snprintf(buf, FMAP_SIGNATURE_SIZE+1, "%s", fmh->fmap_signature);
  printf("fmap_signature   %s\n", buf);
  printf("fmap_version:    %d.%d\n", fmh->fmap_ver_major, fmh->fmap_ver_minor);
  printf("fmap_base:       0x%" PRIx64 "\n", fmh->fmap_base);
  printf("fmap_size:       0x%08x (%d)\n", fmh->fmap_size, fmh->fmap_size);
  snprintf(buf, FMAP_NAMELEN+1, "%s", fmh->fmap_name);
  printf("fmap_name:       %s\n", buf);
  printf("fmap_nareas:     %d\n", fmh->fmap_nareas);

  for (i=0; i<fmh->fmap_nareas; i++) {
    printf("area:            %d\n", i+1);
    printf("area_offset:     0x%08x\n", ah->area_offset);
    printf("area_size:       0x%08x (%d)\n", ah->area_size, ah->area_size);
    snprintf(buf, FMAP_NAMELEN+1, "%s", ah->area_name);
    printf("area_name:       %s\n", buf);

    if (opt_extract) {
      char* s;
      for (s=buf;* s; s++)
        if (*s == ' ')
         *s = '_';
      FILE* fp = fopen(buf,"wb");
      if (!fp) {
        fprintf(stderr, "%s: can't open %s: %s\n",
                progname, buf, strerror(errno));
        retval = 1;
      } else {
        if (ah->area_size &&
            1 != fwrite(base_of_rom + ah->area_offset, ah->area_size, 1, fp)) {
          fprintf(stderr, "%s: can't write %s: %s\n",
                  progname, buf, strerror(errno));
          retval = 1;
        } else {
          printf("saved as \"%s\"\n", buf);
        }
        fclose(fp);
      }
    }

    ah++;
  }

  return retval;
}


int main(int argc, char* argv[]) {
  int c;
  int errorcnt = 0;
  struct stat sb;
  int fd;
  const char* fmap;
  int retval = 1;

  progname = strrchr(argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  opterr = 0;                     /* quiet, you */
  while ((c=getopt(argc, argv, ":x")) != -1) {
    switch (c)
    {
    case 'x':
      opt_extract = 1;
      break;
    case '?':
      fprintf(stderr, "%s: unrecognized switch: -%c\n",
              progname, optopt);
      errorcnt++;
      break;
    case ':':
      fprintf(stderr, "%s: missing argument to -%c\n",
              progname, optopt);
      errorcnt++;
      break;
    default:
      errorcnt++;
      break;
    }
  }

  if (errorcnt || optind >= argc) {
    fprintf(stderr,
            "\nUsage:  %s [-x] FLASHIMAGE\n\n"
            "Display (and extract with -x) the FMAP components from a BIOS image"
            "\n\n",
            progname);
    return 1;
  }

  if (0 != stat(argv[optind], &sb)) {
    fprintf(stderr, "%s: can't stat %s: %s\n",
            progname,
            argv[optind],
            strerror(errno));
    return 1;
  }

  fd = open(argv[optind], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "%s: can't open %s: %s\n",
            progname,
            argv[optind],
            strerror(errno));
    return 1;
  }
  printf("opened %s\n", argv[optind]);

  base_of_rom = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (base_of_rom == (char*)-1) {
    fprintf(stderr, "%s: can't mmap %s: %s\n",
            progname,
            argv[optind],
            strerror(errno));
    close(fd);
    return 1;
  }
  close(fd);                            /* done with this now */

  fmap = FmapFind((char*) base_of_rom, sb.st_size);
  if (fmap) {
    printf("hit at 0x%08x\n", (uint32_t) (fmap - (char*) base_of_rom));
    retval = dump_fmap(fmap);
  }

  if (0 != munmap(base_of_rom, sb.st_size)) {
    fprintf(stderr, "%s: can't munmap %s: %s\n",
            progname,
            argv[optind],
            strerror(errno));
    return 1;
  }

  return retval;
}
