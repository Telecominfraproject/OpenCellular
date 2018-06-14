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

//******************************************************************************
//*     This file contains SWAPSiFs
//******************************************************************************



#include "vsa2.h"
#include "protos.h"
#include "chipset.h"
#include "legacy.h"
#include "pci.h"


// External variables
extern Hardware SystemInfo;

// Local variables
ULONG OHCI_Address[2];
UCHAR WinCE_Flag = 1;


//*****************************************************************
// Fixes 118.438 - OHCI May Not Recognize Device Attach/Removal
//*****************************************************************
void pascal OHCI_SWAPSiF(UCHAR Flag)
{ PRIORITY Priority = 0;
  static PRIORITY LastPriority = 0x5555;

  switch (SystemInfo.Chipset_ID) {
    case DEVICE_ID_5536:
      break;

    default:
      if (Flag == 0) {
        Priority = UNREGISTER_PRIORITY;
      }
      if (Priority != LastPriority) {
        LastPriority = Priority;
        SYS_REGISTER_EVENT(EVENT_TIMER, USBF_PERIOD, USBF_HANDLE, Priority);
      }
      break;
  }
}


#define HcControl 0x0004
  #define HCFS				0x000000C0L
  #define USB_RESET			0x00000000L
  #define USB_RESUME		0x00000040L
  #define USB_OPERATIONAL	0x00000080L
  #define USB_SUSPEND	 	0x000000C0L
  #define IR				0x00000100L
#define HceControl 0x0100

//***********************************************************************
void Do_OHCI_SWAPSif(void)
{ ULONG i, PCI_Addr, HC_Addr, OldValue;

  for (i=0; i<=1; i++) {

    // Get base of Host Controller's registers
    HC_Addr = OHCI_Address[i];

    // Ignore this tick if BAR is being sized
    if (HC_Addr == 0xFFFFF000 || HC_Addr == 0x00000000) {
      continue;
    }

    // Check if Host Controller registers are accessible
    PCI_Addr = 0x80007C00L + (i<<8);
    if (READ_PCI_BYTE(PCI_Addr + COMMAND) & MEM_SPACE) {

      OldValue = READ_MEMORY(HC_Addr + HcControl);

      // WinCE doesn't perform a valid Ownership Change.
      // It disables MIE then performs the Ownership Change request.
      // This code detects that situation and turns emulation off.
      // Otherwise, neither the USB keyboard or PS/2 keyboard works.
      if ((i == 0) && (OldValue & IR) == 0 && WinCE_Flag) {
        WinCE_Flag = 0;   // Only do this once
        WRITE_MEMORY(HC_Addr + HceControl, 0x00);
      }

      // Transition through Operational to clear possible attach/detach glitch
      if ((OldValue & USB_SUSPEND) == USB_SUSPEND) {
        WRITE_MEMORY(HC_Addr + HcControl, USB_OPERATIONAL);
        WRITE_MEMORY(HC_Addr + HcControl, OldValue);
      }
    }
  }
}



//***********************************************************************
// Initialization for the OHCI attach/detach hardware bug
//***********************************************************************
void Init_OHCI_SWAPSiF(UCHAR InitStage)
{
  switch (InitStage) {
    case EARLY_INIT:
      // Trap writes to SB F4 & F5 BAR0 in order keep current OHCI BAR values
      SYS_REGISTER_EVENT(EVENT_PCI_TRAP, 0x7C10, WRITES_ONLY | 0x100, 0);
      break;

    case END_OF_POST_INIT:
      // Turn on the timer
      OHCI_SWAPSiF(1);
      break;
  }
}