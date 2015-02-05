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

static void help_and_quit(const char *prog)
{
	printf(usage, prog, prog, prog);
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


static int do_pcr(int argc, char *argv[])
{
	uint8_t accum[SHA256_DIGEST_SIZE * 2];
	uint8_t pcr[SHA256_DIGEST_SIZE];
	int digest_alg = SHA1_DIGEST_ALGORITHM;
	int digest_size = SHA1_DIGEST_SIZE;
	int opt_init = 0;
	int errorcnt = 0;
	uint8_t *digest;
	int i;

	opterr = 0;		/* quiet, you */
	while ((i = getopt(argc, argv, ":i2")) != -1) {
		switch (i) {
		case 'i':
			opt_init = 1;
			break;
		case '2':
			digest_alg = SHA256_DIGEST_ALGORITHM;
			digest_size = SHA256_DIGEST_SIZE;
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

	if (argc - optind < 1 + opt_init) {
		fprintf(stderr, "You must extend at least one DIGEST\n");
		help_and_quit(argv[0]);
		return 1;
	}

	memset(pcr, 0, sizeof(pcr));

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

		digest = DigestBuf(accum, digest_size * 2, digest_alg);
		if (!digest) {
			fprintf(stderr, "Error computing digest!\n");
			return 1;
		}
		memcpy(pcr, digest, digest_size);
		free(digest);

		printf("PCR: ");
		print_digest(pcr, digest_size);
		printf("\n");
	}

	return 0;
}

DECLARE_FUTIL_COMMAND(pcr, do_pcr,
		      VBOOT_VERSION_ALL,
		      "Simulate a TPM PCR extension operation",
		      help_and_quit);
