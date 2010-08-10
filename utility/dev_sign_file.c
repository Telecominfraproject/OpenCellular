/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Developer file-signing utility
 */

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>  /* For PRIu64 */
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cryptolib.h"
#include "host_common.h"
#include "kernel_blob.h"
#include "vboot_common.h"


/* Global opt */
static int opt_debug = 0;

/* Command line options */
enum {
  OPT_MODE_SIGN = 1000,
  OPT_MODE_VERIFY,
  OPT_KEYBLOCK,
  OPT_SIGNPRIVATE,
  OPT_VBLOCK,
};

static struct option long_opts[] = {
  {"sign", 1, 0,                      OPT_MODE_SIGN               },
  {"verify", 1, 0,                    OPT_MODE_VERIFY             },
  {"keyblock", 1, 0,                  OPT_KEYBLOCK                },
  {"signprivate", 1, 0,               OPT_SIGNPRIVATE             },
  {"vblock", 1, 0,                    OPT_VBLOCK                  },
  {"debug", 0, &opt_debug, 1                                      },
  {NULL, 0, 0, 0}
};


/* Print help and return error */
static int PrintHelp(char *progname) {
  fprintf(stderr,
          "This program is used to sign and verify developer-mode files\n");
  fprintf(stderr,
          "\n"
          "Usage:  %s --sign <file> [PARAMETERS]\n"
          "\n"
          "  Required parameters:\n"
          "    --keyblock <file>         Key block in .keyblock format\n"
          "    --signprivate <file>"
          "      Private key to sign file data, in .vbprivk format\n"
          "    --vblock <file>           Output signature in .vblock format\n"
          "\n",
          progname);
  fprintf(stderr,
          "OR\n\n"
          "Usage:  %s --verify <file> [PARAMETERS]\n"
          "\n"
          "  Required parameters:\n"
          "    --vblock <file>           Signature file in .vblock format\n"
          "\n",
          progname);
  return 1;
}

static void Debug(const char *format, ...) {
  if (!opt_debug)
    return;

  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "DEBUG: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
}


/* Sign a file. We'll reuse the same structs used to sign kernels, to avoid
   having to declare yet another one for just this purpose. */
static int Sign(const char* filename, const char* keyblock_file,
                const char* signprivate_file, const char* outfile) {
  uint8_t *file_data;
  uint64_t file_size;
  VbKeyBlockHeader* key_block;
  uint64_t key_block_size;
  VbPrivateKey* signing_key;
  VbSignature* body_sig;
  VbKernelPreambleHeader* preamble;
  FILE* f;
  uint64_t i;

  /* Read the file that we're going to sign. */
  file_data = ReadFile(filename, &file_size);
  if (!file_data) {
    error("Error reading file to sign.\n");
    return 1;
  }

  /* Get the key block and read the private key corresponding to it. */
  key_block = (VbKeyBlockHeader*)ReadFile(keyblock_file, &key_block_size);
  if (!key_block) {
    error("Error reading key block.\n");
    return 1;
  }
  signing_key = PrivateKeyRead(signprivate_file);
  if (!signing_key) {
    error("Error reading signing key.\n");
    return 1;
  }

  /* Sign the file data */
  body_sig = CalculateSignature(file_data, file_size, signing_key);
  if (!body_sig) {
    error("Error calculating body signature\n");
    return 1;
  }

  /* Create preamble */
  preamble = CreateKernelPreamble(0UL, 0UL, 0UL, 0UL,
                                  body_sig, 0UL, signing_key);
  if (!preamble) {
    error("Error creating preamble.\n");
    return 1;
  }

  /* Write the output file */
  Debug("writing %s...\n", outfile);
  f = fopen(outfile, "wb");
  if (!f) {
    error("Can't open output file %s\n", outfile);
    return 1;
  }
  Debug("0x%" PRIx64 " bytes of key_block\n", key_block_size);
  Debug("0x%" PRIx64 " bytes of preamble\n", preamble->preamble_size);
  i = ((1 != fwrite(key_block, key_block_size, 1, f)) ||
       (1 != fwrite(preamble, preamble->preamble_size, 1, f)));
  if (i) {
    error("Can't write output file %s\n", outfile);
    fclose(f);
    unlink(outfile);
    return 1;
  }
  fclose(f);

  /* Done */
  Free(preamble);
  Free(body_sig);
  Free(signing_key);
  Free(key_block);
  Free(file_data);

  /* Success */
  return 0;
}

static int Verify(const char* filename, const char* vblock_file) {
  uint8_t *file_data;
  uint64_t file_size;
  uint8_t *buf;
  uint64_t buf_size;
  VbKeyBlockHeader* key_block;
  VbKernelPreambleHeader* preamble;
  VbPublicKey* data_key;
  RSAPublicKey* rsa;
  uint64_t now = 0;

  /* Read the file that we're going to verify. */
  file_data = ReadFile(filename, &file_size);
  if (!file_data) {
    error("Error reading file to sign.\n");
    return 1;
  }

  /* Read the vblock that we're going to use on it */
  buf = ReadFile(vblock_file, &buf_size);
  if (!buf) {
    error("Error reading vblock_file.\n");
    return 1;
  }

  /* Find the key block */
  key_block = (VbKeyBlockHeader*)buf;
  Debug("Keyblock is 0x%" PRIx64 " bytes\n", key_block->key_block_size);
  now += key_block->key_block_size;
  if (now > buf_size) {
    error("key_block_size advances past the end of the buffer\n");
    return 1;
  }

  /* Find the preamble */
  preamble = (VbKernelPreambleHeader*)(buf + now);
  Debug("Preamble is 0x%" PRIx64 " bytes\n", preamble->preamble_size);
  now += preamble->preamble_size;
  if (now > buf_size ) {
    error("preamble_size advances past the end of the buffer\n");
    return 1;
  }

  Debug("Now is at 0x%" PRIx64 " bytes\n", now);

  /* Check the keyblock */
  if (0 != KeyBlockVerify(key_block, file_size, NULL)) {
    error("Error verifying key block.\n");
    return 1;
  }

  printf("Key block:\n");
  data_key = &key_block->data_key;
//HEY   printf("  Signature:           %s\n", sign_key ? "valid" : "ignored");
  printf("  Size:                0x%" PRIx64 "\n", key_block->key_block_size);
  printf("  Data key algorithm:  %" PRIu64 " %s\n", data_key->algorithm,
         (data_key->algorithm < kNumAlgorithms ?
          algo_strings[data_key->algorithm] : "(invalid)"));
  printf("  Data key version:    %" PRIu64 "\n", data_key->key_version);
  printf("  Flags:               %" PRIu64 "\n", key_block->key_block_flags);


  /* Verify preamble */
  rsa = PublicKeyToRSA(&key_block->data_key);
  if (!rsa) {
    error("Error parsing data key.\n");
    return 1;
  }
  if (0 != VerifyKernelPreamble(
        preamble, file_size, rsa)) {
    error("Error verifying preamble.\n");
    return 1;
  }

  printf("Preamble:\n");
  printf("  Size:                0x%" PRIx64 "\n", preamble->preamble_size);
  printf("  Header version:      %" PRIu32 ".%" PRIu32"\n",
         preamble->header_version_major, preamble->header_version_minor);
  printf("  Kernel version:      %" PRIu64 "\n", preamble->kernel_version);
  printf("  Body load address:   0x%" PRIx64 "\n", preamble->body_load_address);
  printf("  Body size:           0x%" PRIx64 "\n",
         preamble->body_signature.data_size);
  printf("  Bootloader address:  0x%" PRIx64 "\n",
         preamble->bootloader_address);
  printf("  Bootloader size:     0x%" PRIx64 "\n", preamble->bootloader_size);

  /* Verify body */
  if (0 != VerifyData(file_data, file_size, &preamble->body_signature,
                      rsa)) {
    error("Error verifying kernel body.\n");
    return 1;
  }
  printf("Body verification succeeded.\n");

  // HEY
  return 0;
}


int main(int argc, char* argv[]) {
  char* filename = NULL;
  char* keyblock_file = NULL;
  char* signprivate_file = NULL;
  char* vblock_file = NULL;
  int mode = 0;
  int parse_error = 0;
  int i;

  char *progname = strrchr(argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  while (((i = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) &&
         !parse_error) {
    switch (i) {
      default:
      case '?':
        /* Unhandled option */
        parse_error = 1;
        break;

      case 0:
        /* silently handled option */
        break;

      case OPT_MODE_SIGN:
      case OPT_MODE_VERIFY:
        if (mode && (mode != i)) {
          fprintf(stderr, "Only a single mode can be specified\n");
          parse_error = 1;
          break;
        }
        mode = i;
        filename = optarg;
        break;

      case OPT_KEYBLOCK:
        keyblock_file = optarg;
        break;

      case OPT_SIGNPRIVATE:
        signprivate_file = optarg;
        break;

      case OPT_VBLOCK:
        vblock_file = optarg;
        break;
    }
  }

  if (parse_error)
    return PrintHelp(progname);

  switch(mode) {
    case OPT_MODE_SIGN:
      if (!keyblock_file || !signprivate_file || !vblock_file) {
        fprintf(stderr, "Some required options are missing\n");
        return PrintHelp(progname);
      }
      return Sign(filename, keyblock_file, signprivate_file, vblock_file);

    case OPT_MODE_VERIFY:
      if (!vblock_file) {
        fprintf(stderr, "Some required options are missing\n");
        return PrintHelp(progname);
      }
      return Verify(filename, vblock_file);

    default:
      fprintf(stderr,
              "You must specify either --sign or --verify\n");
      return PrintHelp(progname);
  }

  return 1;
}
