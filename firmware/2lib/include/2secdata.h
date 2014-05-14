/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Secure non-volatile storage routines
 */

#ifndef VBOOT_REFERENCE_VBOOT_SECDATA_H_
#define VBOOT_REFERENCE_VBOOT_SECDATA_H_

/* Expected value of vb2_secdata.version */
#define VB2_SECDATA_VERSION 2

/* Flags for firmware space */
enum vb2_secdata_flags {
	/*
	 * Last boot was developer mode.  TPM ownership is cleared when
	 * transitioning to/from developer mode.  Set/cleared by
	 * vb2_check_dev_switch().
	 */
	VB2_SECDATA_FLAG_LAST_BOOT_DEVELOPER = (1 << 0),

	/*
	 * Virtual developer mode switch is on.  Set/cleared by the
	 * keyboard-controlled dev screens in recovery mode.  Cleared by
	 * vb2_check_dev_switch().
	 */
	VB2_SECDATA_FLAG_DEV_MODE = (1 << 1),
};

/* Secure data area */
struct vb2_secdata {
	/* Struct version, for backwards compatibility */
	uint8_t struct_version;

	/* Flags; see vb2_secdata_flags */
	uint8_t flags;

	/* Firmware versions */
	uint32_t fw_versions;

	/* Reserved for future expansion */
	uint8_t reserved[3];

	/* CRC; must be last field in struct */
	uint8_t crc8;
} __attribute__((packed));

/* Which param to get/set for vb2_secdata_get() / vb2_secdata_set() */
enum vb2_secdata_param {
	/* Flags; see vb2_secdata_flags */
	VB2_SECDATA_FLAGS = 0,

	/* Firmware versions */
	VB2_SECDATA_VERSIONS,
};

/**
 * Check the CRC of the secure storage context.
 *
 * Use this if reading from secure storage may be flaky, and you want to retry
 * reading it several times.
 *
 * This may be called before vb2_context_init().
 *
 * @param ctx		Context pointer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_secdata_check_crc(const struct vb2_context *ctx);

/**
 * Create fresh data in the secure storage context.
 *
 * Use this only when initializing the secure storage context on a new machine
 * the first time it boots.  Do NOT simply use this if vb2_secdata_check_crc()
 * (or any other API in this library) fails; that could allow the secure data
 * to be rolled back to an insecure state.
 *
 * This may be called before vb2_context_init().
 */
int vb2_secdata_create(struct vb2_context *ctx);

/**
 * Initialize the secure storage context and verify its CRC.
 *
 * This must be called before vb2_secdata_get() or vb2_secdata_set().
 *
 * @param ctx		Context pointer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_secdata_init(struct vb2_context *ctx);

/**
 * Read a secure storage value.
 *
 * @param ctx		Context pointer
 * @param param		Parameter to read
 * @param dest		Destination for value
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_secdata_get(struct vb2_context *ctx,
		    enum vb2_secdata_param param,
		    uint32_t *dest);

/**
 * Write a secure storage value.
 *
 * @param ctx		Context pointer
 * @param param		Parameter to write
 * @param value		New value
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_secdata_set(struct vb2_context *ctx,
		    enum vb2_secdata_param param,
		    uint32_t value);

#endif  /* VBOOT_REFERENCE_VBOOT_2SECDATA_H_ */
