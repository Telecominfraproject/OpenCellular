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

enum { FMT_NORMAL, FMT_PRETTY, FMT_FLASHROM };

/* global variables */
static int opt_extract = 0;
static int opt_format = FMT_NORMAL;
static char* progname;
static void* base_of_rom;


/* Return 0 if successful */
static int dump_fmap(const void* ptr, int argc, char *argv[]) {
  int i, retval = 0;
  char buf[80];                         // DWR: magic number
  const FmapHeader* fmh = (const FmapHeader*) ptr;
  const FmapAreaHeader* ah = (const FmapAreaHeader*) (ptr + sizeof(FmapHeader));

  if (FMT_NORMAL == opt_format) {
    snprintf(buf, FMAP_SIGNATURE_SIZE+1, "%s", fmh->fmap_signature);
    printf("fmap_signature   %s\n", buf);
    printf("fmap_version:    %d.%d\n",
           fmh->fmap_ver_major, fmh->fmap_ver_minor);
    printf("fmap_base:       0x%" PRIx64 "\n", fmh->fmap_base);
    printf("fmap_size:       0x%08x (%d)\n", fmh->fmap_size, fmh->fmap_size);
    snprintf(buf, FMAP_NAMELEN+1, "%s", fmh->fmap_name);
    printf("fmap_name:       %s\n", buf);
    printf("fmap_nareas:     %d\n", fmh->fmap_nareas);
  }

  for (i=0; i<fmh->fmap_nareas; i++, ah++) {
    snprintf(buf, FMAP_NAMELEN+1, "%s", ah->area_name);

    if (argc) {
      int j, found=0;
      for (j=0; j<argc; j++)
        if (!strcmp(argv[j], buf)) {
            found = 1;
            break;
          }
      if (!found) {
        continue;
      }
    }

    switch(opt_format)
    {
    case FMT_PRETTY:
      printf("%s %d %d\n", buf, ah->area_offset, ah->area_size);
      break;
    case FMT_FLASHROM:
      if (ah->area_size)
        printf("0x%08x:0x%08x %s\n", ah->area_offset,
                                     ah->area_offset + ah->area_size - 1, buf);
      break;
    default:
      printf("area:            %d\n", i+1);
      printf("area_offset:     0x%08x\n", ah->area_offset);
      printf("area_size:       0x%08x (%d)\n", ah->area_size, ah->area_size);
      printf("area_name:       %s\n", buf);
    }

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
          if (FMT_NORMAL == opt_format)
            printf("saved as \"%s\"\n", buf);
        }
        fclose(fp);
      }
    }
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
  while ((c=getopt(argc, argv, ":xpf")) != -1) {
    switch (c)
    {
    case 'x':
      opt_extract = 1;
      break;
    case 'p':
      opt_format = FMT_PRETTY;
      break;
    case 'f':
      opt_format = FMT_FLASHROM;
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
      "\nUsage:  %s [-x] [-p|-f] FLASHIMAGE [NAME...]\n\n"
      "Display (and extract with -x) the FMAP components from a BIOS image.\n"
      "The -p option makes the output easier to parse by scripts.\n"
      "The -f option emits the FMAP in the format used by flashrom.\n"
      "\n"
      "Specify one or more NAMEs to only print sections that exactly match.\n"
      "\n",
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
  if (FMT_NORMAL == opt_format)
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
    if (FMT_NORMAL == opt_format)
      printf("hit at 0x%08x\n", (uint32_t) (fmap - (char*) base_of_rom));
    retval = dump_fmap(fmap, argc-optind-1, argv+optind+1);
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
