/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Signature validation functions
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "2sha.h"

int vb2_verify_common_header(const void *parent, uint32_t parent_size)
{
	const struct vb2_struct_common *c = parent;

	/* Parent buffer size must be at least the claimed total size */
	if (parent_size < c->total_size)
		return VB2_ERROR_COMMON_TOTAL_SIZE;

	/*
	 * And big enough for the fixed size, which itself must be at least as
	 * big as the common struct header.
	 */
	if (c->total_size < c->fixed_size || c->fixed_size < sizeof(*c))
		return VB2_ERROR_COMMON_FIXED_SIZE;

	/* Make sure sizes are all multiples of 32 bits */
	if (!vb2_aligned(c->total_size, sizeof(uint32_t)))
		return VB2_ERROR_COMMON_TOTAL_UNALIGNED;
	if (!vb2_aligned(c->fixed_size, sizeof(uint32_t)))
		return VB2_ERROR_COMMON_FIXED_UNALIGNED;
	if (!vb2_aligned(c->desc_size, sizeof(uint32_t)))
		return VB2_ERROR_COMMON_DESC_UNALIGNED;

	/* Check description */
	if (c->desc_size > 0) {
		/* Make sure description fits and doesn't wrap */
		if (c->fixed_size + c->desc_size < c->fixed_size)
			return VB2_ERROR_COMMON_DESC_WRAPS;
		if (c->fixed_size + c->desc_size > c->total_size)
			return VB2_ERROR_COMMON_DESC_SIZE;

		/* Description must be null-terminated */
		const uint8_t *desc = (const uint8_t *)c + c->fixed_size;
		if (desc[c->desc_size - 1] != 0)
			return VB2_ERROR_COMMON_DESC_TERMINATOR;
	}

	return VB2_SUCCESS;
}

int vb2_verify_common_member(const void *parent,
			     uint32_t *min_offset,
			     uint32_t member_offset,
			     uint32_t member_size)
{
	const struct vb2_struct_common *c = parent;
	uint32_t member_end = member_offset + member_size;

	/* Make sure member doesn't wrap */
	if (member_end < member_offset)
		return VB2_ERROR_COMMON_MEMBER_WRAPS;

	/* Member offset and size must be 32-bit aligned */
	if (!vb2_aligned(member_offset, sizeof(uint32_t)) ||
	    !vb2_aligned(member_size, sizeof(uint32_t)))
		return VB2_ERROR_COMMON_MEMBER_UNALIGNED;

	/* Initialize minimum offset if necessary */
	if (!*min_offset)
		*min_offset = c->fixed_size + c->desc_size;

	/* Member must be after minimum offset */
	if (member_offset < *min_offset)
		return VB2_ERROR_COMMON_MEMBER_OVERLAP;

	/* Member must end before total size */
	if (member_end > c->total_size)
		return VB2_ERROR_COMMON_MEMBER_SIZE;

	/* Update minimum offset for subsequent checks */
	*min_offset = member_end;

	return VB2_SUCCESS;
}

int vb2_verify_common_subobject(const void *parent,
				uint32_t *min_offset,
				uint32_t member_offset)
{
	const struct vb2_struct_common *p = parent;
	const struct vb2_struct_common *m =
		(const struct vb2_struct_common *)
		((const uint8_t *)parent + member_offset);
	int rv;

	/*
	 * Verify the parent has space at the member offset for the common
	 * header.
	 */
	rv = vb2_verify_common_member(parent, min_offset, member_offset,
				      sizeof(*m));
	if (rv)
		return rv;

	/*
	 * Now it's safe to look at the member's header, and verify any
	 * additional data for the object past its common header fits in the
	 * parent.
	 */
	rv = vb2_verify_common_header(m, p->total_size - member_offset);
	if (rv)
		return rv;

	/* Advance the min offset to the end of the subobject */
	*min_offset = member_offset + m->total_size;

	return VB2_SUCCESS;
}
