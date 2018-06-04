/*
 * This file is part of the coreboot project.
 *
 * Copyright 2016 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/acpigen.h>
#include <arch/acpigen_dsm.h>
#include <stdlib.h>

/* -------------------  I2C HID DSM ---------------------------- */

#define ACPI_DSM_I2C_HID_UUID		"3CDFF6F7-4267-4555-AD05-B30A3D8938DE"

static void i2c_hid_func0_cb(void *arg)
{
	/* ToInteger (Arg1, Local2) */
	acpigen_write_to_integer(ARG1_OP, LOCAL2_OP);
	/* If (LEqual (Local2, 0x0)) */
	acpigen_write_if_lequal_op_int(LOCAL2_OP, 0x0);
	/*   Return (Buffer (One) { 0x1f }) */
	acpigen_write_return_singleton_buffer(0x1f);
	acpigen_pop_len();	/* Pop : If */
	/* Else */
	acpigen_write_else();
	/*   If (LEqual (Local2, 0x1)) */
	acpigen_write_if_lequal_op_int(LOCAL2_OP, 0x1);
	/*     Return (Buffer (One) { 0x3f }) */
	acpigen_write_return_singleton_buffer(0x3f);
	acpigen_pop_len();	/* Pop : If */
	/*   Else */
	acpigen_write_else();
	/*     Return (Buffer (One) { 0x0 }) */
	acpigen_write_return_singleton_buffer(0x0);
	acpigen_pop_len();	/* Pop : Else */
	acpigen_pop_len();	/* Pop : Else */
}

static void i2c_hid_func1_cb(void *arg)
{
	struct dsm_i2c_hid_config *config = arg;
	acpigen_write_return_byte(config->hid_desc_reg_offset);
}

static void (*i2c_hid_callbacks[2])(void *) = {
	i2c_hid_func0_cb,
	i2c_hid_func1_cb,
};

void acpigen_write_dsm_i2c_hid(struct dsm_i2c_hid_config *config)
{
	acpigen_write_dsm(ACPI_DSM_I2C_HID_UUID, i2c_hid_callbacks,
			  ARRAY_SIZE(i2c_hid_callbacks), config);
}

/* ------------------- End: I2C HID DSM ------------------------- */
