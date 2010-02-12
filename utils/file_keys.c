/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for file and key handling.
 */

#include "file_keys.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "rsa_utility.h"
#include "utility.h"

uint8_t* BufferFromFile(char* input_file, int* len) {
  int fd;
  struct stat stat_fd;
  uint8_t* buf = NULL;

  if ((fd = open(input_file, O_RDONLY)) == -1) {
    fprintf(stderr, "Couldn't open file.\n");
    return NULL;
  }

  if (-1 == fstat(fd, &stat_fd)) {
    fprintf(stderr, "Couldn't stat key file\n");
    return NULL;
  }
  *len = stat_fd.st_size;

  /* Read entire key binary blob into a buffer. */
  buf = (uint8_t*) Malloc(*len);
  if (!buf)
    return NULL;

  if (*len != read(fd, buf, *len)) {
    fprintf(stderr, "Couldn't read key into a buffer.\n");
    return NULL;
  }

  close(fd);
  return buf;
}

RSAPublicKey* RSAPublicKeyFromFile(char* input_file) {
  int len;
  uint8_t* buf = BufferFromFile(input_file, &len);
  RSAPublicKey* key = RSAPublicKeyFromBuf(buf, len);
  Free(buf);
  return key;
}
