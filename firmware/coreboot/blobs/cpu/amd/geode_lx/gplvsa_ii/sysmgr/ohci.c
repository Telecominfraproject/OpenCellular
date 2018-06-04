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
//*     Handler for EVENT_USB and EVENT_KEL
//*****************************************************************************




#include "VSA2.H"
#include "PROTOS.H" 
#include "CHIPSET.H"
#include "VPCI.H"
#include "PCI.H"
#include "HCE.H"

extern ULONG MsgPacket[];
extern SmiHeader SMM_Header;
extern Hardware HardwareInfo;
extern VIRTUAL_DEVICE * SouthBridge;
extern void pascal write_flat_size(ULONG, ULONG, UCHAR);

USHORT OHCI_Address;
ULONG OHCI_Command;
ULONG HC_Status;
#define HC ((ULONG)(&((HCOR *)0)


//***********************************************************************
// Restores the OHCI COMMAND register to the original value
//***********************************************************************
void Restore_OHCI_Command()
{
  Virtual_PCI_Write_Handler(OHCI_Address, BYTE_IO, OHCI_Command & ~MEM_SPACE);
}

//***********************************************************************
// Sends EVENT_USB or EVENT_KEL
//***********************************************************************
void Send_OHCI_Event(UCHAR HC_Number)
{ USHORT Hce_Status;
  HCE_CONTROL Hce_Control;
  ULONG HC_Enable, HC_Address;
  ULONG Timeout;
  PCI_HEADER_ENTRY * OHCI_Hdr;

  // Get ptr to virtualized PCI header
  OHCI_Hdr = *(SouthBridge+3 + HC_Number);
  if (!OHCI_Hdr) {
    Log_Error("Invalid HC number 0x%02X", HC_Number);
    return;
  }

  // Get HC address from BAR0
  if (!(HC_Address = (OHCI_Hdr+(BAR0/4))->Value)) {
    return;
  }

  OHCI_Address = 0x7C00 + (((USHORT)HC_Number-1) << 8) + COMMAND;
  // Are memory-mapped registers enabled?
  if (!((OHCI_Hdr+(COMMAND/4))->Value & MEM_SPACE)) {

    OHCI_Command = (OHCI_Hdr+(COMMAND/4))->Value;
    // No, temporarily enable access to them 
    Virtual_PCI_Write_Handler(OHCI_Address, BYTE_IO, OHCI_Command | MEM_SPACE);

    // Schedule a routine to restore the original COMMAND value
    // after the VSM has processed the EVENT_USB or EVENT_KEL.
    Schedule_VSM((USHORT)Restore_OHCI_Command);
  }


  // Get HC's Hce_Control register
  Hce_Control.HceUshort = (USHORT)read_flat(HC_Address + HC->HceControl)));

  // Prepare message packet
  MsgPacket[1] = HC_Address;
  MsgPacket[2] = HC_Number;
  MsgPacket[3] = 0;

  // Is it an emulation event ?
  if (Hce_Control.EmulationInterrupt) {

    // SiBZ 3069/3370: HceStatus[3] is not updated properly
    if (SMM_Header.SMI_Flags.Ext_IO_Trap) {
      if (SMM_Header.SMI_Flags.IO_Write) {

        Hce_Status = (USHORT)read_flat(HC_Address + HC->HceStatus)));
        switch ((USHORT)SMM_Header.IO_addr) {
          case 0x60:
            Hce_Status &= ~CMD_DATA;
            break;
          case 0x64:
            Hce_Status |= CMD_DATA;
            break;
        }
        write_flat_size(HC_Address + HC->HceStatus)), Hce_Status, BYTE_IO);
      }
    }

    // SiBZ 3509/3571: KEL SMIs are level instead of edge-triggered
    MsgPacket[3] = Hce_Control.HceUshort;
    Timeout = 1000;
    while (Timeout--) {

      Hce_Control.HceUshort = (USHORT)read_flat(HC_Address + HC->HceControl)));
      if (Hce_Control.IRQ1Active || Hce_Control.IRQ12Active) {
        // Clear the active IRQ & disable emulation
        Hce_Control.EmulationEnable = 0;
        write_flat_size(HC_Address + HC->HceControl)), Hce_Control.HceUshort , WORD_IO);

        // Read a byte from the data port to dismiss the 8042 interrupt
        in_8(0x60);

        // Re-enable emulation
        Hce_Control.EmulationEnable = 1;
        write_flat_size(HC_Address + HC->HceControl)), Hce_Control.HceUshort, WORD_IO);
      } else {
        break;
      }
    }

    // Report if SiBZ 3571 workaround failed
    if (Timeout == 0) {
      Log_Error("IRQx_ACTIVE won't clear");
    }

    // Send emulation event to the i8042 VSM
    Send_Event(EVENT_KEL, SysMgr_VSM);
  }

  // Any unmasked events pending ?
  HC_Status |= read_flat(HC_Address + HC->HcInterruptStatus)));


  HC_Enable = read_flat(HC_Address + HC->HcInterruptEnable)));
  if (HC_Status & HC_Enable) {
    // SWAPSiF for PBZ 2300:
    MsgPacket[3] = HC_Status;
    write_flat(HC_Address + HC->HcInterruptStatus)), HC_Status);
    
    // Send Host Controller event to the OHCI VSM
    Send_Event(EVENT_USB, 0x00000000);
  }

  HC_Status = 0;
}
