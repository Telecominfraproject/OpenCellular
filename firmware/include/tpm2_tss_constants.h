/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Some TPM constants and type definitions for standalone compilation for use
 * in the firmware
 */

#ifndef __VBOOT_REFERENCE_FIRMWARE_INCLUDE_TPM2_TSS_CONSTANTS_H
#define __VBOOT_REFERENCE_FIRMWARE_INCLUDE_TPM2_TSS_CONSTANTS_H

#define TPM_BUFFER_SIZE 256

/* Tpm2 command tags. */
#define TPM_ST_NO_SESSIONS 0x8001
#define TPM_ST_SESSIONS    0x8002

/* TPM2 command codes. */
#define TPM2_Hierarchy_Control ((TPM_CC)0x00000121)
#define TPM2_NV_Write          ((TPM_CC)0x00000137)
#define TPM2_NV_WriteLock      ((TPM_CC)0x00000138)
#define TPM2_NV_Read           ((TPM_CC)0x0000014E)

/* TCG Spec defined, verify for TPM2.
 * TODO(apronin): find TPM2 RC substitutes for TPM1.2 error codes.
 */
#define TPM_E_BADINDEX              ((uint32_t) 0x00000002)
#define TPM_E_INVALID_POSTINIT      ((uint32_t) 0x00000026)
#define TPM_E_BADTAG                ((uint32_t) 0x0000001E)
#define TPM_E_IOERROR               ((uint32_t) 0x0000001F)
#define TPM_E_MAXNVWRITES           ((uint32_t) 0x00000048)

#define HR_SHIFT               24
#define TPM_HT_NV_INDEX        0x01
#define HR_NV_INDEX           (TPM_HT_NV_INDEX <<  HR_SHIFT)
#define TPM_RH_PLATFORM     0x4000000C
#define TPM_RS_PW           0x40000009


typedef uint8_t TPMI_YES_NO;
typedef uint32_t TPM_CC;
typedef uint32_t TPM_HANDLE;
typedef TPM_HANDLE TPMI_RH_NV_INDEX;
typedef TPM_HANDLE TPMI_RH_ENABLES;

typedef struct {
	uint16_t      size;
	uint8_t       *buffer;
} TPM2B;

typedef union {
	struct {
		uint16_t  size;
		const uint8_t   *buffer;
	} t;
	TPM2B b;
} TPM2B_MAX_NV_BUFFER;

struct tpm2_nv_read_cmd {
	TPMI_RH_NV_INDEX nvIndex;
	uint16_t size;
	uint16_t offset;
};

struct tpm2_nv_write_cmd {
	TPMI_RH_NV_INDEX nvIndex;
	TPM2B_MAX_NV_BUFFER data;
	uint16_t offset;
};

struct tpm2_nv_write_lock_cmd {
	TPMI_RH_NV_INDEX nvIndex;
};

struct tpm2_hierarchy_control_cmd {
	TPMI_RH_ENABLES enable;
	TPMI_YES_NO state;
};

/* Common command/response header. */
struct tpm_header {
	uint16_t tpm_tag;
	uint32_t tpm_size;
	TPM_CC tpm_code;
} __attribute__((packed));

struct nv_read_response {
	uint32_t params_size;
	TPM2B_MAX_NV_BUFFER buffer;
};

struct tpm2_session_attrs {
	uint8_t continueSession : 1;
	uint8_t auditExclusive  : 1;
	uint8_t auditReset      : 1;
	uint8_t reserved3_4     : 2;
	uint8_t decrypt         : 1;
	uint8_t encrypt         : 1;
	uint8_t audit           : 1;
};

struct tpm2_session_header {
	uint32_t session_handle;
	uint16_t nonce_size;
	uint8_t *nonce;
	union {
		struct tpm2_session_attrs session_attr_bits;
		uint8_t session_attrs;
	}  __attribute__((packed));
	uint16_t auth_size;
	uint8_t *auth;
};

struct tpm2_response {
	struct tpm_header hdr;
	union {
		struct nv_read_response nvr;
		struct tpm2_session_header def_space;
	};
};


/* Temp stubs to quiet down compilation errors. */
typedef struct {} TPM_PERMANENT_FLAGS;
typedef struct {} TPM_STCLEAR_FLAGS;

/* TODO(apronin): For TPM2 certain properties must be received using
 * TPM2_GetCapability instead of being hardcoded as they are now:
 * TPM_MAX_COMMAND_SIZE -> use TPM_PT_MAX_COMMAND_SIZE for TPM2.
 * TPM_PCR_DIGEST -> use TPM_PT_MAX_DIGEST for TPM2.
 */
#define TPM_MAX_COMMAND_SIZE	4096
#define TPM_PCR_DIGEST		32

#endif  /* ! __VBOOT_REFERENCE_FIRMWARE_INCLUDE_TPM2_TSS_CONSTANTS_H */
