// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VBOOT_REFERENCE_UTILITY_CGPT_CGPT_H_
#define VBOOT_REFERENCE_UTILITY_CGPT_CGPT_H_

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <features.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "endian.h"
#include "gpt.h"
#include "cgptlib.h"


// Just for clarity
enum {
  CGPT_OK = 0,
  CGPT_FAILED,
};


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
  GptData gpt;
  struct pmbr pmbr;
};


int DriveOpen(const char *drive_path, struct drive *drive);
int DriveClose(struct drive *drive, int update_as_needed);
int CheckValid(const struct drive *drive);

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
int IsZero(const Guid *guid);


int ReadPMBR(struct drive *drive);
int WritePMBR(struct drive *drive);


/* Convert possibly unterminated UTF16 string to UTF8.
 * Caller must prepare enough space for UTF8, which could be up to
 * twice the number of UTF16 chars plus the terminating '\0'.
 */
void UTF16ToUTF8(const uint16_t *utf16, unsigned int maxinput,
                 uint8_t *utf8, unsigned int maxoutput);
/* Convert null-terminated UTF8 string to UTF16.
 * Caller must prepare enough space for UTF16, including a terminating 0x0000
 */
void UTF8ToUTF16(const uint8_t *utf8, uint16_t *utf16, unsigned int maxoutput);

/* Helper functions for supported GPT types. */
int ResolveType(const Guid *type, char *buf);
int SupportedType(const char *name, Guid *type);
void PrintTypes(void);
void EntryDetails(GptEntry *entry, uint32_t index, int raw);

uint32_t GetNumberOfEntries(const GptData *gpt);
GptEntry *GetEntry(GptData *gpt, int secondary, uint32_t entry_index);
void SetPriority(GptData *gpt, int secondary, uint32_t entry_index,
                 int priority);
int GetPriority(GptData *gpt, int secondary, uint32_t entry_index);
void SetTries(GptData *gpt, int secondary, uint32_t entry_index, int tries);
int GetTries(GptData *gpt, int secondary, uint32_t entry_index);
void SetSuccessful(GptData *gpt, int secondary, uint32_t entry_index,
                   int success);
int GetSuccessful(GptData *gpt, int secondary, uint32_t entry_index);

uint8_t RepairHeader(GptData *gpt, const uint32_t valid_headers);
uint8_t RepairEntries(GptData *gpt, const uint32_t valid_entries);
void UpdateCrc(GptData *gpt);
int IsSynonymous(const GptHeader* a, const GptHeader* b);

// For usage and error messages.
extern const char* progname;
extern const char* command;
void Error(const char *format, ...);


// Command functions.
int cmd_show(int argc, char *argv[]);
int cmd_repair(int argc, char *argv[]);
int cmd_create(int argc, char *argv[]);
int cmd_add(int argc, char *argv[]);
int cmd_boot(int argc, char *argv[]);
int cmd_find(int argc, char *argv[]);

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
