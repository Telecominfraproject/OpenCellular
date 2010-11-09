/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* TPM Lightweight Command Library.
 *
 * A low-level library for interfacing to TPM hardware or an emulator.
 */

#ifndef VBOOT_REFERENCE_TLCL_STUB_H_
#define VBOOT_REFERENCE_TLCL_STUB_H_

#include "sysincludes.h"
#include "tss_constants.h"

/*****************************************************************************/
/* Functions to be implemented by the stub library */

/* Initialize the stub library */
void TlclStubInit(void);

/* Close and open the device.  This is needed for running more complex commands
 * at user level, such as TPM_TakeOwnership, since the TPM device can be opened
 * only by one process at a time.
 */
void TlclCloseDevice(void);
void TlclOpenDevice(void);

/* Send data to the TPM and receive a response.  Returns 0 if success,
 * nonzero if error. */
uint32_t TlclStubSendReceive(const uint8_t* request, int request_length,
                             uint8_t* response, int max_length);

#endif  /* VBOOT_REFERENCE_TLCL_STUB_H_ */
