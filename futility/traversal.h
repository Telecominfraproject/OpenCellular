/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_FUTILITY_TRAVERSAL_H_
#define VBOOT_REFERENCE_FUTILITY_TRAVERSAL_H_
#include <stdint.h>


/* What type of things do I know how to handle? */
enum futil_file_type {
	FILE_TYPE_UNKNOWN = 0,
	FILE_TYPE_PUBKEY,			/* 1 VbPublicKey */
	FILE_TYPE_KEYBLOCK,			/* 2 VbKeyBlockHeader */
	FILE_TYPE_FIRMWARE,			/* 3 VbFirmwarePreambleHeader */
	FILE_TYPE_GBB,				/* 4 GoogleBinaryBlockHeader */
	FILE_TYPE_BIOS_IMAGE,			/* 5 Chrome OS BIOS image */
	FILE_TYPE_OLD_BIOS_IMAGE,		/* 6 Old Chrome OS BIOS image */

	NUM_FILE_TYPES
};

/* What are we trying to accomplish? */
enum futil_op_type {
	FUTIL_OP_SHOW,

	NUM_FUTIL_OPS
};

/* What component are we currently handling in the callback routine? */
enum futil_cb_component {
	/* entire input buffer */
	CB_BEGIN_TRAVERSAL,
	CB_END_TRAVERSAL,
	/* fmap areas within a bios image */
	CB_FMAP_GBB,
	CB_FMAP_VBLOCK_A,
	CB_FMAP_VBLOCK_B,
	CB_FMAP_FW_MAIN_A,
	CB_FMAP_FW_MAIN_B,
	/* individual files (extracted from a bios, for example) */
	CB_PUBKEY,
	CB_KEYBLOCK,
	CB_GBB,
	CB_FW_PREAMBLE,

	NUM_CB_COMPONENTS
};

/* Where is the component we're poking at? */
struct cb_area_s {
	uint32_t offset;			/* to avoid pointer math */
	uint8_t *buf;
	uint32_t len;
	uint32_t _flags;			/* for callback use */
};

/* What do we know at this point in time? */
struct futil_traverse_state_s {
	/* These two should be initialized by the caller */
	const char *in_filename;
	enum futil_op_type op;
	/* Current activity during traversal */
	enum futil_cb_component component;
	struct cb_area_s *my_area;
	const char *name;
	/* Other activites, possibly before or after the current one */
	struct cb_area_s cb_area[NUM_CB_COMPONENTS];
	struct cb_area_s recovery_key;
	struct cb_area_s rootkey;
	enum futil_file_type in_type;
	int errors;
};

/*
 * Traverse the input file using the provided state
 * Return nonzero (but no details) if there were any errors.
 */
int futil_traverse(int ifd, struct futil_traverse_state_s *state);

/* These are invoked by the traversal. They also return nonzero on error. */
int futil_cb_show_begin(struct futil_traverse_state_s *state);
int futil_cb_show_key(struct futil_traverse_state_s *state);
int futil_cb_show_gbb(struct futil_traverse_state_s *state);
int futil_cb_show_keyblock(struct futil_traverse_state_s *state);
int futil_cb_show_fw_main(struct futil_traverse_state_s *state);
int futil_cb_show_fw_preamble(struct futil_traverse_state_s *state);

#endif /* VBOOT_REFERENCE_FUTILITY_TRAVERSAL_H_ */
