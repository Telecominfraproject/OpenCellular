// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bmpblk_util.h"


// Returns pointer to buffer containing entire file, sets length.
static void *read_entire_file(const char *filename, size_t *length) {
  int fd;
  struct stat sbuf;
  void *ptr;

  *length = 0;                          // just in case

  if (0 != stat(filename, &sbuf)) {
    fprintf(stderr, "Unable to stat %s: %s\n", filename, strerror(errno));
    return 0;
  }

  if (!sbuf.st_size) {
    fprintf(stderr, "File %s is empty\n", filename);
    return 0;
  }

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
    return 0;
  }

  ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (MAP_FAILED == ptr) {
    fprintf(stderr, "Unable to mmap %s: %s\n", filename, strerror(errno));
    close(fd);
    return 0;
  }

  *length = sbuf.st_size;

  close(fd);

  return ptr;
}


// Reclaims buffer from read_entire_file().
static void discard_file(void *ptr, size_t length) {
  munmap(ptr, length);
}


// Show what's inside
int display_bmpblock(const char *infile) {
  char *ptr;
  size_t length = 0;
  BmpBlockHeader *hdr;

  ptr = (char *)read_entire_file(infile, &length);
  if (!ptr)
    return 1;

  if (length < sizeof(BmpBlockHeader)) {
    fprintf(stderr, "File %s is too small to be a BMPBLOCK\n", infile);
    discard_file(ptr, length);
    return 1;
  }

  if (0 != memcmp(ptr, BMPBLOCK_SIGNATURE, BMPBLOCK_SIGNATURE_SIZE)) {
    fprintf(stderr, "File %s is not a BMPBLOCK\n", infile);
    discard_file(ptr, length);
    return 1;
  }

  hdr = (BmpBlockHeader *)ptr;
  printf("%s:\n", infile);
  printf("  version %d.%d\n", hdr->major_version, hdr->minor_version);
  printf("  %d screens\n", hdr->number_of_screenlayouts);
  printf("  %d localizations\n", hdr->number_of_localizations);
  printf("  %d discrete images\n", hdr->number_of_imageinfos);

  discard_file(ptr, length);

  return 0;
}

int extract_bmpblock(const char *infile, const char *dirname, int force) {
    printf("extract parts from %s into %s %s overwriting\n",
           infile, dirname, force ? "with" : "without");
    printf("NOT YET IMPLEMENTED\n");
    return 0;
}
