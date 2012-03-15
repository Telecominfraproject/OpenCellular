/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot kernel utility
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

static const int DEFAULT_PADDING = 65536;

/* Command line options */
enum {
  OPT_MODE_PACK = 1000,
  OPT_MODE_REPACK,
  OPT_MODE_VERIFY,
  OPT_ARCH,
  OPT_OLDBLOB,
  OPT_KLOADADDR,
  OPT_KEYBLOCK,
  OPT_SIGNPUBKEY,
  OPT_SIGNPRIVATE,
  OPT_VERSION,
  OPT_VMLINUZ,
  OPT_BOOTLOADER,
  OPT_CONFIG,
  OPT_VBLOCKONLY,
  OPT_PAD,
  OPT_VERBOSE,
  OPT_MINVERSION,
};

enum {
  ARCH_ARM,
  ARCH_X86 /* default */
};

static struct option long_opts[] = {
  {"pack", 1, 0,                      OPT_MODE_PACK               },
  {"repack", 1, 0,                    OPT_MODE_REPACK             },
  {"verify", 1, 0,                    OPT_MODE_VERIFY             },
  {"arch", 1, 0,                      OPT_ARCH                    },
  {"oldblob", 1, 0,                   OPT_OLDBLOB                 },
  {"kloadaddr", 1, 0,                 OPT_KLOADADDR               },
  {"keyblock", 1, 0,                  OPT_KEYBLOCK                },
  {"signpubkey", 1, 0,                OPT_SIGNPUBKEY              },
  {"signprivate", 1, 0,               OPT_SIGNPRIVATE             },
  {"version", 1, 0,                   OPT_VERSION                 },
  {"minversion", 1, 0,                OPT_MINVERSION              },
  {"vmlinuz", 1, 0,                   OPT_VMLINUZ                 },
  {"bootloader", 1, 0,                OPT_BOOTLOADER              },
  {"config", 1, 0,                    OPT_CONFIG                  },
  {"vblockonly", 0, 0,                OPT_VBLOCKONLY              },
  {"pad", 1, 0,                       OPT_PAD                     },
  {"verbose", 0, 0,                   OPT_VERBOSE                 },
  {"debug", 0, &opt_debug, 1                                      },
  {NULL, 0, 0, 0}
};


/* Print help and return error */
static int PrintHelp(char *progname) {
  fprintf(stderr,
          "This program creates, signs, and verifies the kernel blob\n");
  fprintf(stderr,
          "\n"
          "Usage:  %s --pack <file> [PARAMETERS]\n"
          "\n"
          "  Required parameters:\n"
          "    --keyblock <file>         Key block in .keyblock format\n"
          "    --signprivate <file>"
          "      Private key to sign kernel data, in .vbprivk format\n"
          "    --version <number>        Kernel version\n"
          "    --vmlinuz <file>          Linux kernel bzImage file\n"
          "    --bootloader <file>       Bootloader stub\n"
          "    --config <file>           Command line file\n"
          "    --arch <arch>             Cpu architecture (default x86)\n"
          "\n"
          "  Optional:\n"
          "    --kloadaddr <address>     Assign kernel body load address\n"
          "    --pad <number>            Verification padding size in bytes\n"
          "    --vblockonly              Emit just the verification blob\n",
          progname);
  fprintf(stderr,
          "\nOR\n\n"
          "Usage:  %s --repack <file> [PARAMETERS]\n"
          "\n"
          "  Required parameters (of --keyblock, --config, and --version \n"
          "    at least one is required):\n"
          "    --keyblock <file>         Key block in .keyblock format\n"
          "    --signprivate <file>"
          "      Private key to sign kernel data, in .vbprivk format\n"
          "    --oldblob <file>          Previously packed kernel blob\n"
          "    --config <file>           New command line file\n"
          "    --version <number>        Kernel version\n"
          "\n"
          "  Optional:\n"
          "    --kloadaddr <address>     Assign kernel body load address\n"
          "    --pad <number>            Verification padding size in bytes\n"
          "    --vblockonly              Emit just the verification blob\n",
          progname);
  fprintf(stderr,
          "\nOR\n\n"
          "Usage:  %s --verify <file> [PARAMETERS]\n"
          "\n"
          "  Optional:\n"
          "    --signpubkey <file>"
          "       Public key to verify kernel keyblock, in .vbpubk format\n"
          "    --verbose                 Print a more detailed report\n"
          "    --keyblock <file>"
          "         Outputs the verified key block, in .keyblock format\n"
          "    --kloadaddr <address>     Assign kernel body load address\n"
          "    --pad <number>            Verification padding size in bytes\n"
          "    --minversion <number>     Minimum combined kernel key version\n"
          "                              and kernel version\n"
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


/* Return an explanation when fread() fails. */
static const char *error_fread(FILE *fp) {
  const char *retval = "beats me why";
  if (feof(fp))
    retval = "EOF";
  else if (ferror(fp))
    retval = strerror(errno);
  clearerr(fp);
  return retval;
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




typedef struct blob_s {
  /* Stuff needed by VbKernelPreambleHeader */
  uint64_t kernel_version;
  uint64_t bootloader_address;          /* in RAM, after loading from disk */
  uint64_t bootloader_size;
  /* Raw kernel blob data */
  uint64_t kern_blob_size;
  uint8_t *kern_blob;

  /* These fields are not always initialized. When they are, they point to the
   * verification block as it's found on-disk. See
   * http://www.chromium.org/chromium-os/chromiumos-design-docs/disk-format */
  uint8_t *vblock_buf;                  /* typically includes padding */
  VbKeyBlockHeader* key_block;          /* within vblock_buf, don't free it */
  VbKernelPreambleHeader* preamble;     /* ditto */
} blob_t;

/* Given a blob return the location of the kernel command line buffer. */
static char* BpCmdLineLocation(blob_t *bp, uint64_t kernel_body_load_address)
{
  return (char*)(bp->kern_blob +
                 bp->bootloader_address - kernel_body_load_address -
                 CROS_CONFIG_SIZE - CROS_PARAMS_SIZE);
}

static void FreeBlob(blob_t *bp) {
  if (bp) {
    if (bp->kern_blob)
      free(bp->kern_blob);
    if (bp->vblock_buf)
      free(bp->vblock_buf);
    free(bp);
  }
}

/*
 * Read the kernel command line from a file. Get rid of \n characters along
 * the way and verify that the line fits into a 4K buffer.
 *
 * Return the buffer contaning the line on success (and set the line length
 * using the passed in parameter), or NULL in case something goes wrong.
 */
static uint8_t* ReadConfigFile(const char* config_file, uint64_t* config_size)
{
  uint8_t* config_buf;
  int ii;

  config_buf = ReadFile(config_file, config_size);
  Debug(" config file size=0x%" PRIx64 "\n", *config_size);
  if (CROS_CONFIG_SIZE <= *config_size) {  /* need room for trailing '\0' */
    VbExError("Config file %s is too large (>= %d bytes)\n",
              config_file, CROS_CONFIG_SIZE);
    return NULL;
  }

  /* Replace newlines with spaces */
  for (ii = 0; ii < *config_size; ii++) {
    if ('\n' == config_buf[ii]) {
      config_buf[ii] = ' ';
    }
  }
  return config_buf;
}

/* Create a blob from its components */
static blob_t *NewBlob(uint64_t version,
                       const char* vmlinuz,
                       const char* bootloader_file,
                       const char* config_file,
                       int arch,
                       uint64_t kernel_body_load_address) {
  blob_t* bp;
  struct linux_kernel_header* lh = 0;
  struct linux_kernel_params* params = 0;
  uint8_t* config_buf;
  uint64_t config_size;
  uint8_t* bootloader_buf;
  uint64_t bootloader_size;
  uint8_t* kernel_buf;
  uint64_t kernel_size;
  uint64_t kernel32_start = 0;
  uint64_t kernel32_size = 0;
  uint32_t cmdline_addr;
  uint8_t* kern_blob = NULL;
  uint64_t now = 0;

  if (!vmlinuz || !bootloader_file || !config_file) {
    VbExError("Must specify all input files\n");
    return 0;
  }

  bp = (blob_t *)malloc(sizeof(blob_t));
  if (!bp) {
    VbExError("Couldn't allocate bytes for blob_t.\n");
    return 0;
  }

  Memset(bp, 0, sizeof(*bp));
  bp->kernel_version = version;

  /* Read the config file */
  Debug("Reading %s\n", config_file);
  config_buf = ReadConfigFile(config_file, &config_size);
  if (!config_buf)
    return 0;

  /* Read the bootloader */
  Debug("Reading %s\n", bootloader_file);
  bootloader_buf = ReadFile(bootloader_file, &bootloader_size);
  if (!bootloader_buf)
    return 0;
  Debug(" bootloader file size=0x%" PRIx64 "\n", bootloader_size);

  /* Read the kernel */
  Debug("Reading %s\n", vmlinuz);
  kernel_buf = ReadFile(vmlinuz, &kernel_size);
  if (!kernel_buf)
    return 0;
  Debug(" kernel file size=0x%" PRIx64 "\n", kernel_size);
  if (!kernel_size) {
    VbExError("Empty kernel file\n");
    return 0;
  }

  if (arch == ARCH_X86) {
    /* The first part of vmlinuz is a header, followed by a real-mode
     * boot stub.  We only want the 32-bit part. */
    lh = (struct linux_kernel_header *)kernel_buf;
    kernel32_start = (lh->setup_sects + 1) << 9;
    if (kernel32_start >= kernel_size) {
      VbExError("Malformed kernel\n");
      return 0;
    }
  } else
    kernel32_start = 0;
  kernel32_size = kernel_size - kernel32_start;
  Debug(" kernel32_start=0x%" PRIx64 "\n", kernel32_start);
  Debug(" kernel32_size=0x%" PRIx64 "\n", kernel32_size);

  /* Allocate and zero the space we need for the kernel blob. */
  bp->kern_blob_size = roundup(kernel32_size, CROS_ALIGN) +
      CROS_CONFIG_SIZE +
      CROS_PARAMS_SIZE +
      roundup(bootloader_size, CROS_ALIGN);
  Debug("kern_blob_size=0x%" PRIx64 "\n", bp->kern_blob_size);
  kern_blob = (uint8_t *)malloc(bp->kern_blob_size);
  if (!kern_blob) {
    VbExError("Couldn't allocate %ld bytes.\n", bp->kern_blob_size);
    return 0;
  }
  Memset(kern_blob, 0, bp->kern_blob_size);
  bp->kern_blob = kern_blob;

  /* Copy the 32-bit kernel. */
  Debug("kernel goes at kern_blob+0x%" PRIx64 "\n", now);
  if (kernel32_size)
    Memcpy(kern_blob + now, kernel_buf + kernel32_start, kernel32_size);
  now += roundup(now + kernel32_size, CROS_ALIGN);

  Debug("config goes at kern_blob+0x%" PRIx64 "\n", now);
  /* Find the load address of the commandline. We'll need it later. */
  cmdline_addr = kernel_body_load_address + now +
      find_cmdline_start((char *)config_buf, config_size);
  Debug(" cmdline_addr=0x%" PRIx64 "\n", cmdline_addr);

  /* Copy the config. */
  if (config_size)
    Memcpy(kern_blob + now, config_buf, config_size);
  now += CROS_CONFIG_SIZE;

  /* The zeropage data is next. Overlay the linux_kernel_header onto it, and
   * tweak a few fields. */
  Debug("params goes at kern_blob+0x%" PRIx64 "\n", now);
  params = (struct linux_kernel_params *)(kern_blob + now);
  if (arch == ARCH_X86)
    Memcpy(&(params->setup_sects), &(lh->setup_sects),
           sizeof(*lh) - offsetof(struct linux_kernel_header, setup_sects));
  else
    Memset(&(params->setup_sects), 0,
           sizeof(*lh) - offsetof(struct linux_kernel_header, setup_sects));
  params->boot_flag = 0;
  params->ramdisk_image = 0;             /* we don't support initrd */
  params->ramdisk_size = 0;
  params->type_of_loader = 0xff;
  params->cmd_line_ptr = cmdline_addr;
  /* A fake e820 memory map with 2 entries */
  params->n_e820_entry = 2;
  params->e820_entries[0].start_addr = 0x00000000;
  params->e820_entries[0].segment_size = 0x00001000;
  params->e820_entries[0].segment_type = E820_TYPE_RAM;
  params->e820_entries[1].start_addr = 0xfffff000;
  params->e820_entries[1].segment_size = 0x00001000;
  params->e820_entries[1].segment_type = E820_TYPE_RESERVED;
  now += CROS_PARAMS_SIZE;

  /* Finally, append the bootloader. Remember where it will load in
   * memory, too. */
  Debug("bootloader goes at kern_blob+0x%" PRIx64 "\n", now);
  bp->bootloader_address = kernel_body_load_address + now;
  bp->bootloader_size = roundup(bootloader_size, CROS_ALIGN);
  Debug(" bootloader_address=0x%" PRIx64 "\n", bp->bootloader_address);
  Debug(" bootloader_size=0x%" PRIx64 "\n", bp->bootloader_size);
  if (bootloader_size)
    Memcpy(kern_blob + now, bootloader_buf, bootloader_size);
  now += bp->bootloader_size;
  Debug("end of kern_blob at kern_blob+0x%" PRIx64 "\n", now);

  /* Free input buffers */
  free(kernel_buf);
  free(config_buf);
  free(bootloader_buf);

  /* Success */
  return bp;
}


/* Pull the blob_t stuff out of a prepacked kernel blob file */
static blob_t *OldBlob(const char* filename, uint64_t pad) {
  FILE* fp = NULL;
  blob_t *bp = NULL;
  struct stat statbuf;
  VbKeyBlockHeader* key_block;
  VbKernelPreambleHeader* preamble;
  uint64_t now = 0;
  uint8_t* buf = NULL;
  int ret_error = 1;

  if (!filename) {
    VbExError("Must specify prepacked blob to read\n");
    return 0;
  }

  if (0 != stat(filename, &statbuf)) {
    VbExError("Unable to stat %s: %s\n", filename, strerror(errno));
    return 0;
  }

  Debug("%s size is 0x%" PRIx64 "\n", filename, statbuf.st_size);
  if (statbuf.st_size < pad) {
    VbExError("%s is too small to be a valid kernel blob\n");
    return 0;
  }

  Debug("Reading %s\n", filename);
  fp = fopen(filename, "rb");
  if (!fp) {
    VbExError("Unable to open file %s: %s\n", filename, strerror(errno));
    return 0;
  }

  buf = malloc(pad);
  if (!buf) {
    VbExError("Unable to allocate padding\n");
    goto unwind_oldblob;
  }

  if (1 != fread(buf, pad, 1, fp)) {
    VbExError("Unable to read header from %s: %s\n", filename, error_fread(fp));
    goto unwind_oldblob;
  }

  /* Skip the key block */
  key_block = (VbKeyBlockHeader*)buf;
  Debug("Keyblock is 0x%" PRIx64 " bytes\n", key_block->key_block_size);
  now += key_block->key_block_size;
  if (now > statbuf.st_size) {
    VbExError("key_block_size advances past the end of the blob\n");
    goto unwind_oldblob;
  }
  if (now > pad) {
    VbExError("key_block_size advances past %" PRIu64 " byte padding\n", pad);
    goto unwind_oldblob;
  }

  /* Skip the preamble */
  preamble = (VbKernelPreambleHeader*)(buf + now);
  Debug("Preamble is 0x%" PRIx64 " bytes\n", preamble->preamble_size);
  now += preamble->preamble_size;
  if (now > statbuf.st_size) {
    VbExError("preamble_size advances past the end of the blob\n");
    goto unwind_oldblob;
  }
  if (now > pad) {
    VbExError("preamble_size advances past %" PRIu64 " byte padding\n", pad);
    goto unwind_oldblob;
  }

  /* Go find the kernel blob */
  Debug("kernel blob is at offset 0x%" PRIx64 "\n", now);
  if (0 != fseek(fp, now, SEEK_SET)) {
    VbExError("Unable to seek to 0x%" PRIx64 " in %s: %s\n", now, filename,
          strerror(errno));
    goto unwind_oldblob;
  }

  /* Remember what we've got */
  bp = (blob_t *)malloc(sizeof(blob_t));
  if (!bp) {
    VbExError("Couldn't allocate bytes for blob_t.\n");
    goto unwind_oldblob;
  }

  bp->vblock_buf = buf;
  bp->key_block = key_block;
  bp->preamble = preamble;

  bp->kernel_version = preamble->kernel_version;
  bp->bootloader_address = preamble->bootloader_address;
  bp->bootloader_size = preamble->bootloader_size;
  bp->kern_blob_size = preamble->body_signature.data_size;

  Debug(" kernel_version = %d\n", bp->kernel_version);
  Debug(" bootloader_address = 0x%" PRIx64 "\n", bp->bootloader_address);
  Debug(" bootloader_size = 0x%" PRIx64 "\n", bp->bootloader_size);
  Debug(" kern_blob_size = 0x%" PRIx64 "\n", bp->kern_blob_size);

  if (!bp->kern_blob_size) {
    VbExError("No kernel blob found\n");
    goto unwind_oldblob;
  }

  bp->kern_blob = (uint8_t *)malloc(bp->kern_blob_size);
  if (!bp->kern_blob) {
    VbExError("Couldn't allocate 0x%" PRIx64 " bytes for blob_t.\n",
              bp->kern_blob_size);
    goto unwind_oldblob;
  }

  /* read it in */
  if (1 != fread(bp->kern_blob, bp->kern_blob_size, 1, fp)) {
    VbExError("Unable to read kernel blob from %s: %s\n", filename,
              error_fread(fp));
    goto unwind_oldblob;
  }

  ret_error = 0;

  /* done */
unwind_oldblob:
  fclose(fp);
  if (ret_error) {
    if (bp) {
      FreeBlob(bp);
      bp = NULL;
    } else if (buf) {
      free(buf);
    }
  }
  return bp;
}


/* Pack a .kernel */
static int Pack(const char* outfile, const char* keyblock_file,
                const char* signprivate, blob_t *bp, uint64_t pad,
                int vblockonly,
                uint64_t kernel_body_load_address) {
  VbPrivateKey* signing_key;
  VbSignature* body_sig;
  VbKernelPreambleHeader* preamble;
  VbKeyBlockHeader* key_block;
  uint64_t key_block_size;
  FILE* f;
  uint64_t i;
  uint64_t written = 0;

  if (!outfile) {
    VbExError("Must specify output filename\n");
    return 1;
  }
  if ((!keyblock_file && !bp->key_block) || !signprivate) {
    VbExError("Must specify all keys\n");
    return 1;
  }
  if (!bp) {
    VbExError("Refusing to pack invalid kernel blob\n");
    return 1;
  }

  /* Get the key block and read the private key. */
  if (keyblock_file) {
    key_block = (VbKeyBlockHeader*)ReadFile(keyblock_file, &key_block_size);
    if (!key_block) {
      VbExError("Error reading key block.\n");
      return 1;
    }
  } else {
    key_block = bp->key_block;
    key_block_size = key_block->key_block_size;
  }

  if (pad < key_block->key_block_size) {
    VbExError("Pad too small\n");
    return 1;
  }

  signing_key = PrivateKeyRead(signprivate);
  if (!signing_key) {
    VbExError("Error reading signing key.\n");
    return 1;
  }

  /* Sign the kernel data */
  body_sig = CalculateSignature(bp->kern_blob, bp->kern_blob_size, signing_key);
  if (!body_sig) {
    VbExError("Error calculating body signature\n");
    return 1;
  }

  /* Create preamble */
  preamble = CreateKernelPreamble(bp->kernel_version,
                                  kernel_body_load_address,
                                  bp->bootloader_address,
                                  bp->bootloader_size,
                                  body_sig,
                                  pad - key_block_size,
                                  signing_key);
  if (!preamble) {
    VbExError("Error creating preamble.\n");
    return 1;
  }

  /* Write the output file */
  Debug("writing %s...\n", outfile);
  f = fopen(outfile, "wb");
  if (!f) {
    VbExError("Can't open output file %s\n", outfile);
    return 1;
  }
  Debug("0x%" PRIx64 " bytes of key_block\n", key_block_size);
  Debug("0x%" PRIx64 " bytes of preamble\n", preamble->preamble_size);
  i = ((1 != fwrite(key_block, key_block_size, 1, f)) ||
       (1 != fwrite(preamble, preamble->preamble_size, 1, f)));
  if (i) {
    VbExError("Can't write output file %s\n", outfile);
    fclose(f);
    unlink(outfile);
    return 1;
  }
  written += key_block_size;
  written += preamble->preamble_size;

  if (!vblockonly) {
    Debug("0x%" PRIx64 " bytes of kern_blob\n", bp->kern_blob_size);
    i = (1 != fwrite(bp->kern_blob, bp->kern_blob_size, 1, f));
    if (i) {
      VbExError("Can't write output file %s\n", outfile);
      fclose(f);
      unlink(outfile);
      return 1;
    }
    written += bp->kern_blob_size;
  }
  Debug("0x%" PRIx64 " bytes total\n", written);
  fclose(f);

  /* Success */
  return 0;
}

/*
 * Replace kernel command line in a blob representing a kernel.
 */
static int ReplaceConfig(blob_t* bp, const char* config_file,
    uint64_t kernel_body_load_address)
{
  uint8_t* new_conf;
  uint64_t config_size;

  if (!config_file) {
    return 0;
  }

  new_conf = ReadConfigFile(config_file, &config_size);
  if (!new_conf) {
    return 1;
  }

  /* fill the config buffer with zeros */
  Memset(BpCmdLineLocation(bp, kernel_body_load_address), 0, CROS_CONFIG_SIZE);
  Memcpy(BpCmdLineLocation(bp, kernel_body_load_address),
      new_conf, config_size);
  free(new_conf);
  return 0;
}

static int Verify(const char* infile, const char* signpubkey, int verbose,
                  const char* key_block_file,
                  uint64_t kernel_body_load_address, uint64_t min_version,
                  uint64_t pad) {

  VbKeyBlockHeader* key_block;
  VbKernelPreambleHeader* preamble;
  VbPublicKey* data_key;
  VbPublicKey* sign_key = NULL;
  RSAPublicKey* rsa;
  blob_t* bp;
  uint64_t now;
  int rv = 1;

  if (!infile) {
    VbExError("Must specify filename\n");
    return 1;
  }

  /* Read public signing key */
  if (signpubkey) {
    sign_key = PublicKeyRead(signpubkey);
    if (!sign_key) {
      VbExError("Error reading signpubkey.\n");
      return 1;
    }
  }

  /* Read blob */
  bp = OldBlob(infile, pad);
  if (!bp) {
    VbExError("Error reading input file\n");
    return 1;
  }

  /* Verify key block */
  key_block = bp->key_block;
  if (0 != KeyBlockVerify(key_block, key_block->key_block_size, sign_key,
                          (sign_key ? 0 : 1))) {
    VbExError("Error verifying key block.\n");
    goto verify_exit;
  }
  now = key_block->key_block_size;

  if (key_block_file) {
    FILE* f = NULL;
    f = fopen(key_block_file, "wb");
    if (!f) {
      VbExError("Can't open key block file %s\n", key_block_file);
      return 1;
    }
    if (1 != fwrite(key_block, key_block->key_block_size, 1, f)) {
      VbExError("Can't write key block file %s\n", key_block_file);
      return 1;
    }
    fclose(f);
  }

  printf("Key block:\n");
  data_key = &key_block->data_key;
  if (verbose)
    printf("  Signature:           %s\n", sign_key ? "valid" : "ignored");
  printf("  Size:                0x%" PRIx64 "\n", key_block->key_block_size);
  printf("  Flags:               %" PRIu64 " ", key_block->key_block_flags);
  if (key_block->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_0)
    printf(" !DEV");
  if (key_block->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_1)
    printf(" DEV");
  if (key_block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_0)
    printf(" !REC");
  if (key_block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_1)
    printf(" REC");
  printf("\n");
  printf("  Data key algorithm:  %" PRIu64 " %s\n", data_key->algorithm,
         (data_key->algorithm < kNumAlgorithms ?
          algo_strings[data_key->algorithm] : "(invalid)"));
  printf("  Data key version:    %" PRIu64 "\n", data_key->key_version);
  printf("  Data key sha1sum:    ");
  PrintPubKeySha1Sum(data_key);
  printf("\n");

  if (data_key->key_version < (min_version >> 16)) {
    VbExError("Data key version %" PRIu64
              " is lower than minimum %" PRIu64".\n",
              data_key->key_version, (min_version >> 16));
    goto verify_exit;
  }

  rsa = PublicKeyToRSA(&key_block->data_key);
  if (!rsa) {
    VbExError("Error parsing data key.\n");
    goto verify_exit;
  }

  /* Verify preamble */
  preamble = bp->preamble;
  if (0 != VerifyKernelPreamble(preamble, preamble->preamble_size, rsa)) {
    VbExError("Error verifying preamble.\n");
    goto verify_exit;
  }
  now += preamble->preamble_size;

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

  if (preamble->kernel_version < (min_version & 0xFFFF)) {
    VbExError("Kernel version %" PRIu64 " is lower than minimum %" PRIu64 ".\n",
              preamble->kernel_version, (min_version & 0xFFFF));
    goto verify_exit;
  }

  /* Verify body */
  if (0 != VerifyData(bp->kern_blob, bp->kern_blob_size,
                      &preamble->body_signature, rsa)) {
    VbExError("Error verifying kernel body.\n");
    goto verify_exit;
  }
  printf("Body verification succeeded.\n");

  rv = 0;

  if (!verbose) {
    goto verify_exit;
  }

  printf("Config:\n%s\n", BpCmdLineLocation(bp, kernel_body_load_address));

verify_exit:
  FreeBlob(bp);
  return rv;
}


int main(int argc, char* argv[]) {
  char* filename = NULL;
  char* oldfile = NULL;
  char* key_block_file = NULL;
  char* signpubkey = NULL;
  char* signprivate = NULL;
  int version = -1;
  char* vmlinuz = NULL;
  char* bootloader = NULL;
  char* config_file = NULL;
  int arch = ARCH_X86;
  int vblockonly = 0;
  int verbose = 0;
  uint64_t kernel_body_load_address = CROS_32BIT_ENTRY_ADDR;
  uint64_t pad = DEFAULT_PADDING;
  int mode = 0;
  int parse_error = 0;
  uint64_t min_version = 0;
  char* e;
  int i,r;
  blob_t *bp;


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

      case OPT_MODE_PACK:
      case OPT_MODE_REPACK:
      case OPT_MODE_VERIFY:
        if (mode && (mode != i)) {
          fprintf(stderr, "Only single mode can be specified\n");
          parse_error = 1;
          break;
        }
        mode = i;
        filename = optarg;
        break;

      case OPT_ARCH:
        /* check the first 3 characters to also detect x86_64 */
        if ((!strncasecmp(optarg, "x86", 3)) ||
            (!strcasecmp(optarg, "amd64")))
          arch = ARCH_X86;
        else if (!strcasecmp(optarg, "arm"))
          arch = ARCH_ARM;
        else {
          fprintf(stderr, "Unknown architecture string: %s\n", optarg);
          parse_error = 1;
        }
        break;

      case OPT_OLDBLOB:
        oldfile = optarg;
        break;

      case OPT_KLOADADDR:
        kernel_body_load_address = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          fprintf(stderr, "Invalid --kloadaddr\n");
          parse_error = 1;
        }
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

      case OPT_VBLOCKONLY:
        vblockonly = 1;
        break;

      case OPT_VERSION:
        version = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          fprintf(stderr, "Invalid --version\n");
          parse_error = 1;
        }
        break;

      case OPT_MINVERSION:
        min_version = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          fprintf(stderr, "Invalid --minversion\n");
          parse_error = 1;
        }
        break;

      case OPT_PAD:
        pad = strtoul(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          fprintf(stderr, "Invalid --pad\n");
          parse_error = 1;
        }
        break;

      case OPT_VERBOSE:
        verbose = 1;
        break;
    }
  }

  if (parse_error)
    return PrintHelp(progname);

  switch(mode) {
    case OPT_MODE_PACK:
      bp = NewBlob(version, vmlinuz, bootloader, config_file, arch,
          kernel_body_load_address);
      if (!bp)
        return 1;
      r = Pack(filename, key_block_file, signprivate, bp, pad, vblockonly,
          kernel_body_load_address);
      FreeBlob(bp);
      return r;

    case OPT_MODE_REPACK:
      if (!config_file && !key_block_file && (version<0)) {
        fprintf(stderr,
                "You must supply at least one of "
                "--config, --keyblock or --version\n");
        return 1;
      }

      bp = OldBlob(oldfile, pad);
      if (!bp)
        return 1;
      r = ReplaceConfig(bp, config_file, kernel_body_load_address);
      if (!r) {
        if (version >= 0) {
          bp->kernel_version = (uint64_t) version;
        }
        r = Pack(filename, key_block_file, signprivate, bp, pad, vblockonly,
            kernel_body_load_address);
      }
      FreeBlob(bp);
      return r;

    case OPT_MODE_VERIFY:
      return Verify(filename, signpubkey, verbose, key_block_file,
                    kernel_body_load_address, min_version, pad);

    default:
      fprintf(stderr,
              "You must specify a mode: --pack, --repack or --verify\n");
      return PrintHelp(progname);
  }
}
