/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware NV storage library.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "2sysincludes.h"

#include "test_common.h"
#include "vboot_common.h"

#include "2api.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2nvstorage_fields.h"

/* Single NV storage field to test */
struct nv_field {
	enum vb2_nv_param param;  /* Parameter index */
	uint32_t default_value;   /* Expected default value */
	uint32_t test_value;      /* Value to test writing */
	uint32_t test_value2;     /* Second value to test writing */
	char *desc;               /* Field description */
};

/* Array of fields to test, terminated with a field with desc==NULL. */
static struct nv_field nvfields[] = {
	{VB2_NV_DEBUG_RESET_MODE, 0, 1, 0, "debug reset mode"},
	{VB2_NV_TRY_NEXT, 0, 1, 0, "try next"},
	{VB2_NV_TRY_COUNT, 0, 6, 15, "try B count"},
	{VB2_NV_FW_TRIED, 0, 1, 0, "firmware tried"},
	{VB2_NV_FW_RESULT, 0, 1, 2, "firmware result"},
	{VB2_NV_FW_PREV_TRIED, 0, 1, 0, "firmware prev tried"},
	{VB2_NV_FW_PREV_RESULT, 0, 1, 3, "firmware prev result"},
	{VB2_NV_RECOVERY_REQUEST, 0, 0x42, 0xED, "recovery request"},
	{VB2_NV_RECOVERY_SUBCODE, 0, 0x56, 0xAC, "recovery subcode"},
	{VB2_NV_LOCALIZATION_INDEX, 0, 0x69, 0xB0, "localization index"},
	{VB2_NV_KERNEL_FIELD, 0, 0x1234, 0xFEDC, "kernel field"},
	{VB2_NV_DEV_BOOT_USB, 0, 1, 0, "dev boot usb"},
	{VB2_NV_DEV_BOOT_LEGACY, 0, 1, 0, "dev boot legacy"},
	{VB2_NV_DEV_BOOT_SIGNED_ONLY, 0, 1, 0, "dev boot custom"},
	{VB2_NV_DEV_BOOT_FASTBOOT_FULL_CAP, 0, 1, 0, "dev boot fb full cap"},
	{VB2_NV_DEV_DEFAULT_BOOT, 0, 1, 2, "dev default boot"},
	{VB2_NV_DISABLE_DEV_REQUEST, 0, 1, 0, "disable dev request"},
	{VB2_NV_CLEAR_TPM_OWNER_REQUEST, 0, 1, 0, "clear tpm owner request"},
	{VB2_NV_CLEAR_TPM_OWNER_DONE, 0, 1, 0, "clear tpm owner done"},
	{VB2_NV_TPM_REQUESTED_REBOOT, 0, 1, 0, "tpm requested reboot"},
	{VB2_NV_REQ_WIPEOUT, 0, 1, 0, "request wipeout"},
	{VB2_NV_OPROM_NEEDED, 0, 1, 0, "oprom needed"},
	{VB2_NV_BACKUP_NVRAM_REQUEST, 0, 1, 0, "backup nvram request"},
	{VB2_NV_FASTBOOT_UNLOCK_IN_FW, 0, 1, 0, "fastboot unlock in fw"},
	{VB2_NV_BOOT_ON_AC_DETECT, 0, 1, 0, "boot on ac detect"},
	{VB2_NV_TRY_RO_SYNC, 0, 1, 0, "try read only software sync"},
	{VB2_NV_BATTERY_CUTOFF_REQUEST, 0, 1, 0, "battery cutoff request"},
	{VB2_NV_KERNEL_MAX_ROLLFORWARD, 0, 0x12345678, 0xFEDCBA98,
	 "kernel max rollforward"},
	{0, 0, 0, 0, NULL}
};

/* Fields added in v2.  The test_value field is the default returned for V1. */
static struct nv_field nv2fields[] = {
	{VB2_NV_FW_MAX_ROLLFORWARD, 0, VB2_FW_MAX_ROLLFORWARD_V1_DEFAULT,
	 0x87654321, "firmware max rollforward"},
	{0, 0, 0, 0, NULL}
};

static void test_changed(struct vb2_context *ctx, int changed, const char *why)
{
	if (changed)
		TEST_NEQ(ctx->flags & VB2_CONTEXT_NVDATA_CHANGED, 0, why);
	else
		TEST_EQ(ctx->flags & VB2_CONTEXT_NVDATA_CHANGED, 0, why);
};

static void nv_storage_test(uint32_t ctxflags)
{
	struct nv_field *vnf;
	uint8_t goodcrc;
	uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE]
		__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_context c = {
		.flags = ctxflags,
		.workbuf = workbuf,
		.workbuf_size = sizeof(workbuf),
	};
	struct vb2_shared_data *sd = vb2_get_sd(&c);

	/* Things that change between V1 and V2 */
	int expect_header = 0x30 | (ctxflags ? VB2_NV_HEADER_SIGNATURE_V2 :
				    VB2_NV_HEADER_SIGNATURE_V1);
	int crc_offs = ctxflags ? VB2_NV_OFFS_CRC_V2 : VB2_NV_OFFS_CRC_V1;

	TEST_EQ(vb2_nv_get_size(&c), ctxflags ? VB2_NVDATA_SIZE_V2 :
				VB2_NVDATA_SIZE, "vb2_nv_get_size()");

	memset(c.nvdata, 0xA6, sizeof(c.nvdata));
	vb2_init_context(&c);

	/* Init with invalid data should set defaults and regenerate CRC */
	vb2_nv_init(&c);
	TEST_EQ(c.nvdata[VB2_NV_OFFS_HEADER], expect_header,
		"vb2_nv_init() reset header byte");
	TEST_NEQ(c.nvdata[crc_offs], 0, "vb2_nv_init() CRC");
	TEST_EQ(sd->status, VB2_SD_STATUS_NV_INIT | VB2_SD_STATUS_NV_REINIT,
		"vb2_nv_init() status changed");
	test_changed(&c, 1, "vb2_nv_init() reset changed");
	goodcrc = c.nvdata[crc_offs];
	TEST_SUCC(vb2_nv_check_crc(&c), "vb2_nv_check_crc() good");

	/* Another init should not cause further changes */
	c.flags = ctxflags;
	sd->status = 0;
	vb2_nv_init(&c);
	test_changed(&c, 0, "vb2_nv_init() didn't re-reset");
	TEST_EQ(c.nvdata[crc_offs], goodcrc,
		"vb2_nv_init() CRC same");
	TEST_EQ(sd->status, VB2_SD_STATUS_NV_INIT,
		"vb2_nv_init() status same");

	/* Perturbing signature bits in the header should force defaults */
	c.nvdata[VB2_NV_OFFS_HEADER] ^= 0x40;
	TEST_EQ(vb2_nv_check_crc(&c),
		VB2_ERROR_NV_HEADER, "vb2_nv_check_crc() bad header");
	vb2_nv_init(&c);
	TEST_EQ(c.nvdata[VB2_NV_OFFS_HEADER], expect_header,
		"vb2_nv_init() reset header byte again");
	test_changed(&c, 1, "vb2_nv_init() corrupt changed");
	TEST_EQ(c.nvdata[crc_offs], goodcrc,
		"vb2_nv_init() CRC same again");

	/* So should perturbing some other byte */
	TEST_EQ(c.nvdata[VB2_NV_OFFS_KERNEL1], 0, "Kernel byte starts at 0");
	c.nvdata[VB2_NV_OFFS_KERNEL1] = 12;
	TEST_EQ(vb2_nv_check_crc(&c),
		VB2_ERROR_NV_CRC, "vb2_nv_check_crc() bad CRC");
	vb2_nv_init(&c);
	TEST_EQ(c.nvdata[VB2_NV_OFFS_KERNEL1], 0,
		"vb2_nv_init() reset kernel byte");
	test_changed(&c, 1, "vb2_nv_init() corrupt elsewhere changed");
	TEST_EQ(c.nvdata[crc_offs], goodcrc,
		"vb2_nv_init() CRC same again");

	/* Clear the kernel and firmware flags */
	vb2_nv_init(&c);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_FIRMWARE_SETTINGS_RESET),
		1, "Firmware settings are reset");
	vb2_nv_set(&c, VB2_NV_FIRMWARE_SETTINGS_RESET, 0);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_FIRMWARE_SETTINGS_RESET),
		0, "Firmware settings are clear");

	TEST_EQ(vb2_nv_get(&c, VB2_NV_KERNEL_SETTINGS_RESET),
		1, "Kernel settings are reset");
	vb2_nv_set(&c, VB2_NV_KERNEL_SETTINGS_RESET, 0);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_KERNEL_SETTINGS_RESET),
		0, "Kernel settings are clear");

	TEST_EQ(c.nvdata[VB2_NV_OFFS_HEADER],
		expect_header & VB2_NV_HEADER_SIGNATURE_MASK,
		"Header byte now just has the signature");
	/* That should have changed the CRC */
	TEST_NEQ(c.nvdata[crc_offs], goodcrc,
		 "vb2_nv_init() CRC changed due to flags clear");

	/* Test explicitly setting the reset flags again */
	vb2_nv_init(&c);
	vb2_nv_set(&c, VB2_NV_FIRMWARE_SETTINGS_RESET, 1);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_FIRMWARE_SETTINGS_RESET),
		1, "Firmware settings forced reset");
	vb2_nv_set(&c, VB2_NV_FIRMWARE_SETTINGS_RESET, 0);

	vb2_nv_set(&c, VB2_NV_KERNEL_SETTINGS_RESET, 1);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_KERNEL_SETTINGS_RESET),
		1, "Kernel settings forced reset");
	vb2_nv_set(&c, VB2_NV_KERNEL_SETTINGS_RESET, 0);

	/* Get/set an invalid field */
	vb2_nv_init(&c);
	vb2_nv_set(&c, -1, 1);
	TEST_EQ(vb2_nv_get(&c, -1), 0, "Get invalid setting");

	/* Test other fields */
	vb2_nv_init(&c);
	for (vnf = nvfields; vnf->desc; vnf++) {
		TEST_EQ(vb2_nv_get(&c, vnf->param), vnf->default_value,
			vnf->desc);
		vb2_nv_set(&c, vnf->param, vnf->test_value);
		TEST_EQ(vb2_nv_get(&c, vnf->param), vnf->test_value, vnf->desc);
		vb2_nv_set(&c, vnf->param, vnf->test_value2);
		TEST_EQ(vb2_nv_get(&c, vnf->param), vnf->test_value2,
			vnf->desc);
	}

	for (vnf = nv2fields; vnf->desc; vnf++) {
		if (ctxflags) {
			TEST_EQ(vb2_nv_get(&c, vnf->param), vnf->default_value,
				vnf->desc);
			vb2_nv_set(&c, vnf->param, vnf->test_value);
			TEST_EQ(vb2_nv_get(&c, vnf->param), vnf->test_value,
				vnf->desc);
			vb2_nv_set(&c, vnf->param, vnf->test_value2);
			TEST_EQ(vb2_nv_get(&c, vnf->param), vnf->test_value2,
				vnf->desc);
		} else {
			/*
			 * V2 fields always return defaults and can't be set if
			 * a V1 struct is present.
			 */
			TEST_EQ(vb2_nv_get(&c, vnf->param), vnf->test_value,
				vnf->desc);
			vb2_nv_set(&c, vnf->param, vnf->test_value2);
			TEST_EQ(vb2_nv_get(&c, vnf->param), vnf->test_value,
				vnf->desc);
		}
	}

	/* None of those changes should have caused a reset to defaults */
	vb2_nv_init(&c);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_FIRMWARE_SETTINGS_RESET),
		0, "Firmware settings are still clear");
	TEST_EQ(vb2_nv_get(&c, VB2_NV_KERNEL_SETTINGS_RESET),
		0, "Kernel settings are still clear");

	/* Writing identical settings doesn't cause the CRC to regenerate */
	c.flags = ctxflags;
	vb2_nv_init(&c);
	test_changed(&c, 0, "No regen CRC on open");
	for (vnf = nvfields; vnf->desc; vnf++)
		vb2_nv_set(&c, vnf->param, vnf->test_value2);
	test_changed(&c, 0, "No regen CRC if data not changed");
	/*
	 * If struct is V2, this is the same test.  If struct is V1, this
	 * verifies that the field couldn't be changed anyway.
	 */
	for (vnf = nv2fields; vnf->desc; vnf++)
		vb2_nv_set(&c, vnf->param, vnf->test_value2);
	test_changed(&c, 0, "No regen CRC if V2 data not changed");

	/* Test out-of-range fields mapping to defaults or failing */
	vb2_nv_init(&c);
	vb2_nv_set(&c, VB2_NV_TRY_COUNT, 16);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_TRY_COUNT),
		15, "Try b count out of range");
	vb2_nv_set(&c, VB2_NV_RECOVERY_REQUEST, 0x101);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_LEGACY, "Recovery request out of range");
	vb2_nv_set(&c, VB2_NV_LOCALIZATION_INDEX, 0x102);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_LOCALIZATION_INDEX),
		0, "Localization index out of range");

	vb2_nv_set(&c, VB2_NV_FW_RESULT, 100);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_FW_RESULT),
		VB2_FW_RESULT_UNKNOWN, "Firmware result out of range");

	vb2_nv_set(&c, VB2_NV_FW_PREV_RESULT, 100);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_FW_PREV_RESULT),
		VB2_FW_RESULT_UNKNOWN, "Fw prev result out of range");

	vb2_nv_set(&c, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_DISK + 100);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_DEV_DEFAULT_BOOT),
		VB2_DEV_DEFAULT_BOOT_DISK, "default to booting from disk");
}

int main(int argc, char* argv[])
{
	printf("Testing V1\n");
	nv_storage_test(0);
	printf("Testing V2\n");
	nv_storage_test(VB2_CONTEXT_NVDATA_V2);

	return gTestSuccess ? 0 : 255;
}
