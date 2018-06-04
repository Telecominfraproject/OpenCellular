/*
* Copyright (c) 2006-2008 Advanced Micro Devices,Inc. ("AMD").
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

//*	  Function:                                                         *
//*     This file implementes the SYSINFO virtual registers.  

#include "vsa2.h"
#include "vr.h"

void pascal Hex_8(UCHAR);
void pascal Hex_16(USHORT);
void pascal Hex_32(ULONG);


// Local variables
extern Hardware SystemInfo;


void Handle_SysInfo_VR(UCHAR i)
{ USHORT Data;


  switch (i) {
    case VRC_SI_VERSION:
      Data = 0x100;				// Version 1.0
      break;

    case VRC_SI_CPU_MHZ:
      Data = SystemInfo.CPU_MHz;
      break;

    case VRC_SI_CHIPSET_BASE_LOW:
      Data = (USHORT) SystemInfo.Chipset_Base;
      break;

    case VRC_SI_CHIPSET_BASE_HI:
      Data = (USHORT) (SystemInfo.Chipset_Base >> 16);
      break;

    case VRC_SI_CHIPSET_ID:
      Data = SystemInfo.Chipset_ID;
      break;

    case VRC_SI_CHIPSET_REV:
      Data = SystemInfo.Chipset_Rev;
      break;

    case VRC_SI_CPU_ID:
      Data = SystemInfo.CPU_ID;
      break;

    case VRC_SI_CPU_REV:
      Data = SystemInfo.CPU_Revision;
      break;

    default:
      Data = 0xFFFF;
      break;
  }

  SET_AX(Data);
}
