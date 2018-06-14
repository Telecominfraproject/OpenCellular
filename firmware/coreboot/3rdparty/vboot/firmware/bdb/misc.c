/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include "bdb.h"
#include "bdb_api.h"
#include "vboot_register.h"

static int did_current_slot_fail(struct vba_context *ctx)
{
	uint32_t val = vbe_get_vboot_register(VBOOT_REGISTER_PERSIST);

	if (ctx->slot)
		return val & VBOOT_REGISTER_FAILED_RW_SECONDARY;
	else
		return val & VBOOT_REGISTER_FAILED_RW_PRIMARY;
}

static int did_other_slot_fail(struct vba_context *ctx)
{
	uint32_t val = vbe_get_vboot_register(VBOOT_REGISTER_PERSIST);

	if (ctx->slot)
		return val & VBOOT_REGISTER_FAILED_RW_PRIMARY;
	else
		return val & VBOOT_REGISTER_FAILED_RW_SECONDARY;
}

static void set_try_other_slot(struct vba_context *ctx)
{
	uint32_t val = vbe_get_vboot_register(VBOOT_REGISTER_PERSIST);

	if (ctx->slot)
		val &= ~VBOOT_REGISTER_TRY_SECONDARY_BDB;
	else
		val |= VBOOT_REGISTER_TRY_SECONDARY_BDB;

	vbe_set_vboot_register(VBOOT_REGISTER_PERSIST, val);
}

static void set_recovery_request(struct vba_context *ctx)
{
	uint32_t val = vbe_get_vboot_register(VBOOT_REGISTER_PERSIST);

	val |= VBOOT_REGISTER_RECOVERY_REQUEST;

	vbe_set_vboot_register(VBOOT_REGISTER_PERSIST, val);
}

static void get_current_slot(struct vba_context *ctx)
{
	/* Assume SP-RO selects slot this way */
	ctx->slot = (vbe_get_vboot_register(VBOOT_REGISTER_PERSIST)
			& VBOOT_REGISTER_TRY_SECONDARY_BDB) ? 1 : 0;
}

static void set_current_slot_failed(struct vba_context *ctx)
{
	uint32_t val = vbe_get_vboot_register(VBOOT_REGISTER_PERSIST);

	if (ctx->slot)
		val |= VBOOT_REGISTER_FAILED_RW_SECONDARY;
	else
		val |= VBOOT_REGISTER_FAILED_RW_PRIMARY;

	vbe_set_vboot_register(VBOOT_REGISTER_PERSIST, val);
}

static void unset_current_slot_failed(struct vba_context *ctx)
{
	uint32_t val = vbe_get_vboot_register(VBOOT_REGISTER_PERSIST);

	if (ctx->slot)
		val &= ~VBOOT_REGISTER_FAILED_RW_SECONDARY;
	else
		val &= ~VBOOT_REGISTER_FAILED_RW_PRIMARY;

	vbe_set_vboot_register(VBOOT_REGISTER_PERSIST, val);
}

int vba_bdb_init(struct vba_context *ctx)
{
	/* Get current slot */
	get_current_slot(ctx);

	/* Check current slot failed or not at the last boot */
	if (!did_current_slot_fail(ctx)) {
		/* If not, we try this slot. Prepare for any accidents */
		set_current_slot_failed(ctx);
		return BDB_SUCCESS;
	}

	/* Check other slot failed or not at the previous boot */
	if (!did_other_slot_fail(ctx)) {
		/* If not, we try the other slot after reboot. */
		set_try_other_slot(ctx);
		return BDB_ERROR_TRY_OTHER_SLOT;
	} else {
		/* Otherwise, both slots are bad. Reboot to recovery */
		set_recovery_request(ctx);
		return BDB_ERROR_RECOVERY_REQUEST;
	}
}

int vba_bdb_finalize(struct vba_context *ctx)
{
	/* Mark the current slot good */
	unset_current_slot_failed(ctx);

	/* Disable NVM bus */

	return BDB_SUCCESS;
}

void vba_bdb_fail(struct vba_context *ctx)
{
	/* We can do some logging here if we want */

	/* Unconditionally reboot. FailedRW flag is already set.
	 * At the next boot, bdb_init will decide what to do. */
	vbe_reset();
}
