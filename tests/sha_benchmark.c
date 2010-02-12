/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

#include "sha.h"
#include "timer_utils.h"
#include "utility.h"

#define NUM_HASH_ALGORITHMS 3
#define TEST_BUFFER_SIZE 4000000

/* Table of hash function pointers and their description. */
typedef uint8_t* (*Hashptr) (const uint8_t*, int, uint8_t*);
typedef struct HashFxTable {
  Hashptr hash;
  char* description;
} HashFxTable;

HashFxTable hash_functions[NUM_HASH_ALGORITHMS] = {
  {SHA1, "SHA1"},
  {SHA256, "SHA256"},
  {SHA512, "SHA512"}
};

int main(int argc, char* argv[]) {
  int i;
  double speed;
  uint32_t msecs;
  uint8_t* buffer = (uint8_t*) Malloc(TEST_BUFFER_SIZE);
  uint8_t* digest = (uint8_t*) Malloc(SHA512_DIGEST_SIZE); /* Maximum size of
                                                            * the digest. */
  ClockTimerState ct;

  /* Iterate through all the hash functions. */
  for(i = 0; i < NUM_HASH_ALGORITHMS; i++) {
    StartTimer(&ct);
    hash_functions[i].hash(buffer, TEST_BUFFER_SIZE, digest);
    StopTimer(&ct);

    msecs = GetDurationMsecs(&ct);
    speed = ((TEST_BUFFER_SIZE / 10e6)
             / (msecs / 10e3)); /* Mbytes/sec */

    fprintf(stderr, "%s Time taken = %u ms, Speed = %f Mbytes/sec\n",
            hash_functions[i].description, msecs, speed);
  }

  Free(digest);
  Free(buffer);
  return 0;
}
