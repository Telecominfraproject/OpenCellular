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

#include "test_common.h"
#include "vboot_common.h"

#include "2api.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"

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
	{VB2_NV_KERNEL_FIELD, 0, 0x12345678, 0xFEDCBA98, "kernel field"},
	{VB2_NV_DEV_BOOT_USB, 0, 1, 0, "dev boot usb"},
	{VB2_NV_DEV_BOOT_LEGACY, 0, 1, 0, "dev boot legacy"},
	{VB2_NV_DEV_BOOT_SIGNED_ONLY, 0, 1, 0, "dev boot custom"},
	{VB2_NV_DISABLE_DEV_REQUEST, 0, 1, 0, "disable dev request"},
	{VB2_NV_CLEAR_TPM_OWNER_REQUEST, 0, 1, 0, "clear tpm owner request"},
	{VB2_NV_CLEAR_TPM_OWNER_DONE, 0, 1, 0, "clear tpm owner done"},
	{VB2_NV_OPROM_NEEDED, 0, 1, 0, "oprom needed"},
	{VB2_NV_BACKUP_NVRAM_REQUEST, 0, 1, 0, "backup nvram request"},
	{0, 0, 0, 0, NULL}
};

static void test_changed(struct vb2_context *ctx, int changed, const char *why)
{
	if (changed)
		TEST_NEQ(ctx->flags & VB2_CONTEXT_NVDATA_CHANGED, 0, why);
	else
		TEST_EQ(ctx->flags & VB2_CONTEXT_NVDATA_CHANGED, 0, why);
};

static void nv_storage_test(void)
{
	struct nv_field *vnf;
	uint8_t goodcrc;
	uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE]
		__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_context c = {
		.flags = 0,
		.workbuf = workbuf,
		.workbuf_size = sizeof(workbuf),
	};
	struct vb2_shared_data *sd = vb2_get_sd(&c);

	memset(c.nvdata, 0xA6, sizeof(c.nvdata));
	vb2_init_context(&c);

	/* Init with invalid data should set defaults and regenerate CRC */
	vb2_nv_init(&c);
	TEST_EQ(c.nvdata[0], 0x70, "vb2_nv_init() reset header byte");
	TEST_NEQ(c.nvdata[15], 0, "vb2_nv_init() CRC");
	TEST_EQ(sd->status, VB2_SD_STATUS_NV_INIT | VB2_SD_STATUS_NV_REINIT,
		"vb2_nv_init() status changed");
	test_changed(&c, 1, "vb2_nv_init() reset changed");
	goodcrc = c.nvdata[15];
	TEST_SUCC(vb2_nv_check_crc(&c), "vb2_nv_check_crc() good");

	/* Another init should not cause further changes */
	c.flags = 0;
	sd->status = 0;
	vb2_nv_init(&c);
	test_changed(&c, 0, "vb2_nv_init() didn't re-reset");
	TEST_EQ(c.nvdata[15], goodcrc, "vb2_nv_init() CRC same");
	TEST_EQ(sd->status, VB2_SD_STATUS_NV_INIT, "vb2_nv_init() status same");

	/* Perturbing the header should force defaults */
	c.nvdata[0] ^= 0x40;
	TEST_EQ(vb2_nv_check_crc(&c),
		VB2_ERROR_NV_HEADER, "vb2_nv_check_crc() bad header");
	vb2_nv_init(&c);
	TEST_EQ(c.nvdata[0], 0x70, "vb2_nv_init() reset header byte again");
	test_changed(&c, 1, "vb2_nv_init() corrupt changed");
	TEST_EQ(c.nvdata[15], goodcrc, "vb2_nv_init() CRC same again");

	/* So should perturbing some other byte */
	TEST_EQ(c.nvdata[11], 0, "Kernel byte starts at 0");
	c.nvdata[11] = 12;
	TEST_EQ(vb2_nv_check_crc(&c),
		VB2_ERROR_NV_CRC, "vb2_nv_check_crc() bad CRC");
	vb2_nv_init(&c);
	TEST_EQ(c.nvdata[11], 0, "vb2_nv_init() reset kernel byte");
	test_changed(&c, 1, "vb2_nv_init() corrupt elsewhere changed");
	TEST_EQ(c.nvdata[15], goodcrc, "vb2_nv_init() CRC same again");

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

	TEST_EQ(c.nvdata[0], 0x40, "Header byte now just has the header bit");
	/* That should have changed the CRC */
	TEST_NEQ(c.nvdata[15], goodcrc,
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

	/* None of those changes should have caused a reset to defaults */
	vb2_nv_init(&c);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_FIRMWARE_SETTINGS_RESET),
		0, "Firmware settings are still clear");
	TEST_EQ(vb2_nv_get(&c, VB2_NV_KERNEL_SETTINGS_RESET),
		0, "Kernel settings are still clear");

	/* Writing identical settings doesn't cause the CRC to regenerate */
	c.flags = 0;
	vb2_nv_init(&c);
	test_changed(&c, 0, "No regen CRC on open");
	for (vnf = nvfields; vnf->desc; vnf++)
		vb2_nv_set(&c, vnf->param, vnf->test_value2);
	test_changed(&c, 0, "No regen CRC if data not changed");

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

	vb2_nv_set(&c, VB2_NV_FW_RESULT, VB2_FW_RESULT_UNKNOWN + 1);
	vb2_nv_set(&c, VB2_NV_FW_RESULT, VB2_FW_RESULT_UNKNOWN + 100);
	TEST_EQ(vb2_nv_get(&c, VB2_NV_FW_RESULT),
		VB2_FW_RESULT_UNKNOWN, "Firmware result out of range");
}

int main(int argc, char* argv[])
{
	nv_storage_test();

	return gTestSuccess ? 0 : 255;
}
