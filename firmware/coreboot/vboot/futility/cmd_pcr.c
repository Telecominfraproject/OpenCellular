/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ctype.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2sha.h"
#include "futility.h"

static const char usage[] = "\n"
	"Usage:  " MYNAME " %s [OPTIONS] DIGEST [...]\n"
	"\n"
	"This simulates a TPM PCR extension, to determine the expected output\n"
	"\n"
	"Each DIGEST arg should be a hex string (spaces optional) of the\n"
	"appropriate length. The PCR is extended with each digest in turn\n"
	"and the new value displayed.\n"
	"\n"
	"Options:\n"
	"  -i      Initialize the PCR with the first DIGEST argument\n"
	"            (the default is to start with all zeros)\n"
	"  -2      Use sha256 DIGESTS (the default is sha1)\n"
	"\n"
	"Examples:\n"
	"\n"
	"  " MYNAME " %s b52791126f96a21a8ba4d511c6f25a1c1eb6dc9e\n"
	"  " MYNAME " %s "
	"'b5 27 91 12 6f 96 a2 1a 8b a4 d5 11 c6 f2 5a 1c 1e b6 dc 9e'\n"
	"\n";

static void print_help(int argc, char *argv[])
{
	printf(usage, argv[0], argv[0], argv[0]);
}

static int parse_hex(uint8_t *val, const char *str)
{
	uint8_t v = 0;
	char c;
	int digit;

	for (digit = 0; digit < 2; digit++) {
		c = *str;
		if (!c)
			return 0;
		if (!isxdigit(c))
			return 0;
		c = tolower(c);
		if (c >= '0' && c <= '9')
			v += c - '0';
		else
			v += 10 + c - 'a';
		if (!digit)
			v <<= 4;
		str++;
	}

	*val = v;
	return 1;
}

static void parse_digest_or_die(uint8_t *buf, int len, const char *str)
{
	const char *s = str;
	int i;

	for (i = 0; i < len; i++) {
		/* skip whitespace */
		while (*s && isspace(*s))
			s++;
		if (!*s)
			break;
		if (!parse_hex(buf, s))
			break;

		/* on to the next byte */
		s += 2;
		buf++;
	}

	if (i != len) {
		fprintf(stderr, "Invalid DIGEST \"%s\"\n", str);
		exit(1);
	}
}

static void print_digest(const uint8_t *buf, int len)
{
	int i;
	for (i = 0; i < len; i++)
		printf("%02x", buf[i]);
}

enum {
	OPT_HELP = 1000,
};
static const struct option long_opts[] = {
	{"help",     0, 0, OPT_HELP},
	{NULL, 0, 0, 0}
};
static int do_pcr(int argc, char *argv[])
{
	uint8_t accum[VB2_MAX_DIGEST_SIZE * 2];
	uint8_t pcr[VB2_MAX_DIGEST_SIZE];
	int digest_alg = VB2_HASH_SHA1;
	int digest_size;
	int opt_init = 0;
	int errorcnt = 0;
	int i;

	opterr = 0;		/* quiet, you */
	while ((i = getopt_long(argc, argv, ":i2", long_opts, NULL)) != -1) {
		switch (i) {
		case 'i':
			opt_init = 1;
			break;
		case '2':
			digest_alg = VB2_HASH_SHA256;
			break;
		case OPT_HELP:
			print_help(argc, argv);
			return !!errorcnt;
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
		print_help(argc, argv);
		return 1;
	}

	if (argc - optind < 1 + opt_init) {
		fprintf(stderr, "You must extend at least one DIGEST\n");
		print_help(argc, argv);
		return 1;
	}

	memset(pcr, 0, sizeof(pcr));

	digest_size = vb2_digest_size(digest_alg);
	if (!digest_size) {
		fprintf(stderr, "Error determining digest size!\n");
		return 1;
	}

	if (opt_init) {
		parse_digest_or_die(pcr, digest_size, argv[optind]);
		optind++;
	}

	printf("PCR: ");
	print_digest(pcr, digest_size);
	printf("\n");

	for (i = optind; i < argc; i++) {
		memcpy(accum, pcr, sizeof(pcr));
		parse_digest_or_die(accum + digest_size, digest_size, argv[i]);

		printf("   + ");
		print_digest(accum + digest_size, digest_size);
		printf("\n");

		if (VB2_SUCCESS != vb2_digest_buffer(accum, digest_size * 2,
						     digest_alg,
						     pcr, digest_size)) {
			fprintf(stderr, "Error computing digest!\n");
			return 1;
		}

		printf("PCR: ");
		print_digest(pcr, digest_size);
		printf("\n");
	}

	return 0;
}

DECLARE_FUTIL_COMMAND(pcr, do_pcr, VBOOT_VERSION_ALL,
		      "Simulate a TPM PCR extension operation");
