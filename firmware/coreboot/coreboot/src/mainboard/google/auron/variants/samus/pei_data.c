/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
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

#include <stdint.h>
#include <string.h>
#include <soc/gpio.h>
#include <soc/pei_data.h>
#include <soc/pei_wrapper.h>

void mainboard_fill_pei_data(struct pei_data *pei_data)
{
	/* DQ byte map for Samus board */
	const u8 dq_map[2][6][2] = {
		{ { 0x0F, 0xF0 }, { 0x00, 0xF0 }, { 0x0F, 0xF0 },
		  { 0x0F, 0x00 }, { 0xFF, 0x00 }, { 0xFF, 0x00 } },
		{ { 0x0F, 0xF0 }, { 0x00, 0xF0 }, { 0x0F, 0xF0 },
		  { 0x0F, 0x00 }, { 0xFF, 0x00 }, { 0xFF, 0x00 } } };
	/* DQS CPU<>DRAM map for Samus board */
	const u8 dqs_map[2][8] = {
		{ 2, 0, 1, 3, 6, 4, 7, 5 },
		{ 2, 1, 0, 3, 6, 5, 4, 7 } };

	pei_data->ec_present = 1;

	/* One installed DIMM per channel */
	pei_data->dimm_channel0_disabled = 2;
	pei_data->dimm_channel1_disabled = 2;

	memcpy(pei_data->dq_map, dq_map, sizeof(dq_map));
	memcpy(pei_data->dqs_map, dqs_map, sizeof(dqs_map));

	/* P0: HOST PORT */
	pei_data_usb2_port(pei_data, 0, 0x0080, 1, 0,
			   USB_PORT_BACK_PANEL);
	/* P1: HOST PORT */
	pei_data_usb2_port(pei_data, 1, 0x0080, 1, 1,
			   USB_PORT_BACK_PANEL);
	/* P2: RAIDEN */
	pei_data_usb2_port(pei_data, 2, 0x0080, 1, USB_OC_PIN_SKIP,
			   USB_PORT_BACK_PANEL);
	/* P3: SD CARD */
	pei_data_usb2_port(pei_data, 3, 0x0040, 1, USB_OC_PIN_SKIP,
			   USB_PORT_INTERNAL);
	/* P4: RAIDEN */
	pei_data_usb2_port(pei_data, 4, 0x0080, 1, USB_OC_PIN_SKIP,
			   USB_PORT_BACK_PANEL);
	/* P5: WWAN (Disabled) */
	pei_data_usb2_port(pei_data, 5, 0x0000, 0, USB_OC_PIN_SKIP,
			   USB_PORT_SKIP);
	/* P6: CAMERA */
	pei_data_usb2_port(pei_data, 6, 0x0040, 1, USB_OC_PIN_SKIP,
			   USB_PORT_INTERNAL);
	/* P7: BT */
	pei_data_usb2_port(pei_data, 7, 0x0040, 1, USB_OC_PIN_SKIP,
			   USB_PORT_INTERNAL);

	/* P1: HOST PORT */
	pei_data_usb3_port(pei_data, 0, 1, 0, 0);
	/* P2: HOST PORT */
	pei_data_usb3_port(pei_data, 1, 1, 1, 0);
	/* P3: RAIDEN */
	pei_data_usb3_port(pei_data, 2, 1, USB_OC_PIN_SKIP, 0);
	/* P4: RAIDEN */
	pei_data_usb3_port(pei_data, 3, 1, USB_OC_PIN_SKIP, 0);
}
