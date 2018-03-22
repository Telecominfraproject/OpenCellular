/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef VBOOT_REFERENCE_TSS_CONSTANTS_H_
#define VBOOT_REFERENCE_TSS_CONSTANTS_H_
#include <stdint.h>

#define TPM_SUCCESS ((uint32_t) 0x00000000)

#define TPM_E_ALREADY_INITIALIZED    ((uint32_t) 0x00005000)  /* vboot local */
#define TPM_E_INTERNAL_INCONSISTENCY ((uint32_t) 0x00005001)  /* vboot local */
#define TPM_E_MUST_REBOOT            ((uint32_t) 0x00005002)  /* vboot local */
#define TPM_E_CORRUPTED_STATE        ((uint32_t) 0x00005003)  /* vboot local */
#define TPM_E_COMMUNICATION_ERROR    ((uint32_t) 0x00005004)  /* vboot local */
#define TPM_E_RESPONSE_TOO_LARGE     ((uint32_t) 0x00005005)  /* vboot local */
#define TPM_E_NO_DEVICE              ((uint32_t) 0x00005006)  /* vboot local */
#define TPM_E_INPUT_TOO_SMALL        ((uint32_t) 0x00005007)  /* vboot local */
#define TPM_E_WRITE_FAILURE          ((uint32_t) 0x00005008)  /* vboot local */
#define TPM_E_READ_EMPTY             ((uint32_t) 0x00005009)  /* vboot local */
#define TPM_E_READ_FAILURE           ((uint32_t) 0x0000500a)  /* vboot local */
#define TPM_E_STRUCT_SIZE            ((uint32_t) 0x0000500b)  /* vboot local */
#define TPM_E_STRUCT_VERSION         ((uint32_t) 0x0000500c)  /* vboot local */

/*
 * AP firmware relies on Tlcl returning these exact TPM1.2 error codes
 * regardless of the TPM spec version in certain sitautions. So, TPM2.0 should
 * map to these errors when necessary. All TPM2.0-spec-defined errors have
 * either 0x100 or 0x80 bit set, so there is no confusion with actual error
 * codes returned from a TPM2.0 chip.
 */
#define TPM_E_BADINDEX              ((uint32_t) 0x00000002)
#define TPM_E_BAD_ORDINAL           ((uint32_t) 0x0000000a)
#define TPM_E_OWNER_SET             ((uint32_t) 0x00000014)
#define TPM_E_BADTAG                ((uint32_t) 0x0000001e)
#define TPM_E_IOERROR               ((uint32_t) 0x0000001f)
#define TPM_E_INVALID_POSTINIT      ((uint32_t) 0x00000026)
#define TPM_E_BAD_PRESENCE          ((uint32_t) 0x0000002d)
#define TPM_E_AREA_LOCKED           ((uint32_t) 0x0000003c)
#define TPM_E_MAXNVWRITES           ((uint32_t) 0x00000048)

#define TPM_E_NON_FATAL 0x800
#define TPM_E_NEEDS_SELFTEST ((uint32_t) (TPM_E_NON_FATAL + 1))
#define TPM_E_DOING_SELFTEST ((uint32_t) (TPM_E_NON_FATAL + 2))

#ifdef TPM2_MODE
#include "tpm2_tss_constants.h"
#else
#include "tpm1_tss_constants.h"
#endif

#endif  /* VBOOT_REFERENCE_TSS_CONSTANTS_H_ */
