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
 * parse.c - Parsing support for the cbootimage tool
 */

/*
 * TODO / Notes
 * - Add doxygen commentary
 * - Do we have endian issues to deal with?
 * - Add support for device configuration data
 * - Add support for bad blocks
 * - Add support for different allocation modes/strategies
 * - Add support for multiple BCTs in journal block
 * - Add support for other missing features.
 */

#include "parse.h"
#include "cbootimage.h"
#include "data_layout.h"
#include "crypto.h"
#include "set.h"

/*
 * Function prototypes
 *
 * ParseXXX() parses XXX in the input
 * SetXXX() sets state based on the parsing results but does not perform
 *      any parsing of its own
 * A ParseXXX() function may call other parse functions and set functions.
 * A SetXXX() function may not call any parseing functions.
 */

static char *parse_u32(char *statement, u_int32_t *val);
static char *parse_u8(char *statement, u_int32_t *val);
static char *parse_filename(char *statement, char *name, int chars_remaining);
static char *parse_enum(build_image_context *context,
			char *statement,
			enum_item *table,
			u_int32_t *val);
static char
*parse_field_name(char *rest, field_item *field_table, field_item **field);
static char
*parse_field_value(build_image_context *context, 
			char *rest,
			field_item *field,
			u_int32_t *value);
static int
parse_array(build_image_context *context, parse_token token, char *rest);
static int
parse_bootloader(build_image_context *context, parse_token token, char *rest);
static int
parse_value_u32(build_image_context *context, parse_token token, char *rest);
static int
parse_bct_file(build_image_context *context, parse_token token, char *rest);
static int
parse_addon(build_image_context *context, parse_token token, char *rest);
static char *parse_string(char *statement, char *uname, int chars_remaining);
static char
*parse_end_state(char *statement, char *uname, int chars_remaining);
static int
parse_dev_param(build_image_context *context, parse_token token, char *rest);

static int process_statement(build_image_context *context, char *statement);

static enum_item s_devtype_table[] = 
{
	{ "NvBootDevType_Sdmmc", nvbct_lib_id_dev_type_sdmmc },
	{ "NvBootDevType_Spi", nvbct_lib_id_dev_type_spi },
	{ "Sdmmc", nvbct_lib_id_dev_type_sdmmc },
	{ "Spi", nvbct_lib_id_dev_type_spi },

	{ NULL, 0 }
};

static enum_item s_sdmmc_data_width_table[] =
{
	{
	  "NvBootSdmmcDataWidth_4Bit",
	  nvbct_lib_id_sdmmc_data_width_4bit
	},
	{
	  "NvBootSdmmcDataWidth_8Bit",
	  nvbct_lib_id_sdmmc_data_width_8bit
	},
	{ "4Bit", nvbct_lib_id_sdmmc_data_width_4bit },
	{ "8Bit", nvbct_lib_id_sdmmc_data_width_8bit },
	{ NULL, 0 }
};

static enum_item s_spi_clock_source_table[] = 
{
	{
	    "NvBootSpiClockSource_PllPOut0",
	    nvbct_lib_id_spi_clock_source_pllp_out0
	},
	{
	    "NvBootSpiClockSource_PllCOut0",
	    nvbct_lib_id_spi_clock_source_pllc_out0
	},
	{
	    "NvBootSpiClockSource_PllMOut0",
	    nvbct_lib_id_spi_clock_source_pllm_out0
	},
	{
	    "NvBootSpiClockSource_ClockM",
	    nvbct_lib_id_spi_clock_source_clockm
	},

	{ "ClockSource_PllPOut0", nvbct_lib_id_spi_clock_source_pllp_out0 },
	{ "ClockSource_PllCOut0", nvbct_lib_id_spi_clock_source_pllc_out0 },
	{ "ClockSource_PllMOut0", nvbct_lib_id_spi_clock_source_pllm_out0 },
	{ "ClockSource_ClockM",   nvbct_lib_id_spi_clock_source_clockm },


	{ "PllPOut0", nvbct_lib_id_spi_clock_source_pllp_out0 },
	{ "PllCOut0", nvbct_lib_id_spi_clock_source_pllc_out0 },
	{ "PllMOut0", nvbct_lib_id_spi_clock_source_pllm_out0 },
	{ "ClockM",   nvbct_lib_id_spi_clock_source_clockm },

	{ NULL, 0 }
};

static field_item s_sdmmc_table[] = 
{
	{ "ClockDivider", token_clock_divider, field_type_u32,  NULL },
	{ "DataWidth", token_data_width,
		field_type_enum,  s_sdmmc_data_width_table },
	{ "MaxPowerClassSupported", token_max_power_class_supported,
		field_type_u32,  NULL },

	{ NULL, 0, 0, NULL }
};

static field_item s_spiflash_table[] = 
{
	{ "ReadCommandTypeFast", token_read_command_type_fast,
		field_type_u8,  NULL },
	{ "ClockDivider", token_clock_divider, field_type_u8,  NULL },
	{ "ClockSource", token_clock_source,
		field_type_enum,  s_spi_clock_source_table },

	{ NULL, 0, 0, NULL }
};

static parse_subfield_item s_device_type_table[] =
{
	{ "SdmmcParams.", token_sdmmc_params,
		s_sdmmc_table, set_sdmmc_param },
	{ "SpiFlashParams.", token_spiflash_params,
		s_spiflash_table, set_spiflash_param },

	{ NULL, 0, NULL }
};

static parse_item s_top_level_items[] =
{
	{ "Bctfile=",	token_bct_file,		parse_bct_file },
	{ "Attribute=",	token_attribute,		parse_value_u32 },
	{ "Attribute[",	token_attribute,		parse_array },
	{ "PageSize=",	token_page_size,		parse_value_u32 },
	{ "BlockSize=",	token_block_size,		parse_value_u32 },
	{ "PartitionSize=",	token_partition_size,	parse_value_u32 },
	{ "DevType[",	token_dev_type,		parse_array },
	{ "DeviceParam[",	token_dev_param,		parse_dev_param },
	{ "BootLoader=",	token_bootloader,		parse_bootloader },
	{ "Redundancy=",	token_redundancy,		parse_value_u32 },
	{ "Version=",	token_version,		parse_value_u32 },
	{ "AddOn[",	token_addon,		parse_addon },
	{ NULL, 0, NULL } /* Must be last */
};

/* Macro to simplify parser code a bit. */
#define PARSE_COMMA(x) if (*rest != ',') return (x); rest++

/* This parsing code was initially borrowed from nvcamera_config_parse.c. */
/* Returns the address of the character after the parsed data. */
static char *
parse_u32(char *statement, u_int32_t *val)
{
	u_int32_t value = 0;

	while (*statement=='0') {
		statement++;
	}

	if (*statement=='x' || *statement=='X') {
		statement++;
		while (((*statement >= '0') && (*statement <= '9')) ||
		((*statement >= 'a') && (*statement <= 'f')) ||
		((*statement >= 'A') && (*statement <= 'F'))) {
			value *= 16;
			if ((*statement >= '0') && (*statement <= '9')) {
				value += (*statement - '0');
			} else if ((*statement >= 'A') &&
					(*statement <= 'F')) {
				value += ((*statement - 'A')+10);
			} else {
				value += ((*statement - 'a')+10);
			}
				statement++;
		}
	} else {
		while (*statement >= '0' && *statement <= '9') {
			value = value*10 + (*statement - '0');
			statement++;
		}
	}
	*val = value;
	return statement;
}

char *
parse_u8(char *statement, u_int32_t *val)
{
	char *retval;

	retval = parse_u32(statement, val);

	if (*val > 0xff) {
		printf("Warning: Parsed 8-bit value that exceeded 8-bits.\n");
		printf("         Parsed value = %d. Remaining text = %s\n",
			 *val, retval);
	}

	return retval;
}


/* This parsing code was initially borrowed from nvcamera_config_parse.c. */
/* Returns the address of the character after the parsed data. */
static char *
parse_filename(char *statement, char *name, int chars_remaining)
{
	while (((*statement >= '0') && (*statement <= '9')) ||
		((*statement >= 'a') && (*statement <= 'z')) ||
		((*statement >= 'A') && (*statement <= 'Z')) ||
		(*statement == '\\') ||
		(*statement == '/' ) ||
		(*statement == '~' ) ||
		(*statement == '_' ) ||
		(*statement == '-' ) ||
		(*statement == '+' ) ||
		(*statement == ':' ) ||
		(*statement == '.' )) {
		/* Check if the filename buffer is out of space, preserving one
		  * character to null terminate the string.
		  */
		chars_remaining--;

		if (chars_remaining < 1)
			return NULL;
		*name++ = *statement++;
	}

	/* Null terminate the filename. */
	*name = '\0';

	return statement;
}

static char
*parse_field_name(char *rest, field_item *field_table, field_item **field)
{
	u_int32_t i;
	u_int32_t field_name_len = 0;

	assert(field_table != NULL);
	assert(rest != NULL);
	assert(field != NULL);

	while(*(rest + field_name_len) != '=')
		field_name_len++;

	/* Parse the field name. */
	for (i = 0; field_table[i].name != NULL; i++) {
		if ((strlen(field_table[i].name) == field_name_len) &&
			!strncmp(field_table[i].name,
			rest,
			field_name_len)) {

			*field = &(field_table[i]);
			rest = rest + field_name_len;
			return rest;
		}
	}

	/* Field wasn't found or a parse error occurred. */
	return NULL;
}

static char
*parse_field_value(build_image_context *context, 
			char *rest,
			field_item *field,
			u_int32_t *value)
{
	assert(rest != NULL);
	assert(field != NULL);
	assert((field->type != field_type_enum)
		|| (field->enum_table != NULL));

	switch (field->type) {
	case field_type_enum:
		rest = parse_enum(context, rest, field->enum_table, value);
		break;

	case field_type_u32:
		rest = parse_u32(rest, value);
		break;

	case field_type_u8:
		rest = parse_u8(rest, value);
		break;

	default:
		printf("Unexpected field type %d at line %d\n",
			field->type, __LINE__);
		rest = NULL;
		break;
	}

	return rest;
}

static char *
parse_enum(build_image_context *context,
		char *statement,
		enum_item *table,
		u_int32_t *val)
{
	int i;
	char *rest;
	int e;

	for (i = 0; table[i].name != NULL; i++) {
		if (!strncmp(table[i].name, statement,
			strlen(table[i].name))) {
		/* Lookup the correct value for the token. */
		e = context->bctlib.get_value(table[i].value,
				val, context->bct);
		if (e) {
			printf("Error looking up token %d.\n", table[i].value);
			printf("\"%s\" is not valid for this chip.\n",
					table[i].name);
			*val = -1;
		}

		rest = statement + strlen(table[i].name);
		return rest;
		}
	}
	return parse_u32(statement, val);

}
/*
 * parse_bootloader(): Processes commands to set a bootloader.
 */
static int parse_bootloader(build_image_context *context,
			parse_token token,
			char *rest)
{
	char filename[MAX_BUFFER];
	char e_state[MAX_STR_LEN];
	u_int32_t load_addr;
	u_int32_t entry_point;

	assert(context != NULL);
	assert(rest != NULL);

	if (context->generate_bct != 0)
		return 0;
	/* Parse the file name. */
	rest = parse_filename(rest, filename, MAX_BUFFER);
	if (rest == NULL)
		return 1;

	PARSE_COMMA(1);

	/* Parse the load address. */
	rest = parse_u32(rest, &load_addr);
	if (rest == NULL)
		return 1;

	PARSE_COMMA(1);

	/* Parse the entry point. */
	rest = parse_u32(rest, &entry_point);
	if (rest == NULL)
		return 1;

	PARSE_COMMA(1);

	/* Parse the end state. */
	rest = parse_end_state(rest, e_state, MAX_STR_LEN);
	if (rest == NULL)
		return 1;
	if (strncmp(e_state, "Complete", strlen("Complete")))
		return 1;

	/* Parsing has finished - set the bootloader */
	return set_bootloader(context, filename, load_addr, entry_point);
}

/*
 * parse_array(): Processes commands to set an array value.
 */
static int
parse_array(build_image_context *context, parse_token token, char *rest)
{
	u_int32_t index;
	u_int32_t value;

	assert(context != NULL);
	assert(rest != NULL);

	/* Parse the index. */
	rest = parse_u32(rest, &index);
	if (rest == NULL)
		return 1;

	/* Parse the closing bracket. */
	if (*rest != ']')
		return 1;
	rest++;

	/* Parse the equals sign.*/
	if (*rest != '=')
		return 1;
	rest++;

	/* Parse the value based on the field table. */
	switch(token) {
	case token_attribute:
		rest = parse_u32(rest, &value);
		break;
	case token_dev_type:
		rest = parse_enum(context, rest, s_devtype_table, &value);
		break;

	default:
	/* Unknown token */
		return 1;
	}

	if (rest == NULL)
		return 1;

	/* Store the result. */
	return context_set_array(context, index, token, value);
}

/*
 * parse_value_u32(): General handler for setting u_int32_t values in config files.
 */
static int parse_value_u32(build_image_context *context,
			parse_token token,
			char *rest)
{
	u_int32_t value;

	assert(context != NULL);
	assert(rest != NULL);

	rest = parse_u32(rest, &value);
	if (rest == NULL)
		return 1;

	return context_set_value(context, token, value);
}

static int
parse_bct_file(build_image_context *context, parse_token token, char *rest)
{
	char   filename[MAX_BUFFER];

	assert(context != NULL);
	assert(rest != NULL);

	/* Parse the file name. */
	rest = parse_filename(rest, filename, MAX_BUFFER);
	if (rest == NULL)
		return 1;

	/* Parsing has finished - set the bctfile */
	context->bct_filename = filename;
	/* Read the bct file to buffer */
	read_bct_file(context);
	return 0;
}

static char *
parse_string(char *statement, char *uname, int chars_remaining)
{
	memset(uname, 0, chars_remaining);
	while (((*statement >= '0') && (*statement <= '9')) ||
		((*statement >= 'A') && (*statement <= 'Z')) ||
		((*statement >= 'a') && (*statement <= 'z'))) {

		*uname++ = *statement++;
		if (--chars_remaining < 0) {
			printf("String length beyond the boundary!!!");
			return NULL;
		}
	}
	*uname = '\0';
	return statement;
}

static char *
parse_end_state(char *statement, char *uname, int chars_remaining)
{
	while (((*statement >= 'a') && (*statement <= 'z')) ||
		((*statement >= 'A') && (*statement <= 'Z'))) {

		*uname++ = *statement++;
		if (--chars_remaining < 0)
			return NULL;
	}
	*uname = '\0';
	return statement;
}


/* Parse the addon component */
static int
parse_addon(build_image_context *context, parse_token token, char *rest)
{
	char filename[MAX_BUFFER];
	char u_name[4];
	char e_state[MAX_STR_LEN];
	u_int32_t index;
	u_int32_t item_attr;
	u_int32_t others;
	char other_str[MAX_STR_LEN];

	assert(context != NULL);
	assert(rest != NULL);

	/* Parse the index. */
	rest = parse_u32(rest, &index);
	if (rest == NULL)
		return 1;

	/* Parse the closing bracket. */
	if (*rest != ']')
		return 1;
	rest++;

	/* Parse the equals sign.*/
	if (*rest != '=')
		return 1;
	rest++;

	rest = parse_filename(rest, filename, MAX_BUFFER);
	if (rest == NULL)
		return 1;
	if (set_addon_filename(context, filename, index) != 0)
		return 1;

	PARSE_COMMA(1);

	rest = parse_string(rest, u_name, 3);
	if (rest == NULL) {
		printf("Unique name should be 3 characters.\n");
		return 1;
	}
	if (set_unique_name(context, u_name, index) != 0)
		return 1;

	PARSE_COMMA(1);

	rest = parse_u32(rest, &item_attr);
	if (rest == NULL)
		return 1;
	if (set_addon_attr(context, item_attr, index) != 0)
		return 1;

	PARSE_COMMA(1);

	if (*rest == '0' && (*(rest + 1) == 'x' ||*(rest + 1) == 'X')) {
		rest = parse_u32(rest, &others);
		if (set_other_field(context, NULL, others, index) != 0)
			return 1;
	} else {
		rest = parse_string(rest, other_str, 16);
		if (set_other_field(context, other_str, 0, index) != 0)
			return 1;
	}
	if (rest == NULL)
		return 1;

	PARSE_COMMA(1);

	rest = parse_end_state(rest, e_state, MAX_STR_LEN);
	if (rest == NULL)
		return 1;
	if (strncmp(e_state, "Complete", strlen("Complete")))
		return 1;
	return 0;
}

static int
parse_dev_param(build_image_context *context, parse_token token, char *rest)
{
	u_int32_t i;
	u_int32_t value;
	field_item *field;
	u_int32_t index;
	parse_subfield_item *device_item = NULL;
    
	assert(context != NULL);
	assert(rest != NULL);

	/* Parse the index. */
	rest = parse_u32(rest, &index);
	if (rest == NULL)
		return 1;

	/* Parse the closing bracket. */
	if (*rest != ']')
		return 1;
	rest++;

	/* Parse the following '.' */
	if (*rest != '.')
		return 1;
	rest++;

	/* Parse the device name. */
	for (i = 0; s_device_type_table[i].prefix != NULL; i++) {
		if (!strncmp(s_device_type_table[i].prefix,
			rest, strlen(s_device_type_table[i].prefix))) {

			device_item = &(s_device_type_table[i]);
			rest = rest + strlen(s_device_type_table[i].prefix);

			/* Parse the field name. */
			rest = parse_field_name(rest,
				s_device_type_table[i].field_table,
				&field);
			if (rest == NULL)
				return 1;

			/* Parse the equals sign.*/
			if (*rest != '=')
				return 1;
			rest++;

			/* Parse the value based on the field table. */
			rest = parse_field_value(context, rest, field, &value);
			if (rest == NULL)
				return 1;
			return device_item->process(context,
						index, field->token, value);
		}
	}

    return 1;

}
/* Return 0 on success, 1 on error */
static int
process_statement(build_image_context *context, char *statement)
{
	int i;
	char *rest;

	for (i = 0; s_top_level_items[i].prefix != NULL; i++) {
		if (!strncmp(s_top_level_items[i].prefix, statement,
			strlen(s_top_level_items[i].prefix))) {
			rest = statement + strlen(s_top_level_items[i].prefix);

			return s_top_level_items[i].process(context,
						s_top_level_items[i].token,
						rest);
		}
	}

	/* If this point was reached, there was a processing error. */
	return 1;
}

/* Note: Basic parsing borrowed from nvcamera_config.c */
void process_config_file(build_image_context *context)
{
	char buffer[MAX_BUFFER];
	int  space = 0;
	char current;
	u_int8_t c_eol_comment_start = 0; // True after first slash
	u_int8_t comment = 0;
	u_int8_t string = 0;
	u_int8_t equal_encounter = 0;

	assert(context != NULL);
	assert(context->config_file != NULL);

	while ((current = fgetc(context->config_file)) !=EOF) {
		if (space >= (MAX_BUFFER-1)) {
			/* if we exceeded the max buffer size, it is likely
			 due to a missing semi-colon at the end of a line */
			printf("Config file parsing error!");
			exit(1);
		}

		/* Handle failure to complete "//" comment token.
		 Insert the '/' into the busffer and proceed with
		 processing the current character. */
		if (c_eol_comment_start && current != '/') {
			c_eol_comment_start = 0;
			buffer[space++] = '/';
		}

		switch (current) {
		case '\"': /* " indicates start or end of a string */
			if (!comment) {
				string ^= 1;
				buffer[space++] = current;
			}
			break;
		case ';':
			if (!string && !comment) {
				buffer[space++] = '\0';

				/* Process a statement. */
				if (process_statement(context, buffer)) {
						goto error;
				}
				space = 0;
				equal_encounter = 0;
			} else if (string) {
				buffer[space++] = current;
			}
			break;

		case '/':
			if (!string && !comment) {
				if (c_eol_comment_start) {
				/* EOL comment started. */
					comment = 1;
					c_eol_comment_start = 0;
				} else {
					/* Potential start of eol comment. */
					c_eol_comment_start = 1;
				}
			} else if (!comment) {
				buffer[space++] = current;
			}
			break;

		/* ignore whitespace.  uses fallthrough */
		case '\n':
		case '\r': /* carriage returns end comments */
			string  = 0;
			comment = 0;
			c_eol_comment_start = 0;
		case ' ':
		case '\t':
			if (string) {
				buffer[space++] = current;
			}
			break;

		case '#':
			if (!string) {
				comment = 1;
			} else {
				buffer[space++] = current;
			}
			break;

		default:
			if (!comment) {
				buffer[space++] = current;
				if (current == '=') {
					if (!equal_encounter) {
						equal_encounter = 1;
					} else {
						goto error;
					}
				}
			}
			break;
		}
	}

	return;

 error:
	printf("Error parsing: %s\n", buffer);
	exit(1);
}
