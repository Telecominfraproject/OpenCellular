// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VBOOT_REFERENCE_UTILITY_CGPT_CGPT_H_
#define VBOOT_REFERENCE_UTILITY_CGPT_CGPT_H_

#include <fcntl.h>
#include <features.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cgpt_endian.h"
#include "cgptlib.h"
#include "gpt.h"
#include "mtdlib.h"

struct legacy_partition {
  uint8_t  status;
  uint8_t  f_head;
  uint8_t  f_sect;
  uint8_t  f_cyl;
  uint8_t  type;
  uint8_t  l_head;
  uint8_t  l_sect;
  uint8_t  l_cyl;
  uint32_t f_lba;
  uint32_t num_sect;
} __attribute__((packed));


// syslinux uses this format:
struct pmbr {
  uint8_t                 bootcode[424];
  Guid                    boot_guid;
  uint32_t                disk_id;
  uint8_t                 magic[2];     // 0x1d, 0x9a
  struct legacy_partition part[4];
  uint8_t                 sig[2];       // 0x55, 0xaa
} __attribute__((packed));

void PMBRToStr(struct pmbr *pmbr, char *str, unsigned int buflen);

// Handle to the drive storing the GPT.
struct drive {
  int fd;           /* file descriptor */
  uint64_t size;    /* total size (in bytes) */
  int is_mtd;
  GptData gpt;
  MtdData mtd;
  struct pmbr pmbr;
};

struct nand_layout {
  int enabled;
  int use_host_ioctl; /* Use ioctl() on /dev/fts to read/write. */
  int bytes_per_page, pages_per_block, fts_block_offset, fts_block_size;
};

/* Write a NAND/MTD image instead of GPT. */
void EnableNandImage(int bytes_per_page, int pages_per_block,
                     int fts_block_offset, int fts_block_size);

/* mode should be O_RDONLY or O_RDWR */
int DriveOpen(const char *drive_path, struct drive *drive, int mode);
int DriveClose(struct drive *drive, int update_as_needed);
int CheckValid(const struct drive *drive);

/* Loads sectors from 'drive'.
 * *buf is pointed to an allocated memory when returned, and should be
 * freed.
 *
 *   drive -- open drive.
 *   buf -- pointer to buffer pointer
 *   sector -- offset of starting sector (in sectors)
 *   sector_bytes -- bytes per sector
 *   sector_count -- number of sectors to load
 *
 * Returns CGPT_OK for successful. Aborts if any error occurs.
 */
int Load(struct drive *drive, uint8_t **buf,
                const uint64_t sector,
                const uint64_t sector_bytes,
                const uint64_t sector_count);

/* Saves sectors to 'drive'.
 *
 *   drive -- open drive
 *   buf -- pointer to buffer
 *   sector -- starting sector offset
 *   sector_bytes -- bytes per sector
 *   sector_count -- number of sector to save
 *
 * Returns CGPT_OK for successful, CGPT_FAILED for failed.
 */
int Save(struct drive *drive, const uint8_t *buf,
                const uint64_t sector,
                const uint64_t sector_bytes,
                const uint64_t sector_count);


/* GUID conversion functions. Accepted format:
 *
 *   "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"
 *
 * At least GUID_STRLEN bytes should be reserved in 'str' (included the tailing
 * '\0').
 */
#define GUID_STRLEN 37
int StrToGuid(const char *str, Guid *guid);
void GuidToStr(const Guid *guid, char *str, unsigned int buflen);
int GuidEqual(const Guid *guid1, const Guid *guid2);
int IsZero(const Guid *guid);

/* Constant global type values to compare against */
extern const Guid guid_chromeos_firmware;
extern const Guid guid_chromeos_kernel;
extern const Guid guid_chromeos_rootfs;
extern const Guid guid_linux_data;
extern const Guid guid_chromeos_reserved;
extern const Guid guid_efi;
extern const Guid guid_unused;

int ReadPMBR(struct drive *drive);
int WritePMBR(struct drive *drive);

/* Convert possibly unterminated UTF16 string to UTF8.
 * Caller must prepare enough space for UTF8, which could be up to
 * twice the byte length of UTF16 string plus the terminating '\0'.
 *
 * Return: CGPT_OK --- all character are converted successfully.
 *         CGPT_FAILED --- convert error, i.e. output buffer is too short.
 */
int UTF16ToUTF8(const uint16_t *utf16, unsigned int maxinput,
                uint8_t *utf8, unsigned int maxoutput);

/* Convert null-terminated UTF8 string to UTF16.
 * Caller must prepare enough space for UTF16, which is the byte length of UTF8
 * plus the terminating 0x0000.
 *
 * Return: CGPT_OK --- all character are converted successfully.
 *         CGPT_FAILED --- convert error, i.e. output buffer is too short.
 */
int UTF8ToUTF16(const uint8_t *utf8, uint16_t *utf16, unsigned int maxoutput);

/* Helper functions for supported GPT types. */
int ResolveType(const Guid *type, char *buf);
int SupportedType(const char *name, Guid *type);
void PrintTypes(void);
void EntryDetails(GptEntry *entry, uint32_t index, int raw);
void MtdEntryDetails(MtdDiskPartition *entry, uint32_t index, int raw);

uint32_t GetNumberOfEntries(const struct drive *drive);
GptEntry *GetEntry(GptData *gpt, int secondary, uint32_t entry_index);
MtdDiskPartition *MtdGetEntry(MtdData *mtd, int secondary, uint32_t index);

void SetPriority(struct drive *drive, int secondary, uint32_t entry_index,
                 int priority);
int GetPriority(struct drive *drive, int secondary, uint32_t entry_index);
void SetTries(struct drive *drive, int secondary, uint32_t entry_index,
              int tries);
int GetTries(struct drive *drive, int secondary, uint32_t entry_index);
void SetSuccessful(struct drive *drive, int secondary, uint32_t entry_index,
                   int success);
int GetSuccessful(struct drive *drive, int secondary, uint32_t entry_index);

void SetRaw(struct drive *drive, int secondary, uint32_t entry_index,
           uint32_t raw);

void UpdateAllEntries(struct drive *drive);

uint8_t RepairHeader(GptData *gpt, const uint32_t valid_headers);
uint8_t RepairEntries(GptData *gpt, const uint32_t valid_entries);
void UpdateCrc(GptData *gpt);
int IsSynonymous(const GptHeader* a, const GptHeader* b);

int IsUnused(struct drive *drive, int secondary, uint32_t index);
int IsKernel(struct drive *drive, int secondary, uint32_t index);
int LookupMtdTypeForGuid(const Guid *type);
const Guid *LookupGuidForMtdType(int type);

// Optional. Applications that need this must provide an implementation.
//
// Explanation:
//   Some external utilities need to manipulate the GPT, but don't create new
//   partitions from scratch. The cgpt executable uses libuuid to provide this
//   functionality, but we don't want to have to build or install a separate
//   instance of that library just for the 32-bit static post-install tool,
//   which doesn't need this function.
int GenerateGuid(Guid *newguid);

// For usage and error messages.
void Error(const char *format, ...);

// Command functions.
int cmd_show(int argc, char *argv[]);
int cmd_repair(int argc, char *argv[]);
int cmd_create(int argc, char *argv[]);
int cmd_add(int argc, char *argv[]);
int cmd_boot(int argc, char *argv[]);
int cmd_find(int argc, char *argv[]);
int cmd_prioritize(int argc, char *argv[]);
int cmd_legacy(int argc, char *argv[]);

#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))
const char *GptError(int errnum);

// Size in chars of the GPT Entry's PartitionName field
#define GPT_PARTNAME_LEN 72

/* The standard "assert" macro goes away when NDEBUG is defined. This doesn't.
 */
#define require(A) do { \
  if (!(A)) { \
    fprintf(stderr, "condition (%s) failed at %s:%d\n", \
            #A, __FILE__, __LINE__); \
    exit(1); } \
  } while (0)

#endif  // VBOOT_REFERENCE_UTILITY_CGPT_CGPT_H_
