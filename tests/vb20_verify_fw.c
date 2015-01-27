/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Routines for verifying a firmware image's signature.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "2sysincludes.h"
#include "2api.h"

const char *gbb_fname;
const char *vblock_fname;
const char *body_fname;

/**
 * Local implementation which reads resources from individual files.  Could be
 * more elegant and read from bios.bin, if we understood the fmap.
 */
int vb2ex_read_resource(struct vb2_context *ctx,
			enum vb2_resource_index index,
			uint32_t offset,
			void *buf,
			uint32_t size)
{
	const char *fname;
	FILE *f;
	int got_size;

	/* Get the filename for the resource */
	switch (index) {
	case VB2_RES_GBB:
		fname = gbb_fname;
		break;
	case VB2_RES_FW_VBLOCK:
		fname = vblock_fname;
		break;
	default:
		return VB2_ERROR_UNKNOWN;
	}

	/* Open file and seek to the requested offset */
	f = fopen(fname, "rb");
	if (!f)
		return VB2_ERROR_UNKNOWN;

	if (fseek(f, offset, SEEK_SET)) {
		fclose(f);
		return VB2_ERROR_UNKNOWN;
	}

	/* Read data and close file */
	got_size = fread(buf, 1, size, f);
	fclose(f);

	/* Return success if we read everything */
	return got_size == size ? VB2_SUCCESS : VB2_ERROR_UNKNOWN;
}

int vb2ex_tpm_clear_owner(struct vb2_context *ctx)
{
	// TODO: implement
	return VB2_SUCCESS;
}

/**
 * Save non-volatile and/or secure data if needed.
 */
static void save_if_needed(struct vb2_context *ctx)
{

	if (ctx->flags & VB2_CONTEXT_NVDATA_CHANGED) {
		// TODO: implement
		ctx->flags &= ~VB2_CONTEXT_NVDATA_CHANGED;
	}

	if (ctx->flags & VB2_CONTEXT_SECDATA_CHANGED) {
		// TODO: implement
		ctx->flags &= ~VB2_CONTEXT_SECDATA_CHANGED;
	}
}

/**
 * Verify firmware body
 */
static int hash_body(struct vb2_context *ctx)
{
	uint32_t expect_size;
	uint8_t block[8192];
	uint32_t size;
	FILE *f;
	int rv;

	/* Open the body data */
	f = fopen(body_fname, "rb");

	/* Start the body hash */
	rv = vb2api_init_hash(ctx, VB2_HASH_TAG_FW_BODY, &expect_size);
	if (rv)
		return rv;

	printf("Expect %d bytes of body...\n", expect_size);

	/* Extend over the body */
	while (expect_size) {
		size = sizeof(block);
		if (size > expect_size)
			size = expect_size;

		/* Read next body block */
		size = fread(block, 1, size, f);
		if (size <= 0)
			break;

		/* Hash it */
		rv = vb2api_extend_hash(ctx, block, size);
		if (rv)
			return rv;

		expect_size -= size;
	}

	/* Check the result */
	rv = vb2api_check_hash(ctx);
	if (rv)
		return rv;

	return VB2_SUCCESS;
}

static void print_help(const char *progname)
{
	printf("Usage: %s <gbb> <vblock> <body>\n", progname);
}

int main(int argc, char *argv[])
{
	struct vb2_context ctx;
	uint8_t workbuf[16384] __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	int rv;

	if (argc < 4) {
		print_help(argv[0]);
		return 1;
	}

	/* Save filenames */
	gbb_fname = argv[1];
	vblock_fname = argv[2];
	body_fname = argv[3];

	/* Set up context */
	memset(&ctx, 0, sizeof(ctx));
	ctx.workbuf = workbuf;
	ctx.workbuf_size = sizeof(workbuf);

	/* Initialize secure context */
	rv = vb2api_secdata_create(&ctx);
	if (rv) {
		fprintf(stderr,
			"error: vb2api_secdata_create() failed (%d)\n", rv);
		return 1;
	}

	// TODO: optional args to set contents for nvdata, secdata?

	/* Do early init */
	printf("Phase 1...\n");
	rv = vb2api_fw_phase1(&ctx);
	if (rv) {
		printf("Phase 1 wants recovery mode.\n");
		save_if_needed(&ctx);
		return rv;
	}

	/* Determine which firmware slot to boot */
	printf("Phase 2...\n");
	rv = vb2api_fw_phase2(&ctx);
	if (rv) {
		printf("Phase 2 wants reboot.\n");
		save_if_needed(&ctx);
		return rv;
	}

	/* Try that slot */
	printf("Phase 3...\n");
	rv = vb2api_fw_phase3(&ctx);
	if (rv) {
		printf("Phase 3 wants reboot.\n");
		save_if_needed(&ctx);
		return rv;
	}

	/* Verify body */
	printf("Hash body...\n");
	rv = hash_body(&ctx);
	save_if_needed(&ctx);
	if (rv) {
		printf("Phase 4 wants reboot.\n");
		return rv;
	}

	printf("Yaay!\n");

	printf("Workbuf used = %d bytes\n", ctx.workbuf_used);

	return 0;
}
