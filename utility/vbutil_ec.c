/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot utility for EC firmware
 */

#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cryptolib.h"
#include "host_common.h"
#include "vboot_common.h"


/* Command line options */
enum {
  OPT_MODE_VBLOCK = 1000,
  OPT_MODE_VERIFY,
  OPT_KEYBLOCK,
  OPT_SIGNPUBKEY,
  OPT_SIGNPRIVATE,
  OPT_VERSION,
  OPT_FV,
  OPT_KERNELKEY,
  OPT_FLAGS,
  OPT_NAME,
};

static struct option long_opts[] = {
  {"vblock", 1, 0,                    OPT_MODE_VBLOCK             },
  {"verify", 1, 0,                    OPT_MODE_VERIFY             },
  {"keyblock", 1, 0,                  OPT_KEYBLOCK                },
  {"signpubkey", 1, 0,                OPT_SIGNPUBKEY              },
  {"signprivate", 1, 0,               OPT_SIGNPRIVATE             },
  {"version", 1, 0,                   OPT_VERSION                 },
  {"fv", 1, 0,                        OPT_FV                      },
  {"flags", 1, 0,                     OPT_FLAGS                   },
  {"name", 1, 0,                      OPT_NAME                    },
  {NULL, 0, 0, 0}
};


/* Print help and return error */
static int PrintHelp(void) {

  puts("vbutil_ec - Verified boot signing utility for EC firmware\n"
       "\n"
       "Usage:  vbutil_ec <--vblock|--verify> <file> [OPTIONS]\n"
       "\n"
       "For '--vblock <file>', required OPTIONS are:\n"
       "  --keyblock <file>           Key block in .keyblock format\n"
       "  --signprivate <file>        Signing private key in .vbprivk format\n"
       "  --version <number>          Firmware version\n"
       "  --fv <file>                 Firmware volume to sign\n"
       "optional OPTIONS are:\n"
       "  --flags <number>            Preamble flags (defaults to 0)\n"
       "  --name <string>             Human-readable description\n"
       "\n"
       "For '--verify <file>', required OPTIONS are:\n"
       "  --fv <file>                 Firmware volume to verify\n"
       "optional OPTIONS are:\n"
       "  --signpubkey <file>         Signing public key in .vbpubk format\n"
       "\n");
  return 1;
}


/* Create an EC firmware .vblock */
static int Vblock(const char* outfile, const char* keyblock_file,
                  const char* signprivate, uint64_t version,
                  const char* fv_file, uint32_t preamble_flags,
                  const char* name) {
  VbPrivateKey* signing_key;
  VbSignature* body_digest;
  VbECPreambleHeader* preamble;
  VbKeyBlockHeader* key_block;
  uint64_t key_block_size;
  uint8_t* fv_data;
  uint64_t fv_size;
  FILE* f;
  uint64_t i;

  if (!outfile)
    VbExError("Must specify output filename\n");

  if (!keyblock_file || !signprivate)
    VbExError("Must specify all keys\n");

  if (!fv_file)
    VbExError("Must specify firmware volume\n");

  /* Read the key block and keys */
  key_block = (VbKeyBlockHeader*)ReadFile(keyblock_file, &key_block_size);
  if (!key_block)
    VbExError("Error reading key block.\n");

  signing_key = PrivateKeyRead(signprivate);
  if (!signing_key)
    VbExError("Error reading signing key.\n");

  /* Read and sign the firmware volume */
  fv_data = ReadFile(fv_file, &fv_size);
  if (!fv_data)
    return 1;
  if (!fv_size)
    VbExError("Empty firmware volume file\n");

  if (name && strlen(name)+1 > sizeof(preamble->name))
    VbExError("Name string is too long\n");

  body_digest = CalculateHash(fv_data, fv_size, signing_key);
  if (!body_digest)
    VbExError("Error calculating body digest\n");
  free(fv_data);

  /* Create preamble */
  preamble = CreateECPreamble(version, body_digest, signing_key,
                              preamble_flags, name);
  if (!preamble)
    VbExError("Error creating preamble.\n");

  /* Write the output file */
  f = fopen(outfile, "wb");
  if (!f)
    VbExError("Can't open output file %s\n", outfile);

  i = ((1 != fwrite(key_block, key_block_size, 1, f)) ||
       (1 != fwrite(preamble, preamble->preamble_size, 1, f)));
  fclose(f);
  if (i) {
    unlink(outfile);
    VbExError("Can't write output file %s\n", outfile);
  }

  /* Success */
  return 0;
}

static int Verify(const char* infile,
                  const char* signpubkey,
                  const char* fv_file) {
  VbKeyBlockHeader* key_block;
  VbECPreambleHeader* preamble;
  VbPublicKey* data_key;
  VbPublicKey* sign_key = 0;
  RSAPublicKey* rsa;
  uint8_t* blob;
  uint64_t blob_size;
  uint8_t* fv_data;
  uint64_t fv_size;
  uint64_t now = 0;

  if (!infile || !fv_file) {
    VbExError("Must specify filename and fv\n");
    return 1;
  }

  /* Read public signing key */
  if (signpubkey) {
    sign_key = PublicKeyRead(signpubkey);
    if (!sign_key)
      VbExError("Error reading signpubkey.\n");
  } else {
    printf("WARNING:  No public key given - signature is not checked\n");
  }

  /* Read blob */
  blob = ReadFile(infile, &blob_size);
  if (!blob)
    VbExError("Error reading input file\n");

  /* Read firmware volume */
  fv_data = ReadFile(fv_file, &fv_size);
  if (!fv_data)
    VbExError("Error reading firmware volume\n");

  /* Verify key block */
  key_block = (VbKeyBlockHeader*)blob;
  if (0 != KeyBlockVerify(key_block, blob_size, sign_key, !signpubkey))
    VbExError("Error verifying key block.\n");

  if (sign_key)
    free(sign_key);
  now += key_block->key_block_size;

  printf("Key block:\n");
  data_key = &key_block->data_key;
  printf("  Size:                %" PRIu64 "\n", key_block->key_block_size);
  printf("  Flags:               %" PRIu64 " (ignored)\n",
         key_block->key_block_flags);
  printf("  Data key algorithm:  %" PRIu64 " %s\n", data_key->algorithm,
         (data_key->algorithm < kNumAlgorithms ?
          algo_strings[data_key->algorithm] : "(invalid)"));
  printf("  Data key version:    %" PRIu64 "\n", data_key->key_version);
  printf("  Data key sha1sum:    ");
  PrintPubKeySha1Sum(data_key);
  printf("\n");

  rsa = PublicKeyToRSA(&key_block->data_key);
  if (!rsa)
    VbExError("Error parsing data key.\n");

  /* Verify preamble */
  preamble = (VbECPreambleHeader*)(blob + now);
  if (0 != VerifyECPreamble(preamble, blob_size - now, rsa))
    VbExError("Error verifying preamble.\n");

  now += preamble->preamble_size;

  printf("Preamble:\n");
  printf("  Size:                  %" PRIu64 "\n", preamble->preamble_size);
  printf("  Header version:        %" PRIu32 ".%" PRIu32"\n",
         preamble->header_version_major, preamble->header_version_minor);
  printf("  Firmware version:      %" PRIu64 "\n", preamble->firmware_version);
  printf("  Firmware body size:    %" PRIu64 "\n",
         preamble->body_digest.data_size);
  printf("  Preamble flags:        %" PRIu32 "\n", preamble->flags);
  printf("  Preamble name:         %s\n", preamble->name);

  /* TODO: verify body size same as signature size */

  /* Verify body */
  if (preamble->flags & VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL) {
    printf("Preamble requests USE_RO_NORMAL; skipping body verification.\n");
  } else {
    if (0 != EqualData(fv_data, fv_size, &preamble->body_digest, rsa))
      VbExError("Error verifying firmware body.\n");
    printf("Body verification succeeded.\n");
  }

  return 0;
}


int main(int argc, char* argv[]) {

  char* filename = NULL;
  char* key_block_file = NULL;
  char* signpubkey = NULL;
  char* signprivate = NULL;
  uint64_t version = 0;
  int got_version = 0;
  char* fv_file = NULL;
  uint32_t preamble_flags = 0;
  char *name = NULL;
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

      case OPT_MODE_VBLOCK:
      case OPT_MODE_VERIFY:
        mode = i;
        filename = optarg;
        break;

      case OPT_KEYBLOCK:
        key_block_file = optarg;
        break;

      case OPT_SIGNPUBKEY:
        signpubkey = optarg;
        break;

      case OPT_SIGNPRIVATE:
        signprivate = optarg;
        break;

      case OPT_FV:
        fv_file = optarg;
        break;

      case OPT_VERSION:
        version = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          printf("Invalid --version\n");
          parse_error = 1;
        }
        got_version = 1;
        break;

      case OPT_FLAGS:
        preamble_flags = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          printf("Invalid --flags\n");
          parse_error = 1;
        }
        break;

    case OPT_NAME:
      name = optarg;
      break;
    }
  }

  if (parse_error)
    return PrintHelp();

  switch(mode) {
    case OPT_MODE_VBLOCK:
      if (!got_version) {
        printf("Must specify a version\n");
        return PrintHelp();
      }
      return Vblock(filename, key_block_file, signprivate, version,
                    fv_file, preamble_flags, name);
    case OPT_MODE_VERIFY:
      return Verify(filename, signpubkey, fv_file);
    default:
      printf("Must specify a mode.\n");
      return PrintHelp();
  }
}
