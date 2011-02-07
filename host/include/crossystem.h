/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_CROSSYSTEM_H_
#define VBOOT_REFERENCE_CROSSYSTEM_H_

/* Reads a system property integer.
 *
 * Returns the property value, or -1 if error. */
int VbGetSystemPropertyInt(const char* name);

/* Read a system property string into a destination buffer of the
 * specified size.  Returned string will be null-terminated.  If the
 * buffer is too small, the returned string will be truncated.
 *
 * Returns the passed buffer, or NULL if error. */
const char* VbGetSystemPropertyString(const char* name, char* dest, int size);

/* Sets a system property integer.
 *
 * Returns 0 if success, -1 if error. */
int VbSetSystemPropertyInt(const char* name, int value);

/* Set a system property string.
 *
 * Returns 0 if success, -1 if error. */
int VbSetSystemPropertyString(const char* name, const char* value);

#endif  /* VBOOT_REFERENCE__CROSSYSTEM_H_ */
