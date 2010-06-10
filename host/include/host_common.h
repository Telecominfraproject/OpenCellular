/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_COMMON_H_
#define VBOOT_REFERENCE_HOST_COMMON_H_

#include <stdint.h>

#include "cryptolib.h"
#include "host_key.h"
#include "host_misc.h"
#include "host_signature.h"
#include "utility.h"
#include "vboot_struct.h"


/* Create a key block header containing [data_key] and [flags], signed
 * by [signing_key].  Caller owns the returned pointer, and must free
 * it with Free(). */
VbKeyBlockHeader* CreateKeyBlock(const VbPublicKey* data_key,
                                 const VbPrivateKey* signing_key,
                                 uint64_t flags);


/* Creates a firmware preamble, signed with [signing_key].
 * Caller owns the returned pointer, and must free it with Free().
 *
 * Returns NULL if error. */
VbFirmwarePreambleHeader* CreateFirmwarePreamble(
    uint64_t firmware_version,
    const VbPublicKey* kernel_subkey,
    const VbSignature* body_signature,
    const VbPrivateKey* signing_key);


/* Creates a kernel preamble, signed with [signing_key].
 * Caller owns the returned pointer, and must free it with Free().
 *
 * Returns NULL if error. */
VbKernelPreambleHeader* CreateKernelPreamble(
    uint64_t kernel_version,
    uint64_t body_load_address,
    uint64_t bootloader_address,
    uint64_t bootloader_size,
    const VbSignature* body_signature,
    uint64_t desired_size,
    const VbPrivateKey* signing_key);

#endif  /* VBOOT_REFERENCE_HOST_COMMON_H_ */
