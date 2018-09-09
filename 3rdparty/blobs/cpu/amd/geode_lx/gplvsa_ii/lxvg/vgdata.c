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
// This file declares the data structures and arrays used by lxvg.



#include "lxvg.h"
#include "vsa2.h"
#include "vr.h"

//---------------------------------------------------------------------------
// MAIN lxvg DATA STRUCTURE
// This data structure maintains the current lxvg state.
//---------------------------------------------------------------------------

VGDATA VGdata;

unsigned long vga_config_addr;

unsigned long GPregister_base;
unsigned long VGregister_base;
unsigned long DFregister_base;
unsigned long VIPregister_base;
unsigned long framebuffer_base;

unsigned long VG_SMI_Mask;

// General system information...
Hardware SystemInfo;

// The main VG state information flag.
unsigned long VGState = SF_DISABLED;
unsigned long lockNest = 0;			// Nested SMI recognition scheme
unsigned long saveLock;				// Locking

// The virtual registers
unsigned short vReg[MAX_VG+1];

// END OF FILE
