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

//*****************************************************************************
//*     This file performs Legacy VSM initialization. 
//*****************************************************************************




#include "vsa2.h"
#include "chipset.h"
#include "sysmgr.h"
#include "vr.h"



// External variables:
extern Hardware SystemInfo;

// Function prototypes:
extern void pascal out_8(USHORT, UCHAR);
extern void CS5536_Early_Init(void);
extern void CS5536_Late_Init(void);
extern void Init_OHCI_SWAPSiF(UCHAR);




//***********************************************************************
// Performs early initialization
//***********************************************************************
void Legacy_Early_Init(void)
{

  // Register for VRC_SYSINFO virtual registers
  SYS_REGISTER_EVENT(EVENT_VIRTUAL_REGISTER, VRC_SYSINFO,  0, NORMAL_PRIORITY);


  switch (SystemInfo.Chipset_ID) {

    case DEVICE_ID_5536:
      CS5536_Early_Init();
      break;
  }
}



//***********************************************************************
// Performs End-of-POST initialization
//***********************************************************************
void Legacy_Late_Init(void)
{

  switch (SystemInfo.Chipset_ID) {

    case DEVICE_ID_5536:
      CS5536_Late_Init();
      break;
  }

  // Initialize A20 to '1MB wrap'
  // SDG - removed for OLPC
  // out_8(0x92, 0);
            
}
