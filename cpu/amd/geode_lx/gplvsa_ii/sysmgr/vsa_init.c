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

//*	 Function:                                                          *
//*     Implements the initialization code for System Manager 

#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"
#include "VR.H"
#include "CHIPSET.H"



// External function prototypes
extern void Initialize_Events(void);
extern void Install_SMI_Routines(void);
extern void Initialize_History(void);
extern void InitChipset(void);
extern void Init_MBus(void);
extern void VirtualPCI_EarlyInit(void);
extern void Init_Virtual_Regs(void);
extern void Record_VSM_Locations(void);
extern void Broadcast_SysMgr_Msg (MSG, UCHAR);
extern Hardware HardwareInfo;

//***********************************************************************
// Gets called from early VSA init, but after VSMs have initialized.
//***********************************************************************
void Post_VSM_InitInit(void) 
{

}



//*****************************************************************************
//
// Initialize the System Manager
//
//*****************************************************************************
void Init_SysMgr(void)
{

  // Initialize the array of registered events
  Initialize_Events();

  // Initialize the history buffers
  Initialize_History();


  // Install SMI routines appropriate to chipset
  Install_SMI_Routines();


  // Initialize MBus related structures & MSRs
  Init_MBus();


  // Initialize virtual PCI headers and MBUS
  VirtualPCI_EarlyInit();

  // Initialize chipset-specific registers
  InitChipset();


  // Initialize virtual register trapping
  Init_Virtual_Regs();


  // Register System Manager as handler for A20
  Register_Event(EVENT_A20, MAX_PRIORITY, SysMgr_VSM, 0, 0);



  // Register System Manager as handler of VRC_MISCELLANEOUS
  Register_Event(EVENT_VIRTUAL_REGISTER, MAX_PRIORITY, SysMgr_VSM, VRC_MISCELLANEOUS, GET_DESCR_INFO);


  // Record locations of VSMs requiring special handling
  Record_VSM_Locations();


  // Schedule control after VSMs have performed initialization
  Schedule_VSM((USHORT)Post_VSM_InitInit);



  //
  // Send a phase 0 initialization message to each VSM
  //
  Broadcast_SysMgr_Msg(MSG_INITIALIZE, EARLY_INIT);


}
