/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This is a very simple binary editor, used to create corrupted structs for
 * testing. It copies stdin to stdout, replacing bytes beginning at the given
 * offset with the specified 8-bit values.
 *
 * There is NO conversion checking of the arguments.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	uint32_t offset, curpos, curarg;
	int c;

	if (argc < 3) {
		fprintf(stderr, "Need two or more args:  OFFSET VAL [VAL...]\n");
		return 1;
	}

	offset = (uint32_t)strtoul(argv[1], 0, 0);
	curarg = 2;
	for ( curpos = 0; (c = fgetc(stdin)) != EOF; curpos++) {

		if (curpos == offset && curarg < argc) {
			c = (uint8_t)strtoul(argv[curarg++], 0, 0);
			offset++;
		}

		fputc(c, stdout);
	}
	return 0;
}
