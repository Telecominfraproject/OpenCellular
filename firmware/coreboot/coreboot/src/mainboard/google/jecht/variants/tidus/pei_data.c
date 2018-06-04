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
	pei_data->ec_present = 0;

	/* P0: VP8 */
	pei_data_usb2_port(pei_data, 0, 0x0064, 1, USB_OC_PIN_SKIP,
			   USB_PORT_MINI_PCIE);
	/* P1: Port 3, USB3 */
	pei_data_usb2_port(pei_data, 1, 0x0040, 1, 0,
			   USB_PORT_INTERNAL);
	/* P2: Port 4, USB4 */
	pei_data_usb2_port(pei_data, 2, 0x0040, 1, 1,
			   USB_PORT_INTERNAL);
	/* P3: Mini Card */
	pei_data_usb2_port(pei_data, 3, 0x0040, 1, USB_OC_PIN_SKIP,
			   USB_PORT_MINI_PCIE);
	/* P4: Port 1, USB1 */
	pei_data_usb2_port(pei_data, 4, 0x0040, 1, 2,
			   USB_PORT_INTERNAL);
	/* P5: Port 2, USB2 */
	pei_data_usb2_port(pei_data, 5, 0x0040, 1, 2,
			   USB_PORT_INTERNAL);
	/* P6: Card Reader */
	pei_data_usb2_port(pei_data, 6, 0x0040, 1, USB_OC_PIN_SKIP,
			   USB_PORT_INTERNAL);
	/* P7: Pin Header */
	pei_data_usb2_port(pei_data, 7, 0x0040, 1, 3,
			   USB_PORT_INTERNAL);

	/* P1: USB1 */
	pei_data_usb3_port(pei_data, 0, 1, 2, 0);
	/* P2: USB2 */
	pei_data_usb3_port(pei_data, 1, 1, 2, 0);
	/* P3: USB3 */
	pei_data_usb3_port(pei_data, 2, 1, 0, 0);
	/* P4: USB4 */
	pei_data_usb3_port(pei_data, 3, 1, 1, 0);
}
