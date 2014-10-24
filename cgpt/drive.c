/* Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "cgpt.h"
#include "fmap.h"

// TODO(namnguyen): Remove RW_UNUSED
#ifdef DEBUG
static const char FMAP_GPT_SECTION[] = "RW_UNUSED";
#else
static const char FMAP_GPT_SECTION[] = "RW_GPT";
#endif

off_t FileSeek(struct drive* drive, off_t offset, int whence) {
  return lseek(drive->fd, offset, whence);
}

ssize_t FileRead(struct drive* drive, void* buf, size_t count) {
  return read(drive->fd, buf, count);
}

ssize_t FileWrite(struct drive* drive, const void* buf, size_t count) {
  return write(drive->fd, buf, count);
}

int FileSync(struct drive* drive) {
  return fsync(drive->fd);
}

int FileClose(struct drive* drive) {
  return close(drive->fd);
}

// Always terminate the buffer after snprintf.
static int tsnprintf(char *buf, size_t size, const char* fmt, ...) {
  if (size == 0) {
    // No space for the null char.
    errno = ENOSPC;
    return -1;
  }
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  if (ret >= 0) {
    buf[size - 1] = '\x00';
  }
  return ret;
}

int FlashInit(struct drive* drive) {
  int return_code = 1;
  char tempdir[] = "/tmp/cgptXXXXXX";
  if (mkdtemp(tempdir) == NULL) {
    Error("Cannot create temp directory for flashrom work.\n");
    return return_code;
  }

  char cmd[256];
  char fmap_name[28];
  tsnprintf(fmap_name, sizeof(fmap_name), "%s/fmap", tempdir);
  tsnprintf(cmd, sizeof(cmd), "/usr/sbin/flashrom -p host -i FMAP:%s -r "
            "> /dev/null 2>&1", fmap_name);
  return_code++;
  if (system(cmd) != 0) {
    Error("Cannot dump FMAP section from flash.\n");
    goto cleanup;
  };

  return_code++;
  int fmap_fd = open(fmap_name, O_RDONLY);
  if (fmap_fd < 0) {
    Error("Cannot open %s.\n", fmap_name);
    goto cleanup;
  }
  // Allocate 4096 bytes. ChromeOS FMAP is usually 2048 bytes.
  return_code++;
  const size_t fmap_alloc_size = 4096;
  uint8_t* fmap = malloc(fmap_alloc_size);
  if (!fmap) {
    Error("Cannot read fmap.\n");
    goto cleanup2;
  }
  return_code++;
  int fmap_size = read(fmap_fd, fmap, fmap_alloc_size);
  if (fmap_size < 0) {
    Error("Cannot read from %s.\n", fmap_name);
    goto cleanup3;
  }

  return_code++;
  FmapAreaHeader* gpt_area;
  if (fmap_find_by_name(fmap, fmap_size, NULL,
                        FMAP_GPT_SECTION, &gpt_area) == NULL) {
    Error("Cannot find GPT section in the FMAP.\n");
    goto cleanup3;
  }

  drive->flash_start = gpt_area->area_offset;
  drive->flash_size = gpt_area->area_size;
  drive->current_position = 0;

  return_code = 0;

cleanup3:
  free(fmap);
cleanup2:
  close(fmap_fd);
cleanup:
  tsnprintf(cmd, sizeof(cmd), "/bin/rm -rf %s", tempdir);
  if (system(cmd)) {
    Warning("Cannot remove temp directory", tempdir);
  }
  return return_code;
}

off_t FlashSeek(struct drive* drive, off_t offset, int whence) {
  off_t new_position;
  switch (whence) {
    case SEEK_SET:
      new_position = offset;
      break;
    case SEEK_CUR:
      new_position = drive->current_position + offset;
      break;
    case SEEK_END:
      new_position = drive->size + offset;
      break;
    default:
      errno = EINVAL;
      return -1;
  }
  if (new_position < 0 || new_position > drive->size) {
    errno = EINVAL;
    return -1;
  }
  drive->current_position = new_position;
  return new_position;
}

// Translate |position| to an address in flash.
// We only use a small area in flash to store the GPT structures. This area is
// identified in FMAP. So the idea is to map |position| from 0 to flash_size to
// the physical position in flash linearly.
// This function returns 0 for success.
static int TranslateToFlash(struct drive* drive, off_t position, size_t count,
                            off_t* translated) {
  if (position < 0 || position + count > drive->flash_size) {
    return -1;
  }
  *translated = position + drive->flash_start;
  return 0;
}

static int CreateLayout(char* file_name, off_t position, size_t count) {
  int fd = mkstemp(file_name);
  if (fd < 0) {
    Error("Cannot create layout file.\n");
    return -1;
  }
  char buf[128];
  tsnprintf(buf, sizeof(buf), "%08X:%08X landmark\n", (unsigned int) position,
            (unsigned int) (position + count - 1));
  int layout_len = strlen(buf);
  int nr_written = write(fd, buf, layout_len);
  close(fd);
  if (nr_written != layout_len) {
    Error("Cannot write out layout for flashrom.\n");
    return -1;
  }

  return 0;
}

ssize_t FlashRead(struct drive* drive, void* buf, size_t count) {
  off_t offset;
  if (TranslateToFlash(drive, drive->current_position, count, &offset) != 0) {
    Error("Cannot translate disk address %08X to SPI address.\n",
          drive->current_position);
    errno = EINVAL;
    return -1;
  }

  char tempdir[] = "/tmp/cgptXXXXXX";
  if (mkdtemp(tempdir) == NULL) {
    Error("Cannot create temp directory for flashrom work.\n");
    errno = EIO;
    return -1;
  }

  int return_value = -1;
  char layout_file[40];
  tsnprintf(layout_file, sizeof(layout_file), "%s/layoutXXXXXX", tempdir);
  if (CreateLayout(layout_file, offset, count) != 0) {
    Error("Cannot create layout file for flashrom.\n");
    goto cleanup;
  }

  char content_file[40];
  tsnprintf(content_file, sizeof(content_file), "%s/contentXXXXXX", tempdir);
  int fd = mkstemp(content_file);
  if (fd < 0) {
    goto cleanup;
  }

  char cmd[256];
  tsnprintf(cmd, sizeof(cmd), "/usr/sbin/flashrom -p host -l %s -i landmark:%s "
            "-r > /dev/null 2>&1", layout_file, content_file);
  if (system(cmd) != 0) {
    Error("Cannot read from SPI flash.\n");
    goto cleanup2;
  }

  return_value = read(fd, buf, count);
  if (return_value != count) {
    Error("Cannot read from retrieved content file.\n");
    return_value = -1;
  } else {
    drive->current_position += return_value;
  }

cleanup2:
  close(fd);
cleanup:
  tsnprintf(cmd, sizeof(cmd), "/bin/rm -rf %s", tempdir);
  if (system(cmd)) {
    Warning("Cannot remove temp directory", tempdir);
  }
  errno = EIO;
  return return_value;
}

ssize_t FlashWrite(struct drive* drive, const void* buf, size_t count) {
  off_t offset;
  if (TranslateToFlash(drive, drive->current_position, count, &offset) != 0) {
    Error("Cannot translate disk address %08X to SPI address.\n",
          drive->current_position);
    errno = EINVAL;
    return -1;
  }

  char tempdir[] = "/tmp/cgptXXXXXX";
  if (mkdtemp(tempdir) == NULL) {
    Error("Cannot create temp directory for flashrom work.\n");
    errno = EIO;
    return -1;
  }

  int return_value = -1;
  char layout_file[40];
  tsnprintf(layout_file, sizeof(layout_file), "%s/layoutXXXXXX", tempdir);
  if (CreateLayout(layout_file, offset, count) != 0) {
    Error("Cannot create layout file for flashrom.\n");
    goto cleanup;
  }

  char content_file[40];
  tsnprintf(content_file, sizeof(content_file), "%s/contentXXXXXX", tempdir);
  int fd = mkstemp(content_file);
  if (fd < 0) {
    goto cleanup;
  }

  return_value = write(fd, buf, count);
  close(fd);
  if (return_value != count) {
    Error("Cannot prepare content file for flashrom.\n");
    return_value = -1;
    goto cleanup;
  }

  char cmd[256];
  // TODO(namnguyen): Add --fast-verify after 428475 is fixed
  tsnprintf(cmd, sizeof(cmd), "/usr/sbin/flashrom -p host -l %s -i landmark:%s "
            "-w > /dev/null 2>&1", layout_file, content_file);
  if (system(cmd) != 0) {
    Error("Cannot write to SPI flash.\n");
    return_value = -1;
  } else {
    drive->current_position += return_value;
  }

cleanup:
  tsnprintf(cmd, sizeof(cmd), "/bin/rm -rf %s", tempdir);
  if (system(cmd)) {
    Warning("Cannot remove temp directory", tempdir);
  }
  errno = EIO;
  return return_value;
}

int FlashSync(struct drive* drive) {
  return 0;
}

int FlashClose(struct drive* drive) {
  return 0;
}
