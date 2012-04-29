/*
* Copyright (c) 2007-2008 Advanced Micro Devices,Inc. ("AMD").
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
* License, or (at your option) any later version.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General
* Public License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place, Suite 330,
* Boston, MA 02111-1307 USA
*/
// This module performs LXVG initialization.


#include "lxvg.h"
#include "vsa2.h"
#include "pci.h"
#include "vr.h"


//---------------------------------------------------------------------------
// lxvg_initialize
//
// This routine initializes LXVG.  This is the routine called when
// VG_CONFIG virtual register is written the first time.
//---------------------------------------------------------------------------

void lxvg_initialize(unsigned short init_parms)
{
	int index;
	unsigned char *ptr;

	// CLEAR THE ENTIRE LXVG DATA STRUCTURE TO ZERO - This is a tedious loop

	ptr = (unsigned char *) &VGdata;
	for (index = 0; index < sizeof(VGdata); index++)
		*ptr++ = 0;

	// Start with virtual registers = 0
	for (index = 0; index < MAX_VG+1; index++)
	{
		if (index != VG_CONFIG)
		{
			vReg[index] = 0;
		}
	}

	// DEVICE DEPENDENT INITIALIZATION
	// -------------------------------
	// This MUST be done first.	 The routine handles all of the MBUS related
	// initialization and determines various register and base address values
	// that get used during later initialization.
	hw_initialize(init_parms);

	// If we haven't initialized because of error, just leave...
	if (VGState & SF_DISABLED) return;


	// LXVG is operating strictly as a secondary controller
	VGState |= SF_DRIVER_ENABLED;

	// Disable the graphics system in the PCI header command register
	WRITE_PCI_BYTE(vga_config_addr+0x04, 0);
	return;
}

// END OF FILE
