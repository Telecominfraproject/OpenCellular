/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Architecture-specific APIs for crossystem
 */

#ifndef VBOOT_REFERENCE_CROSSYSTEM_ARCH_H_
#define VBOOT_REFERENCE_CROSSYSTEM_ARCH_H_

#include <stddef.h>

#include "vboot_nvstorage.h"
#include "vboot_struct.h"

/* Firmware types from BINF.3. Placed in the common file because both x86 and
 * arm use this. The constants are defined in "Chrome OS Main Processor
 * Firmware Spec"
 */
#define BINF3_RECOVERY   0
#define BINF3_NORMAL     1
#define BINF3_DEVELOPER  2
#define BINF3_NETBOOT    3


/* INTERNAL APIS FOR CROSSYSTEM AVAILABLE TO ARCH-SPECIFIC FUNCTIONS */

/* Read an integer property from VbNvStorage.
 *
 * Returns the parameter value, or -1 if error. */
int VbGetNvStorage(VbNvParam param);

/* Write an integer property to VbNvStorage.
 *
 * Returns 0 if success, -1 if error. */
int VbSetNvStorage(VbNvParam param, int value);

/* Return true if the FWID starts with the specified string. */
int FwidStartsWith(const char *start);

/* Return version of VbSharedData struct or -1 if not found. */
int VbSharedDataVersion(void);

/* Apis WITH ARCH-SPECIFIC IMPLEMENTATIONS */

/* Read the non-volatile context from NVRAM.
 *
 * Returns 0 if success, -1 if error. */
int VbReadNvStorage(VbNvContext* vnc);

/* Write the non-volatile context to NVRAM.
 *
 * Returns 0 if success, -1 if error. */
int VbWriteNvStorage(VbNvContext* vnc);

/* Read the VbSharedData buffer.
 *
 * Verifies the buffer contains at least enough data for the
 * VbSharedDataHeader; if not, this is an error.
 *
 * If less data is read than expected, sets the returned structure's data_size
 * to the actual amount of data read.  If this is less than data_used, then
 * some data was not returned; callers must handle this; this is not considered
 * an error.
 *
 * Returns the data buffer, which must be freed by the caller using
 * free(), or NULL if error. */
VbSharedDataHeader* VbSharedDataRead(void);

/* Read an architecture-specific system property integer.
 *
 * Returns the property value, or -1 if error. */
int VbGetArchPropertyInt(const char* name);

/* Read an architecture-specific system property string into a
 * destination buffer of the specified size.  Returned string will be
 * null-terminated.  If the buffer is too small, the returned string
 * will be truncated.
 *
 * Returns the passed buffer, or NULL if error. */
const char* VbGetArchPropertyString(const char* name, char* dest, size_t size);

/* Set an architecture-specific system property integer.
 *
 * Returns 0 if success, -1 if error. */
int VbSetArchPropertyInt(const char* name, int value);

/* Set an architecture-specific system property string.
 *
 * Returns 0 if success, -1 if error. */
int VbSetArchPropertyString(const char* name, const char* value);

#endif  /* VBOOT_REFERENCE__CROSSYSTEM_ARCH_H_ */
