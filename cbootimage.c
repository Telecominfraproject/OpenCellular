/**
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * cbootimage.c - Implementation of the cbootimage tool.
 */

#include "cbootimage.h"
#include "crypto.h"
#include "data_layout.h"
#include "parse.h"
#include "set.h"
#include "context.h"
#include <getopt.h>

/*
 * Global data
 */
int enable_debug                = 0;

static int help_only            = 0; // Only print help & exit

bct_parse_interface *g_bct_parse_interf;
/*
 * Function prototypes
 */
int main(int argc, char *argv[]);

struct option cbootcmd[] = {
	{"help", 0, NULL, 'h'},
	{"debug", 0, NULL, 'd'},
	{"generate", 1, NULL, 'g'},
	{"tegra", 1, NULL, 't'},
	{0, 0, 0, 0},
};

int
write_image_file(build_image_context *context)
{
	assert(context != NULL);

	return write_block_raw(context);
}

static void
usage(void)
{
	printf("Usage: cbootimage [options] configfile imagename\n");
	printf("    options:\n");
	printf("    -h, --help, -?  Display this message.\n");
	printf("    -d, --debug     Output debugging information.\n");
	printf("    -gbct               Generate the new bct file.\n");
	printf("    [-t20|-t25|-t30]   Select one of the possible\n");
	printf("                       target devices, -t20 if unspecified\n");
	printf("    configfile      File with configuration information\n");
	printf("    imagename       Output image name\n");
}

static int
process_command_line(int argc, char *argv[], build_image_context *context)
{
	int arg = 1;
	int c;

	context->generate_bct = 0;

	g_bct_parse_interf = malloc(sizeof(bct_parse_interface));
	if (g_bct_parse_interf == NULL) {
		printf("Insufficient memory to proceed.\n");
		return -EINVAL;
	}
	/* Make the default interface to t20. */
	t20_get_cbootimage_interf(g_bct_parse_interf);
	context->boot_data_version = NVBOOT_BOOTDATA_VERSION(2, 1);

	while ((c = getopt_long(argc, argv, "hdg:t:", cbootcmd, NULL)) != -1) {
		switch (c) {
		case 'h':
			help_only = 1;
			usage();
			return 0;
		case 'd':
			enable_debug = 1;
			arg++;
			break;
		case 'g':
			if (!strcasecmp("bct", optarg)) {
				context->generate_bct = 1;
				arg++;
			} else {
				printf("Invalid argument!\n");
				usage();
				return -EINVAL;
			}
			break;
		case 't':
			if (!(strcasecmp("20", optarg)
				&& strcasecmp("25", optarg))) {
				/* Assign the interface based on the chip. */
				t20_get_cbootimage_interf(g_bct_parse_interf);
				context->boot_data_version =
					NVBOOT_BOOTDATA_VERSION(2, 1);
				arg++;
			} else if (!(strcasecmp("30", optarg))) {
				t30_get_cbootimage_interf(g_bct_parse_interf);
				context->boot_data_version =
					NVBOOT_BOOTDATA_VERSION(3, 1);
				arg++;
			} else {
				printf("Unsupported chipname!\n");
				usage();
				return -EINVAL;
			}
			break;
		}
	}
	/* Handle file specification errors. */
	switch (argc - arg) {
	case 0:
		printf("Missing configuration and image file names.\n");
		usage();
		return -EINVAL;

	case 1:
		printf("Missing configuration or image file name.\n");
		usage();
		return -EINVAL;

	case 2:
		/* Correct args, so break from the switch statement. */
		break;

	default:
		printf("Too many arguments.\n");
		usage();
		return -EINVAL;
	}

	/* Open the configuration file. */
	context->config_file = fopen(argv[arg], "r");
	if (context->config_file == NULL) {
		printf("Error opening config file.\n");
		return -ENODATA;
	}

	/* Record the output filename */
	context->image_filename = argv[arg + 1];

	return 0;
}

int
main(int argc, char *argv[])
{
	int e = 0;
	build_image_context context;

	memset(&context, 0, sizeof(build_image_context));

	/* Process command line arguments. */
	if (process_command_line(argc, argv, &context) != 0)
		return -EINVAL;

	assert(g_bct_parse_interf != NULL);

	if (help_only)
		return 1;

	g_bct_parse_interf->get_value(token_bct_size,
					&context.bct_size,
					context.bct);

	e = init_context(&context);
	if (e != 0) {
		printf("context initialization failed.  Aborting.\n");
		return e;
	}

	if (enable_debug) {
		/* Debugging information... */
		printf("bct size: %d\n", e == 0 ? context.bct_size : -1);
	}

	/* Open the raw output file. */
	context.raw_file = fopen(context.image_filename,
		                 "w+");
	if (context.raw_file == NULL) {
		printf("Error opening raw file %s.\n",
			context.image_filename);
		goto fail;
	}

	/* first, if we aren't generating the bct, read in config file */
	if (context.generate_bct == 0) {
		process_config_file(&context, 1);
	}
	/* Generate the new bct file */
	else {
		/* Initialize the bct memory */
		init_bct(&context);
		/* Parse & process the contents of the config file. */
		process_config_file(&context, 0);
		/* Update the BCT */
		begin_update(&context);
		/* Signing the bct. */
		e = sign_bct(&context, context.bct);
		if (e != 0) 
			printf("Signing BCT failed, error: %d.\n", e);

		fwrite(context.bct, 1, context.bct_size,
			context.raw_file);
		printf("New BCT file %s has been successfully generated!\n",
			context.image_filename);
		goto fail;
	}

	/* Peform final signing & encryption of bct. */
	e = sign_bct(&context, context.bct);
	if (e != 0) {
		printf("Signing BCT failed, error: %d.\n", e);
		goto fail;
	}
	/* Write the image file. */
	/* The image hasn't been written yet. */
	if (write_image_file(&context) != 0)
		printf("Error writing image file.\n");
	else
		printf("Image file %s has been successfully generated!\n",
				context.image_filename);

 fail:
	/* Close the file(s). */
	if (context.raw_file)
		fclose(context.raw_file);

	/* Clean up memory. */
	cleanup_context(&context);
	if (g_bct_parse_interf) {
		free(g_bct_parse_interf);
		g_bct_parse_interf = NULL;
	}
	return e;
}
