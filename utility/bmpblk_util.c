// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <lzma.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bmpblk_util.h"
#include "eficompress.h"
#include "vboot_api.h"

// Returns pointer to buffer containing entire file, sets length.
static void *read_entire_file(const char *filename, size_t *length) {
  int fd;
  struct stat sbuf;
  void *ptr;

  *length = 0;                          // just in case

  if (0 != stat(filename, &sbuf)) {
    fprintf(stderr, "Unable to stat %s: %s\n", filename, strerror(errno));
    return 0;
  }

  if (!sbuf.st_size) {
    fprintf(stderr, "File %s is empty\n", filename);
    return 0;
  }

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
    return 0;
  }

  ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (MAP_FAILED == ptr) {
    fprintf(stderr, "Unable to mmap %s: %s\n", filename, strerror(errno));
    close(fd);
    return 0;
  }

  *length = sbuf.st_size;

  close(fd);

  return ptr;
}


// Reclaims buffer from read_entire_file().
static void discard_file(void *ptr, size_t length) {
  munmap(ptr, length);
}

//////////////////////////////////////////////////////////////////////////////

static int require_dir(const char *dirname) {
  struct stat sbuf;

  if (0 == stat(dirname, &sbuf)) {
    // Something's there. Is it a directory?
    if (S_ISDIR(sbuf.st_mode)) {
      return 0;
    }
    fprintf(stderr, "%s already exists and is not a directory\n", dirname);
    return 1;
  }

  // dirname doesn't exist. Try to create it.
  if (ENOENT == errno) {
    if (0 != mkdir(dirname, 0777)) {
      fprintf(stderr, "Unable to create directory %s: %s\n",
              dirname, strerror(errno));
      return 1;
    }
    return 0;
  }

  fprintf(stderr, "Unable to stat %s: %s\n", dirname, strerror(errno));
  return 1;
}



static void *do_efi_decompress(ImageInfo *img) {
  void *ibuf;
  void *sbuf;
  void *obuf;
  uint32_t isize;
  uint32_t ssize;
  uint32_t osize;
  EFI_STATUS r;

  ibuf = (void*)(img + 1);
  isize = img->compressed_size;

  r = EfiGetInfo(ibuf, isize, &osize, &ssize);
  if (EFI_SUCCESS != r) {
    fprintf(stderr, "EfiGetInfo() failed with code %d\n",
            r);
    return 0;
  }

  sbuf = malloc(ssize);
  if (!sbuf) {
    fprintf(stderr, "Can't allocate %d bytes: %s\n",
            ssize,
            strerror(errno));
    return 0;
  }

  obuf = malloc(osize);
  if (!obuf) {
    fprintf(stderr, "Can't allocate %d bytes: %s\n",
            osize,
            strerror(errno));
    free(sbuf);
    return 0;
  }

  r = EfiDecompress(ibuf, isize, obuf, osize, sbuf, ssize);
  if (r != EFI_SUCCESS) {
    fprintf(stderr, "EfiDecompress failed with code %d\n", r);
    free(obuf);
    free(sbuf);
    return 0;
  }

  free(sbuf);
  return obuf;
}



static void *do_lzma_decompress(ImageInfo *img) {
  void *ibuf;
  void *obuf;
  uint32_t isize;
  uint32_t osize;
  lzma_stream stream = LZMA_STREAM_INIT;
  lzma_ret result;

  ibuf = (void*)(img + 1);
  isize = img->compressed_size;
  osize = img->original_size;
  obuf = malloc(osize);
  if (!obuf) {
    fprintf(stderr, "Can't allocate %d bytes: %s\n",
            osize,
            strerror(errno));
    return 0;
  }

  result = lzma_auto_decoder(&stream, -1, 0);
  if (result != LZMA_OK) {
    fprintf(stderr, "Unable to initialize auto decoder (error: %d)!\n",
            result);
    free(obuf);
    return 0;
  }

  stream.next_in = ibuf;
  stream.avail_in = isize;
  stream.next_out = obuf;
  stream.avail_out = osize;
  result = lzma_code(&stream, LZMA_FINISH);
  if (result != LZMA_STREAM_END) {
    fprintf(stderr, "Unalbe to decode data (error: %d)!\n", result);
    free(obuf);
    return 0;
  }
  lzma_end(&stream);
  return obuf;
}



// Show what's inside. If todir is NULL, just print. Otherwise unpack.
int dump_bmpblock(const char *infile, int show_as_yaml,
                  const char *todir, int overwrite) {
  void *ptr, *data_ptr;
  size_t length = 0;
  BmpBlockHeader *hdr;
  ImageInfo *img;
  ScreenLayout *scr;
  int loc_num;
  int screen_num;
  int i;
  int offset;
  int free_data;
  char image_name[80];
  char full_path_name[PATH_MAX];
  int yfd, bfd;
  FILE *yfp = stdout;
  FILE *bfp = stdout;

  ptr = (void *)read_entire_file(infile, &length);
  if (!ptr)
    return 1;

  if (length < sizeof(BmpBlockHeader)) {
    fprintf(stderr, "File %s is too small to be a BMPBLOCK\n", infile);
    discard_file(ptr, length);
    return 1;
  }

  if (0 != memcmp(ptr, BMPBLOCK_SIGNATURE, BMPBLOCK_SIGNATURE_SIZE)) {
    fprintf(stderr, "File %s is not a BMPBLOCK\n", infile);
    discard_file(ptr, length);
    return 1;
  }

  if (todir) {
    // Unpacking everything. Create the output directory if needed.
    if (0 != require_dir(todir)) {
      discard_file(ptr, length);
      return 1;
    }

    // Open yaml output.
    show_as_yaml = 1;

    sprintf(full_path_name, "%s/%s", todir, "config.yaml");
    yfd = open(full_path_name,
               O_WRONLY | O_CREAT | O_TRUNC | (overwrite ? 0 : O_EXCL),
               0666);
    if (yfd < 0) {
      fprintf(stderr, "Unable to open %s: %s\n", full_path_name,
              strerror(errno));
      discard_file(ptr, length);
      return 1;
    }

    yfp = fdopen(yfd, "wb");
    if (!yfp) {
      fprintf(stderr, "Unable to fdopen %s: %s\n", full_path_name,
              strerror(errno));
      close(yfd);
      discard_file(ptr, length);
      return 1;
    }
  }

  hdr = (BmpBlockHeader *)ptr;

  if (!show_as_yaml) {
    printf("%s:\n", infile);
    printf("  version %d.%d\n", hdr->major_version, hdr->minor_version);
    printf("  %d screens\n", hdr->number_of_screenlayouts);
    printf("  %d localizations\n", hdr->number_of_localizations);
    printf("  %d discrete images\n", hdr->number_of_imageinfos);
    discard_file(ptr, length);
    return 0;
  }

  // Write out yaml
  fprintf(yfp, "bmpblock: %d.%d\n", hdr->major_version, hdr->minor_version);
  offset = sizeof(BmpBlockHeader) +
    (sizeof(ScreenLayout) *
     hdr->number_of_localizations *
     hdr->number_of_screenlayouts);
  // FIXME(chromium-os:12134): The bmbblock structure allows each image to be
  // compressed differently, but we haven't provided a way for the yaml file to
  // specify that. Additionally, we allow the yaml file to specify a default
  // compression scheme for all images, but only if that line appears in the
  // yaml file before any images. Accordingly, we'll just check the first image
  // to see if it has any compression, and if it does, we'll write that out as
  // the default. When this bug is fixed, we should just write each image's
  // compression setting separately.
  img = (ImageInfo *)(ptr + offset);
  if (img->compression)
    fprintf(yfp, "compression: %d\n", img->compression);
  fprintf(yfp, "images:\n");
  for(i=0; i<hdr->number_of_imageinfos; i++) {
    img = (ImageInfo *)(ptr + offset);
    if (img->compressed_size) {
      sprintf(image_name, "img_%08x.bmp", offset);
      if (img->tag == TAG_HWID) {
        fprintf(yfp, "  %s: %s  # %dx%d  %d/%d  tag=%d fmt=%d\n",
                RENDER_HWID, image_name,
                img->width, img->height,
                img->compressed_size, img->original_size,
                img->tag, img->format);
      } else if (img->tag == TAG_HWID_RTOL) {
        fprintf(yfp, "  %s: %s  # %dx%d  %d/%d  tag=%d fmt=%d\n",
                RENDER_HWID_RTOL, image_name,
                img->width, img->height,
                img->compressed_size, img->original_size,
                img->tag, img->format);
      } else {
        fprintf(yfp, "  img_%08x: %s  # %dx%d  %d/%d  tag=%d fmt=%d\n",
                offset, image_name,
                img->width, img->height,
                img->compressed_size, img->original_size,
                img->tag, img->format);
      }
      if (todir) {
        sprintf(full_path_name, "%s/%s", todir, image_name);
        bfd = open(full_path_name,
                   O_WRONLY | O_CREAT | O_TRUNC | (overwrite ? 0 : O_EXCL),
                   0666);
        if (bfd < 0) {
          fprintf(stderr, "Unable to open %s: %s\n", full_path_name,
                  strerror(errno));
          fclose(yfp);
          discard_file(ptr, length);
          return 1;
        }
        bfp = fdopen(bfd, "wb");
        if (!bfp) {
          fprintf(stderr, "Unable to fdopen %s: %s\n", full_path_name,
                  strerror(errno));
          close(bfd);
          fclose(yfp);
          discard_file(ptr, length);
          return 1;
        }
        switch(img->compression) {
        case COMPRESS_NONE:
          data_ptr = ptr + offset + sizeof(ImageInfo);
          free_data = 0;
          break;
        case COMPRESS_EFIv1:
          data_ptr = do_efi_decompress(img);
          if (!data_ptr) {
            fclose(bfp);
            fclose(yfp);
            discard_file(ptr, length);
            return 1;
          }
          free_data = 1;
          break;
        case COMPRESS_LZMA1:
          data_ptr = do_lzma_decompress(img);
          if (!data_ptr) {
            fclose(bfp);
            fclose(yfp);
            discard_file(ptr, length);
            return 1;
          }
          free_data = 1;
          break;
        default:
          fprintf(stderr, "Unsupported compression method encountered.\n");
          fclose(bfp);
          fclose(yfp);
          discard_file(ptr, length);
          return 1;
        }
        if (1 != fwrite(data_ptr, img->original_size, 1, bfp)) {
          fprintf(stderr, "Unable to write %s: %s\n", full_path_name,
                  strerror(errno));
          fclose(bfp);
          fclose(yfp);
          discard_file(ptr, length);
          return 1;
        }
        fclose(bfp);
        if (free_data)
          free(data_ptr);
      }
    }
    offset += sizeof(ImageInfo);
    offset += img->compressed_size;
    // 4-byte aligned
    if ((offset & 3) > 0)
      offset = (offset & ~3) + 4;
  }
  fprintf(yfp, "screens:\n");
  for(loc_num = 0;
      loc_num < hdr->number_of_localizations;
      loc_num++) {
    for(screen_num = 0;
        screen_num < hdr->number_of_screenlayouts;
        screen_num++) {
      fprintf(yfp, "  scr_%d_%d:\n", loc_num, screen_num);
      i = loc_num * hdr->number_of_screenlayouts + screen_num;
      offset = sizeof(BmpBlockHeader) + i * sizeof(ScreenLayout);
      scr = (ScreenLayout *)(ptr + offset);
      for(i=0; i<MAX_IMAGE_IN_LAYOUT; i++) {
        if (scr->images[i].image_info_offset) {
          ImageInfo *iptr =
            (ImageInfo *)(ptr + scr->images[i].image_info_offset);
          if (iptr->tag == TAG_HWID) {
            fprintf(yfp, "    - [%d, %d, %s] # tag=%d fmt=%d c=%d %d/%d\n",
                    scr->images[i].x, scr->images[i].y,
                    RENDER_HWID, iptr->tag, iptr->format, iptr->compression,
                    iptr->compressed_size, iptr->original_size);
          } else if (iptr->tag == TAG_HWID_RTOL) {
            fprintf(yfp, "    - [%d, %d, %s] # tag=%d fmt=%d c=%d %d/%d\n",
                    scr->images[i].x, scr->images[i].y,
                    RENDER_HWID_RTOL, iptr->tag,
                    iptr->format, iptr->compression,
                    iptr->compressed_size, iptr->original_size);
          } else {
            fprintf(yfp, "    - [%d, %d, img_%08x]"
                    " # tag=%d fmt=%d c=%d %d/%d\n",
                    scr->images[i].x, scr->images[i].y,
                    scr->images[i].image_info_offset,
                    iptr->tag, iptr->format, iptr->compression,
                    iptr->compressed_size, iptr->original_size);
          }
        }
      }
    }
  }
  fprintf(yfp, "localizations:\n");
  for(loc_num = 0;
      loc_num < hdr->number_of_localizations;
      loc_num++) {
    fprintf(yfp, "  - [");
    for(screen_num = 0;
        screen_num < hdr->number_of_screenlayouts;
        screen_num++) {
      fprintf(yfp, " scr_%d_%d", loc_num, screen_num);
      if (screen_num != hdr->number_of_screenlayouts - 1)
        fprintf(yfp, ",");
    }
    fprintf(yfp, " ]\n");
  }

  if (hdr->locale_string_offset) {
    char *loc_ptr = (char *)ptr + hdr->locale_string_offset;
    char c;
    fprintf(yfp, "locale_index:\n");
    while ((c = *loc_ptr) != '\0') {
      fprintf(yfp, "  - ");
      do {
        fputc(c, yfp);
        loc_ptr++;
      } while((c = *loc_ptr) != '\0');
      loc_ptr++;
      fputc('\n', yfp);
    }
  }

  if (todir)
    fclose(yfp);

  discard_file(ptr, length);

  return 0;
}

