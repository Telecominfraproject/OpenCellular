/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdio.h>

#include "file_type.h"
#include "fmap.h"
#include "futility.h"
#include "traversal.h"

/* What functions do we invoke for a particular operation and component? */

/* FUTIL_OP_SHOW */
static int (* const cb_show_funcs[])(struct futil_traverse_state_s *state) = {
	futil_cb_show_begin,		/* CB_BEGIN_TRAVERSAL */
	NULL,				/* CB_END_TRAVERSAL */
	futil_cb_show_gbb,		/* CB_FMAP_GBB */
	futil_cb_show_fw_preamble,	/* CB_FMAP_VBLOCK_A */
	futil_cb_show_fw_preamble,	/* CB_FMAP_VBLOCK_B */
	futil_cb_show_fw_main,		/* CB_FMAP_FW_MAIN_A */
	futil_cb_show_fw_main,		/* CB_FMAP_FW_MAIN_B */
	futil_cb_show_pubkey,		/* CB_PUBKEY */
	futil_cb_show_keyblock,		/* CB_KEYBLOCK */
	futil_cb_show_gbb,		/* CB_GBB */
	futil_cb_show_fw_preamble,	/* CB_FW_PREAMBLE */
	futil_cb_show_kernel_preamble,	/* CB_KERN_PREAMBLE */
	NULL,				/* CB_RAW_FIRMWARE */
	NULL,				/* CB_RAW_KERNEL */
	futil_cb_show_privkey,		/* CB_PRIVKEY */
};
BUILD_ASSERT(ARRAY_SIZE(cb_show_funcs) == NUM_CB_COMPONENTS);

/* FUTIL_OP_SIGN */
static int (* const cb_sign_funcs[])(struct futil_traverse_state_s *state) = {
	futil_cb_sign_begin,		/* CB_BEGIN_TRAVERSAL */
	futil_cb_sign_end,		/* CB_END_TRAVERSAL */
	NULL,				/* CB_FMAP_GBB */
	futil_cb_sign_fw_vblock,	/* CB_FMAP_VBLOCK_A */
	futil_cb_sign_fw_vblock,	/* CB_FMAP_VBLOCK_B */
	futil_cb_sign_fw_main,		/* CB_FMAP_FW_MAIN_A */
	futil_cb_sign_fw_main,		/* CB_FMAP_FW_MAIN_B */
	futil_cb_sign_pubkey,		/* CB_PUBKEY */
	NULL,				/* CB_KEYBLOCK */
	NULL,				/* CB_GBB */
	NULL,				/* CB_FW_PREAMBLE */
	futil_cb_resign_kernel_part,	/* CB_KERN_PREAMBLE */
	futil_cb_sign_raw_firmware,	/* CB_RAW_FIRMWARE */
	futil_cb_create_kernel_part,	/* CB_RAW_KERNEL */
	NULL,				/* CB_PRIVKEY */
};
BUILD_ASSERT(ARRAY_SIZE(cb_sign_funcs) == NUM_CB_COMPONENTS);

static int (* const * const cb_func[])(struct futil_traverse_state_s *state) = {
	cb_show_funcs,
	cb_sign_funcs,
};
BUILD_ASSERT(ARRAY_SIZE(cb_func) == NUM_FUTIL_OPS);

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
	{CB_FW_PREAMBLE,   "FW Preamble"},	/* FILE_TYPE_FW_PREAMBLE */
	{CB_GBB,           "GBB"},		/* FILE_TYPE_GBB */
	{0,                NULL},		/* FILE_TYPE_BIOS_IMAGE */
	{0,                NULL},		/* FILE_TYPE_OLD_BIOS_IMAGE */
	{CB_KERN_PREAMBLE, "Kernel Preamble"},	/* FILE_TYPE_KERN_PREAMBLE */
	{CB_RAW_FIRMWARE,  "raw firmware"},	/* FILE_TYPE_RAW_FIRMWARE */
	{CB_RAW_KERNEL,    "raw kernel"},	/* FILE_TYPE_RAW_KERNEL */
	{0,                "chromiumos disk"},	/* FILE_TYPE_CHROMIUMOS_DISK */
	{CB_PRIVKEY,       "VbPrivateKey"},	/* FILE_TYPE_PRIVKEY */
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
	/* We must have all the expected areas */
	for (; area->name; area++)
		if (!fmap_find_by_name(buf, len, fmap, area->name, 0))
			return 0;

	/* Found 'em all */
	return 1;
}

enum futil_file_type recognize_bios_image(uint8_t *buf, uint32_t len)
{
	FmapHeader *fmap = fmap_find(buf, len);
	if (fmap) {
		if (has_all_areas(buf, len, fmap, bios_area))
			return FILE_TYPE_BIOS_IMAGE;
		if (has_all_areas(buf, len, fmap, old_bios_area))
			return FILE_TYPE_OLD_BIOS_IMAGE;
	}
	return FILE_TYPE_UNKNOWN;
}

static const char * const futil_cb_component_str[] = {
	"CB_BEGIN_TRAVERSAL",
	"CB_END_TRAVERSAL",
	"CB_FMAP_GBB",
	"CB_FMAP_VBLOCK_A",
	"CB_FMAP_VBLOCK_B",
	"CB_FMAP_FW_MAIN_A",
	"CB_FMAP_FW_MAIN_B",
	"CB_PUBKEY",
	"CB_KEYBLOCK",
	"CB_GBB",
	"CB_FW_PREAMBLE",
	"CB_KERN_PREAMBLE",
	"CB_RAW_FIRMWARE",
	"CB_RAW_KERNEL",
	"CB_PRIVKEY",
};
BUILD_ASSERT(ARRAY_SIZE(futil_cb_component_str) == NUM_CB_COMPONENTS);

static int invoke_callback(struct futil_traverse_state_s *state,
			   enum futil_cb_component c, const char *name,
			   uint32_t offset, uint8_t *buf, uint32_t len)
{
	Debug("%s: name \"%s\" op %d component %s"
	      " offset=0x%08x len=0x%08x, buf=%p\n",
	      __func__, name, state->op, futil_cb_component_str[c],
	      offset, len, buf);

	if ((int) c < 0 || c >= NUM_CB_COMPONENTS) {
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

static void fmap_limit_area(FmapAreaHeader *ah, uint32_t len)
{
	uint32_t sum = ah->area_offset + ah->area_size;
	if (sum < ah->area_size || sum > len) {
		Debug("%s(%s) 0x%x + 0x%x > 0x%x\n",
		      __func__, ah->area_name,
		      ah->area_offset, ah->area_size, len);
		ah->area_offset = 0;
		ah->area_size = 0;
	}
}

int futil_traverse(uint8_t *buf, uint32_t len,
		   struct futil_traverse_state_s *state,
		   enum futil_file_type type)
{
	FmapHeader *fmap;
	FmapAreaHeader *ah = 0;
	const struct bios_area_s *area;
	int retval = 0;

	if ((int) state->op < 0 || state->op >= NUM_FUTIL_OPS) {
		fprintf(stderr, "Invalid op %d\n", state->op);
		return 1;
	}

	if (type == FILE_TYPE_UNKNOWN)
		type = futil_file_type_buf(buf, len);
	state->in_type = type;

	state->errors = retval;
	retval |= invoke_callback(state, CB_BEGIN_TRAVERSAL, "<begin>",
				  0, buf, len);
	state->errors = retval;

	switch (type) {
	case FILE_TYPE_BIOS_IMAGE:
		/* We've already checked, so we know this will work. */
		fmap = fmap_find(buf, len);
		for (area = bios_area; area->name; area++) {
			/* We know this will work, too */
			fmap_find_by_name(buf, len, fmap, area->name, &ah);
			/* But the file might be truncated */
			fmap_limit_area(ah, len);
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
			/* But the file might be truncated */
			fmap_limit_area(ah, len);
			retval |= invoke_callback(state,
						  area->component,
						  area->name,
						  ah->area_offset,
						  buf + ah->area_offset,
						  ah->area_size);
			state->errors = retval;
		}
		break;

	case FILE_TYPE_UNKNOWN:
	case FILE_TYPE_CHROMIUMOS_DISK:
		/* Nothing to do for these file types (yet) */
		break;

	default:
		/* All other file types have their own callbacks */
		retval |= invoke_callback(state,
					  direct_callback[type].component,
					  direct_callback[type].name,
					  0, buf, len);
		state->errors = retval;
		break;
	}

	retval |= invoke_callback(state, CB_END_TRAVERSAL, "<end>",
				  0, buf, len);
	return retval;
}
