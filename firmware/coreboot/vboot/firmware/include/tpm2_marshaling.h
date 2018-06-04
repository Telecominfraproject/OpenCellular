/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __SRC_LIB_TPM2_MARSHALING_H
#define __SRC_LIB_TPM2_MARSHALING_H

#include "tss_constants.h"

/* The below functions are used to serialize/deserialize TPM2 commands. */

/**
 * tpm_marshal_command
 *
 * Given a structure containing a TPM2 command, serialize the structure for
 * sending it to the TPM.
 *
 * @command: code of the TPM2 command to marshal
 * @tpm_command_body: a pointer to the command specific structure
 * @buffer: buffer where command is marshaled to
 * @buffer_size: size of the buffer
 *
 * Returns number of bytes placed in the buffer, or -1 on error.
 *
 */
int tpm_marshal_command(TPM_CC command, void *tpm_command_body,
			void *buffer, int buffer_size);

/**
 * tpm_unmarshal_response
 *
 * Given a buffer received from the TPM in response to a certain command,
 * deserialize the buffer into the expeced response structure.
 *
 * @command: code of the TPM2 command for which a response is unmarshaled
 * @response_body: buffer containing the serialized response.
 * @response_size: number of bytes in the buffer containing response
 * @response: structure to be filled with deserialized response,
 *            struct tpm2_response is a union of all possible responses.
 *
 * Returns 0 on success, or -1 on error.
 */
int tpm_unmarshal_response(TPM_CC command,
			   void *response_body,
			   int response_size,
			   struct tpm2_response *response);

/**
 * tpm_get_packet_size
 *
 * @packet: pointer to the start of the command or response packet.
 *
 * Returns the size of the tpm packet.
 */
uint32_t tpm_get_packet_size(const uint8_t *packet);

/**
 * tpm_get_packet_response_code
 *
 * @packet: pointer to the start of the response packet.
 *
 * Returns the response code.
 */
uint32_t tpm_get_packet_response_code(const uint8_t *packet);

/**
 * tpm_set_ph_disabled
 *
 * Sets the flag that indicates if platform hierarchy is disabled.
 * Certain commands, like NV_Read, may need to use different
 * authorization if platform hierarchy is disabled.
 *
 * @flag: 1 if platform hierarchy is disabled, 0 otherwise
 */
void tpm_set_ph_disabled(int flag);

/**
 * tpm_is_ph_disabled
 *
 * Gets the flag that indicates if platform hierarchy is disabled.
 * Certain commands, like NV_Read, may need to use different
 * authorization if platform hierarchy is disabled.
 *
 * Returns 1 if platform hierarchy is disabled, 0 otherwise
 */
int tpm_is_ph_disabled(void);

#endif // __SRC_LIB_TPM2_MARSHALING_H
