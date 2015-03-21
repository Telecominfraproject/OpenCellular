/*
 * Copyright (c) 2012-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

/*
 * cbootimage.c - Implementation of the cbootimage tool.
 */

#include <strings.h>
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
int enable_debug;
static int help_only; // Only print help & exit
cbootimage_soc_config * g_soc_config;

/*
 * Function prototypes
 */
int main(int argc, char *argv[]);

struct option cbootcmd[] = {
	{"help", 0, NULL, 'h'},
	{"debug", 0, NULL, 'd'},
	{"generate", 1, NULL, 'g'},
	{"tegra", 1, NULL, 't'},
	{"odmdata", 1, NULL, 'o'},
	{"soc", 1, NULL, 's'},
	{"update", 0, NULL, 'u'},
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
	printf("Usage: cbootimage [options] configfile [inputimage] outputimage\n");
	printf("    options:\n");
	printf("    -h, --help, -?        Display this message.\n");
	printf("    -d, --debug           Output debugging information.\n");
	printf("    -gbct                 Generate the new bct file.\n");
	printf("    -o<ODM_DATA>          Specify the odm_data(in hex).\n");
	printf("    -t|--tegra NN         Select target device. Must be one of:\n");
	printf("                          20, 30, 114, 124, 132, 210.\n");
	printf("                          Default: 20.\n");
	printf("    -s|--soc tegraNN      Select target device. Must be one of:\n");
	printf("                          tegra20, tegra30, tegra114, tegra124,\n");
	printf("                          tegra132, tegra210.\n");
	printf("                          Default: tegra20.\n");
	printf("    -u|--update           Copy input image data and update bct\n");
	printf("                          configs into new image file.\n");
	printf("                          This feature is only for tegra114/124.\n");
	printf("    configfile            File with configuration information\n");
	printf("    inputimage            Input image name. This is required\n");
	printf("                          if -u|--update option is used.\n");
	printf("    outputimage           Output image name\n");
}

static int
process_command_line(int argc, char *argv[], build_image_context *context)
{
	int c, num_filenames = 2;

	context->generate_bct = 0;

	while ((c = getopt_long(argc, argv, "hdg:t:o:s:u", cbootcmd, NULL)) != -1) {
		switch (c) {
		case 'h':
			help_only = 1;
			usage();
			return 0;
		case 'd':
			enable_debug = 1;
			break;
		case 'g':
			if (!strcasecmp("bct", optarg)) {
				context->generate_bct = 1;
			} else {
				printf("Invalid argument!\n");
				usage();
				return -EINVAL;
			}
			break;
		case 's':
			if (strncmp("tegra", optarg, 5)) {
				printf("Unsupported chipname!\n");
				usage();
				return -EINVAL;
			}
			optarg += 5;
			/* Deliberate fall-through */
		case 't':
			/* Assign the soc_config based on the chip. */
			if (!strcasecmp("20", optarg)) {
				t20_get_soc_config(context, &g_soc_config);
			} else if (!strcasecmp("30", optarg)) {
				t30_get_soc_config(context, &g_soc_config);
			} else if (!strcasecmp("114", optarg)) {
				t114_get_soc_config(context, &g_soc_config);
			} else if (!strcasecmp("124", optarg)) {
				t124_get_soc_config(context, &g_soc_config);
			} else if (!strcasecmp("132", optarg)) {
				t132_get_soc_config(context, &g_soc_config);
			} else if (!strcasecmp("210", optarg)) {
				t210_get_soc_config(context, &g_soc_config);
			} else {
				printf("Unsupported chipname!\n");
				usage();
				return -EINVAL;
			}
			break;
		case 'o':
			context->odm_data = strtoul(optarg, NULL, 16);
			break;
		case 'u':
			context->update_image = 1;
			num_filenames = 3;
			break;
		}
	}

	if (argc - optind != num_filenames) {
		printf("Missing configuration and/or image file name.\n");
		usage();
		return -EINVAL;
	}

	/* If SoC is not specified, make the default soc_config to t20. */
	if (!context->boot_data_version)
		t20_get_soc_config(context, &g_soc_config);

	/* Open the configuration file. */
	context->config_file = fopen(argv[optind++], "r");
	if (context->config_file == NULL) {
		printf("Error opening config file.\n");
		return -ENODATA;
	}

	/* Record the input image filename if update_image is necessary */
	if (context->update_image)
	{
		if (context->boot_data_version != BOOTDATA_VERSION_T114 &&
			context->boot_data_version != BOOTDATA_VERSION_T124) {
			printf("Update image feature is only for Tegra114 and Tegra124.\n");
			return -EINVAL;
		}

		context->input_image_filename = argv[optind++];
	}

	/* Record the output filename */
	context->output_image_filename = argv[optind++];

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

	if (help_only)
		return 1;

	assert(g_soc_config != NULL);

	g_soc_config->get_value(token_bct_size,
					&context.bct_size,
					context.bct);

	e = init_context(&context);
	if (e != 0) {
		printf("context initialization failed.  Aborting.\n");
		return e;
	}

	if (enable_debug) {
		/* Debugging information... */
		printf("bct size: %d\n", context.bct_size);
	}

	/* Open the raw output file. */
	context.raw_file = fopen(context.output_image_filename, "w+");
	if (context.raw_file == NULL) {
		printf("Error opening raw file %s.\n",
			context.output_image_filename);
		goto fail;
	}

	/* Read the bct data from image if bct configs needs to be updated */
	if (context.update_image) {
		u_int32_t offset = 0, bct_size, actual_size;
		u_int8_t *data_block;
		struct stat stats;

		if (stat(context.input_image_filename, &stats) != 0) {
			printf("Error: Unable to query info on input file path %s\n",
			context.input_image_filename);
			goto fail;
		}

		/* Get BCT_SIZE from input image file  */
		bct_size = get_bct_size_from_image(&context);
		if (!bct_size) {
			printf("Error: Invalid input image file %s\n",
			context.input_image_filename);
			goto fail;
		}

		while (stats.st_size > offset) {
			/* Read a block of data into memory */
			if (read_from_image(context.input_image_filename, offset, bct_size,
					&data_block, &actual_size, file_type_bct)) {
				printf("Error reading image file %s.\n", context.input_image_filename);
				goto fail;
			}

			/* Check if memory data is valid BCT */
			context.bct = data_block;
			if (data_is_valid_bct(&context)) {
				fseek(context.config_file, 0, SEEK_SET);
				process_config_file(&context, 0);
				e = sign_bct(&context, context.bct);
				if (e != 0) {
					printf("Signing BCT failed, error: %d.\n", e);
					goto fail;
				}
			}

			/* Write the block of data to file */
			if (actual_size != write_data_block(context.raw_file, offset, actual_size, data_block)) {
				printf("Error writing image file %s.\n", context.output_image_filename);
				goto fail;
			}

			offset += bct_size;
		}

		printf("Image file %s has been successfully generated!\n",
			context.output_image_filename);
		goto fail;
	}
	/* If we aren't generating the bct, read in config file */
	else if (context.generate_bct == 0)
		process_config_file(&context, 1);
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
			context.output_image_filename);
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
				context.output_image_filename);

 fail:
	/* Close the file(s). */
	if (context.raw_file)
		fclose(context.raw_file);

	/* Clean up memory. */
	cleanup_context(&context);
	return e;
}
