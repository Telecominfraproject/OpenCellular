/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_COMMON_H_
#define VBOOT_REFERENCE_HOST_COMMON_H_

/*
 * Host is allowed direct use of stdlib funcs such as malloc() and free(),
 * since it's using the stub implementation from firmware/lib/stub.
 */
#define _STUB_IMPLEMENTATION_

#include "cryptolib.h"
#include "host_key.h"
#include "host_key2.h"
#include "host_keyblock.h"
#include "host_misc.h"
#include "host_signature.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_struct.h"

/**
 * Create a firmware preamble.
 *
 * @param firmware_version	Firmware version
 * @param kernel_subkey		Kernel subkey to store in preamble
 * @param body_signature	Signature of firmware body
 * @param signing_key		Private key to sign header with
 * @param flags			Firmware preamble flags
 *
 * @return The preamble, or NULL if error.  Caller must free() it.
 */
struct vb2_fw_preamble *vb2_create_fw_preamble(
	uint32_t firmware_version,
	const struct vb2_packed_key *kernel_subkey,
	const struct vb2_signature *body_signature,
	const struct vb2_private_key *signing_key,
	uint32_t flags);


/**
 * Create a kernel preamble.
 *
 * @param kernel_version		Firmware version
 * @param body_load_address		Load address for kernel body
 * @param bootloader_address		Load address for bootloader
 * @param bootloader_size		Size of bootloader in bytes
 * @param body_signature		Signature of kernel body
 * @param vmlinuz_header_address	Load address for 16-bit vmlinuz header
 * @param vmlinuz_header_size		Size of 16-bit vmlinuz header in bytes
 * @param flags				Kernel preamble flags
 * @param desired_size			Minimum size of preamble in bytes
 * @param signing_key			Private key to sign header with
 *
 * @return The preamble, or NULL if error.  Caller must free() it.
 */
struct vb2_kernel_preamble *vb2_create_kernel_preamble(
	uint32_t kernel_version,
	uint64_t body_load_address,
	uint64_t bootloader_address,
	uint32_t bootloader_size,
	const struct vb2_signature *body_signature,
	uint64_t vmlinuz_header_address,
	uint32_t vmlinuz_header_size,
	uint32_t flags,
	uint32_t desired_size,
	const struct vb2_private_key *signing_key);

#endif  /* VBOOT_REFERENCE_HOST_COMMON_H_ */
