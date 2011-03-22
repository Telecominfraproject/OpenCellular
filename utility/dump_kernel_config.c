/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Exports the kernel commandline from a given partition/image.
 */

#include <getopt.h>
#include <inttypes.h>  /* For uint64_t */
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "kernel_blob.h"
#include "utility.h"
#include "vboot_common.h"
#include "vboot_struct.h"

enum {
  OPT_KLOADADDR = 1000,
};

static struct option long_opts[] = {
  { "kloadaddr", 1, 0, OPT_KLOADADDR },
  { NULL, 0, 0, 0 }
};

/* Print help and return error */
static int PrintHelp(void) {
  puts("dump_kernel_config - Prints the kernel command line\n"
       "\n"
       "Usage:  dump_kernel_config [--kloadaddr <ADDRESS>] "
       "<image/blockdevice>\n"
       "\n"
       "");
  return 1;
}

static uint8_t* find_kernel_config(uint8_t* blob, uint64_t blob_size,
    uint64_t kernel_body_load_address) {
  VbKeyBlockHeader* key_block;
  VbKernelPreambleHeader* preamble;
  struct linux_kernel_params *params;
  uint32_t now = 0;
  uint32_t offset = 0;

  /* Skip the key block */
  key_block = (VbKeyBlockHeader*)blob;
  now += key_block->key_block_size;
  if (now + blob > blob + blob_size) {
    error("key_block_size advances past the end of the blob\n");
    return NULL;
  }

  /* Open up the preamble */
  preamble = (VbKernelPreambleHeader*)(blob + now);
  now += preamble->preamble_size;
  if (now + blob > blob + blob_size) {
    error("preamble_size advances past the end of the blob\n");
    return NULL;
  }

  /* The parameters are packed before the bootloader and there is no specific
   * pointer to it so we just walk back by its allocated size. */
  offset = preamble->bootloader_address -
           (kernel_body_load_address + CROS_PARAMS_SIZE) + now;
  if (offset > blob_size) {
    error("params are outside of the memory blob: %x\n", offset);
    return NULL;
  }
  params = (struct linux_kernel_params *)(blob + offset);

  /* Grab the offset to the kernel command line using the supplied pointer. */
  offset = params->cmd_line_ptr - kernel_body_load_address + now;
  if (offset > blob_size) {
    error("cmdline is outside of the memory blob: %x\n", offset);
    return NULL;
  }
  return (uint8_t *)(blob + offset);
}

static void* MapFile(const char *filename, size_t *size) {
  FILE* f;
  uint8_t* buf;
  long file_size = 0;

  f = fopen(filename, "rb");
  if (!f) {
    VBDEBUG(("Unable to open file %s\n", filename));
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  file_size = ftell(f);
  rewind(f);

  if (file_size <= 0) {
    fclose(f);
    return NULL;
  }
  *size = (size_t) file_size;

  /* Uses a host primitive as this is not meant for firmware use. */
  buf = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fileno(f), 0);
  if (buf == MAP_FAILED) {
    error("Failed to mmap the file %s\n", filename);
    fclose(f);
    return NULL;
  }

  fclose(f);
  return buf;
}

int main(int argc, char* argv[]) {
  uint8_t* blob;
  size_t blob_size;
  char* infile = NULL;
  uint8_t *config = NULL;
  uint64_t kernel_body_load_address = CROS_32BIT_ENTRY_ADDR;
  int parse_error = 0;
  char *e;
  int i;

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

      case OPT_KLOADADDR:
        kernel_body_load_address = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          fprintf(stderr, "Invalid --kloadaddr\n");
          parse_error = 1;
        }
        break;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Expected argument after options\n");
    parse_error = 1;
  } else
    infile = argv[optind];

  if (parse_error)
    return PrintHelp();

  if (!infile || !*infile) {
    error("Must specify filename\n");
    return 1;
  }

  /* Map the kernel image blob. */
  blob = MapFile(infile, &blob_size);
  if (!blob) {
    error("Error reading input file\n");
    return 1;
  }

  config = find_kernel_config(blob, (uint64_t)blob_size,
      kernel_body_load_address);
  if (!config) {
    error("Error parsing input file\n");
    munmap(blob, blob_size);
    return 1;
  }

  printf("%.*s", CROS_CONFIG_SIZE, config);
  munmap(blob, blob_size);
  return 0;
}
