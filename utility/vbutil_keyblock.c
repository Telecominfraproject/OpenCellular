/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot key block utility
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
  OPT_MODE_PACK = 1000,
  OPT_MODE_UNPACK,
  OPT_DATAPUBKEY,
  OPT_SIGNPUBKEY,
  OPT_SIGNPRIVATE,
  OPT_ALGORITHM,
  OPT_FLAGS,
};

static struct option long_opts[] = {
  {"pack", 1, 0,                      OPT_MODE_PACK               },
  {"unpack", 1, 0,                    OPT_MODE_UNPACK             },
  {"datapubkey", 1, 0,                OPT_DATAPUBKEY              },
  {"signpubkey", 1, 0,                OPT_SIGNPUBKEY              },
  {"signprivate", 1, 0,               OPT_SIGNPRIVATE             },
  {"algorithm", 1, 0,                 OPT_ALGORITHM               },
  {"flags", 1, 0,                     OPT_FLAGS                   },
  {NULL, 0, 0, 0}
};


/* Print help and return error */
static int PrintHelp(void) {
  int i;

  puts("vbutil_keyblock - Verified boot key block utility\n"
       "\n"
       "Usage:  vbutil_keyblock <--pack|--unpack> <file> [OPTIONS]\n"
       "\n"
       "For '--pack <file>', required OPTIONS are:\n"
       "  --datapubkey <file>         Data public key in .vbpubk format\n"
       "  --signprivate <file>        Signing private key in .pem format\n"
       "  --algorithm <algoid>        Signing algorithm for key, one of:");

  for (i = 0; i < kNumAlgorithms; i++)
    printf("                                %d (%s)\n", i, algo_strings[i]);

  puts("\n"
       "Optional OPTIONS are:\n"
       "  --flags <number>            Flags\n"
       "\n"
       "For '--unpack <file>', required OPTIONS are:\n"
       "  --signpubkey <file>         Signing public key in .vbpubk format\n"
       "Optional OPTIONS are:\n"
       "  --datapubkey <file>         Data public key output file\n"
       "");
  return 1;
}


/* Pack a .keyblock */
static int Pack(const char* outfile, const char* datapubkey,
                const char* signprivate, uint64_t algorithm,
                uint64_t flags) {
  VbPublicKey* data_key;
  VbPrivateKey* signing_key;
  VbKeyBlockHeader* block;

  if (!outfile) {
    fprintf(stderr, "vbutil_keyblock: Must specify output filename\n");
    return 1;
  }
  if (!datapubkey || !signprivate) {
    fprintf(stderr, "vbutil_keyblock: Must specify all keys\n");
    return 1;
  }
  if (algorithm >= kNumAlgorithms) {
    fprintf(stderr, "Invalid algorithm\n");
    return 1;
  }

  data_key = PublicKeyRead(datapubkey);
  if (!data_key) {
    fprintf(stderr, "vbutil_keyblock: Error reading data key.\n");
    return 1;
  }
  signing_key = PrivateKeyRead(signprivate, algorithm);
  if (!signing_key) {
    fprintf(stderr, "vbutil_keyblock: Error reading signing key.\n");
    return 1;
  }

  block = KeyBlockCreate(data_key, signing_key, flags);
  Free(data_key);
  Free(signing_key);

  if (0 != KeyBlockWrite(outfile, block)) {
    fprintf(stderr, "vbutil_keyblock: Error writing key block.\n");
    return 1;
  }
  Free(block);
  return 0;
}


static int Unpack(const char* infile, const char* datapubkey,
                  const char* signpubkey) {
  VbPublicKey* data_key;
  VbPublicKey* sign_key;
  VbKeyBlockHeader* block;

  if (!infile || !signpubkey) {
    fprintf(stderr, "vbutil_keyblock: Must specify filename and signpubkey\n");
    return 1;
  }

  sign_key = PublicKeyRead(signpubkey);
  if (!sign_key) {
    fprintf(stderr, "vbutil_keyblock: Error reading signpubkey.\n");
    return 1;
  }

  block = KeyBlockRead(infile);
  if (!block) {
    fprintf(stderr, "vbutil_keyblock: Error reading key block.\n");
    return 1;
  }
  /* Verify the block with the signing public key, since
   * KeyBlockRead() only verified the hash. */
  /* TODO: should just print a warning, since self-signed key blocks
   * won't have a public key; signpubkey should also be an optional
   * argument. */
  if (0 != KeyBlockVerify(block, block->key_block_size, sign_key)) {
    fprintf(stderr, "vbutil_keyblock: Error verifying key block.\n");
    return 1;
  }
  Free(sign_key);

  printf("Key block file:      %s\n", infile);
  printf("Flags:               %" PRIu64 "\n", block->key_block_flags);

  data_key = &block->data_key;
  printf("Data key algorithm:  %" PRIu64 " %s\n", data_key->algorithm,
         (data_key->algorithm < kNumAlgorithms ?
          algo_strings[data_key->algorithm] : "(invalid)"));
  printf("Data key version:    %" PRIu64 "\n", data_key->key_version);

  /* TODO: write key data, if output file specified */

  Free(block);
  return 0;
}


int main(int argc, char* argv[]) {

  char* filename = NULL;
  char* datapubkey = NULL;
  char* signpubkey = NULL;
  char* signprivate = NULL;
  uint64_t flags = 0;
  uint64_t algorithm = kNumAlgorithms;
  int mode = 0;
  int parse_error = 0;
  char* e;
  int i;

  while ((i = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
    switch (i) {
      case '?':
        /* Unhandled option */
        printf("Unknown option\n");
        parse_error = 1;
        break;

      case OPT_MODE_PACK:
      case OPT_MODE_UNPACK:
        mode = i;
        filename = optarg;
        break;

      case OPT_DATAPUBKEY:
        datapubkey = optarg;
        break;

      case OPT_SIGNPUBKEY:
        signpubkey = optarg;
        break;

      case OPT_SIGNPRIVATE:
        signprivate = optarg;
        break;

      case OPT_ALGORITHM:
        algorithm = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          printf("Invalid --algorithm\n");
          parse_error = 1;
        }
        break;

      case OPT_FLAGS:
        flags = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          printf("Invalid --flags\n");
          parse_error = 1;
        }
        break;
    }
  }

  if (parse_error)
    return PrintHelp();

  switch(mode) {
    case OPT_MODE_PACK:
      return Pack(filename, datapubkey, signprivate, algorithm, flags);
    case OPT_MODE_UNPACK:
      return Unpack(filename, datapubkey, signpubkey);
    default:
      printf("Must specify a mode.\n");
      return PrintHelp();
  }
}
