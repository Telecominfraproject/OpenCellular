/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot kernel utility
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
  OPT_MODE_PACK = 1000,
  OPT_MODE_VERIFY,
  OPT_KEYBLOCK,
  OPT_SIGNPUBKEY,
  OPT_SIGNPRIVATE,
  OPT_VERSION,
  OPT_VMLINUZ,
  OPT_BOOTLOADER,
  OPT_CONFIG,
  OPT_PAD,
};

static struct option long_opts[] = {
  {"pack", 1, 0,                      OPT_MODE_PACK               },
  {"verify", 1, 0,                    OPT_MODE_VERIFY             },
  {"keyblock", 1, 0,                  OPT_KEYBLOCK                },
  {"signpubkey", 1, 0,                OPT_SIGNPUBKEY              },
  {"signprivate", 1, 0,               OPT_SIGNPRIVATE             },
  {"version", 1, 0,                   OPT_VERSION                 },
  {"vmlinuz", 1, 0,                   OPT_VMLINUZ                 },
  {"bootloader", 1, 0,                OPT_BOOTLOADER              },
  {"config", 1, 0,                    OPT_CONFIG                  },
  {"pad", 1, 0,                       OPT_PAD                     },
  {NULL, 0, 0, 0}
};


/* Print help and return error */
static int PrintHelp(void) {

  puts("vbutil_kernel - Verified boot key block utility\n"
       "\n"
       "Usage:  vbutil_kernel <--pack|--verify> <file> [OPTIONS]\n"
       "\n"
       "For '--pack <file>', required OPTIONS are:\n"
       "  --keyblock <file>           Key block in .keyblock format\n"
       "  --signprivate <file>        Signing private key in .pem format\n"
       "  --version <number>          Kernel version\n"
       "  --vmlinuz <file>            Linux kernel image\n"
       "  --bootloader <file>         Bootloader stub\n"
       "  --config <file>             Config file\n"
       "Optional OPTIONS are:\n"
       "  --pad <number>              Padding size in bytes\n"
       "\n"
       "For '--verify <file>', required OPTIONS are:\n"
       "  --signpubkey <file>         Signing public key in .vbpubk format\n"
       "");
  return 1;
}


/* Return the smallest integral multiple of [alignment] that is equal
 * to or greater than [val]. Used to determine the number of
 * pages/sectors/blocks/whatever needed to contain [val]
 * items/bytes/etc. */
static uint64_t roundup(uint64_t val, uint64_t alignment) {
  uint64_t rem = val % alignment;
  if ( rem )
    return val + (alignment - rem);
  return val;
}


/* Match regexp /\b--\b/ to delimit the start of the kernel commandline. If we
 * don't find one, we'll use the whole thing. */
static unsigned int find_cmdline_start(char *input, unsigned int max_len) {
  int start = 0;
  int i;
  for(i = 0; i < max_len - 1 && input[i]; i++) {
    if ('-' == input[i] && '-' == input[i + 1]) {  /* found a "--" */
      if ((i == 0 || ' ' == input[i - 1]) &&   /* nothing before it */
          (i + 2 >= max_len || ' ' == input[i+2])) {  /* nothing after it */
        start = i+2;          /* note: hope there's a trailing '\0' */
        break;
      }
    }
  }
  while(' ' == input[start])                    /* skip leading spaces */
    start++;

  return start;
}


/* Pack a .kernel */
static int Pack(const char* outfile, const char* keyblock_file,
                const char* signprivate, uint64_t version,
                const char* vmlinuz, const char* bootloader_file,
                const char* config_file, uint64_t pad) {

  struct linux_kernel_header *lh = 0;
  struct linux_kernel_params *params = 0;
  VbPrivateKey* signing_key;
  VbSignature* body_sig;
  VbKernelPreambleHeader* preamble;
  VbKeyBlockHeader* key_block;
  uint64_t key_block_size;
  uint8_t* config_buf;
  uint64_t config_size;
  uint8_t* bootloader_buf;
  uint64_t bootloader_size;
  uint64_t bootloader_mem_start;
  uint64_t bootloader_mem_size;
  uint8_t* kernel_buf;
  uint64_t kernel_size;
  uint64_t kernel32_start = 0;
  uint64_t kernel32_size = 0;
  uint32_t cmdline_addr;
  uint8_t* blob = NULL;
  uint64_t blob_size;
  uint64_t now = 0;
  FILE* f;
  uint64_t i;

  if (!outfile) {
    error("Must specify output filename\n");
    return 1;
  }
  if (!keyblock_file || !signprivate) {
    error("Must specify all keys\n");
    return 1;
  }
  if (!vmlinuz || !bootloader_file || !config_file) {
    error("Must specify all input files\n");
    return 1;
  }

  /* Read the key block and private key */
  key_block = (VbKeyBlockHeader*)ReadFile(keyblock_file, &key_block_size);
  if (!key_block) {
    error("Error reading key block.\n");
    return 1;
  }
  if (pad < key_block->key_block_size) {
    error("Pad too small\n");
    return 1;
  }

  signing_key = PrivateKeyRead(signprivate, key_block->data_key.algorithm);
  if (!signing_key) {
    error("Error reading signing key.\n");
    return 1;
  }

  /* Read the config file */
  config_buf = ReadFile(config_file, &config_size);
  if (!config_buf)
    return 1;
  if (CROS_CONFIG_SIZE <= config_size) {  /* need room for trailing '\0' */
    error("Config file %s is too large (>= %d bytes)\n",
          config_file, CROS_CONFIG_SIZE);
    return 1;
  }
  /* Replace newlines with spaces */
  for (i = 0; i < config_size; i++)
    if ('\n' == config_buf[i])
      config_buf[i] = ' ';

  /* Read the bootloader */
  bootloader_buf = ReadFile(bootloader_file, &bootloader_size);
  if (!bootloader_buf)
    return 1;

  /* Read the kernel */
  kernel_buf = ReadFile(vmlinuz, &kernel_size);
  if (!kernel_buf)
    return 1;
  if (!kernel_size) {
    error("Empty kernel file\n");
    return 1;
  }

  /* The first part of vmlinuz is a header, followed by a real-mode
   * boot stub.  We only want the 32-bit part. */
  lh = (struct linux_kernel_header *)kernel_buf;
  kernel32_start = (lh->setup_sects + 1) << 9;
  if (kernel32_start >= kernel_size) {
    error("Malformed kernel\n");
    return 1;
  }
  kernel32_size = kernel_size - kernel32_start;

  /* Allocate and zero the blob we need. */
  blob_size = roundup(kernel32_size, CROS_ALIGN) +
      CROS_CONFIG_SIZE +
      CROS_PARAMS_SIZE +
      roundup(bootloader_size, CROS_ALIGN);
  blob = (uint8_t *)Malloc(blob_size);
  if (!blob) {
    error("Couldn't allocate %ld bytes.\n", blob_size);
    return 1;
  }
  Memset(blob, 0, blob_size);

  /* Copy the 32-bit kernel. */
  if (kernel32_size)
    Memcpy(blob + now, kernel_buf + kernel32_start, kernel32_size);
  now += roundup(now + kernel32_size, CROS_ALIGN);

  /* Find the load address of the commandline. We'll need it later. */
  cmdline_addr = CROS_32BIT_ENTRY_ADDR + now +
      find_cmdline_start((char *)config_buf, config_size);

  /* Copy the config. */
  if (config_size)
    Memcpy(blob + now, config_buf, config_size);
  now += CROS_CONFIG_SIZE;

  /* The zeropage data is next. Overlay the linux_kernel_header onto it, and
   * tweak a few fields. */
  params = (struct linux_kernel_params *)(blob + now);
  Memcpy(&(params->setup_sects), &(lh->setup_sects),
         sizeof(*lh) - offsetof(struct linux_kernel_header, setup_sects));
  params->boot_flag = 0;
  params->ramdisk_image = 0;             /* we don't support initrd */
  params->ramdisk_size = 0;
  params->type_of_loader = 0xff;
  params->cmd_line_ptr = cmdline_addr;
  now += CROS_PARAMS_SIZE;

  /* Finally, append the bootloader. Remember where it will load in
   * memory, too. */
  bootloader_mem_start = CROS_32BIT_ENTRY_ADDR + now;
  bootloader_mem_size = roundup(bootloader_size, CROS_ALIGN);
  if (bootloader_size)
    Memcpy(blob + now, bootloader_buf, bootloader_size);
  now += bootloader_mem_size;

  /* Free input buffers */
  Free(kernel_buf);
  Free(config_buf);
  Free(bootloader_buf);

  /* Sign the kernel data */
  body_sig = CalculateSignature(blob, blob_size, signing_key);
  if (!body_sig) {
    error("Error calculating body signature\n");
    return 1;
  }

  /* Create preamble */
  preamble = CreateKernelPreamble(version,
                                  CROS_32BIT_ENTRY_ADDR,
                                  bootloader_mem_start,
                                  bootloader_mem_size,
                                  body_sig,
                                  pad - key_block_size,
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
       (1 != fwrite(preamble, preamble->preamble_size, 1, f)) ||
       (1 != fwrite(blob, blob_size, 1, f)));
  fclose(f);
  if (i) {
    error("Can't write output file %s\n", outfile);
    unlink(outfile);
    return 1;
  }

  /* Success */
  return 0;
}


static int Verify(const char* infile, const char* signpubkey) {

  VbKeyBlockHeader* key_block;
  VbKernelPreambleHeader* preamble;
  VbPublicKey* data_key;
  VbPublicKey* sign_key;
  RSAPublicKey* rsa;
  uint8_t* blob;
  uint64_t blob_size;
  uint64_t now = 0;

  if (!infile || !signpubkey) {
    error("Must specify filename and signpubkey\n");
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

  /* Verify key block */
  key_block = (VbKeyBlockHeader*)blob;
  if (0 != KeyBlockVerify(key_block, blob_size, sign_key)) {
    error("Error verifying key block.\n");
    return 1;
  }
  Free(sign_key);
  now += key_block->key_block_size;

  printf("Key block:\n");
  data_key = &key_block->data_key;
  printf("  Size:                %" PRIu64 "\n", key_block->key_block_size);
  printf("  Data key algorithm:  %" PRIu64 " %s\n", data_key->algorithm,
         (data_key->algorithm < kNumAlgorithms ?
          algo_strings[data_key->algorithm] : "(invalid)"));
  printf("  Data key version:    %" PRIu64 "\n", data_key->key_version);
  printf("  Flags:               %" PRIu64 "\n", key_block->key_block_flags);

  rsa = PublicKeyToRSA(&key_block->data_key);
  if (!rsa) {
    error("Error parsing data key.\n");
    return 1;
  }

  /* Verify preamble */
  preamble = (VbKernelPreambleHeader*)(blob + now);
  if (0 != VerifyKernelPreamble2(preamble, blob_size - now, rsa)) {
    error("Error verifying preamble.\n");
    return 1;
  }
  now += preamble->preamble_size;

  printf("Preamble:\n");
  printf("  Size:                %" PRIu64 "\n", preamble->preamble_size);
  printf("  Header version:      %" PRIu32 ".%" PRIu32"\n",
         preamble->header_version_major, preamble->header_version_minor);
  printf("  Kernel version:      %" PRIu64 "\n", preamble->kernel_version);
  printf("  Body load address:   %" PRIu64 "\n", preamble->body_load_address);
  printf("  Body size:           %" PRIu64 "\n",
         preamble->body_signature.data_size);
  printf("  Bootloader address:  %" PRIu64 "\n", preamble->bootloader_address);
  printf("  Bootloader size:     %" PRIu64 "\n", preamble->bootloader_size);

  /* Verify body */
  if (0 != VerifyData(blob + now, &preamble->body_signature, rsa)) {
    error("Error verifying kernel body.\n");
    return 1;
  }
  printf("Body verification succeeded.\n");
  return 0;
}


int main(int argc, char* argv[]) {

  char* filename = NULL;
  char* key_block_file = NULL;
  char* signpubkey = NULL;
  char* signprivate = NULL;
  uint64_t version = 0;
  char* vmlinuz = NULL;
  char* bootloader = NULL;
  char* config_file = NULL;
  uint64_t pad = 65536;
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

      case OPT_VMLINUZ:
        vmlinuz = optarg;
        break;

      case OPT_BOOTLOADER:
        bootloader = optarg;
        break;

      case OPT_CONFIG:
        config_file = optarg;
        break;

      case OPT_VERSION:
        version = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          printf("Invalid --version\n");
          parse_error = 1;
        }
        break;

      case OPT_PAD:
        pad = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          printf("Invalid --pad\n");
          parse_error = 1;
        }
        break;
    }
  }

  if (parse_error)
    return PrintHelp();

  switch(mode) {
    case OPT_MODE_PACK:
      return Pack(filename, key_block_file, signprivate, version, vmlinuz,
                  bootloader, config_file, pad);
    case OPT_MODE_VERIFY:
      return Verify(filename, signpubkey);
    default:
      printf("Must specify a mode.\n");
      return PrintHelp();
  }
}
