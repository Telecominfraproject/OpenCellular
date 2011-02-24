/**
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
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
#include "nvbctlib.h"
#include "crypto.h"
#include "data_layout.h"
#include "parse.h"
#include "set.h"
#include "context.h"

/*
 * Global data
 */
int enable_debug                = 0;

static int help_only            = 0; // Only print help & exit

/*
 * Function prototypes
 */
int main(int argc, char *argv[]);

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
	printf("    configfile      File with configuration information\n");
	printf("    imagename       Output image name\n");
}

static int
process_command_line(int argc, char *argv[], build_image_context *context)
{
	int arg = 1;

	context->generate_bct = 0;

	while (arg < argc) {
		/* Process the next argument. */
		if (!strcmp(argv[arg], "-h") ||
		    !strcmp(argv[arg], "--help") ||
		    !strcmp(argv[arg], "-?")) {
			help_only = 1;
			usage();
			return 0;
		} else if (!strcmp(argv[arg], "-d") ||
	 	!strcmp(argv[arg], "--debug")) {
			enable_debug = 1;
			arg++;
		} else if (!strcmp(argv[arg], "-gbct")) {
			context->generate_bct = 1;
			arg++;
		} else if (argv[arg][0] == '-') {
			printf("Illegal option %s\n", argv[arg]);
			usage();
			return -EINVAL;
		}
		else
			break; /* Finished with options */
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

	/* Set up the Nvbctlib function pointers. */
	nvbct_lib_get_fns(&(context->bctlib));

	return 0;
}

int
main(int argc, char *argv[])
{
	int e = 0;
	build_image_context context;
	u_int32_t data = 0;

	memset(&context, 0, sizeof(build_image_context));

	/* Process command line arguments. */
	if (process_command_line(argc, argv, &context) != 0)
		return -EINVAL;

	if (help_only)
		return 1;

	e = init_context(&context);
	if (e != 0) {
		printf("context initialization failed.  Aborting.\n");
		return e;
	}

	if (enable_debug) {
		/* Debugging information... */
		e = context.bctlib.get_value(nvbct_lib_id_bct_size,
				&data, context.bct);
		printf("bct size: %d\n", e == 0 ? data : -1);
	}

	/* Open the raw output file. */
	context.raw_file = fopen(context.image_filename,
		                 "w+");
	if (context.raw_file == NULL) {
		printf("Error opening raw file %s.\n",
			context.image_filename);
		goto fail;
	}

	/* Parse & process the contents of the config file. */
	process_config_file(&context);

	/* Generate the new bct file */
	if (context.generate_bct != 0) {
		/* Signing the bct. */
		e = sign_bct(&context, context.bct);
		if (e != 0) 
			printf("Signing BCT failed, error: %d.\n", e);

		fwrite(context.bct, 1, sizeof(nvboot_config_table),
			context.raw_file);
		printf("New BCT file %s has been successfully generated!\n",
			context.image_filename);
		goto fail;
	}

	/* Update the bct file */
	/* Update the add on file */
	e = update_addon_item(&context);
	if ( e!= 0) {
		printf("Write addon item failed, error: %d.\n", e);
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

	return e;
}
