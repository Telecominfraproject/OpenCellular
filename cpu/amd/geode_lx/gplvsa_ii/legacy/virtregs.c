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
//*     This file handles virtual registers. 

#include "vsa2.h"
#include "chipset.h"
#include "vr.h"
#include "protos.h"
#include "acpi.h"

extern void Handle_Misc_VR(UCHAR, USHORT);
extern void Handle_SysInfo_VR(UCHAR);
extern void Handle_5536_UART(UCHAR, UCHAR, USHORT);

extern USHORT PCI_Int_AB, PCI_Int_CD;
extern USHORT DiskTimeout, SerialTimeout, ParallelTimeout, FloppyTimeout;
USHORT WatchDogDelay;

void Handle_VirtualRegs(UCHAR Class, UCHAR Index, UCHAR WrFlag, USHORT Data)
{
  ULONG Param[MAX_MSG_PARAM];  // Select on virtual register class
  switch (Class) {

    case VRC_MISCELLANEOUS:
      if (WrFlag) {
        if (Index == WATCHDOG) {
          static ULONG WatchDogKey=0;
          ULONG Key;

          Key = GET_REGISTER(R_ECX);
          if (WatchDogKey) {
            // The key has already been set...check it
            if (WatchDogKey != Key) {
/*MEJ              // Broadcast to all VSM's
              Param[0] = S5_STATE;
              Param[1] = CLASS_ALL;
              SYS_BROADCAST_MSG(MSG_SET_POWER_STATE, &Param[0], VSM_ANY);
*/
            }
          } else {
            // Set the key (only once)
            WatchDogKey = Key;
          }
          WatchDogDelay = Data;
          SYS_REGISTER_EVENT(EVENT_TIMER, WatchDogDelay*1000L, ONE_SHOT, 0);
          break;
        }
        Handle_Misc_VR(Index, Data);
      } else {
        switch (Index) {
          case PCI_INT_AB:
            SET_AX(PCI_Int_AB);
            break;

          case PCI_INT_CD:
            SET_AX(PCI_Int_CD);
            break;

          case WATCHDOG:
            SET_AX(WatchDogDelay);
            break;
        }
      }
      break;

    case VRC_PM:
      // Ignore READs
      if (WrFlag) {
/*MEJ        switch (Index) {
          case DISK_TIMEOUT:
            DiskTimeout = Data;
            break;

          case SERIAL_TIMEOUT:
            SerialTimeout = Data;
            break;

          case PARALLEL_TIMEOUT:
            ParallelTimeout = Data;
            break;

          case FLOPPY_TIMEOUT:
            FloppyTimeout = Data;
            break;
        }
*/
      }
      break;

    case VRC_SYSINFO:
      if (!WrFlag) {
        Handle_SysInfo_VR(Index);
      }
      break;


    case VRC_CHIPSET:
      switch (Index) {
        case VRC_CS_UART1:
        case VRC_CS_UART2:
          Handle_5536_UART(Index, WrFlag, Data);
          break;

      }
      break;
  }
}






