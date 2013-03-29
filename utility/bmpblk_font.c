// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bmpblk_font.h"
#include "image_types.h"
#include "vboot_api.h"

static char *progname;

static void error(const char *fmt, ...)
{
    va_list args;
    va_start( args, fmt );
    fprintf(stderr, "%s: ", progname);
    vfprintf( stderr, fmt, args );
    va_end( args );
}
#define fatal(args...) do { error(args); exit(1); } while(0)


/* Command line options */
enum {
  OPT_OUTFILE = 1000,
};

#define DEFAULT_OUTFILE "font.bin"


static struct option long_opts[] = {
  {"outfile", 1, 0,                   OPT_OUTFILE             },
  {NULL, 0, 0, 0}
};


/* Print help and return error */
static void HelpAndDie(void) {
  fprintf(stderr,
          "\n"
          "%s - Create a vboot fontfile from a set of BMP files.\n"
          "\n"
          "Usage:  %s [OPTIONS] BMPFILE [BMPFILE...]\n"
          "\n"
          "Each BMP file must match *_HEX.bmp, where HEX is the hexadecimal\n"
          "representation of the character that the file displays. The images\n"
          "will be encoded in the given order. Typically the first image is\n"
          "reused to represent any missing characters.\n"
          "\n"
          "OPTIONS are:\n"
          "  --outfile <filename>      Output file (default is %s)\n"
          "\n", progname, progname, DEFAULT_OUTFILE);
  exit(1);
}

//////////////////////////////////////////////////////////////////////////////

// Returns pointer to buffer containing entire file, sets length.
static void *read_entire_file(const char *filename, size_t *length) {
  int fd;
  struct stat sbuf;
  void *ptr;

  *length = 0;                          // just in case

  if (0 != stat(filename, &sbuf)) {
    error("Unable to stat %s: %s\n", filename, strerror(errno));
    return 0;
  }

  if (!sbuf.st_size) {
    error("File %s is empty\n", filename);
    return 0;
  }

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    error("Unable to open %s: %s\n", filename, strerror(errno));
    return 0;
  }

  ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (MAP_FAILED == ptr) {
    error("Unable to mmap %s: %s\n", filename, strerror(errno));
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



int main(int argc, char* argv[]) {
  char* outfile = DEFAULT_OUTFILE;
  int numimages = 0;
  int parse_error = 0;
  int i;
  FILE *ofp;
  FontArrayHeader header;
  FontArrayEntryHeader entry;

  progname = strrchr(argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  while ((i = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
    switch (i) {
      case OPT_OUTFILE:
        outfile = optarg;
        break;

    default:
        /* Unhandled option */
        printf("Unknown option\n");
        parse_error = 1;
        break;
    }
  }

  numimages = argc - optind;

  if (parse_error || numimages < 1)
    HelpAndDie();

  printf("outfile is %s\n", outfile);
  printf("numimages is %d\n", numimages);

  ofp = fopen(outfile, "wb");
  if (!ofp)
    fatal("Unable to open %s: %s\n", outfile, strerror(errno));

  memcpy(&header.signature, FONT_SIGNATURE, FONT_SIGNATURE_SIZE);
  header.num_entries = numimages;
  if (1 != fwrite(&header, sizeof(header), 1, ofp)) {
    error("Can't write header to %s: %s\n", outfile, strerror(errno));
    goto bad1;
  }

  for(i=0; i<numimages; i++) {
    char *imgfile = argv[optind+i];
    char *s;
    uint32_t ascii;
    void *imgdata = 0;
    size_t imgsize, filesize, diff;

    s = strrchr(imgfile, '_');
    if (!s || 1 != sscanf(s, "_%x.bmp", &ascii)) { // This is not foolproof.
      error("Unable to parse the character from filename %s\n", imgfile);
      goto bad1;
    }

    imgdata = read_entire_file(imgfile, &imgsize);
    if (!imgdata)
      goto bad1;

    if (FORMAT_BMP != identify_image_type(imgdata, imgsize, &entry.info)) {
      error("%s does not contain a valid BMP image\n", imgfile);
      goto bad1;
    }

    // Pad the image to align it on a 4-byte boundary.
    filesize = imgsize;
    if (imgsize % 4)
      filesize = ((imgsize + 4) / 4) * 4;
    diff = filesize - imgsize;

    entry.ascii = ascii;
    entry.info.tag = TAG_NONE;
    entry.info.compression = COMPRESS_NONE; // we'll compress it all later
    entry.info.original_size = filesize;
    entry.info.compressed_size = filesize;

    printf("%s => 0x%x %dx%d\n", imgfile, entry.ascii,
           entry.info.width, entry.info.height);

    if (1 != fwrite(&entry, sizeof(entry), 1, ofp)) {
      error("Can't write entry to %s: %s\n", outfile, strerror(errno));
      goto bad1;
    }
    if (1 != fwrite(imgdata, imgsize, 1, ofp)) {
      error("Can't write image to %s: %s\n", outfile, strerror(errno));
      goto bad1;
    }
    if (diff && 1 != fwrite("\0\0\0\0\0\0\0\0", diff, 1, ofp)) {
      error("Can't write padding to %s: %s\n", outfile, strerror(errno));
      goto bad1;
    }


    discard_file(imgdata, imgsize);
  }

  fclose(ofp);
  return 0;

bad1:
  fclose(ofp);
  error("Aborting\n");
  (void) unlink(outfile);
  exit(1);
}
