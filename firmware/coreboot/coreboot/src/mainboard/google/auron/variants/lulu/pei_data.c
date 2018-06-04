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
	pei_data->ec_present = 1;

	/* One installed DIMM per channel -- can be changed by SPD init */
	pei_data->dimm_channel0_disabled = 2;
	pei_data->dimm_channel1_disabled = 2;

	/* P0: Port B, CN01 (IOBoard) */
	pei_data_usb2_port(pei_data, 0, 0x0150, 1, 0,
			   USB_PORT_BACK_PANEL);
	/* P1: Port A, CN01 */
	pei_data_usb2_port(pei_data, 1, 0x0040, 1, 2,
			   USB_PORT_BACK_PANEL);
	/* P2: CCD */
	pei_data_usb2_port(pei_data, 2, 0x0080, 1, USB_OC_PIN_SKIP,
			   USB_PORT_INTERNAL);
	/* P3: BT */
	pei_data_usb2_port(pei_data, 3, 0x0040, 1, USB_OC_PIN_SKIP,
			   USB_PORT_MINI_PCIE);
	/* P4: Empty */
	pei_data_usb2_port(pei_data, 4, 0x0000, 0, USB_OC_PIN_SKIP,
			   USB_PORT_SKIP);
	/* P5: EMPTY */
	pei_data_usb2_port(pei_data, 5, 0x0000, 0, USB_OC_PIN_SKIP,
			   USB_PORT_SKIP);
	/* P6: SD Card */
	pei_data_usb2_port(pei_data, 6, 0x0040, 1, USB_OC_PIN_SKIP,
			   USB_PORT_FLEX);
	/* P7: EMPTY */
	pei_data_usb2_port(pei_data, 7, 0x0000, 0, USB_OC_PIN_SKIP,
			   USB_PORT_SKIP);

	/* P0: PORTB*/
	pei_data_usb3_port(pei_data, 0, 1, 0, 0);
	/* P1: PORTA */
	pei_data_usb3_port(pei_data, 1, 1, 2, 0);
	/* P2: EMPTY */
	pei_data_usb3_port(pei_data, 2, 0, USB_OC_PIN_SKIP, 0);
	/* P3: EMPTY */
	pei_data_usb3_port(pei_data, 3, 0, USB_OC_PIN_SKIP, 0);
}
