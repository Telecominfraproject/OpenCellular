/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for misc library
 */

#include "2sysincludes.h"
#include "2api.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2secdata.h"

#include "test_common.h"

/* Common context for tests */
static uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE]
	__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
static struct vb2_context cc;
static struct vb2_shared_data *sd;

/* Mocked function data */
enum vb2_resource_index mock_resource_index;
void *mock_resource_ptr;
uint32_t mock_resource_size;
int mock_tpm_clear_called;
int mock_tpm_clear_retval;


static void reset_common_data(void)
{
	memset(workbuf, 0xaa, sizeof(workbuf));

	memset(&cc, 0, sizeof(cc));
	cc.workbuf = workbuf;
	cc.workbuf_size = sizeof(workbuf);

	vb2_init_context(&cc);
	sd = vb2_get_sd(&cc);

	vb2_nv_init(&cc);

	vb2_secdata_create(&cc);
	vb2_secdata_init(&cc);

	mock_tpm_clear_called = 0;
	mock_tpm_clear_retval = VB2_SUCCESS;
};

/* Mocked functions */

int vb2ex_read_resource(struct vb2_context *ctx,
			enum vb2_resource_index index,
			uint32_t offset,
			void *buf,
			uint32_t size)
{
	if (index != mock_resource_index)
		return VB2_ERROR_EX_READ_RESOURCE_INDEX;

	if (offset > mock_resource_size || offset + size > mock_resource_size)
		return VB2_ERROR_EX_READ_RESOURCE_SIZE;

	memcpy(buf, (uint8_t *)mock_resource_ptr + offset, size);
	return VB2_SUCCESS;
}

int vb2ex_tpm_clear_owner(struct vb2_context *ctx)
{
	mock_tpm_clear_called++;

	return mock_tpm_clear_retval;
}

/* Tests */

static void init_context_tests(void)
{
	/* Use our own context struct so we can re-init it */
	struct vb2_context c = {
		.workbuf = workbuf,
		.workbuf_size = sizeof(workbuf),
	};

	reset_common_data();

	TEST_SUCC(vb2_init_context(&c), "Init context good");
	TEST_EQ(c.workbuf_used, sizeof(struct vb2_shared_data),
		"Init vbsd");

	/* Don't re-init if used is non-zero */
	c.workbuf_used = 200;
	TEST_SUCC(vb2_init_context(&c), "Re-init context good");
	TEST_EQ(c.workbuf_used, 200, "Didn't re-init");

	/* Handle workbuf errors */
	c.workbuf_used = 0;
	c.workbuf_size = sizeof(struct vb2_shared_data) - 1;
	TEST_EQ(vb2_init_context(&c),
		VB2_ERROR_INITCTX_WORKBUF_SMALL, "Init too small");
	c.workbuf_size = sizeof(workbuf);

	/* Handle workbuf unaligned */
	c.workbuf++;
	TEST_EQ(vb2_init_context(&c),
		VB2_ERROR_INITCTX_WORKBUF_ALIGN, "Init unaligned");
}

static void misc_tests(void)
{
	struct vb2_workbuf wb;

	reset_common_data();
	cc.workbuf_used = 16;

	vb2_workbuf_from_ctx(&cc, &wb);

	TEST_PTR_EQ(wb.buf, workbuf + 16, "vb_workbuf_from_ctx() buf");
	TEST_EQ(wb.size, cc.workbuf_size - 16, "vb_workbuf_from_ctx() size");
}

static void gbb_tests(void)
{
	struct vb2_gbb_header gbb = {
		.signature = {'$', 'G', 'B', 'B'},
		.major_version = VB2_GBB_MAJOR_VER,
		.minor_version = VB2_GBB_MINOR_VER,
		.header_size = sizeof(struct vb2_gbb_header),
		.flags = 0x1234,
		.rootkey_offset = 240,
		.rootkey_size = 1040,
	};

	struct vb2_gbb_header gbbdest;

	TEST_EQ(sizeof(struct vb2_gbb_header),
		EXPECTED_VB2_GBB_HEADER_SIZE,
		"sizeof(struct vb2_gbb_header)");

	reset_common_data();

	/* Good contents */
	mock_resource_index = VB2_RES_GBB;
	mock_resource_ptr = &gbb;
	mock_resource_size = sizeof(gbb);
	TEST_SUCC(vb2_read_gbb_header(&cc, &gbbdest), "read gbb header good");
	TEST_SUCC(memcmp(&gbb, &gbbdest, sizeof(gbb)), "read gbb contents");

	mock_resource_index = VB2_RES_GBB + 1;
	TEST_EQ(vb2_read_gbb_header(&cc, &gbbdest),
		VB2_ERROR_EX_READ_RESOURCE_INDEX, "read gbb header missing");
	mock_resource_index = VB2_RES_GBB;

	gbb.signature[0]++;
	TEST_EQ(vb2_read_gbb_header(&cc, &gbbdest),
		VB2_ERROR_GBB_MAGIC, "read gbb header bad magic");
	gbb.signature[0]--;

	gbb.major_version = VB2_GBB_MAJOR_VER + 1;
	TEST_EQ(vb2_read_gbb_header(&cc, &gbbdest),
		VB2_ERROR_GBB_VERSION, "read gbb header major version");
	gbb.major_version = VB2_GBB_MAJOR_VER;

	gbb.minor_version = VB2_GBB_MINOR_VER + 1;
	TEST_SUCC(vb2_read_gbb_header(&cc, &gbbdest),
		  "read gbb header minor++");
	gbb.minor_version = 1;
	TEST_SUCC(vb2_read_gbb_header(&cc, &gbbdest),
		  "read gbb header 1.1");
	gbb.minor_version = 0;
	TEST_EQ(vb2_read_gbb_header(&cc, &gbbdest),
		VB2_ERROR_GBB_TOO_OLD, "read gbb header 1.0 fails");
	gbb.minor_version = VB2_GBB_MINOR_VER;

	gbb.header_size--;
	TEST_EQ(vb2_read_gbb_header(&cc, &gbbdest),
		VB2_ERROR_GBB_HEADER_SIZE, "read gbb header size");
	TEST_EQ(vb2_fw_parse_gbb(&cc),
		VB2_ERROR_GBB_HEADER_SIZE, "parse gbb failure");
	gbb.header_size++;

	/* Parse GBB */
	TEST_SUCC(vb2_fw_parse_gbb(&cc), "parse gbb");
	TEST_EQ(sd->gbb_flags, gbb.flags, "gbb flags");
	TEST_EQ(sd->gbb_rootkey_offset, gbb.rootkey_offset, "rootkey offset");
	TEST_EQ(sd->gbb_rootkey_size, gbb.rootkey_size, "rootkey size");

	/* Workbuf failure */
	reset_common_data();
	cc.workbuf_used = cc.workbuf_size - 4;
	TEST_EQ(vb2_fw_parse_gbb(&cc),
		VB2_ERROR_GBB_WORKBUF, "parse gbb no workbuf");
}

static void fail_tests(void)
{
	/* Early fail (before even NV init) */
	reset_common_data();
	sd->status &= ~VB2_SD_STATUS_NV_INIT;
	vb2_fail(&cc, 1, 2);
	TEST_NEQ(sd->status & VB2_SD_STATUS_NV_INIT, 0, "vb2_fail inits NV");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST),
		1, "vb2_fail request");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_SUBCODE),
		2, "vb2_fail subcode");

	/* Repeated fail doesn't overwrite the error code */
	vb2_fail(&cc, 3, 4);
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST),
		1, "vb2_fail repeat");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_SUBCODE),
		2, "vb2_fail repeat2");

	/* Fail with other slot good doesn't trigger recovery */
	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_TRY_COUNT, 3);
	vb2_nv_set(&cc, VB2_NV_FW_RESULT, VB2_FW_RESULT_UNKNOWN);
	sd->status |= VB2_SD_STATUS_CHOSE_SLOT;
	sd->fw_slot = 0;
	sd->last_fw_slot = 1;
	sd->last_fw_result = VB2_FW_RESULT_UNKNOWN;
	vb2_fail(&cc, 5, 6);
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST), 0, "vb2_failover");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_RESULT),
		VB2_FW_RESULT_FAILURE, "vb2_fail this fw");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_TRY_COUNT), 0, "vb2_fail use up tries");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_TRY_NEXT), 1, "vb2_fail try other slot");

	/* Fail with other slot already failing triggers recovery */
	reset_common_data();
	sd->status |= VB2_SD_STATUS_CHOSE_SLOT;
	sd->fw_slot = 1;
	sd->last_fw_slot = 0;
	sd->last_fw_result = VB2_FW_RESULT_FAILURE;
	vb2_fail(&cc, 7, 8);
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST), 7,
		"vb2_fail both slots bad");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_RESULT),
		VB2_FW_RESULT_FAILURE, "vb2_fail this fw");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_TRY_NEXT), 0, "vb2_fail try other slot");
}

static void recovery_tests(void)
{
	/* No recovery */
	reset_common_data();
	vb2_check_recovery(&cc);
	TEST_EQ(sd->recovery_reason, 0, "No recovery reason");
	TEST_EQ(sd->flags & VB2_SD_FLAG_MANUAL_RECOVERY,
		0, "Not manual recovery");
	TEST_EQ(cc.flags & VB2_CONTEXT_RECOVERY_MODE,
		0, "Not recovery mode");

	/* From request */
	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_RECOVERY_REQUEST, 3);
	vb2_check_recovery(&cc);
	TEST_EQ(sd->recovery_reason, 3, "Recovery reason from request");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST), 0, "NV cleared");
	TEST_EQ(sd->flags & VB2_SD_FLAG_MANUAL_RECOVERY,
		0, "Not manual recovery");
	TEST_NEQ(cc.flags & VB2_CONTEXT_RECOVERY_MODE,
		 0, "Recovery mode");

	/* From request, but already failed */
	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_RECOVERY_REQUEST, 4);
	sd->recovery_reason = 5;
	vb2_check_recovery(&cc);
	TEST_EQ(sd->recovery_reason, 5, "Recovery reason already failed");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST),
		0, "NV still cleared");

	/* Override */
	reset_common_data();
	sd->recovery_reason = 6;
	cc.flags |= VB2_CONTEXT_FORCE_RECOVERY_MODE;
	vb2_check_recovery(&cc);
	TEST_EQ(sd->recovery_reason, VB2_RECOVERY_RO_MANUAL,
		"Recovery reason forced");
	TEST_NEQ(sd->flags & VB2_SD_FLAG_MANUAL_RECOVERY,
		 0, "SD flag set");
}

static void dev_switch_tests(void)
{
	uint32_t v;

	/* Normal mode */
	reset_common_data();
	TEST_SUCC(vb2_check_dev_switch(&cc), "dev mode off");
	TEST_EQ(sd->flags & VB2_SD_DEV_MODE_ENABLED, 0, "sd not in dev");
	TEST_EQ(cc.flags & VB2_CONTEXT_DEVELOPER_MODE, 0, "ctx not in dev");
	TEST_EQ(mock_tpm_clear_called, 0, "no tpm clear");

	/* Dev mode */
	reset_common_data();
	vb2_secdata_set(&cc, VB2_SECDATA_FLAGS,
			(VB2_SECDATA_FLAG_DEV_MODE |
			 VB2_SECDATA_FLAG_LAST_BOOT_DEVELOPER));
	TEST_SUCC(vb2_check_dev_switch(&cc), "dev mode on");
	TEST_NEQ(sd->flags & VB2_SD_DEV_MODE_ENABLED, 0, "sd in dev");
	TEST_NEQ(cc.flags & VB2_CONTEXT_DEVELOPER_MODE, 0, "ctx in dev");
	TEST_EQ(mock_tpm_clear_called, 0, "no tpm clear");

	/* Any normal mode boot clears dev boot flags */
	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_DEV_BOOT_USB, 1);
	vb2_nv_set(&cc, VB2_NV_DEV_BOOT_LEGACY, 1);
	vb2_nv_set(&cc, VB2_NV_DEV_BOOT_SIGNED_ONLY, 1);
	TEST_SUCC(vb2_check_dev_switch(&cc), "dev mode off");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_DEV_BOOT_USB),
		0, "cleared dev boot usb");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_DEV_BOOT_LEGACY),
		0, "cleared dev boot legacy");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_DEV_BOOT_SIGNED_ONLY),
		0, "cleared dev boot signed only");

	/* Normal-dev transition clears TPM */
	reset_common_data();
	vb2_secdata_set(&cc, VB2_SECDATA_FLAGS, VB2_SECDATA_FLAG_DEV_MODE);
	TEST_SUCC(vb2_check_dev_switch(&cc), "to dev mode");
	TEST_EQ(mock_tpm_clear_called, 1, "tpm clear");
	vb2_secdata_get(&cc, VB2_SECDATA_FLAGS, &v);
	TEST_EQ(v, (VB2_SECDATA_FLAG_DEV_MODE |
		    VB2_SECDATA_FLAG_LAST_BOOT_DEVELOPER),
		"last boot developer now");

	/* Dev-normal transition clears TPM too */
	reset_common_data();
	vb2_secdata_set(&cc, VB2_SECDATA_FLAGS,
			VB2_SECDATA_FLAG_LAST_BOOT_DEVELOPER);
	TEST_SUCC(vb2_check_dev_switch(&cc), "from dev mode");
	TEST_EQ(mock_tpm_clear_called, 1, "tpm clear");
	vb2_secdata_get(&cc, VB2_SECDATA_FLAGS, &v);
	TEST_EQ(v, 0, "last boot not developer now");

	/* Disable dev mode */
	reset_common_data();
	vb2_secdata_set(&cc, VB2_SECDATA_FLAGS,
			(VB2_SECDATA_FLAG_DEV_MODE |
			 VB2_SECDATA_FLAG_LAST_BOOT_DEVELOPER));
	vb2_nv_set(&cc, VB2_NV_DISABLE_DEV_REQUEST, 1);
	TEST_SUCC(vb2_check_dev_switch(&cc), "disable dev request");
	TEST_EQ(sd->flags & VB2_SD_DEV_MODE_ENABLED, 0, "sd not in dev");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_DISABLE_DEV_REQUEST),
		0, "request cleared");

	/* Force enabled by gbb */
	reset_common_data();
	sd->gbb_flags |= VB2_GBB_FLAG_FORCE_DEV_SWITCH_ON;
	TEST_SUCC(vb2_check_dev_switch(&cc), "dev on via gbb");
	TEST_NEQ(sd->flags & VB2_SD_DEV_MODE_ENABLED, 0, "sd in dev");
	vb2_secdata_get(&cc, VB2_SECDATA_FLAGS, &v);
	TEST_EQ(v, VB2_SECDATA_FLAG_LAST_BOOT_DEVELOPER,
		"doesn't set dev on in secdata but does set last boot dev");
	TEST_EQ(mock_tpm_clear_called, 1, "tpm clear");

	/* Force enabled by ctx flag */
	reset_common_data();
	cc.flags |= VB2_CONTEXT_FORCE_DEVELOPER_MODE;
	TEST_SUCC(vb2_check_dev_switch(&cc), "dev on via ctx flag");
	TEST_NEQ(sd->flags & VB2_SD_DEV_MODE_ENABLED, 0, "sd in dev");
	vb2_secdata_get(&cc, VB2_SECDATA_FLAGS, &v);
	TEST_EQ(v, VB2_SECDATA_FLAG_LAST_BOOT_DEVELOPER,
		"doesn't set dev on in secdata but does set last boot dev");
	TEST_EQ(mock_tpm_clear_called, 1, "tpm clear");

	/* Simulate clear owner failure */
	reset_common_data();
	vb2_secdata_set(&cc, VB2_SECDATA_FLAGS,
			VB2_SECDATA_FLAG_LAST_BOOT_DEVELOPER);
	mock_tpm_clear_retval = VB2_ERROR_EX_TPM_CLEAR_OWNER;
	TEST_EQ(vb2_check_dev_switch(&cc),
		VB2_ERROR_EX_TPM_CLEAR_OWNER, "tpm clear fail");
	TEST_EQ(mock_tpm_clear_called, 1, "tpm clear");
	vb2_secdata_get(&cc, VB2_SECDATA_FLAGS, &v);
	TEST_EQ(v, VB2_SECDATA_FLAG_LAST_BOOT_DEVELOPER,
		"last boot still developer");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_TPM_CLEAR_OWNER, "requests recovery");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_SUBCODE),
		(uint8_t)VB2_ERROR_EX_TPM_CLEAR_OWNER, "recovery subcode");
}

static void tpm_clear_tests(void)
{
	/* No clear request */
	reset_common_data();
	TEST_SUCC(vb2_check_tpm_clear(&cc), "no clear request");
	TEST_EQ(mock_tpm_clear_called, 0, "tpm not cleared");

	/* Successful request */
	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_CLEAR_TPM_OWNER_REQUEST, 1);
	TEST_SUCC(vb2_check_tpm_clear(&cc), "clear request");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_CLEAR_TPM_OWNER_REQUEST),
		0, "request cleared");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_CLEAR_TPM_OWNER_DONE),
		1, "done set");
	TEST_EQ(mock_tpm_clear_called, 1, "tpm cleared");

	/* Failed request */
	reset_common_data();
	mock_tpm_clear_retval = VB2_ERROR_EX_TPM_CLEAR_OWNER;
	vb2_nv_set(&cc, VB2_NV_CLEAR_TPM_OWNER_REQUEST, 1);
	TEST_EQ(vb2_check_tpm_clear(&cc),
		VB2_ERROR_EX_TPM_CLEAR_OWNER, "clear failure");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_CLEAR_TPM_OWNER_REQUEST),
		0, "request cleared");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_CLEAR_TPM_OWNER_DONE),
		0, "done not set");
}

static void select_slot_tests(void)
{
	/* Slot A */
	reset_common_data();
	TEST_SUCC(vb2_select_fw_slot(&cc), "select slot A");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_RESULT),
		VB2_FW_RESULT_UNKNOWN, "result unknown");
	TEST_NEQ(sd->status & VB2_SD_STATUS_CHOSE_SLOT, 0, "chose slot");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_TRIED), 0, "tried A");
	TEST_EQ(sd->fw_slot, 0, "selected A");
	TEST_EQ(cc.flags & VB2_CONTEXT_FW_SLOT_B, 0, "didn't choose B");

	/* Slot B */
	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_TRY_NEXT, 1);
	TEST_SUCC(vb2_select_fw_slot(&cc), "select slot B");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_RESULT),
		VB2_FW_RESULT_UNKNOWN, "result unknown");
	TEST_NEQ(sd->status & VB2_SD_STATUS_CHOSE_SLOT, 0, "chose slot");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_TRIED), 1, "tried B");
	TEST_EQ(sd->fw_slot, 1, "selected B");
	TEST_NEQ(cc.flags & VB2_CONTEXT_FW_SLOT_B, 0, "ctx says choose B");

	/* Slot A ran out of tries */
	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_FW_RESULT, VB2_FW_RESULT_TRYING);
	TEST_SUCC(vb2_select_fw_slot(&cc), "select slot A out of tries");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_TRY_NEXT), 1, "try B next");
	TEST_NEQ(sd->status & VB2_SD_STATUS_CHOSE_SLOT, 0, "chose slot");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_TRIED), 1, "tried B");
	TEST_EQ(sd->fw_slot, 1, "selected B");
	TEST_NEQ(cc.flags & VB2_CONTEXT_FW_SLOT_B, 0, "ctx says choose B");

	/* Slot A used up a try */
	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_TRY_COUNT, 3);
	TEST_SUCC(vb2_select_fw_slot(&cc), "try slot A");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_RESULT),
		VB2_FW_RESULT_TRYING, "result trying");
	TEST_NEQ(sd->status & VB2_SD_STATUS_CHOSE_SLOT, 0, "chose slot");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_TRIED), 0, "tried A");
	TEST_EQ(sd->fw_slot, 0, "selected A");
	TEST_EQ(cc.flags & VB2_CONTEXT_FW_SLOT_B, 0, "didn't choose B");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_TRY_COUNT), 2, "tries decremented");

	/* Tried/result get copied to the previous fields */
	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_FW_TRIED, 0);
	vb2_nv_set(&cc, VB2_NV_FW_RESULT, VB2_FW_RESULT_SUCCESS);
	vb2_select_fw_slot(&cc);
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_PREV_TRIED), 0, "prev A");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_PREV_RESULT),	VB2_FW_RESULT_SUCCESS,
		"prev success");

	reset_common_data();
	vb2_nv_set(&cc, VB2_NV_FW_TRIED, 1);
	vb2_nv_set(&cc, VB2_NV_FW_RESULT, VB2_FW_RESULT_FAILURE);
	vb2_select_fw_slot(&cc);
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_PREV_TRIED), 1, "prev B");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_FW_PREV_RESULT),	VB2_FW_RESULT_FAILURE,
		"prev failure");
}

int main(int argc, char* argv[])
{
	init_context_tests();
	misc_tests();
	gbb_tests();
	fail_tests();
	recovery_tests();
	dev_switch_tests();
	tpm_clear_tests();
	select_slot_tests();

	return gTestSuccess ? 0 : 255;
}
