/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Intel Corporation
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

#include <soc/ramstage.h>

void board_silicon_USB2_override(SILICON_INIT_UPD *params)
{
	if (SocStepping() >= SocD0) {
		//D-Stepping
		//USB2[1] right external port
		params->Usb2Port1PerPortPeTxiSet = 7;
		params->Usb2Port1PerPortTxiSet = 3;
		params->Usb2Port1IUsbTxEmphasisEn = 2;
		params->Usb2Port1PerPortTxPeHalf = 1;

		//USB2[2] left external port
		params->Usb2Port2PerPortPeTxiSet = 7;
		params->Usb2Port2PerPortTxiSet = 0;
		params->Usb2Port2IUsbTxEmphasisEn = 2;
		params->Usb2Port2PerPortTxPeHalf = 1;

		//USB2[3] CCD
		params->Usb2Port3PerPortPeTxiSet = 7;
		params->Usb2Port3PerPortTxiSet = 0;
		params->Usb2Port3IUsbTxEmphasisEn = 2;
		params->Usb2Port3PerPortTxPeHalf = 1;
	}
}
