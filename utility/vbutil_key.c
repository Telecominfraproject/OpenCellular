/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot key utility
 */

#include <getopt.h>
#include <inttypes.h>  /* For PRIu64 */
#include <stdio.h>
#include <stdlib.h>

#include "cryptolib.h"
#include "host_common.h"
#include "vboot_common.h"


/* Command line options */
enum {
  OPT_IN = 1000,
  OPT_OUT,
  OPT_KEY_VERSION,
  OPT_ALGORITHM,
  OPT_MODE_PACK,
  OPT_MODE_UNPACK,
};

static struct option long_opts[] = {
  {"in", 1, 0,                        OPT_IN                      },
  {"out", 1, 0,                       OPT_OUT                     },
  {"version", 1, 0,                   OPT_KEY_VERSION             },
  {"algorithm", 1, 0,                 OPT_ALGORITHM               },
  {"pack", 0, 0,                      OPT_MODE_PACK               },
  {"unpack", 0, 0,                    OPT_MODE_UNPACK             },
  {NULL, 0, 0, 0}
};


/* Print help and return error */
static int PrintHelp(void) {
  int i;

  puts("vbutil_key - Verified boot key utility\n"
       "\n"
       "Usage:  vbutil_key <--pack|--unpack> [OPTIONS]\n"
       "\n"
       "For '--pack', required OPTIONS are:\n"
       "  --in <infile>                 Input key in .keyb format\n"
       "  --out <outfile>               Output file for .vbpubk format\n"
       "  --version <number>            Key version number\n"
       "  --algorithm <algoid>          Signing algorithm for key, one of:");

  for (i = 0; i < kNumAlgorithms; i++)
    printf("                                %d (%s)\n", i, algo_strings[i]);

  puts("\n"
       "For '--unpack', required OPTIONS are:\n"
       "  --in <infile>                 Input key in .vbpubk format\n"
       "Optional OPTIONS are:\n"
       "  --out <outfile>               Output file for .keyb format\n"
       "");
  return 1;
}


/* Pack a .keyb file into a .vbpubk */
static int Pack(const char *infile, const char *outfile, uint64_t algorithm,
                uint64_t version) {
  VbPublicKey* key;

  if (!infile || !outfile) {
    fprintf(stderr, "vbutil_key: Must specify --in and --out\n");
    return 1;
  }

  key = PublicKeyReadKeyb(infile, algorithm, version);
  if (!key) {
    fprintf(stderr, "vbutil_key: Error reading key.\n");
    return 1;
  }

  if (0 != PublicKeyWrite(outfile, key)) {
    fprintf(stderr, "vbutil_key: Error writing key.\n");
    return 1;
  }

  Free(key);
  return 0;
}


/* Unpack a .vbpubk */
static int Unpack(const char *infile, const char *outfile) {
  VbPublicKey* key;

  if (!infile) {
    fprintf(stderr, "vbutil_key: Must specify --in\n");
    return 1;
  }

  key = PublicKeyRead(infile);
  if (!key) {
    fprintf(stderr, "vbutil_key: Error reading key.\n");
    return 1;
  }

  printf("Key file:    %s\n", infile);
  printf("Algorithm:   %" PRIu64 " %s\n", key->algorithm,
         (key->algorithm < kNumAlgorithms ?
          algo_strings[key->algorithm] : "(invalid)"));
  printf("Version:     %" PRIu64 "\n", key->key_version);

  /* TODO: write key data, if any */

  Free(key);
  return 0;
}


int main(int argc, char* argv[]) {

  char *infile = NULL;
  char *outfile = NULL;
  int mode = 0;
  int parse_error = 0;
  uint64_t version = 1;
  uint64_t algorithm = kNumAlgorithms;
  char* e;
  int i;

  while ((i = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
    switch (i) {
      case '?':
        /* Unhandled option */
        printf("Unknown option\n");
        parse_error = 1;
        break;

      case OPT_IN:
        infile = optarg;
        break;

      case OPT_OUT:
        outfile = optarg;
        break;

      case OPT_KEY_VERSION:
        version = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          printf("Invalid --version\n");
          parse_error = 1;
        }
        break;

      case OPT_ALGORITHM:
        algorithm = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          printf("Invalid --algorithm\n");
          parse_error = 1;
        }
        break;

      case OPT_MODE_PACK:
      case OPT_MODE_UNPACK:
        mode = i;
        break;
    }
  }

  if (parse_error)
    return PrintHelp();

  switch(mode) {
    case OPT_MODE_PACK:
      return Pack(infile, outfile, algorithm, version);
    case OPT_MODE_UNPACK:
      return Unpack(infile, outfile);
    default:
      printf("Must specify a mode.\n");
      return PrintHelp();
  }
}
