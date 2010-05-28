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

#include "cryptolib.h"
#include "signature_digest.h"
#include "utility.h"

uint8_t* BufferFromFile(const char* input_file, uint64_t* len) {
  int fd;
  struct stat stat_fd;
  uint8_t* buf = NULL;

  if ((fd = open(input_file, O_RDONLY)) == -1) {
    debug("Couldn't open file.\n");
    return NULL;
  }

  if (-1 == fstat(fd, &stat_fd)) {
    debug("Couldn't stat file.\n");
    return NULL;
  }
  *len = stat_fd.st_size;

  buf = (uint8_t*) Malloc(*len);
  if (!buf) {
    error("Couldn't allocate %ld bytes.\n", *len);
    return NULL;
  }

  if (*len != read(fd, buf, *len)) {
    debug("Couldn't read file into a buffer.\n");
    return NULL;
  }

  close(fd);
  return buf;
}

RSAPublicKey* RSAPublicKeyFromFile(const char* input_file) {
  uint64_t len;
  RSAPublicKey* key = NULL;
  uint8_t* buf = BufferFromFile(input_file, &len);
  if (buf)
    key = RSAPublicKeyFromBuf(buf, len);
  Free(buf);
  return key;
}

uint8_t* DigestFile(char* input_file, int sig_algorithm) {
  int input_fd, len;
  uint8_t data[SHA1_BLOCK_SIZE];
  uint8_t* digest = NULL;
  DigestContext ctx;

  if( (input_fd = open(input_file, O_RDONLY)) == -1 ) {
    debug("Couldn't open input file.\n");
    return NULL;
  }
  DigestInit(&ctx, sig_algorithm);
  while ( (len = read(input_fd, data, SHA1_BLOCK_SIZE)) ==
          SHA1_BLOCK_SIZE)
    DigestUpdate(&ctx, data, len);
  if (len != -1)
    DigestUpdate(&ctx, data, len);
  digest = DigestFinal(&ctx);
  close(input_fd);
  return digest;
}

uint8_t* SignatureFile(const char* input_file, const char* key_file,
                       int algorithm) {
  char* sign_utility = "./sign_data.sh";
  char* cmd;  /* Command line to invoke. */
  int cmd_len;
  FILE* cmd_out;  /* File descriptor to command output. */
  uint8_t* signature = NULL;
  int signature_size = siglen_map[algorithm];

  /* Build command line:
   * sign_data.sh <algorithm> <key file> <input file>
   */
  cmd_len = (strlen(sign_utility) + 1 + /* +1 for space. */
             2 + 1 + /* For [algorithm]. */
             strlen(key_file) + 1 + /* +1 for space. */
             strlen(input_file) +
             1);  /* For the trailing '\0'. */
  cmd = (char*) Malloc(cmd_len);
  snprintf(cmd, cmd_len, "%s %d %s %s", sign_utility, algorithm, key_file,
           input_file);
  cmd_out = popen(cmd, "r");
  Free(cmd);
  if (!cmd_out) {
    debug("Couldn't execute: %s\n", cmd);
    return NULL;
  }

  signature = (uint8_t*) Malloc(signature_size);
  if (fread(signature, signature_size, 1, cmd_out) != 1) {
    debug("Couldn't read signature.\n");
    pclose(cmd_out);
    Free(signature);
    return NULL;
  }

  pclose(cmd_out);
  return signature;
}
