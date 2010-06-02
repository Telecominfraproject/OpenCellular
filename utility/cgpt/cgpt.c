/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility for ChromeOS-specific GPT partitions, Please see corresponding .c
 * files for more details.
 */
/* To compile on host without compatility to BSD, we include
 * endian.h under chroot. */
#define _BSD_SOURCE
#include "endian.h"

#define __USE_LARGEFILE64
#define __USE_FILE_OFFSET64
#define _LARGEFILE64_SOURCE
#include "cgpt.h"
#include "cgpt_tofix.h"
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "cgptlib_internal.h"
#include "utility.h"

/* For usage print */
const char* progname;

/* Lists all command here. */
struct {
  const char *name;
  int (*fp)(int argc, char *argv[]);
  const char *comment;
} cmds[] = {
  {"add", CgptAdm, "Add a partition to drive"},
  {"delete", CgptAdm, "Delete a partition on drive"},
  {"modify", CgptAdm, "Modify the partition on drive"},
  {"attribute", CgptAttribute, "Update GPT attribute bits "
                               "(for ChromeOS kernel entry only)"},
  {"dev", CgptDev, "Developper mode"},
  {"repair", CgptRepair, "Repair primary and secondary headers and tables"},
  {"show", CgptShow, "Show partition details"},
};

/* Shows main menu. If 'message' is non-NULL, shows it as header. Then, this
 * traverses cmds[] and shows supported commands and their comments. */
void Usage(const char *message) {
  int i;

  if (message) printf("%s\n", message);
  printf("Usage: %s COMMAND [OPTIONS]\n\n"
         "Supported COMMANDs:\n\n",
         progname);
  for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i) {
    printf("    %-10s  %s\n", cmds[i].name, cmds[i].comment);
  }
  printf("\nFor more detailed usage, use %s COMMAND --help.\n\n", progname);
}

/* GUID conversion functions. Accepted format:
 *
 *   "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"
 *
 * Returns CGPT_OK if parsing is successful; otherwise CGPT_FAILED.
 */
int StrToGuid(const char *str, Guid *guid) {
  uint32_t time_low, time_mid, time_high_and_version;

  if (11 > sscanf(str, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                  &time_low,
                  (unsigned int *)&time_mid,
                  (unsigned int *)&time_high_and_version,
                  (unsigned int *)&guid->u.Uuid.clock_seq_high_and_reserved,
                  (unsigned int *)&guid->u.Uuid.clock_seq_low,
                  (unsigned int *)&guid->u.Uuid.node[0],
                  (unsigned int *)&guid->u.Uuid.node[1],
                  (unsigned int *)&guid->u.Uuid.node[2],
                  (unsigned int *)&guid->u.Uuid.node[3],
                  (unsigned int *)&guid->u.Uuid.node[4],
                  (unsigned int *)&guid->u.Uuid.node[5])) return CGPT_FAILED;

  guid->u.Uuid.time_low = htole32(time_low);
  guid->u.Uuid.time_mid = htole16(time_mid);
  guid->u.Uuid.time_high_and_version = htole16(time_high_and_version);

  return CGPT_OK;
}

void GuidToStr(const Guid *guid, char *str) {
  sprintf(str, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
          le32toh(guid->u.Uuid.time_low), le16toh(guid->u.Uuid.time_mid),
          le16toh(guid->u.Uuid.time_high_and_version),
          guid->u.Uuid.clock_seq_high_and_reserved, guid->u.Uuid.clock_seq_low,
          guid->u.Uuid.node[0], guid->u.Uuid.node[1], guid->u.Uuid.node[2],
          guid->u.Uuid.node[3], guid->u.Uuid.node[4], guid->u.Uuid.node[5]);
}

/* Convert UTF16 string to UTF8. Rewritten from gpt utility.
 * Caller must prepare enough space for UTF8. The rough estimation is:
 *
 *   utf8 length = bytecount(utf16) * 1.5
 */
#define SIZEOF_GPTENTRY_NAME 36  /* sizeof(GptEntry.name[]) */
void UTF16ToUTF8(const uint16_t *utf16, uint8_t *utf8)
{
  size_t s8idx, s16idx, s16len;
  uint32_t utfchar;
  unsigned int next_utf16;

  for (s16len = 0; s16len < SIZEOF_GPTENTRY_NAME && utf16[s16len]; ++s16len);

  *utf8 = s8idx = s16idx = 0;
  while (s16idx < s16len) {
    utfchar = le16toh(utf16[s16idx++]);
    if ((utfchar & 0xf800) == 0xd800) {
      next_utf16 = le16toh(utf16[s16idx]);
      if ((utfchar & 0x400) != 0 || (next_utf16 & 0xfc00) != 0xdc00)
        utfchar = 0xfffd;
      else
        s16idx++;
    }
    if (utfchar < 0x80) {
      utf8[s8idx++] = utfchar;
    } else if (utfchar < 0x800) {
      utf8[s8idx++] = 0xc0 | (utfchar >> 6);
      utf8[s8idx++] = 0x80 | (utfchar & 0x3f);
    } else if (utfchar < 0x10000) {
      utf8[s8idx++] = 0xe0 | (utfchar >> 12);
      utf8[s8idx++] = 0x80 | ((utfchar >> 6) & 0x3f);
      utf8[s8idx++] = 0x80 | (utfchar & 0x3f);
    } else if (utfchar < 0x200000) {
      utf8[s8idx++] = 0xf0 | (utfchar >> 18);
      utf8[s8idx++] = 0x80 | ((utfchar >> 12) & 0x3f);
      utf8[s8idx++] = 0x80 | ((utfchar >> 6) & 0x3f);
      utf8[s8idx++] = 0x80 | (utfchar & 0x3f);
    }
  }
}

/* Convert UTF8 string to UTF16. Rewritten from gpt utility.
 * Caller must prepare enough space for UTF16. The conservative estimation is:
 *
 *   utf16 bytecount = bytecount(utf8) / 3 * 4
 */
void UTF8ToUTF16(const uint8_t *utf8, uint16_t *utf16)
{
  size_t s16idx, s8idx, s8len;
  uint32_t utfchar;
  unsigned int c, utfbytes;

  for (s8len = 0; utf8[s8len]; ++s8len);

  s8idx = s16idx = 0;
  utfbytes = 0;
  do {
    c = utf8[s8idx++];
    if ((c & 0xc0) != 0x80) {
      /* Initial characters. */
      if (utfbytes != 0) {
        /* Incomplete encoding. */
        utf16[s16idx++] = 0xfffd;
      }
      if ((c & 0xf8) == 0xf0) {
        utfchar = c & 0x07;
        utfbytes = 3;
      } else if ((c & 0xf0) == 0xe0) {
        utfchar = c & 0x0f;
        utfbytes = 2;
      } else if ((c & 0xe0) == 0xc0) {
        utfchar = c & 0x1f;
        utfbytes = 1;
      } else {
        utfchar = c & 0x7f;
        utfbytes = 0;
      }
    } else {
      /* Followup characters. */
      if (utfbytes > 0) {
        utfchar = (utfchar << 6) + (c & 0x3f);
        utfbytes--;
      } else if (utfbytes == 0)
        utfbytes = -1;
        utfchar = 0xfffd;
    }
    if (utfbytes == 0) {
      if (utfchar >= 0x10000) {
        utf16[s16idx++] = htole16(0xd800 | ((utfchar>>10)-0x40));
        if (s16idx >= SIZEOF_GPTENTRY_NAME) break;
        utf16[s16idx++] = htole16(0xdc00 | (utfchar & 0x3ff));
      } else {
        utf16[s16idx++] = htole16(utfchar);
      }
    }
  } while (c != 0 && s16idx < SIZEOF_GPTENTRY_NAME);
  if (s16idx < SIZEOF_GPTENTRY_NAME)
    utf16[s16idx++] = 0;
}

struct {
  Guid type;
  char *name;
  char *description;
} supported_types[] = {
 {GPT_ENT_TYPE_UNUSED, "unused", "Unused partition"},
 {GPT_ENT_TYPE_EFI, "efi", "EFI partition"},
 {GPT_ENT_TYPE_CHROMEOS_KERNEL, "croskern", "ChromeOS kernel"},
 {GPT_ENT_TYPE_CHROMEOS_ROOTFS, "crosroot", "ChromeOS rootfs"},
 {GPT_ENT_TYPE_CHROMEOS_RESERVED, "crosresv", "ChromeOS reserved"},
};

/* Resolves human-readable GPT type.
 * Returns CGPT_OK if found.
 * Returns CGPT_FAILED if no known type found. */
int ResolveType(const Guid *type, char *buf) {
  int i;
  for (i = 0; i < ARRAY_COUNT(supported_types); ++i) {
    if (!Memcmp(type, &supported_types[i].type, sizeof(Guid))) {
      strcpy(buf, supported_types[i].description);
      return CGPT_OK;
    }
  }
  return CGPT_FAILED;
}

int SupportedType(const char *name, Guid *type) {
  int i;
  for (i = 0; i < ARRAY_COUNT(supported_types); ++i) {
    if (!strcmp(name, supported_types[i].name)) {
      Memcpy(type, &supported_types[i].type, sizeof(Guid));
      return CGPT_OK;
    }
  }
  return CGPT_FAILED;
}

void PrintTypes(void) {
  int i;
  printf("\n* For --type option, you can use the following alias, "
         "instead of hex values:\n");
  for (i = 0; i < ARRAY_COUNT(supported_types); ++i) {
    printf("  %-10s %s\n", supported_types[i].name,
                          supported_types[i].description);
  }
  printf("\n");
}

/* Loads sectors from 'fd'.
 * *buf is pointed to an allocated memory when returned, and should be
 * freed by cgpt_close().
 *
 *   fd -- file descriptot.
 *   buf -- pointer to buffer pointer
 *   sector -- offset of starting sector (in sectors)
 *   sector_bytes -- bytes per sector
 *   sector_count -- number of sectors to load
 *
 * Returns CGPT_OK for successful. Aborts if any error occurs.
 */
int Load(const int fd, uint8_t **buf,
         const uint64_t sector,
         const uint64_t sector_bytes,
         const uint64_t sector_count) {
  int count;  /* byte count to read */
  int nread;

  assert(buf);
  count = sector_bytes * sector_count;
  *buf = Malloc(count);
  assert(*buf);

  if (-1 == lseek64(fd, sector * sector_bytes, SEEK_SET))
    goto error_free;

  nread = read(fd, *buf, count);
  if (nread < count)
    goto error_free;

  return CGPT_OK;

error_free:
  Free(*buf);
  *buf = 0;
  abort();
}

/* Saves sectors to 'fd'.
 *
 *   fd -- file descriptot.
 *   buf -- pointer to buffer
 *   sector -- starting sector offset
 *   sector_bytes -- bytes per sector
 *   sector_count -- number of sector to save
 *
 * Returns CGPT_OK for successful, CGPT_FAILED for failed.
 */
int Save(const int fd, const uint8_t *buf,
         const uint64_t sector,
         const uint64_t sector_bytes,
         const uint64_t sector_count) {
  int count;  /* byte count to write */
  int nwrote;

  assert(buf);
  count = sector_bytes * sector_count;

  if (-1 == lseek64(fd, sector * sector_bytes, SEEK_SET))
    return CGPT_FAILED;

  nwrote = write(fd, buf, count);
  if (nwrote < count)
    return CGPT_FAILED;

  return CGPT_OK;
}

int CheckValid(const struct drive *drive) {
  if ((drive->gpt.valid_headers != MASK_BOTH) ||
      (drive->gpt.valid_entries != MASK_BOTH)) {
    printf("\n[ERROR] any of GPT header/entries is invalid, "
           "please run '%s repair' first\n", progname);
    return CGPT_FAILED;
  }
  return CGPT_OK;
}

/* Opens a block device (a regular file works well too).
 *
 * Returns CGPT_FAILED if any error happens.
 * Returns CGPT_OK if success and information are stored in 'drive'. */
int DriveOpen(const char *drive_path, struct drive *drive) {
  struct stat stat;
  int gpt_retval;

  assert(drive_path);
  assert(drive);

  Memset(drive, 0, sizeof(struct drive));
  drive->fd = open(drive_path, O_RDWR | O_LARGEFILE);
  if (drive->fd == -1) {
    printf("[ERROR] Cannot open drive file [%s]: %s\n",
           drive_path, strerror(errno));
    return CGPT_FAILED;
  }

  if (fstat(drive->fd, &stat) == -1) {
    goto error_close;
  }
  if ((stat.st_mode & S_IFMT) != S_IFREG) {
    if (ioctl(drive->fd, BLKGETSIZE64, &drive->size) < 0) {
      printf("[ERROR] Cannot get sector size from drive file [%s]: %s\n",
             drive_path, strerror(errno));
      goto error_close;
    }
    if (ioctl(drive->fd, BLKSSZGET, &drive->gpt.sector_bytes) < 0) {
      printf("[ERROR] Cannot get drive size from drive file [%s]: %s\n",
             drive_path, strerror(errno));
      goto error_close;
    }
  } else {
    drive->gpt.sector_bytes = 512;  /* bytes */
    drive->size = stat.st_size;
  }
  if (drive->size % drive->gpt.sector_bytes) {
    printf("[ERROR] Media size (%llu) is not the multiple of sector size(%d)\n",
           (long long unsigned int)drive->size, drive->gpt.sector_bytes);
    goto error_close;
  }
  drive->gpt.drive_sectors = drive->size / drive->gpt.sector_bytes;

  Load(drive->fd, &drive->gpt.primary_header, GPT_PMBR_SECTOR,
       drive->gpt.sector_bytes, GPT_HEADER_SECTOR);
  Load(drive->fd, &drive->gpt.secondary_header,
       drive->gpt.drive_sectors - GPT_PMBR_SECTOR,
       drive->gpt.sector_bytes, GPT_HEADER_SECTOR);
  Load(drive->fd, &drive->gpt.primary_entries,
       GPT_PMBR_SECTOR + GPT_HEADER_SECTOR,
       drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS);
  Load(drive->fd, &drive->gpt.secondary_entries,
       drive->gpt.drive_sectors - GPT_HEADER_SECTOR - GPT_ENTRIES_SECTORS,
       drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS);

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive->gpt))) {
    printf("[ERROR] GptSanityCheck(): %s\n", GptError(gpt_retval));
    goto error_close;
  }

  drive->inited = 1;

  return CGPT_OK;

error_close:
  close(drive->fd);
  return CGPT_FAILED;
}

int DriveClose(struct drive *drive) {
  if (drive->inited) {
    if (drive->gpt.modified & GPT_MODIFIED_HEADER1)
      assert(CGPT_OK ==
          Save(drive->fd, drive->gpt.primary_header, GPT_PMBR_SECTOR,
               drive->gpt.sector_bytes, GPT_HEADER_SECTOR));
    if (drive->gpt.modified & GPT_MODIFIED_HEADER2)
      assert(CGPT_OK ==
          Save(drive->fd, drive->gpt.secondary_header,
               drive->gpt.drive_sectors - GPT_PMBR_SECTOR,
               drive->gpt.sector_bytes, GPT_HEADER_SECTOR));
    if (drive->gpt.modified & GPT_MODIFIED_ENTRIES1)
      assert(CGPT_OK ==
          Save(drive->fd, drive->gpt.primary_entries,
               GPT_PMBR_SECTOR + GPT_HEADER_SECTOR,
               drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS));
    if (drive->gpt.modified & GPT_MODIFIED_ENTRIES2)
      assert(CGPT_OK ==
          Save(drive->fd, drive->gpt.secondary_entries,
               drive->gpt.drive_sectors - GPT_HEADER_SECTOR -
                                          GPT_ENTRIES_SECTORS,
               drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS));

    close(drive->fd);
  }

  Free(drive->gpt.primary_header);
  drive->gpt.primary_header = 0;
  Free(drive->gpt.primary_entries);
  drive->gpt.primary_entries = 0;
  Free(drive->gpt.secondary_header);
  drive->gpt.secondary_header = 0;
  Free(drive->gpt.secondary_entries);
  drive->gpt.secondary_entries = 0;

  drive->inited = 0;
  return CGPT_OK;
}

int main(int argc, char *argv[]) {
  char *cmd;
  int i;

  progname = argv[0];
  printf("Copyright (c) 2010 The Chromium OS Authors. All rights reserved.\n");
  cmd = argv[optind++];
  for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i) {
    if (cmd && !strcmp(cmds[i].name, cmd))
      return cmds[i].fp(argc, argv);
  }

  Usage(0);
  return CGPT_FAILED;
}
