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

//*   Function:                                                         *
//*     Handler for blocked PIO during UDMA    

#include "vsa2.h"
#include "protos.h"


#define RESET_DRIVE     0x08


USHORT UDMA_IO_Base;


//***********************************************************************
// Enables/disables BLOCKIO
//***********************************************************************
void pascal BlockIO(UCHAR EnableFlag) 
{ USHORT Priority=UNREGISTER_PRIORITY;

  if (EnableFlag) {
    Priority = MAX_PRIORITY;
  }

  SYS_REGISTER_EVENT(EVENT_BLOCKIO, 0, 0, Priority);
}





//***********************************************************************
// Handler for EVENT_BLOCKIO
//***********************************************************************
void Handle_BLOCKIO(USHORT IO_Address, ULONG Data, UCHAR DataSize)
{ UCHAR Command;

  switch (IO_Address) {

    case 0x1F7:
      // Check for drive reset command
      if ((UCHAR)Data == RESET_DRIVE) {
        break;
      }
      return;

    case 0x3F6:
      if ((UCHAR)Data & 4) {
        break;
      }  

    default:
      // Ignore I/O to all other IDE registers
	  return;
  }

  // Disable BLOCKIO
  BlockIO(0);

  // Terminate bus mastering
  Command = in_8(UDMA_IO_Base);
  out_8(UDMA_IO_Base, (UCHAR)(Command & ~1));

  // Re-issue the I/O
  out_8(IO_Address, (UCHAR)Data);

  // Re-enable BLOCKIO
  BlockIO(1);

}
