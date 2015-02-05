/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fmap.h"
#include "futility.h"


static const char usage[] = "\n"
	"Usage:  " MYNAME " %s [OPTIONS] FILE AREA:file [AREA:file ...]\n"
	"\n"
	"Replace the contents of specific FMAP areas. This is the complement\n"
	"of " MYNAME " dump_fmap -x FILE AREA [AREA ...]\n"
	"\n"
	"Options:\n"
	"  -o OUTFILE     Write the result to this file, instead of modifying\n"
	"                   the input file. This is safer, since there are no\n"
	"                   safeguards against doing something stupid.\n"
	"\n"
	"Example:\n"
	"\n"
	"  This will clear the RO_VPD area, and scramble VBLOCK_B:\n"
	"\n"
	"  " MYNAME " %s bios.bin RO_VPD:/dev/zero VBLOCK_B:/dev/urandom\n"
	"\n";

static void help_and_quit(const char *prog)
{
	printf(usage, prog, prog);
}

static const struct option long_opts[] = {
	/* name    hasarg *flag  val */
	{NULL,          0, NULL, 0},
};
static char *short_opts = ":o:";


static int copy_to_area(char *file, uint8_t *buf, uint32_t len, char *area)
{
	FILE *fp;
	int retval = 0;
	int n;

	fp = fopen(file, "r");
	if (!fp) {
		fprintf(stderr, "area %s: can't open %s for reading: %s\n",
			area, file, strerror(errno));
		return 1;
	}

	n = fread(buf, 1, len, fp);
	if (n == 0) {
		if (feof(fp))
			fprintf(stderr, "area %s: unexpected EOF on %s\n",
				area, file);
		if (ferror(fp))
			fprintf(stderr, "area %s: can't read from %s: %s\n",
				area, file, strerror(errno));
		retval = 1;
	} else if (n < len) {
		fprintf(stderr, "Warning on area %s: only read %d "
			"(not %d) from %s\n", area, n, len, file);
	}

	if (0 != fclose(fp)) {
		fprintf(stderr, "area %s: error closing %s: %s\n",
			area, file, strerror(errno));
		retval = 1;
	}

	return retval;
}


static int do_load_fmap(int argc, char *argv[])
{
	char *infile = 0;
	char *outfile = 0;
	uint8_t *buf;
	uint32_t len;
	FmapHeader *fmap;
	FmapAreaHeader *ah;
	int errorcnt = 0;
	int fd, i;

	opterr = 0;		/* quiet, you */
	while ((i = getopt_long(argc, argv, short_opts, long_opts, 0)) != -1) {
		switch (i) {
		case 'o':
			outfile = optarg;
			break;
		case '?':
			if (optopt)
				fprintf(stderr, "Unrecognized option: -%c\n",
					optopt);
			else
				fprintf(stderr, "Unrecognized option\n");
			errorcnt++;
			break;
		case ':':
			fprintf(stderr, "Missing argument to -%c\n", optopt);
			errorcnt++;
			break;
		default:
			DIE;
		}
	}

	if (errorcnt) {
		help_and_quit(argv[0]);
		return 1;
	}

	if (argc - optind < 2) {
		fprintf(stderr,
			"You must specify an input file"
			" and at least one AREA:file argument\n");
		help_and_quit(argv[0]);
		return 1;
	}

	infile = argv[optind++];

	/* okay, let's do it ... */
	if (outfile)
		futil_copy_file_or_die(infile, outfile);
	else
		outfile = infile;

	fd = open(outfile, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Can't open %s: %s\n",
			outfile, strerror(errno));
		return 1;
	}

	errorcnt |= futil_map_file(fd, MAP_RW, &buf, &len);
	if (errorcnt)
		goto done_file;

	fmap = fmap_find(buf, len);
	if (!fmap) {
		fprintf(stderr, "Can't find an FMAP in %s\n", infile);
		errorcnt++;
		goto done_map;
	}

	for (i = optind; i < argc; i++) {
		char *a = argv[i];
		char *f = strchr(a, ':');

		if (!f || a == f || *(f+1) == '\0') {
			fprintf(stderr, "argument \"%s\" is bogus\n", a);
			errorcnt++;
			break;
		}
		*f++ = '\0';
		uint8_t *area_buf = fmap_find_by_name(buf, len, fmap, a, &ah);
		if (!area_buf) {
			fprintf(stderr, "Can't find area \"%s\" in FMAP\n", a);
			errorcnt++;
			break;
		}

		if (0 != copy_to_area(f, area_buf, ah->area_size, a)) {
			errorcnt++;
			break;
		}
	}

done_map:
	errorcnt |= futil_unmap_file(fd, 1, buf, len);

done_file:

	if (0 != close(fd)) {
		fprintf(stderr, "Error closing %s: %s\n",
			outfile, strerror(errno));
		errorcnt++;
	}

	return !!errorcnt;
}

DECLARE_FUTIL_COMMAND(load_fmap, do_load_fmap,
		      VBOOT_VERSION_ALL,
		      "Replace the contents of specified FMAP areas",
		      help_and_quit);
