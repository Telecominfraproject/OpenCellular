/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * vboot nv storage related functions exported for use by userspace programs
 */

#ifndef VBOOT_REFERENCE_CROSSYSTEM_VBNV_H_
#define VBOOT_REFERENCE_CROSSYSTEM_VBNV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vboot_nvstorage.h>

/**
 * Attempt to read VbNvContext using mosys.
 *
 * Returns 0 if success, non-zero if error.
 */
int VbReadNvStorage_mosys(VbNvContext* vnc);

/**
 * Attempt to write VbNvContext using mosys.
 *
 * Returns 0 if success, non-zero if error.
 */
int VbWriteNvStorage_mosys(VbNvContext* vnc);

#ifdef __cplusplus
}
#endif

#endif  /* VBOOT_REFERENCE_CROSSYSTEM_VBNV_H_ */
