/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* APIs between calling firmware and vboot_reference
 *
 * DO NOT INCLUDE THE HEADERS BELOW DIRECTLY!  ONLY INCLUDE THIS FILE!
 *
 * Using vb2api.h as the single point of contact between calling firmware and
 * vboot allows subsequent refactoring of vboot (renaming of headers, etc.)
 * without churning other projects' source code.
 */

#ifndef VBOOT_VB2_API_H_
#define VBOOT_VB2_API_H_

/* Standard APIs */
#include "../2lib/include/2api.h"

/* SHA library */
#ifdef NEED_VB2_SHA_LIBRARY
#include "../2lib/include/2sha.h"
#endif

/*
 * Coreboot should not need access to vboot2 internals.  But right now it does.
 * At least this forces it to do so through a relatively narrow hole so vboot2
 * refactoring can continue.
 *
 * Please do not rip this into a wider hole, or expect this hole to continue.
 *
 * TODO: Make cleaner APIs to this stuff.
 */
#ifdef NEED_VB20_INTERNALS
#include "../2lib/include/2nvstorage.h"
#include "../2lib/include/2nvstorage_fields.h"
#include "../2lib/include/2struct.h"
#include "../lib20/include/vb2_struct.h"
#endif

#endif  /* VBOOT_VB2_API_H_ */
