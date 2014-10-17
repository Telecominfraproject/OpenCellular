/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 */

#ifndef VBOOT_REFERENCE_VBOOT_2COMMON_H_
#define VBOOT_REFERENCE_VBOOT_2COMMON_H_

#include "2api.h"
#include "2return_codes.h"
#include "2sha.h"
#include "2struct.h"

struct vb2_public_key;

/*
 * Return the greater of A and B.  This is used in macros which calculate the
 * required buffer size, so can't be turned into a static inline function.
 */
#ifndef VB2_MAX
#define VB2_MAX(A, B) ((A) > (B) ? (A) : (B))
#endif

/*
 * Debug output. printf() for tests. otherwise, it's platform-dependent.
 */
#if defined(VBOOT_DEBUG)
#  if defined(FOR_TEST)
#    define VB2_DEBUG(format, args...) printf(format, ## args)
#  else
#    define VB2_DEBUG(format, args...) vb2ex_printf(__func__, format, ## args)
#  endif
#else
#  define VB2_DEBUG(format, args...)
#endif

/* Alignment for work buffer pointers/allocations */
#define VB2_WORKBUF_ALIGN 8

/* Work buffer */
struct vb2_workbuf {
	uint8_t *buf;
	uint32_t size;
};

/**
 * Initialize a work buffer.
 *
 * @param wb		Work buffer to init
 * @param buf		Pointer to work buffer data
 * @param size		Size of work buffer data in bytes
 */
void vb2_workbuf_init(struct vb2_workbuf *wb, uint8_t *buf, uint32_t size);

/**
 * Allocate space in a work buffer.
 *
 * Note that the returned buffer will always be aligned to VB2_WORKBUF_ALIGN.
 *
 * The work buffer acts like a stack, and detailed tracking of allocs and frees
 * is not done.  The caller must track the size of each allocation and free via
 * vb2_workbuf_free() in the reverse order they were allocated.
 *
 * @param wb		Work buffer
 * @param size		Requested size in bytes
 * @return A pointer to the allocated space, or NULL if error.
 */
void *vb2_workbuf_alloc(struct vb2_workbuf *wb, uint32_t size);

/**
 * Reallocate space in a work buffer.
 *
 * Note that the returned buffer will always be aligned to VB2_WORKBUF_ALIGN.
 * The work buffer acts like a stack, so this must only be done to the most
 * recently allocated buffer.
 *
 * @param wb		Work buffer
 * @param oldsize	Old allocation size in bytes
 * @param newsize	Requested size in bytes
 * @return A pointer to the allocated space, or NULL if error.
 */
void *vb2_workbuf_realloc(struct vb2_workbuf *wb,
			  uint32_t oldsize,
			  uint32_t newsize);

/**
 * Free the preceding allocation.
 *
 * Note that the work buffer acts like a stack, and detailed tracking of
 * allocs and frees is not done.  The caller must track the size of each
 * allocation and free them in reverse order.
 *
 * @param wb		Work buffer
 * @param size		Size of data to free
 */
void vb2_workbuf_free(struct vb2_workbuf *wb, uint32_t size);

/* Check if a pointer is aligned on an align-byte boundary */
#define vb_aligned(ptr, align) (!(((uintptr_t)(ptr)) & ((align) - 1)))

/**
 * Safer memcmp() for use in crypto.
 *
 * Compares the buffers to see if they are equal.  Time taken to perform
 * the comparison is dependent only on the size, not the relationship of
 * the match between the buffers.  Note that unlike memcmp(), this only
 * indicates inequality, not which buffer is lesser.
 *
 * @param s1		First buffer
 * @param s2		Second buffer
 * @param size		Number of bytes to compare
 * @return 0 if match or size=0, non-zero if at least one byte mismatched.
 */
int vb2_safe_memcmp(const void *s1, const void *s2, size_t size);

/**
 * Align a buffer and check its size.
 *
 * @param **ptr		Pointer to pointer to align
 * @param *size		Points to size of buffer pointed to by *ptr
 * @param align		Required alignment (must be power of 2)
 * @param want_size	Required size
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_align(uint8_t **ptr,
	      uint32_t *size,
	      uint32_t align,
	      uint32_t want_size);

/**
 * Return offset of ptr from base.
 *
 * @param base		Base pointer
 * @param ptr		Pointer at some offset from base
 * @return The offset of ptr from base.
 */
ptrdiff_t vb2_offset_of(const void *base, const void *ptr);

/*
 * Helper functions to get data pointed to by a public key or signature.
 */

const uint8_t *vb2_packed_key_data(const struct vb2_packed_key *key);
uint8_t *vb2_signature_data(struct vb2_signature *sig);

/**
 * Verify the data pointed to by a subfield is inside the parent data.
 *
 * The subfield has a header pointed to by member, and a separate data
 * field at an offset relative to the header.  That is:
 *
 *   struct parent {
 *     (possibly other parent fields)
 *     struct member {
 *        (member header fields)
 *     };
 *     (possibly other parent fields)
 *   };
 *   (possibly some other parent data)
 *   (member data)
 *   (possibly some other parent data)
 *
 * @param parent		Parent data
 * @param parent_size		Parent size in bytes
 * @param member		Subfield header
 * @param member_size		Size of subfield header in bytes
 * @param member_data_offset	Offset of member data from start of member
 * @param member_data_size	Size of member data in bytes
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_member_inside(const void *parent, size_t parent_size,
			     const void *member, size_t member_size,
			     ptrdiff_t member_data_offset,
			     size_t member_data_size);

/**
 * Verify a signature is fully contained in its parent data
 *
 * @param parent	Parent data
 * @param parent_size	Parent size in bytes
 * @param sig		Signature pointer
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_signature_inside(const void *parent,
				uint32_t parent_size,
				const struct vb2_signature *sig);

/**
 * Verify a packed key is fully contained in its parent data
 *
 * @param parent	Parent data
 * @param parent_size	Parent size in bytes
 * @param key		Packed key pointer
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_packed_key_inside(const void *parent,
				 uint32_t parent_size,
				 const struct vb2_packed_key *key);

/**
 * Unpack a RSA key for use in verification
 *
 * The elements of the unpacked key will point into the source buffer, so don't
 * free the source buffer until you're done with the key.
 *
 * @param key		Destintion for unpacked key
 * @param buf		Source buffer containing packed key
 * @param size		Size of buffer in bytes
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_unpack_key(struct vb2_public_key *key,
		   const uint8_t *buf,
		   uint32_t size);

/* Size of work buffer sufficient for vb2_rsa_verify_digest() worst case */
#define VB2_VERIFY_DIGEST_WORKBUF_BYTES VB2_VERIFY_RSA_DIGEST_WORKBUF_BYTES

/**
 * Verify a signature against an expected hash digest.
 *
 * @param key		Key to use in signature verification
 * @param sig		Signature to verify (may be destroyed in process)
 * @param digest	Digest of signed data
 * @param wb		Work buffer
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_digest(const struct vb2_public_key *key,
		      struct vb2_signature *sig,
		      const uint8_t *digest,
		      struct vb2_workbuf *wb);

/* Size of work buffer sufficient for vb2_verify_data() worst case */
#define VB2_VERIFY_DATA_WORKBUF_BYTES					\
	(VB2_SHA512_DIGEST_SIZE +					\
	 VB2_MAX(VB2_VERIFY_DIGEST_WORKBUF_BYTES,			\
		 sizeof(struct vb2_digest_context)))

/**
 * Verify data matches signature.
 *
 * @param data		Data to verify
 * @param size		Size of data buffer.  Note that amount of data to
 *			actually validate is contained in sig->data_size.
 * @param sig		Signature of data (destroyed in process)
 * @param key		Key to use to validate signature
 * @param wb		Work buffer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_verify_data(const uint8_t *data,
		    uint32_t size,
		    struct vb2_signature *sig,
		    const struct vb2_public_key *key,
		    struct vb2_workbuf *wb);

/* Size of work buffer sufficient for vb2_verify_keyblock() worst case */
#define VB2_KEY_BLOCK_VERIFY_WORKBUF_BYTES VB2_VERIFY_DATA_WORKBUF_BYTES

/**
 * Check the sanity of a key block using a public key.
 *
 * Header fields are also checked for sanity.  Does not verify key index or key
 * block flags.  Signature inside block is destroyed during check.
 *
 * @param block		Key block to verify
 * @param size		Size of key block buffer
 * @param key		Key to use to verify block
 * @param wb		Work buffer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_verify_keyblock(struct vb2_keyblock *block,
			uint32_t size,
			const struct vb2_public_key *key,
			struct vb2_workbuf *wb);

/* Size of work buffer sufficient for vb2_verify_fw_preamble() worst case */
#define VB2_VERIFY_FIRMWARE_PREAMBLE_WORKBUF_BYTES VB2_VERIFY_DATA_WORKBUF_BYTES

/**
 * Check the sanity of a firmware preamble using a public key.
 *
 * The signature in the preamble is destroyed during the check.
 *
 * @param preamble     	Preamble to verify
 * @param size		Size of preamble buffer
 * @param key		Key to use to verify preamble
 * @param wb		Work buffer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_verify_fw_preamble(struct vb2_fw_preamble *preamble,
			   uint32_t size,
			   const struct vb2_public_key *key,
			   struct vb2_workbuf *wb);

#endif  /* VBOOT_REFERENCE_VBOOT_2COMMON_H_ */
