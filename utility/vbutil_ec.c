/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot utility for EC firmware
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cryptolib.h"
#include "fmap.h"
#include "host_common.h"
#include "vboot_common.h"


/* Command line options */
enum {
  OPT_MODE_SIGN = 1000,
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
  {"sign", 1, 0,                      OPT_MODE_SIGN               },
  {"verify", 1, 0,                    OPT_MODE_VERIFY             },
  {"keyblock", 1, 0,                  OPT_KEYBLOCK                },
  {"signpubkey", 1, 0,                OPT_SIGNPUBKEY              },
  {"signprivate", 1, 0,               OPT_SIGNPRIVATE             },
  {"version", 1, 0,                   OPT_VERSION                 },
  {"flags", 1, 0,                     OPT_FLAGS                   },
  {"name", 1, 0,                      OPT_NAME                    },
  {NULL, 0, 0, 0}
};


/* Print help and return error */
static int PrintHelp(void) {

  puts("vbutil_ec - Verified boot signing utility for EC firmware\n"
       "\n"
       "This will sign, re-sign, or test a complete EC firmware image.\n"
       "The EC image is initially completely unsigned. To make it bootable\n"
       "the pubic root key must be installed in the RO section, and each RW\n"
       "section must be signed with the appropriate private keys.\n"
       "\n"
       "To sign an image:   vbutil_ec --sign <file> [OPTIONS]\n"
       "\n"
       "For signing, these options are required:\n"
       "\n"
       "  --keyblock <file>           Key block in .keyblock format\n"
       "  --signprivate <file>        Signing private key in .vbprivk format\n"
       "  --version <number>          Firmware version\n"
       "\n"
       "If the RO public key has not been installed, you will also need\n"
       "\n"
       "  --signpubkey <file>         Signing public key in .vbpubk format\n"
       "\n"
       "Optional args are:\n"
       "\n"
       "  --flags <number>            Preamble flags (defaults to 0)\n"
       "  --name <string>             Human-readable description\n"
       "\n"
       "\n"
       "To verify an image:   vbutil_ec --verify <file>\n"
       "\n");
  return 1;
}


static int FindInFmap(FmapHeader *fh, const char *name,
                      uint8_t *base, uint64_t base_size,
                      uint8_t **data, uint64_t *size) {
  const FmapAreaHeader *ah;
  int i;

  ah = (FmapAreaHeader *)(fh + 1);
  for (i = 0; i < fh->fmap_nareas; i++)
    if (!strncmp(ah[i].area_name, name, FMAP_NAMELEN)) {
      if (ah[i].area_size + ah[i].area_offset > base_size) {
        printf("FMAP region %s extends off image file\n", name);
        return 0;
      }
      if (data)
        *data = base + ah[i].area_offset;
      if (size)
        *size = ah[i].area_size;
      return 1;
    }

  return 0;
}

static int GoodKey(VbPublicKey *key, uint64_t region_size)
{
  uint64_t key_size;

  if (0 != VerifyPublicKeyInside(key, region_size, key))
    return 0;

  if (key->algorithm >= kNumAlgorithms)
    return 0;

  /* Currently, TPM only supports 16-bit version */
  if (key->key_version > 0xFFFF)
    return 0;

  if (!RSAProcessedKeySize(key->algorithm, &key_size) ||
      key_size != key->key_size)
    return 0;

  return 1;
}


/* We build the image file with a non-FF byte at the end of each RW firmware,
 * just so we can do this. */
static uint64_t FindImageEnd(uint8_t *data, uint64_t size)
{
  for (size-- ; size && data[size] == 0xff; size--)
    ;
  return size;
}

static void SignImage(const char *filename,
                      VbKeyBlockHeader *key_block, uint64_t key_block_size,
                      VbPrivateKey *privkey, uint64_t version,
                      VbPublicKey *pubkey, uint32_t preamble_flags,
                      const char *name) {
  struct stat sb;
  int fd;
  void *image;
  uint64_t image_size;
  FmapHeader* fmap;
  VbECPreambleHeader *preamble;
  uint8_t *fv_data = 0;
  uint8_t *vblock_data = 0;
  uint64_t fv_size, vblock_size;
  VbSignature* body_digest;

  if (name && strlen(name)+1 > sizeof(preamble->name))
    VbExError("Name string is too long\n");

  if (0 != stat(filename, &sb))
    VbExError("Can't stat %s: %s\n", filename, strerror(errno));

  fd = open(filename, O_RDWR);
  if (fd < 0)
    VbExError("Can't open %s: %s\n", filename, strerror(errno));

  image = mmap(0, sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (image == (void *)-1)
    VbExError("Can't mmap %s: %s\n", filename, strerror(errno));
  close(fd);                            /* done with this now */

  fmap = (FmapHeader *)FmapFind(image, sb.st_size);
  if (!fmap)
    VbExError("File %s doesn't have an FMAP - can't continue.\n");

  if (fmap->fmap_size > sb.st_size)
    VbExError("FMAP is bigger than file size (%ld vs %ld)\n",
              fmap->fmap_size, sb.st_size);

  image_size = sb.st_size;

  /* Install pubkey if provided */
  if (pubkey) {
    if (!FindInFmap(fmap, "ROOT_KEY", image, image_size,
                    &vblock_data, &vblock_size))
      VbExError("Can't find ROOT_KEY in %s\n", filename);

    if (pubkey->key_offset + pubkey->key_size > vblock_size)
      VbExError("ROOT_KEY is too small for pubkey (%d bytes, needs %d)\n",
                vblock_size, pubkey->key_offset + pubkey->key_size);

    memcpy(vblock_data, pubkey, pubkey->key_offset + pubkey->key_size);
  }


  /* Sign FW A */
  if (!FindInFmap(fmap, "FW_MAIN_A", image, image_size, &fv_data, &fv_size))
    VbExError("Can't find FW_MAIN_A in %s\n", filename);

  if (!FindInFmap(fmap, "VBLOCK_A", image, image_size,
                  &vblock_data, &vblock_size))
    VbExError("Can't find VBLOCK_A in %s\n", filename);

  fv_size = FindImageEnd(fv_data, fv_size);

  body_digest = CalculateHash(fv_data, fv_size, privkey);
  if (!body_digest)
    VbExError("Error calculating body digest\n");

  preamble = CreateECPreamble(version, body_digest, privkey,
                              preamble_flags, name);
  if (!preamble)
    VbExError("Error creating preamble.\n");

  if (key_block_size + preamble->preamble_size > vblock_size)
    VbExError("VBLOCK_A is too small for digest (%d bytes, needs %d)\n",
              vblock_size, key_block_size + preamble->preamble_size);

  memcpy(vblock_data, key_block, key_block_size);
  memcpy(vblock_data + key_block_size, preamble, preamble->preamble_size);

  free(body_digest);
  free(preamble);


  /* Sign FW B - skip if there isn't one */
  if (!FindInFmap(fmap, "FW_MAIN_B", image, image_size, &fv_data, &fv_size) ||
      !FindInFmap(fmap, "VBLOCK_B", image, image_size,
                  &vblock_data, &vblock_size)) {
    printf("Image does not contain FW B - ignoring that part\n");
  } else {
    fv_size = FindImageEnd(fv_data, fv_size);

    body_digest = CalculateHash(fv_data, fv_size, privkey);
    if (!body_digest)
      VbExError("Error calculating body digest\n");

    preamble = CreateECPreamble(version, body_digest, privkey,
                                preamble_flags, name);
    if (!preamble)
      VbExError("Error creating preamble.\n");

    if (key_block_size + preamble->preamble_size > vblock_size)
      VbExError("VBLOCK_B is too small for digest (%d bytes, needs %d)\n",
                vblock_size, key_block_size + preamble->preamble_size);

    memcpy(vblock_data, key_block, key_block_size);
    memcpy(vblock_data + key_block_size, preamble, preamble->preamble_size);

    free(body_digest);
    free(preamble);
  }

  /* Unmap to write changes to disk. */
  if (0 != munmap(image, sb.st_size))
    VbExError("Can't munmap %s: %s\n", filename, strerror(errno));

  printf("Image signing completed\n");

}

static int Verify(const char *filename) {
  struct stat sb;
  int fd;
  void *image;
  uint64_t image_size;
  FmapHeader* fmap;
  VbECPreambleHeader *preamble;
  VbPublicKey *pubkey;
  uint64_t pubkey_size;
  VbKeyBlockHeader *key_block;
  uint64_t key_block_size;
  uint8_t *fv_data = 0;
  uint64_t fv_size;
  VbPublicKey *data_key;
  RSAPublicKey* rsa;
  int errorcnt = 0;
  char buf[80];
  int i;

  if (0 != stat(filename, &sb))
    VbExError("Can't stat %s: %s\n", filename, strerror(errno));

  fd = open(filename, O_RDONLY);
  if (fd < 0)
    VbExError("Can't open %s: %s\n", filename, strerror(errno));

  image = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (image == (void *)-1)
    VbExError("Can't mmap %s: %s\n", filename, strerror(errno));
  close(fd);                            /* done with this now */

  fmap = (FmapHeader *)FmapFind(image, sb.st_size);
  if (!fmap)
    VbExError("File %s doesn't have an FMAP - can't continue.\n");

  if (fmap->fmap_size > sb.st_size)
    VbExError("FMAP is bigger than file size (%ld vs %ld)\n",
              fmap->fmap_size, sb.st_size);

  image_size = sb.st_size;

  /* Read pubkey */
  if (!FindInFmap(fmap, "ROOT_KEY", image, image_size,
                  (uint8_t **)&pubkey, &pubkey_size)) {
    printf("Can't find ROOT_KEY in %s\n", filename);
    errorcnt++;
  } else if (!GoodKey(pubkey, pubkey_size)) {
    printf("ROOT_KEY is invalid\n");
    errorcnt++;
  } else {
    printf("ROOT_KEY\n");
    printf("  Algorithm:         %" PRIu64 " %s\n", pubkey->algorithm,
           (pubkey->algorithm < kNumAlgorithms ?
            algo_strings[pubkey->algorithm] : "(invalid)"));
    printf("  Key Version:       %" PRIu64 "\n", pubkey->key_version);
    printf("  Key sha1sum:       ");
    PrintPubKeySha1Sum(pubkey);
    printf("\n");
  }

  for (i = 'A'; i <= 'B'; i++) {

    fv_data = 0;
    key_block = 0;
    preamble = 0;

    printf("FW %c\n", i);
    sprintf(buf, "FW_MAIN_%c", i);
    if (!FindInFmap(fmap, buf, image, image_size, &fv_data, &fv_size)) {
      printf("Can't find %s in %s\n", buf, filename);
      errorcnt++;
      continue;
    }

    sprintf(buf, "VBLOCK_%c", i);
    if (!FindInFmap(fmap, buf, image, image_size,
                    (uint8_t **)&key_block, &key_block_size)) {
      printf("Can't find %s in %s\n", buf, filename);
      errorcnt++;
      continue;
    }

    if (0 != KeyBlockVerify(key_block, key_block_size, pubkey, !pubkey)) {
      printf("Error verifying key block for %s.\n", buf);
      errorcnt++;
      continue;
    }
    printf("  Key block:\n");
    data_key = &key_block->data_key;
    printf("    Size:                %" PRIu64 "\n",
           key_block->key_block_size);
    printf("    Flags:               %" PRIu64 " (ignored)\n",
           key_block->key_block_flags);
    printf("    Data key algorithm:  %" PRIu64 " %s\n", data_key->algorithm,
           (data_key->algorithm < kNumAlgorithms ?
            algo_strings[data_key->algorithm] : "(invalid)"));
    printf("    Data key version:    %" PRIu64 "\n", data_key->key_version);
    printf("    Data key sha1sum:    ");
    PrintPubKeySha1Sum(data_key);
    printf("\n");

    preamble = (VbECPreambleHeader*)
      ((uint8_t *)key_block + key_block->key_block_size);

    rsa = PublicKeyToRSA(&key_block->data_key);
    if (!rsa) {
      printf("Error parsing data key.\n");
      errorcnt++;
    }
    /* Verify preamble */
    if (0 != VerifyECPreamble(preamble,
                              key_block_size - key_block->key_block_size,
                              rsa)) {
      printf("Error verifying preamble.\n");
      errorcnt++;
      free(rsa);
      continue;
    }
    printf("  Preamble:\n");
    printf("    Size:                  %" PRIu64 "\n",
           preamble->preamble_size);
    printf("    Header version:        %" PRIu32 ".%" PRIu32"\n",
           preamble->header_version_major,
           preamble->header_version_minor);
    printf("    Firmware version:      %" PRIu64 "\n",
           preamble->firmware_version);
    printf("    Firmware body size:    %" PRIu64 "\n",
           preamble->body_digest.data_size);
    printf("    Preamble flags:        %" PRIu32 "\n", preamble->flags);
    printf("    Preamble name:         %s\n", preamble->name);

    /* TODO: verify body size same as signature size */

    /* Verify body */
    if (preamble->flags & VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL) {
      printf("Preamble requests USE_RO_NORMAL; skipping verification.\n");
    } else {
      if (0 != EqualData(fv_data, fv_size,
                         &preamble->body_digest, rsa)) {
        printf("Error verifying firmware body.\n");
        errorcnt++;
      }
    }
    free(rsa);
  }

  /* Done */
  if (0 != munmap(image, sb.st_size))
    VbExError("Can't munmap %s: %s\n", filename, strerror(errno));

  printf("Done\n");
  return errorcnt;
}

int main(int argc, char* argv[]) {

  char* filename = NULL;
  uint64_t version = 0;
  int got_version = 0;
  uint32_t preamble_flags = 0;
  char *name = NULL;
  int mode = 0;
  VbKeyBlockHeader* key_block = 0;
  VbPrivateKey* privkey = 0;
  VbPublicKey* pubkey = 0;
  uint64_t key_block_size;
  int errorcnt = 0;
  char* e;
  int i;

  while ((i = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
    switch (i) {
    case '?':
      /* Unhandled option */
      printf("Unknown option\n");
      errorcnt++;
      break;

    case OPT_MODE_SIGN:
    case OPT_MODE_VERIFY:
      mode = i;
      filename = optarg;
      break;

    case OPT_KEYBLOCK:
      /* Read the key block and keys */
      key_block = (VbKeyBlockHeader*)ReadFile(optarg, &key_block_size);
      if (!key_block) {
        printf("Error reading key block from %s\n", optarg);
        errorcnt++;
      }
      break;

    case OPT_SIGNPUBKEY:
      pubkey = PublicKeyRead(optarg);
      if (!pubkey) {
        printf("Error reading public key from %s\n", optarg);
        errorcnt++;
      }
      break;

    case OPT_SIGNPRIVATE:
      privkey = PrivateKeyRead(optarg);
      if (!privkey) {
        printf("Error reading private key from %s\n", optarg);
        errorcnt++;
      }
      break;

    case OPT_VERSION:
      version = strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e)) {
        printf("Invalid --version argument: \"%s\"\n", optarg);
        errorcnt++;
      }
      got_version = 1;
      break;

    case OPT_FLAGS:
      preamble_flags = strtoul(optarg, &e, 0);
      if (!*optarg || (e && *e)) {
        printf("Invalid --flags argument: \"%s\"\n", optarg);
        errorcnt++;
      }
      break;

    case OPT_NAME:
      name = optarg;
      break;
    }
  }

  switch(mode) {

  case OPT_MODE_SIGN:
    /* Check required args */
    if (!key_block) {
      printf("The ----keyblock arg is required when signing\n");
      errorcnt++;
    }
    if (!privkey) {
      printf("The --signprivate arg is required when signing\n");
      errorcnt++;
    }
    if (!got_version) {
      printf("The --version arg is required when signing\n");
      errorcnt++;
    }

    if (errorcnt)
      return PrintHelp();

    /* Sign or die */
    SignImage(filename, key_block, key_block_size,
              privkey, version, pubkey, preamble_flags, name);

    /* fall through and verify what we've just done */

  case OPT_MODE_VERIFY:
    return Verify(filename);

  default:
    printf("\nMust specify a mode, either --sign or --verify.\n\n");
    return PrintHelp();
  }
}
