/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
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

/* Global opts */
static int opt_debug = 0;
static int opt_verbose = 0;
static int opt_vblockonly = 0;
static uint64_t opt_pad = 65536;


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

typedef enum {
  ARCH_ARM,
  ARCH_X86 /* default */
} arch_t;

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
  {"verbose", 0, &opt_verbose, 1                                  },
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
          "    --signprivate <file>      Private key to sign kernel data,\n"
          "                                in .vbprivk format\n"
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
          "  Required parameters:\n"
          "    --signprivate <file>      Private key to sign kernel data,\n"
          "                                in .vbprivk format\n"
          "    --oldblob <file>          Previously packed kernel blob\n"
          "                                (including verfication blob)\n"
          "\n"
          "  Optional:\n"
          "    --keyblock <file>         Key block in .keyblock format\n"
          "    --config <file>           New command line file\n"
          "    --version <number>        Kernel version\n"
          "    --kloadaddr <address>     Assign kernel body load address\n"
          "    --pad <number>            Verification blob size in bytes\n"
          "    --vblockonly              Emit just the verification blob\n",
          progname);
  fprintf(stderr,
          "\nOR\n\n"
          "Usage:  %s --verify <file> [PARAMETERS]\n"
          "\n"
          "  Optional:\n"
          "    --signpubkey <file>"
          "       Public key to verify kernel keyblock,\n"
          "                                in .vbpubk format\n"
          "    --verbose                 Print a more detailed report\n"
          "    --keyblock <file>         Outputs the verified key block,\n"
          "                                in .keyblock format\n"
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

static void Fatal(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(1);
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


/****************************************************************************/
/* Here are globals containing all the bits & pieces I'm working on. */

/* The individual parts that go into the kernel blob */
uint8_t *g_kernel_data;
uint64_t g_kernel_size;
uint8_t *g_param_data;
uint64_t g_param_size;
uint8_t *g_config_data;
uint64_t g_config_size;
uint8_t *g_bootloader_data;
uint64_t g_bootloader_size;
uint64_t g_bootloader_address;


/* The individual parts of the verification blob (including the data that
 * immediately follows the headers) */
VbKeyBlockHeader* g_keyblock;
VbKernelPreambleHeader* g_preamble;

/****************************************************************************/

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


/* Offset of kernel command line string from start of packed kernel blob */
static uint64_t CmdLineOffset(VbKernelPreambleHeader *preamble) {
  return preamble->bootloader_address - preamble->body_load_address -
    CROS_CONFIG_SIZE - CROS_PARAMS_SIZE;
}

/* This initializes g_vmlinuz and g_param from a standard vmlinuz file.
 * It returns 0 on error. */
static int ImportVmlinuzFile(const char *vmlinuz_file, arch_t arch,
                             uint64_t kernel_body_load_address) {
  uint8_t *kernel_buf;
  uint64_t kernel_size;
  uint64_t kernel32_start = 0;
  uint64_t kernel32_size = 0;
  struct linux_kernel_header* lh = 0;
  struct linux_kernel_params* params = 0;

  /* Read the kernel */
  Debug("Reading %s\n", vmlinuz_file);
  kernel_buf = ReadFile(vmlinuz_file, &kernel_size);
  if (!kernel_buf)
    return 0;
  Debug(" kernel file size=0x%" PRIx64 "\n", kernel_size);
  if (!kernel_size)
    Fatal("Empty kernel file\n");

  /* Go ahead and allocate the param region anyway. I don't think we need it
   * for non-x86, but let's keep it for now. */
  g_param_size = CROS_PARAMS_SIZE;
  g_param_data= VbExMalloc(g_param_size);
  Memset(g_param_data, 0, g_param_size);

  /* Unless we're handling x86, the kernel is the kernel, so we're done. */
  if (arch != ARCH_X86) {
    g_kernel_data = kernel_buf;
    g_kernel_size = kernel_size;
    return 1;
  }

  /* The first part of the x86 vmlinuz is a header, followed by a real-mode
   * boot stub.  We only want the 32-bit part. */
  lh = (struct linux_kernel_header *)kernel_buf;
  kernel32_start = (lh->setup_sects + 1) << 9;
  if (kernel32_start >= kernel_size)
    Fatal("Malformed kernel\n");
  kernel32_size = kernel_size - kernel32_start;

  Debug(" kernel32_start=0x%" PRIx64 "\n", kernel32_start);
  Debug(" kernel32_size=0x%" PRIx64 "\n", kernel32_size);

  /* Keep just the 32-bit kernel. */
  if (kernel32_size) {
    g_kernel_size = kernel32_size;
    g_kernel_data = VbExMalloc(g_kernel_size);
    Memcpy(g_kernel_data, kernel_buf + kernel32_start, kernel32_size);
  }

  /* Copy the original zeropage data from kernel_buf into g_param_data, then
   * tweak a few fields for our purposes */
  params = (struct linux_kernel_params *)(g_param_data);
  Memcpy(&(params->setup_sects), &(lh->setup_sects),
         sizeof(*lh) - offsetof(struct linux_kernel_header, setup_sects));
  params->boot_flag = 0;
  params->ramdisk_image = 0; /* we don't support initrd */
  params->ramdisk_size = 0;
  params->type_of_loader = 0xff;
  /* We need to point to the kernel commandline arg. On disk, it will come
   *  right after the 32-bit part of the kernel. */
  params->cmd_line_ptr = kernel_body_load_address +
    roundup(kernel32_size, CROS_ALIGN) +
    find_cmdline_start((char *)g_config_data, g_config_size);
  Debug(" cmdline_addr=0x%x\n", params->cmd_line_ptr);
  /* A fake e820 memory map with 2 entries */
  params->n_e820_entry = 2;
  params->e820_entries[0].start_addr = 0x00000000;
  params->e820_entries[0].segment_size = 0x00001000;
  params->e820_entries[0].segment_type = E820_TYPE_RAM;
  params->e820_entries[1].start_addr = 0xfffff000;
  params->e820_entries[1].segment_size = 0x00001000;
  params->e820_entries[1].segment_type = E820_TYPE_RESERVED;

  /* done */
  free(kernel_buf);
  return 1;
}

/* This returns just the kernel blob, with the verification blob separated
 * and copied to new memory in g_keyblock and g_preamble. */
static uint8_t* ReadOldBlobFromFileOrDie(const char *filename,
                                         uint64_t* size_ptr) {
  FILE* fp = NULL;
  struct stat statbuf;
  VbKeyBlockHeader* key_block;
  VbKernelPreambleHeader* preamble;
  uint64_t now = 0;
  uint8_t* buf;
  uint8_t* kernel_blob_data;
  uint64_t kernel_blob_size;

  if (0 != stat(filename, &statbuf))
    Fatal("Unable to stat %s: %s\n", filename, strerror(errno));

  Debug("%s size is 0x%" PRIx64 "\n", filename, statbuf.st_size);
  if (statbuf.st_size < opt_pad)
    Fatal("%s is too small to be a valid kernel blob\n");

  Debug("Reading %s\n", filename);
  fp = fopen(filename, "rb");
  if (!fp)
    Fatal("Unable to open file %s: %s\n", filename, strerror(errno));

  buf = VbExMalloc(opt_pad);
  if (1 != fread(buf, opt_pad, 1, fp))
    Fatal("Unable to read header from %s: %s\n", filename, error_fread(fp));

  /* Sanity-check the key_block */
  key_block = (VbKeyBlockHeader*)buf;
  Debug("Keyblock is 0x%" PRIx64 " bytes\n", key_block->key_block_size);
  now += key_block->key_block_size;
  if (now > statbuf.st_size)
    Fatal("key_block_size advances past the end of the blob\n");
  if (now > opt_pad)
    Fatal("key_block_size advances past %" PRIu64 " byte padding\n",
              opt_pad);
  /* LGTM */
  g_keyblock = (VbKeyBlockHeader*)VbExMalloc(key_block->key_block_size);
  Memcpy(g_keyblock, key_block, key_block->key_block_size);

  /* And the preamble */
  preamble = (VbKernelPreambleHeader*)(buf + now);
  Debug("Preamble is 0x%" PRIx64 " bytes\n", preamble->preamble_size);
  now += preamble->preamble_size;
  if (now > statbuf.st_size)
    Fatal("preamble_size advances past the end of the blob\n");
  if (now > opt_pad)
    Fatal("preamble_size advances past %" PRIu64 " byte padding\n",
              opt_pad);
  /* LGTM */
  Debug(" kernel_version = %d\n", preamble->kernel_version);
  Debug(" bootloader_address = 0x%" PRIx64 "\n", preamble->bootloader_address);
  Debug(" bootloader_size = 0x%" PRIx64 "\n", preamble->bootloader_size);
  Debug(" kern_blob_size = 0x%" PRIx64 "\n",
        preamble->body_signature.data_size);
  g_preamble = (VbKernelPreambleHeader*)VbExMalloc(preamble->preamble_size);
  Memcpy(g_preamble, preamble, preamble->preamble_size);

  /* Now for the kernel blob */
  Debug("kernel blob is at offset 0x%" PRIx64 "\n", now);
  if (0 != fseek(fp, now, SEEK_SET))
    Fatal("Unable to seek to 0x%" PRIx64 " in %s: %s\n", now, filename,
          strerror(errno));

  /* Sanity check */
  kernel_blob_size = statbuf.st_size - now;
  if (!kernel_blob_size)
    Fatal("No kernel blob found\n");
  if (kernel_blob_size < preamble->body_signature.data_size)
    fprintf(stderr, "Warning: kernel file only has 0x%" PRIx64 " bytes\n",
      kernel_blob_size);
  kernel_blob_data = VbExMalloc(kernel_blob_size);

  /* Read it in */
  if (1 != fread(kernel_blob_data, kernel_blob_size, 1, fp))
    Fatal("Unable to read kernel blob from %s: %s\n", filename,
              error_fread(fp));

  /* Done */
  VbExFree(buf);

  if (size_ptr)
    *size_ptr = kernel_blob_size;

  return kernel_blob_data;
}


/* Split a kernel blob into separate g_kernel, g_param, g_config, and
 * g_bootloader parts. */
static void UnpackKernelBlob(uint8_t *kernel_blob_data,
                            uint64_t kernel_blob_size) {

  uint64_t k_blob_size = g_preamble->body_signature.data_size;
  uint64_t k_blob_ofs = 0;
  uint64_t b_size = g_preamble->bootloader_size;
  uint64_t b_ofs = k_blob_ofs + g_preamble->bootloader_address -
    g_preamble->body_load_address;
  uint64_t p_ofs = b_ofs - CROS_CONFIG_SIZE;
  uint64_t c_ofs = p_ofs - CROS_PARAMS_SIZE;

  Debug("k_blob_size    = 0x%" PRIx64 "\n", k_blob_size   );
  Debug("k_blob_ofs     = 0x%" PRIx64 "\n", k_blob_ofs    );
  Debug("b_size         = 0x%" PRIx64 "\n", b_size        );
  Debug("b_ofs          = 0x%" PRIx64 "\n", b_ofs         );
  Debug("p_ofs          = 0x%" PRIx64 "\n", p_ofs         );
  Debug("c_ofs          = 0x%" PRIx64 "\n", c_ofs         );

  g_kernel_size = c_ofs;
  g_kernel_data = VbExMalloc(g_kernel_size);
  Memcpy(g_kernel_data, kernel_blob_data, g_kernel_size);

  g_param_size = CROS_PARAMS_SIZE;
  g_param_data = VbExMalloc(g_param_size);
  Memcpy(g_param_data, kernel_blob_data + p_ofs, g_param_size);

  g_config_size = CROS_CONFIG_SIZE;
  g_config_data = VbExMalloc(g_config_size);
  Memcpy(g_config_data, kernel_blob_data + c_ofs, g_config_size);

  g_bootloader_size = b_size;
  g_bootloader_data = VbExMalloc(g_bootloader_size);
  Memcpy(g_bootloader_data, kernel_blob_data + b_ofs, g_bootloader_size);
}



/****************************************************************************/

static uint8_t* CreateKernelBlob(uint64_t kernel_body_load_address,
                                 arch_t arch,
                                 uint64_t *size_ptr) {
  uint8_t *kern_blob;
  uint64_t kern_blob_size;
  uint64_t now;
  uint64_t bootloader_size = roundup(g_bootloader_size, CROS_ALIGN);

  /* Put the kernel blob together */
  kern_blob_size = roundup(g_kernel_size, CROS_ALIGN) +
    CROS_CONFIG_SIZE + CROS_PARAMS_SIZE + bootloader_size;
  Debug("kern_blob_size=0x%" PRIx64 "\n", kern_blob_size);
  kern_blob = VbExMalloc(kern_blob_size);
  Memset(kern_blob, 0, kern_blob_size);
  now = 0;

  Debug("kernel goes at kern_blob+0x%" PRIx64 "\n", now);

  Memcpy(kern_blob+now, g_kernel_data, g_kernel_size);
  now += roundup(g_kernel_size, CROS_ALIGN);

  Debug("config goes at kern_blob+0x%" PRIx64 "\n", now);
  if (g_config_size)
    Memcpy(kern_blob + now, g_config_data, g_config_size);
  now += CROS_CONFIG_SIZE;

  Debug("params goes at kern_blob+0x%" PRIx64 "\n", now);
  if (g_param_size) {
    Memcpy(kern_blob + now, g_param_data, g_param_size);
  }
  now += CROS_PARAMS_SIZE;

  Debug("bootloader goes at kern_blob+0x%" PRIx64 "\n", now);
  g_bootloader_address = kernel_body_load_address + now;
  Debug(" bootloader_address=0x%" PRIx64 "\n", g_bootloader_address);
  Debug(" bootloader_size=0x%" PRIx64 "\n", bootloader_size);
  if (bootloader_size)
    Memcpy(kern_blob + now, g_bootloader_data, g_bootloader_size);
  now += bootloader_size;
  Debug("end of kern_blob at kern_blob+0x%" PRIx64 "\n", now);

  /* Done */
  if (size_ptr)
    *size_ptr = kern_blob_size;

  return kern_blob;
}

static int Pack(const char* outfile,
                uint8_t *kernel_blob,
                uint64_t kernel_size,
                int version,
                uint64_t kernel_body_load_address,
                VbPrivateKey* signpriv_key) {
  VbSignature* body_sig;
  FILE* f;
  uint64_t i;
  uint64_t written = 0;

  /* Sign the kernel data */
  body_sig = CalculateSignature(kernel_blob, kernel_size, signpriv_key);
  if (!body_sig)
    Fatal("Error calculating body signature\n");

  /* Create preamble */
  g_preamble = CreateKernelPreamble(version,
                                    kernel_body_load_address,
                                    g_bootloader_address,
                                    roundup(g_bootloader_size, CROS_ALIGN),
                                    body_sig,
                                    opt_pad - g_keyblock->key_block_size,
                                    signpriv_key);
  if (!g_preamble) {
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
  Debug("0x%" PRIx64 " bytes of key_block\n", g_keyblock->key_block_size);
  Debug("0x%" PRIx64 " bytes of preamble\n", g_preamble->preamble_size);
  i = ((1 != fwrite(g_keyblock, g_keyblock->key_block_size, 1, f)) ||
       (1 != fwrite(g_preamble, g_preamble->preamble_size, 1, f)));
  if (i) {
    VbExError("Can't write output file %s\n", outfile);
    fclose(f);
    unlink(outfile);
    return 1;
  }
  written += g_keyblock->key_block_size;
  written += g_preamble->preamble_size;

  if (!opt_vblockonly) {
    Debug("0x%" PRIx64 " bytes of kern_blob\n", kernel_size);
    i = (1 != fwrite(kernel_blob, kernel_size, 1, f));
    if (i) {
      fclose(f);
      unlink(outfile);
      Fatal("Can't write output file %s\n", outfile);
    }
    written += kernel_size;
  }
  Debug("0x%" PRIx64 " bytes total\n", written);
  fclose(f);

  /* Success */
  return 0;
}

static int Verify(uint8_t* kernel_blob,
                  uint64_t kernel_size,
                  VbPublicKey* signpub_key,
                  const char* keyblock_outfile,
                  uint64_t min_version) {
  VbPublicKey* data_key;
  RSAPublicKey* rsa;

  if (0 != KeyBlockVerify(g_keyblock, g_keyblock->key_block_size,
                          signpub_key, (0 == signpub_key)))
    Fatal("Error verifying key block.\n");

  printf("Key block:\n");
  data_key = &g_keyblock->data_key;
  if (opt_verbose)
    printf("  Signature:           %s\n", signpub_key ? "valid" : "ignored");
  printf("  Size:                0x%" PRIx64 "\n", g_keyblock->key_block_size);
  printf("  Flags:               %" PRIu64 " ", g_keyblock->key_block_flags);
  if (g_keyblock->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_0)
    printf(" !DEV");
  if (g_keyblock->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_1)
    printf(" DEV");
  if (g_keyblock->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_0)
    printf(" !REC");
  if (g_keyblock->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_1)
    printf(" REC");
  printf("\n");
  printf("  Data key algorithm:  %" PRIu64 " %s\n", data_key->algorithm,
         (data_key->algorithm < kNumAlgorithms ?
          algo_strings[data_key->algorithm] : "(invalid)"));
  printf("  Data key version:    %" PRIu64 "\n", data_key->key_version);
  printf("  Data key sha1sum:    ");
  PrintPubKeySha1Sum(data_key);
  printf("\n");

  if (keyblock_outfile) {
    FILE* f = NULL;
    f = fopen(keyblock_outfile, "wb");
    if (!f)
      Fatal("Can't open key block file %s\n", keyblock_outfile);
    if (1 != fwrite(g_keyblock, g_keyblock->key_block_size, 1, f))
      Fatal("Can't write key block file %s\n", keyblock_outfile);
    fclose(f);
  }

  if (data_key->key_version < (min_version >> 16))
    Fatal("Data key version %" PRIu64
              " is lower than minimum %" PRIu64".\n",
              data_key->key_version, (min_version >> 16));

  rsa = PublicKeyToRSA(data_key);
  if (!rsa)
    Fatal("Error parsing data key.\n");

  /* Verify preamble */
  if (0 != VerifyKernelPreamble(
        g_preamble, g_preamble->preamble_size, rsa))
    Fatal("Error verifying preamble.\n");

  printf("Preamble:\n");
  printf("  Size:                0x%" PRIx64 "\n", g_preamble->preamble_size);
  printf("  Header version:      %" PRIu32 ".%" PRIu32"\n",
         g_preamble->header_version_major, g_preamble->header_version_minor);
  printf("  Kernel version:      %" PRIu64 "\n", g_preamble->kernel_version);
  printf("  Body load address:   0x%" PRIx64 "\n",
         g_preamble->body_load_address);
  printf("  Body size:           0x%" PRIx64 "\n",
         g_preamble->body_signature.data_size);
  printf("  Bootloader address:  0x%" PRIx64 "\n",
         g_preamble->bootloader_address);
  printf("  Bootloader size:     0x%" PRIx64 "\n",
         g_preamble->bootloader_size);

  if (g_preamble->kernel_version < (min_version & 0xFFFF))
    Fatal("Kernel version %" PRIu64 " is lower than minimum %" PRIu64 ".\n",
              g_preamble->kernel_version, (min_version & 0xFFFF));

  /* Verify body */
  if (0 != VerifyData(kernel_blob, kernel_size,
                      &g_preamble->body_signature, rsa))
    Fatal("Error verifying kernel body.\n");
  printf("Body verification succeeded.\n");


   if (opt_verbose)
     printf("Config:\n%s\n", kernel_blob + CmdLineOffset(g_preamble));

  return 0;
}

/****************************************************************************/

int main(int argc, char* argv[]) {
  char* filename = NULL;
  char* oldfile = NULL;
  char* keyblock_file = NULL;
  char* signpubkey_file = NULL;
  char* signprivkey_file = NULL;
  char* version_str = NULL;
  int version = -1;
  char* vmlinuz_file = NULL;
  char* bootloader_file = NULL;
  char* config_file = NULL;
  arch_t arch = ARCH_X86;
  char *address_str = NULL;
  uint64_t kernel_body_load_address = CROS_32BIT_ENTRY_ADDR;
  int mode = 0;
  int parse_error = 0;
  uint64_t min_version = 0;
  char* e;
  int i;
  VbPrivateKey* signpriv_key = NULL;
  VbPublicKey* signpub_key = NULL;
  uint8_t* kernel_blob = NULL;
  uint64_t kernel_size = 0;

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
        fprintf(stderr, "Only a single mode can be specified\n");
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
      address_str = optarg;
      kernel_body_load_address = strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e)) {
        fprintf(stderr, "Invalid --kloadaddr\n");
        parse_error = 1;
      }
      break;

    case OPT_KEYBLOCK:
      keyblock_file = optarg;
      break;

    case OPT_SIGNPUBKEY:
      signpubkey_file = optarg;
      break;

    case OPT_SIGNPRIVATE:
      signprivkey_file = optarg;
      break;

    case OPT_VMLINUZ:
      vmlinuz_file = optarg;
      break;

    case OPT_BOOTLOADER:
      bootloader_file = optarg;
      break;

    case OPT_CONFIG:
      config_file = optarg;
      break;

    case OPT_VBLOCKONLY:
      opt_vblockonly = 1;
      break;

    case OPT_VERSION:
      version_str = optarg;
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
      opt_pad = strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e)) {
        fprintf(stderr, "Invalid --pad\n");
        parse_error = 1;
      }
      break;
    }
  }

  if (parse_error)
    return PrintHelp(progname);

  switch(mode) {
  case OPT_MODE_PACK:

    /* Required */

    if (!keyblock_file)
      Fatal("Missing required keyblock file.\n");

    g_keyblock = (VbKeyBlockHeader*)ReadFile(keyblock_file, 0);
    if (!g_keyblock)
      Fatal("Error reading key block.\n");

    if (!signprivkey_file)
      Fatal("Missing required signprivate file.\n");

    signpriv_key = PrivateKeyRead(signprivkey_file);
    if (!signpriv_key)
      Fatal("Error reading signing key.\n");

    /* Optional */

    if (config_file) {
      Debug("Reading %s\n", config_file);
      g_config_data = ReadConfigFile(config_file, &g_config_size);
      if (!g_config_data)
        Fatal("Error reading config file.\n");
    }

    if (vmlinuz_file)
      if (!ImportVmlinuzFile(vmlinuz_file, arch, kernel_body_load_address))
        Fatal("Error reading kernel file.\n");

    if (bootloader_file) {
      Debug("Reading %s\n", bootloader_file);
      g_bootloader_data = ReadFile(bootloader_file, &g_bootloader_size);
      if (!g_bootloader_data)
        Fatal("Error reading bootloader file.\n");
      Debug(" bootloader file size=0x%" PRIx64 "\n", g_bootloader_size);
    }

    /* Do it */

    kernel_blob = CreateKernelBlob(kernel_body_load_address, arch,
                                   &kernel_size);

    return Pack(filename, kernel_blob, kernel_size,
                version, kernel_body_load_address,
                signpriv_key);

  case OPT_MODE_REPACK:

    /* Required */

    if (!signprivkey_file)
      Fatal("Missing required signprivate file.\n");

    signpriv_key = PrivateKeyRead(signprivkey_file);
    if (!signpriv_key)
      Fatal("Error reading signing key.\n");

    if (!oldfile)
      Fatal("Missing previously packed blob.\n");

    /* Load the old blob */

    kernel_blob = ReadOldBlobFromFileOrDie(oldfile, &kernel_size);
    if (0 != Verify(kernel_blob, kernel_size, 0, 0, 0))
      Fatal("The oldblob doesn't verify\n");

    /* Take it apart */

    UnpackKernelBlob(kernel_blob, kernel_size);
    free(kernel_blob);

    /* Load optional params */

    if (!version_str)
      version = g_preamble->kernel_version;

    if (!address_str)
      kernel_body_load_address = g_preamble->body_load_address;

    if (config_file) {
      if (g_config_data)
        free(g_config_data);
      Debug("Reading %s\n", config_file);
      g_config_data = ReadConfigFile(config_file, &g_config_size);
      if (!g_config_data)
        Fatal("Error reading config file.\n");
    }

    if (keyblock_file) {
      if (g_keyblock)
        free(g_keyblock);
      g_keyblock = (VbKeyBlockHeader*)ReadFile(keyblock_file, 0);
      if (!g_keyblock)
        Fatal("Error reading key block.\n");
    }

    /* Put it back together */

    kernel_blob = CreateKernelBlob(kernel_body_load_address, arch,
                                   &kernel_size);

    return Pack(filename, kernel_blob, kernel_size,
                version, kernel_body_load_address,
                signpriv_key);


  case OPT_MODE_VERIFY:

    /* Optional */

    if (signpubkey_file) {
      signpub_key = PublicKeyRead(signpubkey_file);
      if (!signpub_key)
        Fatal("Error reading public key.\n");
    }

    /* Do it */

    kernel_blob = ReadOldBlobFromFileOrDie(filename, &kernel_size);

    return Verify(kernel_blob, kernel_size, signpub_key,
                  keyblock_file, min_version);
  }

  fprintf(stderr, "You must specify a mode: --pack, --repack or --verify\n");
  return PrintHelp(progname);
}
