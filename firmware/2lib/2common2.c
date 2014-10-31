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

uint32_t vb2_sig_size(enum vb2_signature_algorithm sig_alg,
		      enum vb2_hash_algorithm hash_alg)
{
	uint32_t digest_size = vb2_digest_size(hash_alg);

	/* Fail if we don't support the hash algorithm */
	if (!digest_size)
		return 0;

	/* Handle unsigned hashes */
	if (sig_alg == VB2_SIG_NONE)
		return digest_size;

	return vb2_rsa_sig_size(sig_alg);
}

int vb2_verify_signature2(const struct vb2_signature2 *sig,
			  uint32_t size)
{
	uint32_t min_offset = 0;
	uint32_t expect_sig_size;
	int rv;

	/* Check magic number */
	if (sig->c.magic != VB2_MAGIC_SIGNATURE2)
		return VB2_ERROR_SIG_MAGIC;

	/* Make sure common header is good */
	rv = vb2_verify_common_header(sig, size);
	if (rv)
		return rv;

	/*
	 * Check for compatible version.  No need to check minor version, since
	 * that's compatible across readers matching the major version, and we
	 * haven't added any new fields.
	 */
	if (sig->c.struct_version_major != VB2_SIGNATURE2_VERSION_MAJOR)
		return VB2_ERROR_SIG_VERSION;

	/* Make sure header is big enough for signature */
	if (sig->c.fixed_size < sizeof(*sig))
		return VB2_ERROR_SIG_HEADER_SIZE;

	/* Make sure signature data is inside */
	rv = vb2_verify_common_member(sig, &min_offset,
				      sig->sig_offset, sig->sig_size);
	if (rv)
		return rv;

	/* Make sure signature size is correct for the algorithm */
	expect_sig_size = vb2_sig_size(sig->sig_alg, sig->hash_alg);
	if (!expect_sig_size)
		return VB2_ERROR_SIG_ALGORITHM;
	if (sig->sig_size != expect_sig_size)
		return VB2_ERROR_SIG_SIZE;

	return VB2_SUCCESS;
}
