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

#ifdef TPM2_MODE
#include "tpm2_tss_constants.h"
#else
#include "tpm1_tss_constants.h"
#endif

#endif  /* VBOOT_REFERENCE_TSS_CONSTANTS_H_ */
