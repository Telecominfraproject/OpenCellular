/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of utility functions which call their linux-specific
 * equivalents.
 */

#include <stdint.h>

#include "2sysincludes.h"
#include "2common.h"

#include "tlcl.h"
#include "tlcl_internal.h"
#include "utility.h"
#include "vboot_api.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>


#define TPM_DEVICE_PATH "/dev/tpm0"
/* Retry failed open()s for 5 seconds in 10ms polling intervals. */
#define OPEN_RETRY_DELAY_NS (10 * 1000 * 1000)
#define OPEN_RETRY_MAX_NUM  500
#define COMM_RETRY_MAX_NUM  3

/* TODO: these functions should pass errors back rather than returning void */
/* TODO: if the only callers to these are just wrappers, should just
 * remove the wrappers and call us directly. */


/* The file descriptor for the TPM device.
 */
static int tpm_fd = -1;
/* If the library should exit during an OS-level TPM failure.
 */
static int exit_on_failure = 1;

/* Similar to VbExError, only handle the non-exit case.
 */
static VbError_t DoError(VbError_t result, const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, format, ap);
	va_end(ap);
	if (exit_on_failure)
		exit(1);
	return result;
}

/* Print |n| bytes from array |a| to stderr, with newlines.
 */
__attribute__((unused)) static void DbgPrintBytes(const uint8_t* a, int n)
{
	int i;
	fprintf(stderr, "DEBUG: ");
	for (i = 0; i < n; i++) {
		if (i && i % 16 == 0)
			fprintf(stderr, "\nDEBUG: ");
		fprintf(stderr, "%02x ", a[i]);
	}
	fprintf(stderr, "\n");
}


/* Executes a command on the TPM.
 */
static VbError_t TpmExecute(const uint8_t *in, const uint32_t in_len,
			    uint8_t *out, uint32_t *pout_len)
{
	uint8_t response[TPM_MAX_COMMAND_SIZE];
	if (in_len <= 0) {
		return DoError(TPM_E_INPUT_TOO_SMALL,
			       "invalid command length %d for command 0x%x\n",
			       in_len, in[9]);
	} else if (tpm_fd < 0) {
		return DoError(TPM_E_NO_DEVICE,
			       "the TPM device was not opened.  " \
			       "Forgot to call TlclLibInit?\n");
	} else {
		int n;
		int retries = 0;
		int first_errno = 0;

		/* Write command. Retry in case of communication errors.
		 */
		for ( ; retries < COMM_RETRY_MAX_NUM; ++retries) {
			n = write(tpm_fd, in, in_len);
			if (n >= 0) {
				break;
			}
			if (retries == 0) {
				first_errno = errno;
			}
			VB2_DEBUG("TPM: write attempt %d failed: %s\n",
				  retries + 1, strerror(errno));
		}
		if (n < 0) {
			return DoError(TPM_E_WRITE_FAILURE,
				       "write failure to TPM device: %s "
				       "(first error %d)\n",
				       strerror(errno), first_errno);
		} else if (n != in_len) {
			return DoError(TPM_E_WRITE_FAILURE,
				       "bad write size to TPM device: %d vs %u "
				       "(%d retries, first error %d)\n",
				       n, in_len, retries, first_errno);
		}

		/* Read response. Retry in case of communication errors.
		 */
		for (retries = 0, first_errno = 0;
		     retries < COMM_RETRY_MAX_NUM; ++retries) {
			n = read(tpm_fd, response, sizeof(response));
			if (n >= 0) {
				break;
			}
			if (retries == 0) {
				first_errno = errno;
			}
			VB2_DEBUG("TPM: read attempt %d failed: %s\n",
				  retries + 1, strerror(errno));
		}
		if (n == 0) {
			return DoError(TPM_E_READ_EMPTY,
				       "null read from TPM device\n");
		} else if (n < 0) {
			return DoError(TPM_E_READ_FAILURE,
				       "read failure from TPM device: %s "
				       "(first error %d)\n",
				       strerror(errno), first_errno);
		} else {
			if (n > *pout_len) {
				return DoError(TPM_E_RESPONSE_TOO_LARGE,
					       "TPM response too long for "
					       "output buffer\n");
			} else {
				*pout_len = n;
				memcpy(out, response, n);
			}
		}
	}
	return VBERROR_SUCCESS;
}

/* Gets the tag field of a TPM command.
 */
__attribute__((unused))
static inline int TpmTag(const uint8_t* buffer)
{
	uint16_t tag;
	FromTpmUint16(buffer, &tag);
	return (int) tag;
}

/* Gets the size field of a TPM command.
 */
__attribute__((unused))
static inline int TpmResponseSize(const uint8_t* buffer)
{
	uint32_t size;
	FromTpmUint32(buffer + sizeof(uint16_t), &size);
	return (int) size;
}

VbError_t VbExTpmInit(void)
{
	char *no_exit = getenv("TPM_NO_EXIT");
	if (no_exit)
		exit_on_failure = !atoi(no_exit);
	return VbExTpmOpen();
}

VbError_t VbExTpmClose(void)
{
	if (tpm_fd != -1) {
		close(tpm_fd);
		tpm_fd = -1;
	}
	return VBERROR_SUCCESS;
}

VbError_t VbExTpmOpen(void)
{
	char* device_path;
	struct timespec delay;
	int retries, saved_errno;

	if (tpm_fd >= 0)
		return VBERROR_SUCCESS;  /* Already open */

	device_path = getenv("TPM_DEVICE_PATH");
	if (device_path == NULL) {
		device_path = TPM_DEVICE_PATH;
	}

	/* Retry TPM opens on EBUSY failures. */
	for (retries = 0; retries < OPEN_RETRY_MAX_NUM; ++ retries) {
		errno = 0;
		tpm_fd = open(device_path, O_RDWR | O_CLOEXEC);
		saved_errno = errno;
		if (tpm_fd >= 0)
			return VBERROR_SUCCESS;
		if (saved_errno != EBUSY)
			break;

		VB2_DEBUG("TPM: retrying %s: %s\n",
			  device_path, strerror(errno));

		/* Stall until TPM comes back. */
		delay.tv_sec = 0;
		delay.tv_nsec = OPEN_RETRY_DELAY_NS;
		nanosleep(&delay, NULL);
	}
	return DoError(TPM_E_NO_DEVICE, "TPM: Cannot open TPM device %s: %s\n",
		       device_path, strerror(saved_errno));
}

VbError_t VbExTpmSendReceive(const uint8_t* request, uint32_t request_length,
                             uint8_t* response, uint32_t* response_length)
{
	/*
	 * In a real firmware implementation, this function should contain
	 * the equivalent API call for the firmware TPM driver which takes a
	 * raw sequence of bytes as input command and a pointer to the
	 * output buffer for putting in the results.
	 *
	 * For EFI firmwares, this can make use of the EFI TPM driver as
	 * follows (based on page 16, of TCG EFI Protocol Specs Version 1.20
	 * availaible from the TCG website):
	 *
	 * EFI_STATUS status;
	 * status = TcgProtocol->EFI_TCG_PASS_THROUGH_TO_TPM(
	 *		TpmCommandSize(request),
	 *              request,
	 *              max_length,
	 *              response);
	 * // Error checking depending on the value of the status above
	 */
#ifndef NDEBUG
	int tag, response_tag;
#endif
	VbError_t result;

#ifdef VBOOT_DEBUG
	struct timeval before, after;
	VB2_DEBUG("request (%d bytes):\n", request_length);
	DbgPrintBytes(request, request_length);
	gettimeofday(&before, NULL);
#endif

	result = TpmExecute(request, request_length, response, response_length);
	if (result != VBERROR_SUCCESS)
		return result;

#ifdef VBOOT_DEBUG
	gettimeofday(&after, NULL);
	VB2_DEBUG("response (%d bytes):\n", *response_length);
	DbgPrintBytes(response, *response_length);
	VB2_DEBUG("execution time: %dms\n",
		  (int) ((after.tv_sec - before.tv_sec) * 1000 +
			 (after.tv_usec - before.tv_usec) / 1000));
#endif

#ifndef NDEBUG
	/* sanity checks */
	tag = TpmTag(request);
	response_tag = TpmTag(response);
	assert(
	    (tag == TPM_TAG_RQU_COMMAND &&
	     response_tag == TPM_TAG_RSP_COMMAND) ||
	    (tag == TPM_TAG_RQU_AUTH1_COMMAND &&
	     response_tag == TPM_TAG_RSP_AUTH1_COMMAND) ||
	    (tag == TPM_TAG_RQU_AUTH2_COMMAND &&
	     response_tag == TPM_TAG_RSP_AUTH2_COMMAND));
	assert(*response_length == TpmResponseSize(response));
#endif

	return VBERROR_SUCCESS;
}

VbError_t VbExTpmGetRandom(uint8_t *buf, uint32_t length)
{
	static int urandom_fd = -1;
	if (urandom_fd < 0) {
		urandom_fd = open("/dev/urandom", O_RDONLY);
		if (urandom_fd == -1) {
			return VBERROR_UNKNOWN;
		}
	}

	if (length != read(urandom_fd, buf, length)) {
		return VBERROR_UNKNOWN;
	}

	return VBERROR_SUCCESS;
}
