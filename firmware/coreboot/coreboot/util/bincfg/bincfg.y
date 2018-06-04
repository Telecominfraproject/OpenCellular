/*
 * bincfg - Compiler/Decompiler for data blobs with specs
 * Copyright (C) 2017 Damien Zammit <damien@zamaudio.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

%{
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "bincfg.h"
//#define YYDEBUG 1

static void check_pointer (void *ptr)
{
	if (ptr == NULL) {
		printf("Error: Out of memory\n");
		exit(1);
	}
}

static unsigned char* value_to_bits (unsigned int v, unsigned int w)
{
	unsigned int i;
	unsigned char* bitarr;

	if (w > MAX_WIDTH) w = MAX_WIDTH;
	bitarr = (unsigned char *) malloc (w * sizeof (unsigned char));
	check_pointer(bitarr);
	memset (bitarr, 0, w);

	for (i = 0; i < w; i++) {
		bitarr[i] = VALID_BIT | ((v & (1 << i)) >> i);
	}
	return bitarr;
}

/* Store each bit of a bitfield in a new byte sequentially 0x80 or 0x81 */
static void append_field_to_blob (unsigned char b[], unsigned int w)
{
	unsigned int i, j;
	binary->blb = (unsigned char *) realloc (binary->blb,
						 binary->bloblen + w);
	check_pointer(binary->blb);
	for (j = 0, i = binary->bloblen; i < binary->bloblen + w; i++, j++) {
		binary->blb[i] = VALID_BIT | (b[j] & 1);
		//fprintf (stderr, "blob[%d] = %d\n", i, binary->blb[i] & 1);
	}
	binary->bloblen += w;
}

static void set_bitfield(char *name, unsigned int value)
{
	unsigned long long i;
	struct field *bf = getsym (name);
	if (bf) {
		bf->value = value & 0xffffffff;
		i = (1 << bf->width) - 1;
		if (bf->width > 8 * sizeof (unsigned int)) {
			fprintf(stderr,
				"Overflow in bitfield, truncating bits to"
				" fit\n");
			bf->value = value & i;
		}
		//fprintf(stderr, "Setting `%s` = %d\n", bf->name, bf->value);
	} else {
		fprintf(stderr, "Can't find bitfield `%s` in spec\n", name);
	}
}

static void set_bitfield_array(char *name, unsigned int n, unsigned int value)
{
	unsigned int i;
	unsigned long len = strlen (name);
	char *namen = (char *) malloc ((len + 9) * sizeof (char));
	check_pointer(namen);
	for (i = 0; i < n; i++) {
		snprintf (namen, len + 8, "%s%x", name, i);
		set_bitfield (namen, value);
	}
	free(namen);
}

static void create_new_bitfield(char *name, unsigned int width)
{
	struct field *bf;

	if (!(bf = putsym (name, width))) return;
	//fprintf(stderr, "Added bitfield `%s` : %d\n", bf->name, width);
}

static void create_new_bitfields(char *name, unsigned int n, unsigned int width)
{
	unsigned int i;
	unsigned long len = strlen (name);
	char *namen = (char *) malloc ((len + 9) * sizeof (char));
	check_pointer(namen);
	for (i = 0; i < n; i++) {
		snprintf (namen, len + 8, "%s%x", name, i);
		create_new_bitfield (namen, width);
	}
	free(namen);
}

static struct field *putsym (char const *sym_name, unsigned int w)
{
	if (getsym(sym_name)) {
		fprintf(stderr, "Cannot add duplicate named bitfield `%s`\n",
			sym_name);
		return 0;
	}
	struct field *ptr = (struct field *) malloc (sizeof (struct field));
	check_pointer(ptr);
	ptr->name = (char *) malloc (strlen (sym_name) + 1);
	check_pointer(ptr->name);
	strcpy (ptr->name, sym_name);
	ptr->width = w;
	ptr->value = 0;
	ptr->next = (struct field *)0;
	if (sym_table_tail) {
		sym_table_tail->next = ptr;
	} else {
		sym_table = ptr;
	}
	sym_table_tail = ptr;
	return ptr;
}

static struct field *getsym (char const *sym_name)
{
	struct field *ptr;
	for (ptr = sym_table; ptr != (struct field *) 0;
			ptr = (struct field *)ptr->next) {
		if (strcmp (ptr->name, sym_name) == 0)
			return ptr;
	}
	return 0;
}

static void dump_all_values (void)
{
	struct field *ptr;
	for (ptr = sym_table; ptr != (struct field *) 0;
			ptr = (struct field *)ptr->next) {
		fprintf(stderr, "%s = %d (%d bits)\n",
				ptr->name,
				ptr->value,
				ptr->width);
	}
}

static void empty_field_table(void)
{
	struct field *ptr;
	struct field *ptrnext;

	for (ptr = sym_table; ptr != (struct field *) 0; ptr = ptrnext) {
		if (ptr) {
			ptrnext = ptr->next;
			free(ptr);
		} else {
			ptrnext = (struct field *) 0;
		}
	}
	sym_table = 0;
	sym_table_tail = 0;
}

static void create_binary_blob (void)
{
	if (binary && binary->blb) {
		free(binary->blb);
		free(binary);
	}
	binary = (struct blob *) malloc (sizeof (struct blob));
	check_pointer(binary);
	binary->blb = (unsigned char *) malloc (sizeof (unsigned char));
	check_pointer(binary->blb);
	binary->bloblen = 0;
	binary->blb[0] = VALID_BIT;
}

static void interpret_next_blob_value (struct field *f)
{
	unsigned int i;
	unsigned int v = 0;

	if (binary->bloblen >= binary->lenactualblob * 8) {
		f->value = 0;
		return;
	}

	for (i = 0; i < f->width; i++) {
		v |= (binary->blb[binary->bloblen++] & 1) << i;
	}

	f->value = v;
}

/* {}%BIN -> {} */
static void generate_setter_bitfields(FILE* fp, unsigned char *bin)
{
	unsigned int i;
	struct field *ptr;

	/* Convert bytes to bit array */
	for (i = 0; i < binary->lenactualblob; i++) {
		append_field_to_blob (value_to_bits(bin[i], 8), 8);
	}

	/* Reset blob position to zero */
	binary->bloblen = 0;

	fprintf (fp, "# AUTOGENERATED SETTER BY BINCFG\n{\n");

	/* Traverse spec and output bitfield setters based on blob values */
	for (ptr = sym_table; ptr != (struct field *) 0; ptr = ptr->next) {

		interpret_next_blob_value(ptr);
		fprintf (fp, "\t\"%s\" = 0x%x,\n", ptr->name, ptr->value);
	}
	fseek(fp, -2, SEEK_CUR);
	fprintf (fp, "\n}\n");
}

static void generate_binary_with_gbe_checksum(FILE* fp)
{
	int i;
	unsigned short checksum;

	/* traverse spec, push to blob and add up for checksum */
	struct field *ptr;
	unsigned int uptochksum = 0;
	for (ptr = sym_table; ptr != (struct field *) 0; ptr = ptr->next) {
		if (strcmp (ptr->name, "checksum_gbe") == 0) {
			/* Stop traversing because we hit checksum */
			ptr = ptr->next;
			break;
		}
		append_field_to_blob (
			value_to_bits(ptr->value, ptr->width),
			ptr->width);
		uptochksum += ptr->width;
	}

	/* deserialize bits of blob up to checksum */
	for (i = 0; i < uptochksum; i += 8) {
		unsigned char byte = (((binary->blb[i+0] & 1) << 0)
					| ((binary->blb[i+1] & 1) << 1)
					| ((binary->blb[i+2] & 1) << 2)
					| ((binary->blb[i+3] & 1) << 3)
					| ((binary->blb[i+4] & 1) << 4)
					| ((binary->blb[i+5] & 1) << 5)
					| ((binary->blb[i+6] & 1) << 6)
					| ((binary->blb[i+7] & 1) << 7)
		);
		fprintf(fp, "%c", byte);

		/* incremental 16 bit checksum */
		if ((i % 16) < 8) {
			binary->checksum += byte;
		} else {
			binary->checksum += byte << 8;
		}
	}

	checksum = (0xbaba - binary->checksum) & 0xffff;

	/* Now write checksum */
	set_bitfield ("checksum_gbe", checksum);

	fprintf(fp, "%c", checksum & 0xff);
	fprintf(fp, "%c", (checksum & 0xff00) >> 8);

	append_field_to_blob (value_to_bits(checksum, 16), 16);

	for (; ptr != (struct field *) 0; ptr = ptr->next) {
		append_field_to_blob (
			value_to_bits(ptr->value, ptr->width), ptr->width);
	}

	/* deserialize rest of blob past checksum */
	for (i = uptochksum + CHECKSUM_SIZE; i < binary->bloblen; i += 8) {
		unsigned char byte = (((binary->blb[i+0] & 1) << 0)
					| ((binary->blb[i+1] & 1) << 1)
					| ((binary->blb[i+2] & 1) << 2)
					| ((binary->blb[i+3] & 1) << 3)
					| ((binary->blb[i+4] & 1) << 4)
					| ((binary->blb[i+5] & 1) << 5)
					| ((binary->blb[i+6] & 1) << 6)
					| ((binary->blb[i+7] & 1) << 7)
		);
		fprintf(fp, "%c", byte);
	}
}

/* {}{} -> BIN */
static void generate_binary(FILE* fp)
{
	unsigned int i;
	struct field *ptr;

	if (binary->bloblen % 8) {
		fprintf (stderr,
			 "ERROR: Spec must be multiple of 8 bits wide\n");
		exit (1);
	}

	if (getsym ("checksum_gbe")) {
		generate_binary_with_gbe_checksum(fp);
		return;
	}

	/* traverse spec, push to blob */
	for (ptr = sym_table; ptr != (struct field *) 0; ptr = ptr->next) {
		append_field_to_blob (
			value_to_bits(ptr->value, ptr->width),
			ptr->width);
	}

	/* deserialize bits of blob */
	for (i = 0; i < binary->bloblen; i += 8) {
		unsigned char byte = (((binary->blb[i+0] & 1) << 0)
				| ((binary->blb[i+1] & 1) << 1)
				| ((binary->blb[i+2] & 1) << 2)
				| ((binary->blb[i+3] & 1) << 3)
				| ((binary->blb[i+4] & 1) << 4)
				| ((binary->blb[i+5] & 1) << 5)
				| ((binary->blb[i+6] & 1) << 6)
				| ((binary->blb[i+7] & 1) << 7)
		);
		fprintf(fp, "%c", byte);
	}
}

%}

%union
{
	char *str;
	unsigned int u32;
	unsigned int *u32array;
	unsigned char u8;
	unsigned char *u8array;
}
%parse-param {FILE* fp}

%token <str> name
%token <u32> val
%token <u32array> vals
%token <u8> hexbyte
%token <u8array> binblob
%token <u8> eof

%left '%'
%left '{' '}'
%left ','
%left ':'
%left '='

%%

input:
  /* empty */
| input spec setter eof		{ empty_field_table(); YYACCEPT;}
| input spec blob		{ fprintf (stderr, "Parsed all bytes\n");
				  empty_field_table(); YYACCEPT;}
;

blob:
  '%' eof			{ generate_setter_bitfields(fp,
				  binary->actualblob); }
;

spec:
  '{' '}'		{	fprintf (stderr, "No spec\n"); }
| '{' specmembers '}'	{	fprintf (stderr, "Parsed all spec\n");
				create_binary_blob(); }
;

specmembers:
  specpair
| specpair ',' specmembers
;

specpair:
  name ':' val		{	create_new_bitfield($1, $3); }
| name '[' val ']' ':' val	{ create_new_bitfields($1, $3, $6); }
;

setter:
  '{' '}'		{	fprintf (stderr, "No values\n"); }
| '{' valuemembers '}'	{	fprintf (stderr, "Parsed all values\n");
				generate_binary(fp); }
;

valuemembers:
  setpair
| setpair ',' valuemembers
;

setpair:
  name '=' val		{	set_bitfield($1, $3); }
| name '[' val ']' '=' val	{ set_bitfield_array($1, $3, $6); }
;

%%

/* Called by yyparse on error.  */
static void yyerror (FILE* fp, char const *s)
{
	fprintf (stderr, "yyerror: %s\n", s);
}

/* Declarations */
void set_input_string(char* in);

/* This function parses a string */
static int parse_string(FILE* fp, unsigned char* in) {
	set_input_string ((char *)in);
	return yyparse (fp);
}

static unsigned int loadfile (FILE* fp, char *file, char *filetype,
	unsigned char **parsestring, unsigned int lenstr)
{
	unsigned int lenfile;

	if ((fp = fopen(file, "r")) == NULL) {
		printf("Error: Could not open %s file: %s\n",filetype,file);
		exit(1);
	}
	fseek(fp, 0, SEEK_END);
	lenfile = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (lenstr == 0)
		*parsestring = (unsigned char *) malloc (lenfile + 2);
	else
		*parsestring = (unsigned char *) realloc (*parsestring,
						lenfile + lenstr);

	check_pointer(*parsestring);
	fread(*parsestring + lenstr, 1, lenfile, fp);
	fclose(fp);
	return lenfile;
}

int main (int argc, char *argv[])
{
	unsigned int lenspec;
	unsigned char *parsestring;
	unsigned char c;
	unsigned int pos = 0;
	int ret = 0;
	FILE* fp;

#if YYDEBUG == 1
	yydebug = 1;
#endif
	create_binary_blob();
	binary->lenactualblob = 0;

	if (argc == 4 && strcmp(argv[1], "-d") != 0) {
		/* Compile mode */

		/* Load Spec */
		lenspec = loadfile(fp, argv[1], "spec", &parsestring, 0);
		loadfile(fp, argv[2], "setter", &parsestring, lenspec);

		/* Open output and parse string - output to fp */
		if ((fp = fopen(argv[3], "wb")) == NULL) {
			printf("Error: Could not open output file: %s\n",
			       argv[3]);
			exit(1);
		}
		ret = parse_string(fp, parsestring);
		free(parsestring);
	} else if (argc == 5 && strcmp (argv[1], "-d") == 0) {
		/* Decompile mode */

		/* Load Spec */
		lenspec = loadfile(fp, argv[2], "spec", &parsestring, 0);

		parsestring[lenspec] = '%';
		parsestring[lenspec + 1] = '\0';

		/* Load Actual Binary */
		if ((fp = fopen(argv[3], "rb")) == NULL) {
			printf("Error: Could not open binary file: %s\n",
			       argv[3]);
			exit(1);
		}
		fseek(fp, 0, SEEK_END);
		binary->lenactualblob = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		binary->actualblob = (unsigned char *)malloc(
			binary->lenactualblob);
		check_pointer(binary->actualblob);
		fread(binary->actualblob, 1, binary->lenactualblob, fp);
		fclose(fp);

		/* Open output and parse - output to fp */
		if ((fp = fopen(argv[4], "w")) == NULL) {
			printf("Error: Could not open output file: %s\n",
			       argv[4]);
			exit(1);
		}
		ret = parse_string(fp, parsestring);
		free(parsestring);
		free(binary->actualblob);
		fclose(fp);
	} else {
		printf("Usage: Compile mode\n\n");
		printf("       bincfg    spec  setter  binaryoutput\n");
		printf("                  (file) (file)     (file)\n");
		printf(" OR  : Decompile mode\n\n");
		printf("       bincfg -d spec  binary  setteroutput\n");
	}
	return ret;
}
