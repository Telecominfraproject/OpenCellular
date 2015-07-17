/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "2struct.h"
#include "cryptolib.h"
#include "futility.h"
#include "gbb_header.h"

#define SEARCH_STRIDE 4

/**
 * Check if the pointer contains the magic string.  We need to use a
 * case-swapped version, so that the actual magic string doesn't appear in the
 * code, to avoid falsely finding it when searching for the struct.
 */
static int is_magic(const void *ptr)
{
	const char magic_inv[RYU_ROOT_KEY_HASH_MAGIC_SIZE] =
		RYU_ROOT_KEY_HASH_MAGIC_INVCASE;
	const char *magic = ptr;
	int i;

	for (i = 0; i < RYU_ROOT_KEY_HASH_MAGIC_SIZE; i++) {
		if (magic[i] != (magic_inv[i] ^ 0x20))
			return 0;
	}

	return 1;
}

static int valid_ryu_root_header(struct vb2_ryu_root_key_hash *hash,
				 size_t size)
{
	if (!is_magic(hash->magic))
		return 0;  /* Wrong magic */

	if (hash->header_version_major != RYU_ROOT_KEY_HASH_VERSION_MAJOR)
		return 0;  /* Version we can't parse */

	if (hash->struct_size < EXPECTED_VB2_RYU_ROOT_KEY_HASH_SIZE)
		return 0;  /* Header too small */

	if (hash->struct_size > size)
		return 0;  /* Claimed size doesn't fit in buffer */

	return 1;
}

/**
 * Find the root key hash struct and return it or NULL if error.
 */
static struct vb2_ryu_root_key_hash *find_ryu_root_header(uint8_t *ptr,
							  size_t size)
{
	size_t i;
	struct vb2_ryu_root_key_hash *tmp, *hash = NULL;
	int count = 0;

	/* Look for the ryu root key hash header */
	for (i = 0; i <= size - SEARCH_STRIDE; i += SEARCH_STRIDE) {
		if (!is_magic(ptr + i))
			continue;

		/* Found something. See if it's any good. */
		tmp = (struct vb2_ryu_root_key_hash *) (ptr + i);
		if (valid_ryu_root_header(tmp, size - i))
			if (!count++)
				hash = tmp;
	}

	switch (count) {
	case 0:
		return NULL;
	case 1:
		return hash;
	default:
		fprintf(stderr,
			"WARNING: multiple ryu root hash headers found\n");
		/* But hey, it's only a warning.  Use the first one. */
		return hash;
	}
}

static void calculate_root_key_hash(uint8_t *digest, size_t digest_size,
				    const GoogleBinaryBlockHeader *gbb)
{
	const uint8_t *gbb_base = (const uint8_t *)gbb;

	internal_SHA256(gbb_base + gbb->rootkey_offset,
			gbb->rootkey_size,
			digest);
}

int fill_ryu_root_header(uint8_t *ptr, size_t size,
			 const GoogleBinaryBlockHeader *gbb)
{
	struct vb2_ryu_root_key_hash *hash;

	/*
	 * Find the ryu root header.  If not found, nothing we can do, but
	 * that's ok because most images don't have the header.
	 */
	hash = find_ryu_root_header(ptr, size);
	if (!hash)
		return 0;

	/* Update the hash stored in the header based on the root key */
	calculate_root_key_hash(hash->root_key_hash_digest,
				sizeof(hash->root_key_hash_digest),
				gbb);

	printf(" - calculate ryu root hash: success\n");
	return 0;
}

int verify_ryu_root_header(uint8_t *ptr, size_t size,
			   const GoogleBinaryBlockHeader *gbb)
{
	uint8_t digest[SHA256_DIGEST_SIZE] = {0};

	struct vb2_ryu_root_key_hash *hash;

	/*
	 * Find the ryu root header.  If not found, nothing we can do, but
	 * that's ok because most images don't have the header.
	 */
	hash = find_ryu_root_header(ptr, size);
	if (!hash) {
		printf(" - ryu root hash not found\n");
		return 0;
	}

	/* Check for all 0's, which means hash hasn't been set */
	if (0 == memcmp(digest, hash->root_key_hash_digest,
			SHA256_DIGEST_SIZE)) {
		printf(" - ryu root hash is unset\n");
		return 0;
	}

	/* Update the hash stored in the header based on the root key */
	calculate_root_key_hash(digest, sizeof(digest), gbb);

	if (0 == memcmp(digest, hash->root_key_hash_digest,
			SHA256_DIGEST_SIZE)) {
		printf(" - ryu root hash verified\n");
		return 0;
	} else {
		printf(" - ryu root hash does not verify\n");
		return -1;
	}
}
