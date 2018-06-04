/* Copyright 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Exports the kernel commandline from a given partition/image.
 */

#include <getopt.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "futility.h"
#include "vboot_host.h"

enum {
	OPT_KLOADADDR = 1000,
	OPT_HELP,
};

static const struct option long_opts[] = {
	{"kloadaddr", 1, NULL, OPT_KLOADADDR},
	{"help", 0, 0, OPT_HELP},
	{NULL, 0, NULL, 0}
};

/* Print help and return error */
static void print_help(int argc, char *argv[])
{
	printf("\nUsage:  " MYNAME " %s [--kloadaddr ADDRESS] "
	       "KERNEL_PARTITION\n\n", argv[0]);
}

static int do_dump_kern_cfg(int argc, char *argv[])
{
	char *infile = NULL;
	char *config = NULL;
	uint64_t kernel_body_load_address = USE_PREAMBLE_LOAD_ADDR;
	int parse_error = 0;
	char *e;
	int i;

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

		case OPT_KLOADADDR:
			kernel_body_load_address = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --kloadaddr\n");
				parse_error = 1;
			}
			break;

		case OPT_HELP:
			print_help(argc, argv);
			return 0;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Expected argument after options\n");
		parse_error = 1;
	} else
		infile = argv[optind];

	if (parse_error) {
		print_help(argc, argv);
		return 1;
	}

	if (!infile || !*infile) {
		fprintf(stderr, "Must specify filename\n");
		return 1;
	}

	config = FindKernelConfig(infile, kernel_body_load_address);
	if (!config)
		return 1;

	printf("%s", config);

	free(config);
	return 0;
}

DECLARE_FUTIL_COMMAND(dump_kernel_config, do_dump_kern_cfg, VBOOT_VERSION_ALL,
		      "Prints the kernel command line");

