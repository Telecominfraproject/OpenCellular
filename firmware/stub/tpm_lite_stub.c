/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of utility functions which call their linux-specific
 * equivalents.
 */

#define _STUB_IMPLEMENTATION_
#include "tlcl.h"
#include "tlcl_internal.h"
#include "utility.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define TPM_DEVICE_PATH "/dev/tpm0"

/* TODO: these functions should pass errors back rather than returning void */
/* TODO: if the only callers to these are just wrappers, should just
 * remove the wrappers and call us directly. */


/* The file descriptor for the TPM device.
 */
static int tpm_fd = -1;


/* Print |n| bytes from array |a|, with newlines.
 */
POSSIBLY_UNUSED static void PrintBytes(const uint8_t* a, int n) {
  int i;
  for (i = 0; i < n; i++) {
    VBDEBUG(("%02x ", a[i]));
    if ((i + 1) % 16 == 0) {
      VBDEBUG(("\n"));
    }
  }
  if (i % 16 != 0) {
    VBDEBUG(("\n"));
  }
}


/* Executes a command on the TPM.
 */
static void TpmExecute(const uint8_t *in, const uint32_t in_len,
                uint8_t *out, uint32_t *pout_len) {
  uint8_t response[TPM_MAX_COMMAND_SIZE];
  if (in_len <= 0) {
    error("invalid command length %d for command 0x%x\n", in_len, in[9]);
  } else if (tpm_fd < 0) {
    error("the TPM device was not opened.  Forgot to call TlclLibInit?\n");
  } else {
    int n = write(tpm_fd, in, in_len);
    if (n != in_len) {
      error("write failure to TPM device: %s\n", strerror(errno));
    }
    n = read(tpm_fd, response, sizeof(response));
    if (n == 0) {
      error("null read from TPM device\n");
    } else if (n < 0) {
      error("read failure from TPM device: %s\n", strerror(errno));
    } else {
      if (n > *pout_len) {
        error("TPM response too long for output buffer\n");
      } else {
        *pout_len = n;
        Memcpy(out, response, n);
      }
    }
  }
}


/* Gets the tag field of a TPM command.
 */
POSSIBLY_UNUSED static INLINE int TpmTag(const uint8_t* buffer) {
  uint16_t tag;
  FromTpmUint16(buffer, &tag);
  return (int) tag;
}


/* Gets the size field of a TPM command.
 */
POSSIBLY_UNUSED static INLINE int TpmResponseSize(const uint8_t* buffer) {
  uint32_t size;
  FromTpmUint32(buffer + sizeof(uint16_t), &size);
  return (int) size;
}


uint32_t TlclStubInit(void) {
  return TlclOpenDevice();
}


uint32_t TlclCloseDevice(void) {
  if (tpm_fd != -1) {
    close(tpm_fd);
    tpm_fd = -1;
  }
  return 0;
}


uint32_t TlclOpenDevice(void) {
  char* device_path;

  if (tpm_fd >= 0)
    return 0;  /* Already open */

  device_path = getenv("TPM_DEVICE_PATH");
  if (device_path == NULL) {
    device_path = TPM_DEVICE_PATH;
  }

  tpm_fd = open(device_path, O_RDWR);
  if (tpm_fd < 0) {
    VBDEBUG(("TPM: Cannot open TPM device %s: %s\n", device_path,
             strerror(errno)));
    return TPM_E_IOERROR;
  }

  return 0;
}


uint32_t TlclStubSendReceive(const uint8_t* request, int request_length,
                             uint8_t* response, int max_length) {
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
   * status = TcgProtocol->EFI_TCG_PASS_THROUGH_TO_TPM(TpmCommandSize(request),
   *                                                   request,
   *                                                   max_length,
   *                                                   response);
   * // Error checking depending on the value of the status above
   */
  uint32_t response_length = max_length;
#ifndef NDEBUG
  int tag, response_tag;
#endif

  struct timeval before, after;
  gettimeofday(&before, NULL);
  TpmExecute(request, request_length, response, &response_length);
  gettimeofday(&after, NULL);

#ifdef VBOOT_DEBUG
  {
    int x = request_length;
    int y = response_length;
    VBDEBUG(("request (%d bytes): ", x));
    PrintBytes(request, 10);
    PrintBytes(request + 10, x - 10);
    VBDEBUG(("response (%d bytes): ", y));
    PrintBytes(response, 10);
    PrintBytes(response + 10, y - 10);
    VBDEBUG(("execution time: %dms\n",
            (int) ((after.tv_sec - before.tv_sec) * 1000 +
                   (after.tv_usec - before.tv_usec) / 1000)));
  }
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
  assert(response_length == TpmResponseSize(response));
#endif

  return 0;  /* Success */
}
