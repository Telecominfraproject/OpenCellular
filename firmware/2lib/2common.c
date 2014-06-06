/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 * (Firmware portion)
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "2sha.h"

int vb2_align(uint8_t **ptr, uint32_t *size, uint32_t align, uint32_t want_size)
{
	uintptr_t p = (uintptr_t)*ptr;
	uintptr_t offs = p & (align - 1);

	if (offs) {
		offs = align - offs;

		if (*size < offs)
			return VB2_ERROR_ALIGN_BIGGER_THAN_SIZE;

		*ptr += offs;
		*size -= offs;
	}

	if (*size < want_size)
		return VB2_ERROR_ALIGN_SIZE;

	return VB2_SUCCESS;
}

void vb2_workbuf_init(struct vb2_workbuf *wb, uint8_t *buf, uint32_t size)
{
	wb->buf = buf;
	wb->size = size;

	/* Align the buffer so allocations will be aligned */
	if (vb2_align(&wb->buf, &wb->size, VB2_WORKBUF_ALIGN, 0))
		wb->size = 0;
}

/**
 * Round up a number to a multiple of VB2_WORKBUF_ALIGN
 *
 * @param v		Number to round up
 * @return The number, rounded up.
 */
static __inline uint32_t wb_round_up(uint32_t v)
{
	return (v + VB2_WORKBUF_ALIGN - 1) & ~(VB2_WORKBUF_ALIGN - 1);
}

void *vb2_workbuf_alloc(struct vb2_workbuf *wb, uint32_t size)
{
	uint8_t *ptr = wb->buf;

	/* Round up size to work buffer alignment */
	size = wb_round_up(size);

	if (size > wb->size)
		return NULL;

	wb->buf += size;
	wb->size -= size;

	return ptr;
}

void *vb2_workbuf_realloc(struct vb2_workbuf *wb,
			  uint32_t oldsize,
			  uint32_t newsize)
{
	/*
	 * Just free and allocate to update the size.  No need to move/copy
	 * memory, since the new pointer is guaranteed to be the same as the
	 * old one.  The new allocation can fail, if the new size is too big.
	 */
	vb2_workbuf_free(wb, oldsize);
	return vb2_workbuf_alloc(wb, newsize);
}

void vb2_workbuf_free(struct vb2_workbuf *wb, uint32_t size)
{
	/* Round up size to work buffer alignment */
	size = wb_round_up(size);

	wb->buf -= size;
	wb->size += size;
}

const uint8_t *vb2_packed_key_data(const struct vb2_packed_key *key)
{
	return (const uint8_t *)key + key->key_offset;
}

uint8_t *vb2_signature_data(struct vb2_signature *sig)
{
	return (uint8_t *)sig + sig->sig_offset;
}

ptrdiff_t vb2_offset_of(const void *base, const void *ptr)
{
	return (uintptr_t)ptr - (uintptr_t)base;
}

int vb2_verify_member_inside(const void *parent, size_t parent_size,
			     const void *member, size_t member_size,
			     ptrdiff_t member_data_offset,
			     size_t member_data_size)
{
	const size_t psize = (size_t)parent_size;
	const uintptr_t parent_end = (uintptr_t)parent + parent_size;
	const ptrdiff_t member_offs = vb2_offset_of(parent, member);
	const ptrdiff_t member_end_offs = member_offs + member_size;
	const ptrdiff_t data_offs = member_offs + member_data_offset;
	const ptrdiff_t data_end_offs = data_offs + member_data_size;

	/* Make sure parent doesn't wrap */
	if (parent_end < (uintptr_t)parent)
		return VB2_ERROR_INSIDE_PARENT_WRAPS;

	/*
	 * Make sure the member is fully contained in the parent and doesn't
	 * wrap.  Use >, not >=, since member_size = 0 is possible.
	 */
	if (member_end_offs < member_offs)
		return VB2_ERROR_INSIDE_MEMBER_WRAPS;
	if (member_offs < 0 || member_offs > psize || member_end_offs > psize)
		return VB2_ERROR_INSIDE_MEMBER_OUTSIDE;

	/* Make sure parent fully contains member data */
	if (data_end_offs < data_offs)
		return VB2_ERROR_INSIDE_DATA_WRAPS;
	if (data_offs < 0 || data_offs > psize || data_end_offs > psize)
		return VB2_ERROR_INSIDE_DATA_OUTSIDE;

	return VB2_SUCCESS;
}

int vb2_verify_signature_inside(const void *parent,
				uint32_t parent_size,
				const struct vb2_signature *sig)
{
	return vb2_verify_member_inside(parent, parent_size,
					sig, sizeof(*sig),
					sig->sig_offset, sig->sig_size);
}

int vb2_verify_packed_key_inside(const void *parent,
				 uint32_t parent_size,
				 const struct vb2_packed_key *key)
{
	return vb2_verify_member_inside(parent, parent_size,
					key, sizeof(*key),
					key->key_offset, key->key_size);
}

int vb2_unpack_key(struct vb2_public_key *key,
		   const uint8_t *buf,
		   uint32_t size)
{
	const struct vb2_packed_key *packed_key =
		(const struct vb2_packed_key *)buf;
	const uint32_t *buf32;
	uint32_t expected_key_size;
	int rv;

	/* Make sure passed buffer is big enough for the packed key */
	rv = vb2_verify_packed_key_inside(buf, size, packed_key);
	if (rv)
		return rv;

	if (packed_key->algorithm >= VB2_ALG_COUNT) {
		VB2_DEBUG("Invalid algorithm.\n");
		return VB2_ERROR_BAD_ALGORITHM;
	}

	expected_key_size = vb2_packed_key_size(packed_key->algorithm);
	if (!expected_key_size || expected_key_size != packed_key->key_size) {
		VB2_DEBUG("Wrong key size for algorithm\n");
		return VB2_ERROR_BAD_KEY;
	}

	/* Make sure source buffer is 32-bit aligned */
	buf32 = (const uint32_t *)vb2_packed_key_data(packed_key);
	if (!vb_aligned(buf32, sizeof(uint32_t)))
		return VB2_ERROR_BUFFER_UNALIGNED;

	/* Sanity check key array size */
	key->arrsize = buf32[0];
	if (key->arrsize * sizeof(uint32_t) !=
	    vb2_rsa_sig_size(packed_key->algorithm))
		return VB2_ERROR_BAD_KEY;

	key->n0inv = buf32[1];

	/* Arrays point inside the key data */
	key->n = buf32 + 2;
	key->rr = buf32 + 2 + key->arrsize;

	key->algorithm = packed_key->algorithm;

	return VB2_SUCCESS;
}

int vb2_verify_data(const uint8_t *data,
		    uint32_t size,
		    struct vb2_signature *sig,
		    const struct vb2_public_key *key,
		    struct vb2_workbuf *wb)
{
	struct vb2_workbuf wblocal = *wb;
	struct vb2_digest_context *dc;
	uint8_t *digest;
	uint32_t digest_size;
	int rv;

	if (key->algorithm >= VB2_ALG_COUNT)
		return VB2_ERROR_BAD_ALGORITHM;

	if (sig->sig_size != vb2_rsa_sig_size(key->algorithm)) {
		VB2_DEBUG("Wrong data signature size for algorithm, "
			 "sig_size=%d, expected %d for algorithm %d.\n",
			 (int)sig->sig_size, vb2_rsa_sig_size(key->algorithm),
			 key->algorithm);
		return VB2_ERROR_BAD_SIGNATURE;
	}

	if (sig->data_size > size) {
		VB2_DEBUG("Data buffer smaller than length of signed data.\n");
		return VB2_ERROR_UNKNOWN;
	}

	/* Digest goes at start of work buffer */
	digest_size = vb2_digest_size(key->algorithm);
	digest = vb2_workbuf_alloc(&wblocal, digest_size);
	if (!digest)
		return VB2_ERROR_WORKBUF_TOO_SMALL;

	/* Hashing requires temp space for the context */
	dc = vb2_workbuf_alloc(&wblocal, sizeof(*dc));
	if (!dc)
		return VB2_ERROR_WORKBUF_TOO_SMALL;

	rv = vb2_digest_init(dc, key->algorithm);
	if (rv)
		return rv;

	rv = vb2_digest_extend(dc, data, sig->data_size);
	if (rv)
		return rv;

	rv = vb2_digest_finalize(dc, digest, digest_size);
	if (rv)
		return rv;

	vb2_workbuf_free(&wblocal, sizeof(*dc));

	return vb2_verify_digest(key, vb2_signature_data(sig), digest,
				 &wblocal);
}

int vb2_verify_keyblock(struct vb2_keyblock *block,
			uint32_t size,
			const struct vb2_public_key *key,
			struct vb2_workbuf *wb)
{
	struct vb2_signature *sig;
	int rv;

	/* Sanity checks before attempting signature of data */
	if(size < sizeof(*block)) {
		VB2_DEBUG("Not enough space for key block header.\n");
		return VB2_ERROR_BAD_KEYBLOCK;
	}
	if (memcmp(block->magic, KEY_BLOCK_MAGIC, KEY_BLOCK_MAGIC_SIZE)) {
		VB2_DEBUG("Not a valid verified boot key block.\n");
		return VB2_ERROR_BAD_KEYBLOCK;
	}
	if (block->header_version_major != KEY_BLOCK_HEADER_VERSION_MAJOR) {
		VB2_DEBUG("Incompatible key block header version.\n");
		return VB2_ERROR_BAD_KEYBLOCK;
	}
	if (size < block->keyblock_size) {
		VB2_DEBUG("Not enough data for key block.\n");
		return VB2_ERROR_BAD_KEYBLOCK;
	}

	/* Check signature */
	sig = &block->keyblock_signature;

	if (vb2_verify_signature_inside(block, block->keyblock_size, sig)) {
		VB2_DEBUG("Key block signature off end of block\n");
		return VB2_ERROR_BAD_KEYBLOCK;
	}

	/* Make sure advertised signature data sizes are sane. */
	if (block->keyblock_size < sig->data_size) {
		VB2_DEBUG("Signature calculated past end of block\n");
		return VB2_ERROR_BAD_KEYBLOCK;
	}

	VB2_DEBUG("Checking key block signature...\n");
	rv = vb2_verify_data((const uint8_t *)block, size, sig, key, wb);
	if (rv) {
		VB2_DEBUG("Invalid key block signature.\n");
		return VB2_ERROR_BAD_SIGNATURE;
	}

	/* Verify we signed enough data */
	if (sig->data_size < sizeof(struct vb2_keyblock)) {
		VB2_DEBUG("Didn't sign enough data\n");
		return VB2_ERROR_BAD_KEYBLOCK;
	}

	/* Verify data key is inside the block and inside signed data */
	if (vb2_verify_packed_key_inside(block, block->keyblock_size,
					 &block->data_key)) {
		VB2_DEBUG("Data key off end of key block\n");
		return VB2_ERROR_BAD_KEYBLOCK;
	}
	if (vb2_verify_packed_key_inside(block, sig->data_size,
					 &block->data_key)) {
		VB2_DEBUG("Data key off end of signed data\n");
		return VB2_ERROR_BAD_KEYBLOCK;
	}

	/* Success */
	return VB2_SUCCESS;
}

int vb2_verify_fw_preamble(struct vb2_fw_preamble *preamble,
			   uint32_t size,
			   const struct vb2_public_key *key,
			   struct vb2_workbuf *wb)
{
	struct vb2_signature *sig = &preamble->preamble_signature;

	VB2_DEBUG("Verifying preamble.\n");

	/* Sanity checks before attempting signature of data */
	if(size < EXPECTED_VB2FIRMWAREPREAMBLEHEADER2_1_SIZE) {
		VB2_DEBUG("Not enough data for preamble header 2.1.\n");
		return VB2_ERROR_BAD_PREAMBLE;
	}
	if (preamble->header_version_major !=
	    FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR) {
		VB2_DEBUG("Incompatible firmware preamble header version.\n");
		return VB2_ERROR_BAD_PREAMBLE;
	}

	if (preamble->header_version_minor < 1) {
		VB2_DEBUG("Only preamble header 2.1+ supported\n");
		return VB2_ERROR_BAD_PREAMBLE;
	}

	if (size < preamble->preamble_size) {
		VB2_DEBUG("Not enough data for preamble.\n");
		return VB2_ERROR_BAD_PREAMBLE;
	}

	/* Check signature */
	if (vb2_verify_signature_inside(preamble, preamble->preamble_size,
					sig)) {
		VB2_DEBUG("Preamble signature off end of preamble\n");
		return VB2_ERROR_BAD_PREAMBLE;
	}

	/* Make sure advertised signature data sizes are sane. */
	if (preamble->preamble_size < sig->data_size) {
		VB2_DEBUG("Signature calculated past end of the block\n");
		return VB2_ERROR_BAD_PREAMBLE;
	}

	if (vb2_verify_data((const uint8_t *)preamble, size, sig, key, wb)) {
		VB2_DEBUG("Preamble signature validation failed\n");
		return VB2_ERROR_BAD_SIGNATURE;
	}

	/* Verify we signed enough data */
	if (sig->data_size < sizeof(struct vb2_fw_preamble)) {
		VB2_DEBUG("Didn't sign enough data\n");
		return VB2_ERROR_BAD_PREAMBLE;
	}

	/* Verify body signature is inside the signed data */
	if (vb2_verify_signature_inside(preamble, sig->data_size,
					&preamble->body_signature)) {
		VB2_DEBUG("Firmware body signature off end of preamble\n");
		return VB2_ERROR_BAD_PREAMBLE;
	}

	/* Verify kernel subkey is inside the signed data */
	if (vb2_verify_packed_key_inside(preamble, sig->data_size,
					 &preamble->kernel_subkey)) {
		VB2_DEBUG("Kernel subkey off end of preamble\n");
		return VB2_ERROR_BAD_PREAMBLE;
	}

	/* Success */
	return VB2_SUCCESS;
}
