/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "fmap.h"
#include "futility.h"
#include "gbb_header.h"
#include "host_common.h"
#include "host_key.h"
#include "traversal.h"

/* What functions do we invoke for a particular operation and component? */

/* FUTIL_OP_SHOW */
static int (* const cb_show_funcs[])(struct futil_traverse_state_s *state) =
{
	futil_cb_show_begin,		/* CB_BEGIN_TRAVERSAL */
	NULL,				/* CB_END_TRAVERSAL */
	futil_cb_show_gbb,		/* CB_FMAP_GBB */
	futil_cb_show_fw_preamble,	/* CB_FMAP_VBLOCK_A */
	futil_cb_show_fw_preamble,	/* CB_FMAP_VBLOCK_B */
	futil_cb_show_fw_main,     	/* CB_FMAP_FW_MAIN_A */
	futil_cb_show_fw_main,     	/* CB_FMAP_FW_MAIN_B */
	futil_cb_show_key,      	/* CB_PUBKEY */
	futil_cb_show_keyblock,      	/* CB_KEYBLOCK */
	futil_cb_show_gbb,		/* CB_GBB */
	futil_cb_show_fw_preamble,	/* CB_FW_PREAMBLE */
};
BUILD_ASSERT(ARRAY_SIZE(cb_show_funcs) == NUM_CB_COMPONENTS);

/* FUTIL_OP_SIGN */
static int (* const cb_sign_funcs[])(struct futil_traverse_state_s *state) =
{
	futil_cb_sign_begin,		/* CB_BEGIN_TRAVERSAL */
	futil_cb_sign_end,		/* CB_END_TRAVERSAL */
	NULL,				/* CB_FMAP_GBB */
	futil_cb_sign_fw_preamble,	/* CB_FMAP_VBLOCK_A */
	futil_cb_sign_fw_preamble,	/* CB_FMAP_VBLOCK_B */
	futil_cb_sign_fw_main,     	/* CB_FMAP_FW_MAIN_A */
	futil_cb_sign_fw_main,     	/* CB_FMAP_FW_MAIN_B */
	futil_cb_sign_bogus,      	/* CB_PUBKEY */
	futil_cb_sign_notyet,      	/* CB_KEYBLOCK */
	futil_cb_sign_bogus,		/* CB_GBB */
	futil_cb_sign_fw_preamble,	/* CB_FW_PREAMBLE */
};
BUILD_ASSERT(ARRAY_SIZE(cb_sign_funcs) == NUM_CB_COMPONENTS);

static int (* const * const cb_func[])(struct futil_traverse_state_s *state) =
{
	cb_show_funcs,
	cb_sign_funcs,
};
BUILD_ASSERT(ARRAY_SIZE(cb_func) == NUM_FUTIL_OPS);


static int invoke_callback(struct futil_traverse_state_s *state,
			   enum futil_cb_component c, const char *name,
			   uint32_t offset, uint8_t *buf, uint32_t len)
{

	VBDEBUG(("%s: name \"%s\" op %d component %d"
		" offset=0x%08x len=0x%08x, buf=%p\n",
		 __func__, name, state->op, c, offset, len, buf));

	if (c < 0 || c >= NUM_CB_COMPONENTS) {
		fprintf(stderr, "Invalid component %d\n", c);
		return 1;
	}

	state->component = c;
	state->name = name;
	state->cb_area[c].offset = offset;
	state->cb_area[c].buf = buf;
	state->cb_area[c].len = len;
	state->my_area = &state->cb_area[c];

	if (cb_func[state->op][c])
		return cb_func[state->op][c](state);

	return 0;
}

/*
 * File types that don't need iterating can use a lookup table to determine the
 * callback component and name. The index is the file type.
 */
static const struct {
	enum futil_cb_component component;
	const char * const name;
} direct_callback[] = {
	{0,                NULL},		/* FILE_TYPE_UNKNOWN */
	{CB_PUBKEY,        "VbPublicKey"},	/* FILE_TYPE_PUBKEY */
	{CB_KEYBLOCK,      "VbKeyBlock"},	/* FILE_TYPE_KEYBLOCK */
	{CB_FW_PREAMBLE,   "FW Preamble"},	/* FILE_TYPE_FIRMWARE */
	{CB_GBB,           "GBB"},		/* FILE_TYPE_GBB */
	{0,                NULL},		/* FILE_TYPE_BIOS_IMAGE */
	{0,                NULL},		/* FILE_TYPE_OLD_BIOS_IMAGE */
};
BUILD_ASSERT(ARRAY_SIZE(direct_callback) == NUM_FILE_TYPES);

/*
 * The Chrome OS BIOS must contain specific FMAP areas, and we generally want
 * to look at each one in a certain order.
 */
struct bios_area_s {
	const char * const name;
	enum futil_cb_component component;
};

/* This are the expected areas, in order of traversal. */
static const struct bios_area_s bios_area[] = {
	{"GBB",       CB_FMAP_GBB},
	{"FW_MAIN_A", CB_FMAP_FW_MAIN_A},
	{"FW_MAIN_B", CB_FMAP_FW_MAIN_B},
	{"VBLOCK_A",  CB_FMAP_VBLOCK_A},
	{"VBLOCK_B",  CB_FMAP_VBLOCK_B},
	{0, 0}
};

/* Really old BIOS images had different names, but worked the same. */
static const struct bios_area_s old_bios_area[] = {
	{"GBB Area",        CB_FMAP_GBB},
	{"Firmware A Data", CB_FMAP_FW_MAIN_A},
	{"Firmware B Data", CB_FMAP_FW_MAIN_B},
	{"Firmware A Key",  CB_FMAP_VBLOCK_A},
	{"Firmware B Key",  CB_FMAP_VBLOCK_B},
	{0, 0}
};

static int has_all_areas(uint8_t *buf, uint32_t len, FmapHeader *fmap,
			 const struct bios_area_s *area)
{
	// We must have all the expected areas
	for ( ; area->name; area++)
		if (!fmap_find_by_name(buf, len, fmap, area->name, 0))
			return 0;

	/* Found 'em all */
	return 1;
}

static enum futil_file_type what_is_this(uint8_t *buf, uint32_t len)
{
	VbPublicKey *pubkey = (VbPublicKey *)buf;
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)buf;
	GoogleBinaryBlockHeader *gbb = (GoogleBinaryBlockHeader *)buf;
	VbFirmwarePreambleHeader *fw_preamble;
	RSAPublicKey *rsa;
	FmapHeader *fmap;

	/*
	 * Complex structs may begin with simpler structs first, so try them
	 * in reverse order.
	 */

	fmap = fmap_find(buf, len);
	if (fmap) {
		if (has_all_areas(buf, len, fmap, bios_area))
			return FILE_TYPE_BIOS_IMAGE;
		if (has_all_areas(buf, len, fmap, old_bios_area))
			return FILE_TYPE_OLD_BIOS_IMAGE;
	}

	if (futil_looks_like_gbb(gbb, len))
		return FILE_TYPE_GBB;

	if (VBOOT_SUCCESS == KeyBlockVerify(key_block, len, NULL, 1)) {
		/* and firmware preamble too? */
		fw_preamble = (VbFirmwarePreambleHeader *)
			(buf + key_block->key_block_size);
		uint32_t more = key_block->key_block_size;
		rsa = PublicKeyToRSA(&key_block->data_key);
		if (VBOOT_SUCCESS ==
		    VerifyFirmwarePreamble(fw_preamble, len - more, rsa))
			return FILE_TYPE_FIRMWARE;

		/* no, just keyblock */
		return FILE_TYPE_KEYBLOCK;
	}

	if (PublicKeyLooksOkay(pubkey, len)) {
		return FILE_TYPE_PUBKEY;
	}

	return FILE_TYPE_UNKNOWN;
}

static int traverse_buffer(uint8_t *buf, uint32_t len,
			   struct futil_traverse_state_s *state)
{
	FmapHeader *fmap;
	FmapAreaHeader *ah = 0;
	const struct bios_area_s *area;
	enum futil_file_type type;
	int retval = 0;

	type = what_is_this(buf, len);
	state->in_type = type;

	state->errors = retval;
	retval |= invoke_callback(state, CB_BEGIN_TRAVERSAL, "<begin>",
				  0, buf, len);
	state->errors = retval;

	switch (type) {
	case FILE_TYPE_PUBKEY:
	case FILE_TYPE_KEYBLOCK:
	case FILE_TYPE_FIRMWARE:
	case FILE_TYPE_GBB:
		retval |= invoke_callback(state,
					  direct_callback[type].component,
					  direct_callback[type].name,
					  0, buf, len);
		state->errors = retval;
		break;

	case FILE_TYPE_BIOS_IMAGE:
		/* We've already checked, so we know this will work. */
		fmap = fmap_find(buf, len);
		for (area = bios_area; area->name; area++) {
			/* We know this will work, too */
			fmap_find_by_name(buf, len, fmap, area->name, &ah);
			retval |= invoke_callback(state,
						  area->component,
						  area->name,
						  ah->area_offset,
						  buf + ah->area_offset,
						  ah->area_size);
			state->errors = retval;
		}
		break;

	case FILE_TYPE_OLD_BIOS_IMAGE:
		/* We've already checked, so we know this will work. */
		fmap = fmap_find(buf, len);
		for (area = old_bios_area; area->name; area++) {
			/* We know this will work, too */
			fmap_find_by_name(buf, len, fmap, area->name, &ah);
			retval |= invoke_callback(state,
						  area->component,
						  area->name,
						  ah->area_offset,
						  buf + ah->area_offset,
						  ah->area_size);
			state->errors = retval;
		}
		break;

	default:
		retval = 1;
	}

	retval |= invoke_callback(state, CB_END_TRAVERSAL, "<end>",
				  0, buf, len);
	return retval;
}

int futil_traverse(int ifd, struct futil_traverse_state_s *state,
		   int writeable)
{
	void *mmap_ptr = 0;
	uint32_t len;
	int errorcnt = 0;

	if (state->op < 0 || state->op >= NUM_FUTIL_OPS) {
		fprintf(stderr, "Invalid op %d\n", state->op);
		return 1;
	}

	if (0 != map_it(ifd, writeable, &mmap_ptr, &len))
		return 1;

	errorcnt |= traverse_buffer(mmap_ptr, len, state);

	errorcnt |= unmap_it(ifd, writeable, mmap_ptr, len);

	return errorcnt;
}
