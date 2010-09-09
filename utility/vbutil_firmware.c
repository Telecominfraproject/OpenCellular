/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot firmware utility
 */

#include <getopt.h>
#include <inttypes.h>  /* For PRIu64 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cryptolib.h"
#include "host_common.h"
#include "kernel_blob.h"
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
};

static struct option long_opts[] = {
  {"vblock", 1, 0,                    OPT_MODE_VBLOCK             },
  {"verify", 1, 0,                    OPT_MODE_VERIFY             },
  {"keyblock", 1, 0,                  OPT_KEYBLOCK                },
  {"signpubkey", 1, 0,                OPT_SIGNPUBKEY              },
  {"signprivate", 1, 0,               OPT_SIGNPRIVATE             },
  {"version", 1, 0,                   OPT_VERSION                 },
  {"fv", 1, 0,                        OPT_FV                      },
  {"kernelkey", 1, 0,                 OPT_KERNELKEY               },
  {NULL, 0, 0, 0}
};


/* Print help and return error */
static int PrintHelp(void) {

  puts("vbutil_firmware - Verified boot key block utility\n"
       "\n"
       "Usage:  vbutil_firmware <--vblock|--verify> <file> [OPTIONS]\n"
       "\n"
       "For '--vblock <file>', required OPTIONS are:\n"
       "  --keyblock <file>           Key block in .keyblock format\n"
       "  --signprivate <file>        Signing private key in .vbprivk format\n"
       "  --version <number>          Firmware version\n"
       "  --fv <file>                 Firmware volume to sign\n"
       "  --kernelkey <file>          Kernel subkey in .vbpubk format\n"
       "\n"
       "For '--verify <file>', required OPTIONS are:\n"
       "  --signpubkey <file>         Signing public key in .vbpubk format\n"
       "  --fv <file>                 Firmware volume to verify\n"
       "\n"
       "For '--verify <file>', optional OPTIONS are:\n"
       "  --kernelkey <file>          Write the kernel subkey to this file\n"
       "");
  return 1;
}


/* Create a firmware .vblock */
static int Vblock(const char* outfile, const char* keyblock_file,
                  const char* signprivate, uint64_t version,
                  const char* fv_file, const char* kernelkey_file) {

  VbPrivateKey* signing_key;
  VbPublicKey* kernel_subkey;
  VbSignature* body_sig;
  VbFirmwarePreambleHeader* preamble;
  VbKeyBlockHeader* key_block;
  uint64_t key_block_size;
  uint8_t* fv_data;
  uint64_t fv_size;
  FILE* f;
  uint64_t i;

  if (!outfile) {
    error("Must specify output filename\n");
    return 1;
  }
  if (!keyblock_file || !signprivate || !kernelkey_file) {
    error("Must specify all keys\n");
    return 1;
  }
  if (!fv_file) {
    error("Must specify firmware volume\n");
    return 1;
  }

  /* Read the key block and keys */
  key_block = (VbKeyBlockHeader*)ReadFile(keyblock_file, &key_block_size);
  if (!key_block) {
    error("Error reading key block.\n");
    return 1;
  }

  signing_key = PrivateKeyRead(signprivate);
  if (!signing_key) {
    error("Error reading signing key.\n");
    return 1;
  }

  kernel_subkey = PublicKeyRead(kernelkey_file);
  if (!kernel_subkey) {
    error("Error reading kernel subkey.\n");
    return 1;
  }

  /* Read and sign the firmware volume */
  fv_data = ReadFile(fv_file, &fv_size);
  if (!fv_data)
    return 1;
  if (!fv_size) {
    error("Empty firmware volume file\n");
    return 1;
  }
  body_sig = CalculateSignature(fv_data, fv_size, signing_key);
  if (!body_sig) {
    error("Error calculating body signature\n");
    return 1;
  }
  Free(fv_data);

  /* Create preamble */
  preamble = CreateFirmwarePreamble(version,
                                    kernel_subkey,
                                    body_sig,
                                    signing_key);
  if (!preamble) {
    error("Error creating preamble.\n");
    return 1;
  }

  /* Write the output file */
  f = fopen(outfile, "wb");
  if (!f) {
    error("Can't open output file %s\n", outfile);
    return 1;
  }
  i = ((1 != fwrite(key_block, key_block_size, 1, f)) ||
       (1 != fwrite(preamble, preamble->preamble_size, 1, f)));
  fclose(f);
  if (i) {
    error("Can't write output file %s\n", outfile);
    unlink(outfile);
    return 1;
  }

  /* Success */
  return 0;
}

static int Verify(const char* infile, const char* signpubkey,
                  const char* fv_file, const char* kernelkey_file) {

  VbKeyBlockHeader* key_block;
  VbFirmwarePreambleHeader* preamble;
  VbPublicKey* data_key;
  VbPublicKey* sign_key;
  VbPublicKey* kernel_subkey;
  RSAPublicKey* rsa;
  uint8_t* blob;
  uint64_t blob_size;
  uint8_t* fv_data;
  uint64_t fv_size;
  uint64_t now = 0;

  if (!infile || !signpubkey || !fv_file) {
    error("Must specify filename, signpubkey, and fv\n");
    return 1;
  }

  /* Read public signing key */
  sign_key = PublicKeyRead(signpubkey);
  if (!sign_key) {
    error("Error reading signpubkey.\n");
    return 1;
  }

  /* Read blob */
  blob = ReadFile(infile, &blob_size);
  if (!blob) {
    error("Error reading input file\n");
    return 1;
  }

  /* Read firmware volume */
  fv_data = ReadFile(fv_file, &fv_size);
  if (!fv_data) {
    error("Error reading firmware volume\n");
    return 1;
  }

  /* Verify key block */
  key_block = (VbKeyBlockHeader*)blob;
  if (0 != KeyBlockVerify(key_block, blob_size, sign_key, 0)) {
    error("Error verifying key block.\n");
    return 1;
  }
  Free(sign_key);
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
  if (!rsa) {
    error("Error parsing data key.\n");
    return 1;
  }

  /* Verify preamble */
  preamble = (VbFirmwarePreambleHeader*)(blob + now);
  if (0 != VerifyFirmwarePreamble(preamble, blob_size - now, rsa)) {
    error("Error verifying preamble.\n");
    return 1;
  }
  now += preamble->preamble_size;

  printf("Preamble:\n");
  printf("  Size:                  %" PRIu64 "\n", preamble->preamble_size);
  printf("  Header version:        %" PRIu32 ".%" PRIu32"\n",
         preamble->header_version_major, preamble->header_version_minor);
  printf("  Firmware version:      %" PRIu64 "\n", preamble->firmware_version);
  kernel_subkey = &preamble->kernel_subkey;
  printf("  Kernel key algorithm:  %" PRIu64 " %s\n",
         kernel_subkey->algorithm,
         (kernel_subkey->algorithm < kNumAlgorithms ?
          algo_strings[kernel_subkey->algorithm] : "(invalid)"));
  printf("  Kernel key version:    %" PRIu64 "\n",
         kernel_subkey->key_version);
  printf("  Kernel key sha1sum:    ");
  PrintPubKeySha1Sum(kernel_subkey);
  printf("\n");
  printf("  Firmware body size:    %" PRIu64 "\n",
         preamble->body_signature.data_size);

  /* TODO: verify body size same as signature size */

  /* Verify body */
  if (0 != VerifyData(fv_data, fv_size, &preamble->body_signature, rsa)) {
    error("Error verifying firmware body.\n");
    return 1;
  }
  printf("Body verification succeeded.\n");

  if (kernelkey_file) {
    if (0 != PublicKeyWrite(kernelkey_file, kernel_subkey)) {
      fprintf(stderr,
              "vbutil_firmware: unable to write kernel subkey\n");
      return 1;
    }
  }

  return 0;
}


int main(int argc, char* argv[]) {

  char* filename = NULL;
  char* key_block_file = NULL;
  char* signpubkey = NULL;
  char* signprivate = NULL;
  uint64_t version = 0;
  char* fv_file = NULL;
  char* kernelkey_file = NULL;
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

      case OPT_KERNELKEY:
        kernelkey_file = optarg;
        break;

      case OPT_VERSION:
        version = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          printf("Invalid --version\n");
          parse_error = 1;
        }
        break;
    }
  }

  if (parse_error)
    return PrintHelp();

  switch(mode) {
    case OPT_MODE_VBLOCK:
      return Vblock(filename, key_block_file, signprivate, version, fv_file,
                    kernelkey_file);
    case OPT_MODE_VERIFY:
      return Verify(filename, signpubkey, fv_file, kernelkey_file);
    default:
      printf("Must specify a mode.\n");
      return PrintHelp();
  }
}
