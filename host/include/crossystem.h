/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_CROSSYSTEM_H_
#define VBOOT_REFERENCE_CROSSYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* Recommended size for string property buffers used with
 * VbGetSystemPropertyString(). */
#define VB_MAX_STRING_PROPERTY     ((size_t) 8192)

/* Reads a system property integer.
 *
 * Returns the property value, or -1 if error. */
int VbGetSystemPropertyInt(const char* name);

/* Read a system property string into a destination buffer of the
 * specified size.  Returned string will be null-terminated.  If the
 * buffer is too small, the returned string will be truncated.
 *
 * The caller can expect an un-truncated value if the size provided is
 * at least VB_MAX_STRING_PROPERTY.
 *
 * Returns the passed buffer, or NULL if error. */
const char* VbGetSystemPropertyString(const char* name, char* dest,
                                      size_t size);

/* Sets a system property integer.
 *
 * Returns 0 if success, -1 if error. */
int VbSetSystemPropertyInt(const char* name, int value);

/* Set a system property string.
 *
 * The maximum length of the value accepted depends on the specific
 * property, not on VB_MAX_STRING_PROPERTY.
 *
 * Returns 0 if success, -1 if error. */
int VbSetSystemPropertyString(const char* name, const char* value);

#ifdef __cplusplus
}
#endif

#endif  /* VBOOT_REFERENCE__CROSSYSTEM_H_ */
