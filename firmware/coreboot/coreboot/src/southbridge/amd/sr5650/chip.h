/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
 * Copyright (C) 2015 Timothy Pearson <tpearson@raptorengineeringinc.com>, Raptor Engineering
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

#ifndef SR5650_CHIP_H
#define SR5650_CHIP_H

/* Member variables are defined in Config.lb. */
struct southbridge_amd_sr5650_config
{
	u8 gpp1_configuration;		/* The configuration of General Purpose Port. */
	u8 gpp2_configuration;		/* The configuration of General Purpose Port. */
	u8 gpp3a_configuration;		/* The configuration of General Purpose Port. */
	u16 port_enable;		/* Which port is enabled? GPP(2,3,4,5,6,7,9,10,11,12,13) */
	uint32_t pcie_settling_time;	/* How long to wait after link training for PCI-e devices to
					 * initialize before probing PCI-e busses (in microseconds).
					 */
};

#endif /* SR5650_CHIP_H */
