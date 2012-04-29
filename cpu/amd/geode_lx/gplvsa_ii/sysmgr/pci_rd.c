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
//*     Implements reads of virtualized PCI configuration headers
//*****************************************************************************


#include "VSA2.H"
#include "PCI.H"
#include "VPCI.H"
#include "PROTOS.H"
#include "SYSMGR.H"
#include "CHIPSET.H"


// External function declarations:
ULONG pascal Get_Device_Status(PCI_HEADER_ENTRY *);
ULONG pascal Handle_PCI_PM_Rd(PCI_HEADER_ENTRY *);
ULONG pascal Handle_EHCI_Rd(PCI_HEADER_ENTRY *);
ULONG pascal Read_MSR_LO(ULONG);
UCHAR pascal Get_Latency(PCI_HEADER_ENTRY *);
PCI_HEADER_ENTRY * pascal Get_Structure(USHORT);
extern Hardware HardwareInfo;



// External variable declarations:
extern DESCRIPTOR MSRs[];
extern UCHAR Shift, AlignedReg, Function;
extern PCI_HEADER_ENTRY * CommandPtr;
extern ULONG ATA_Error;



//***********************************************************************
// Reads an embedded PCI register
//***********************************************************************
ULONG pascal Read_EPCI(UCHAR AlignedReg)
{ ULONG MsrAddr;

  MsrAddr = MSRs[CommandPtr->Link].MsrAddr;
  if (MsrAddr) {  
    (UCHAR)MsrAddr = AlignedReg/4;
    return Read_MSR_LO(MsrAddr);
  } else {
    return 0;  
  }  
}


//***********************************************************************
//
// This routine implements reads to virtualized configuration space.
//
// NOTES:
// 1) Misaligned accesses are handled.  If an access crosses a DWORD 
//    boundary, only the bytes within the addressed DWORD are read.
//    The remaining bytes return FFs.
// 2) The variable Pci points to a PCI_HEADER_ENTRY entry that defines
//    the state and characteristics of the register being read.
//***********************************************************************
ULONG pascal Virtual_PCI_Read_Handler(USHORT PCI_Address)
{ ULONG Data;
  register PCI_HEADER_ENTRY * Pci;


  // Get ptr to virtualized table entry
  Pci = Get_Structure(PCI_Address);


  switch ((USHORT)Pci) {

    case UNIMPLEMENTED_FUNCTION:
      Data = 0xFFFFFFFF;
      break;

    case UNIMPLEMENTED_REGISTER:
      Data = 0x00000000;
      break;

    default:
      // Handle special cases
      if (Pci->Flag & EPCI_R && AlignedReg < 0x40) {

        // Embedded PCI device: read its h/w PCI configuration space
        Data = Read_EPCI(AlignedReg);

        if (AlignedReg == COMMAND) {
          // Mark device '66 MHz capable'
          Data |= PCI_66MHZ_CAPABLE;
        }
        break;    
      }

      Data = Pci->Value;

      switch (AlignedReg) {

        case COMMAND:
          // Get device's Status
          Data |= Get_Device_Status(Pci);
          break;

        case BAR0:
        case BAR1:
        case BAR2:
        case BAR3:
        case BAR4:
        case BAR5:
          // If I/O BAR, set bit 0
          if (Pci->Flag & IO_BAR) {
            Data |= 1;
          }
          break;

        case CACHE_LINE:
          Pci->LatencyTimer = Get_Latency(Pci);
          Data = Pci->Value;
          break;

#if SUPPORT_CAPABILITIES
        case (PCI_PM_REG+4):  // 0x44
          if (Pci->Flag & PCI_PM) {
            Data = Handle_PCI_PM_Rd(Pci);
            break;
          }

        case USBLEGCTLSTS:
          if (Pci->Flag & PCI_EHCI) {
            Data = Handle_EHCI_Rd(Pci);
            break;
          }
#endif

        // Thor ATA vendor-specific registers:
        case IDE_CFG:   // 0x40
        case IDE_DTC:	// 0x48
        case IDE_CAST:  // 0x4C
        case IDE_ETC:   // 0x50
          if (Pci->LBar) {
            ULONG MsrAddr = ATA_Error;

            (USHORT)MsrAddr = (USHORT)Pci->LBar;
            Data = Read_MSR_LO(MsrAddr);
          }
          break;

      }
  }


  // Handle non-dword aligned accesses
  Data >>= Shift;
  Data  |= 0xFFFFFFFFL << (32-Shift);

  return Data;
}   



