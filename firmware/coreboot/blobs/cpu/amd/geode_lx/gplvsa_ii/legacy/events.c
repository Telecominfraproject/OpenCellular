/*
* Copyright (c) 2006 Advanced Micro Devices,Inc. ("AMD").
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

//*   Function:                                                         *
//*     This file handles the Legacy VSM's event messages. 

#include "vsa2.h"
#include "chipset.h"
#include "vr.h"
#include "legacy.h"
#include "protos.h"
#include "pci.h"
#include "isa.h"

// Function prototypes
extern void Handle_BLOCKIO(USHORT, ULONG, UCHAR);
extern void Handle_VirtualRegs(UCHAR, UCHAR, UCHAR, USHORT);
extern void Do_OHCI_SWAPSiF(void);

// External variables:
extern ULONG ChipsetBase;
extern USHORT DiskTimeout, SerialTimeout, ParallelTimeout, FloppyTimeout;

// Local variables:
USHORT IDE_Interface = MASTER_IDE;  // Default to "bus master capable"

typedef void (* PCI_HANDLER)(USHORT, USHORT, ULONG);
PCI_HANDLER Handle_PCI_Traps;


//***********************************************************************
// Register I/O trap to fix 118.409	- Block/Demand mode fails
//***********************************************************************
void Register_DMA_Fix(void)
{
#define DMA_FLAGS (WRITES_ONLY | ONE_SHOT)

   SYS_REGISTER_EVENT(EVENT_IO_TRAP, DMA1_MODE, DMA_FLAGS | 1, 0);
}   


//***********************************************************************
// Handler for MSG_EVENT
//***********************************************************************
void Handle_Events(ULONG * Param)
{ USHORT Event, IO_Address;
  ULONG Data;
  UCHAR WrFlag, DataSize, Class, Index;

  Event = (USHORT)Param[0];
  IO_Address = (USHORT)Param[2];
  DataSize = (UCHAR)(Param[2] >> 16);
  Data = Param[3];

  switch (Event) {

    case EVENT_IO_TRAP:
      // If it is an I/O to the 8237 DMA controller...
      if (IO_Address == DMA1_MODE) {
        static UCHAR Mode = 0x00;
        UCHAR ByteData;

        // and it is a I/O write...
        if (Param[1] & 2) {
          ByteData = (UCHAR)Data;
          // and the Mode is either Block or Demand...
          Mode = ByteData & MODE_MASK;
          if (Mode == MODE_DEMAND || Mode == MODE_BLOCK) {
            // then change the mode to Single
            ByteData &= ~MODE_MASK;
            ByteData |=  MODE_SINGLE;
          }

          // Re-issue the I/O (trapping is disabled since it was a ONE_SHOT)
          out_8(DMA1_MODE, ByteData);
	    }
        // Re-register for the I/O trap
        Register_DMA_Fix();
        break;
      }

      // Handle PM traps here...
      break;

    case EVENT_IO_TIMEOUT:
      // Handle PM timeout here...
      break;

    case EVENT_SOFTWARE_SMI:
      if (Param[1] == SYS_DMA_DRIVER) {
        // Allow the UDMA driver to see that IDE is bus master capable.
        IDE_Interface = MASTER_IDE; 
      } else if (Param[1] == SYS_DMA_DRIVER+1) {
        // Prevent the UDMA driver from seeing that IDE is bus master capable.
        IDE_Interface = 0; 
      }
      break;

    case EVENT_PCI_TRAP:
      Handle_PCI_Traps((USHORT)Param[1], (USHORT)Param[2], Data);
      break;

    case EVENT_VIRTUAL_REGISTER:
      // Extract virtual register parameters from message
      Class = (UCHAR)(Param[1] >> 8);
      Index = (UCHAR)Param[1];
      WrFlag = (UCHAR)Param[2];
      Handle_VirtualRegs(Class, Index, WrFlag, (USHORT)Data);
      break;

    case EVENT_BLOCKIO:
      Handle_BLOCKIO(IO_Address, Data, DataSize);
      break;

    case EVENT_TIMER:
      // Fix for attach/detach hardware bug
      if (Param[2] == USBF_HANDLE) {
        Do_OHCI_SWAPSiF();
        break;
      }

/*MEJ      // Broadcast to all VSM's
      Param[0] = S5_STATE;
      Param[1] = CLASS_ALL;
      SYS_BROADCAST_MSG(MSG_SET_POWER_STATE, &Param[0], VSM_ANY);
*/
      break;
  }
}
