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
//*     Implements writes of virtualized PCI configuration headers
//*****************************************************************************


#include "VSA2.H"
#include "PCI.H"
#include "GX2.H"
#include "VPCI.H"
#include "SYSMGR.H"
#include "CHIPSET.H"
#include "PROTOS.H"
#include "MAPPER.H"


// External function declarations:
PCI_HEADER_ENTRY * pascal Get_Structure(USHORT);
UCHAR pascal Update_BusMaster(PCI_HEADER_ENTRY *, UCHAR);
void pascal Set_Latency(UCHAR);
void pascal Clear_MBus_Error(PCI_HEADER_ENTRY *, ULONG);
void pascal SMINT_Handler(USHORT);
void pascal PCI_Interrupt_Steering(USHORT);
void pascal Handle_EHCI_Wr(PCI_HEADER_ENTRY *);
void pascal Handle_PCI_PM_Wr(PCI_HEADER_ENTRY *, USHORT);
void pascal Update_Special_Cycles(USHORT);
void Deallocate_Descriptor(DESCRIPTOR *);

// External variable declarations:
extern DESCRIPTOR MSRs[];
extern ULONG ATA_Error;
extern UCHAR Shift, AlignedReg, Function;
extern PCI_HEADER_ENTRY * CommandPtr;
extern PCI_HEADER_ENTRY * HdrPtr;
extern VIRTUAL_DEVICE * IDSELs;
extern Hardware HardwareInfo;


//***********************************************************************
// Writes an embedded PCI register
//***********************************************************************
void pascal Write_EPCI(UCHAR AlignedReg, ULONG Data)
{ ULONG MsrAddr;

  MsrAddr = MSRs[CommandPtr->Link].MsrAddr;
  if (MsrAddr) {
    (UCHAR)MsrAddr = AlignedReg/4;
    Write_MSR_LO(MsrAddr, Data);
  }	 
}


//***********************************************************************
//
// Handle changes to the COMMAND register
//
//***********************************************************************
void pascal Update_Command(USHORT PreviousValue, USHORT NewValue)
{ UCHAR Changes, EnableFlag;
  register PCI_HEADER_ENTRY * Pci;
  ULONG OldValue;
  
  Changes = (UCHAR)(PreviousValue ^ NewValue);

  //
  // Handle changes to Special Cycles
  //
  if (Changes & SPECIAL_CYCLES) {
    Update_Special_Cycles((UCHAR)NewValue & SPECIAL_CYCLES);
  }


  //
  // Handle changes to bus master & address space enables
  //
  if (Changes & (IO_SPACE | MEM_SPACE | BUS_MASTER)) {

    Pci = CommandPtr + 2;

    // Walk through BAR entries & enable/disable corresponding descriptors/LBARs
    do {

      // Advance ptr to next implemented register
      Pci++;

      // Only concerned with allocated BARs
      if (Pci->Flag & (MMIO_BAR | MEM_BAR | IO_BAR)) {

        // Only write descriptors for BARs that have had resources allocated
        // and have a non-zero value.
        if ((Pci->Link) && Pci->Value) {

          // Check correct Command bit according to BAR type
          EnableFlag = (Pci->Flag & IO_BAR) ? IO_SPACE : MEM_SPACE;

          // Handle changes to address space enable
          if (Changes & EnableFlag) {
            EnableFlag &= (UCHAR)NewValue;

            // Update the descriptor(s)
            OldValue = Pci->Value;
            Update_BAR(Pci, EnableFlag);
            Pci->Value = OldValue;
          }


          //
          // Handle changes to bus master enable
          //
          if (Changes & BUS_MASTER) {
            EnableFlag = (UCHAR)NewValue & BUS_MASTER;
            if (Update_BusMaster(Pci, EnableFlag)) {
              // Early-out (e.g. bridge)
              Changes &= ~BUS_MASTER;
            }
          }
        }
      }

    } while (!(Pci->Flag & EOL));
  }

}




//***********************************************************************
//
// This routine implements writes to virtualized configuration space.
//
// NOTES:
// 1) Misaligned accesses are handled.  If an access crosses a DWORD 
//    boundary, only the bytes within the addressed DWORD are written.
// 2) The variable Pci points to a PCI_HEADER_ENTRY entry that defines
//    the state and behavior of the accessed register.
//
//***********************************************************************
ULONG pascal Virtual_PCI_Write_Handler(USHORT PCI_Address, UCHAR Size, ULONG NewData)
{ UCHAR EnableFlag;
  ULONG Mask, Data, PreviousData;
  register PCI_HEADER_ENTRY * Pci;


  // Get ptr to virtualized table entry
  Pci = Get_Structure(PCI_Address);

  // Unimplemented function ?
  if ((USHORT)Pci == UNIMPLEMENTED_FUNCTION) {
    return NewData;
  }

  // Unimplemented register ?
  if ((USHORT)Pci == UNIMPLEMENTED_REGISTER) {
    // Removing an entire PCI function ?
    if (AlignedReg == 0x7C && NewData == 0xDEADBEEF) {
      UCHAR i, Link;
      DESCRIPTOR * Descr;


#if SUPPORT_CAPABILITIES
      Pci = HdrPtr + BAR0/4;
      do {
        Pci++;
        // If this device is PCI PM-aware...
        if (Pci->Flag & PCI_PM) {
          // then set device to D3
          (UCHAR)PCI_Address = 0x44;
          Virtual_PCI_Write_Handler(PCI_Address, BYTE_IO, 0x03);
          break;
        }
      } while (!(Pci->Flag & EOL));
#endif
      // Make function unimplemented
      IDSELs[Function] = 0x0000;

      // Walk through all BARs and deallocate associated MSRs
      Pci = HdrPtr + BAR0/4;
      if (Pci->Reg != BAR0) {
        return NewData;
      }

      for (i = BAR0; i <= BAR5; i += 4) {
        if (Pci->Flag & (MMIO_BAR | MEM_BAR | IO_BAR)) {
          Link = Pci->Link;

          // For each linked item, update the associated MSR
          while (Link) {
            Descr = &MSRs[Link];

            // Get link to next MSR
            Link = Descr->Link;

            // Deallocate the descriptor & set MSR to default value
            Deallocate_Descriptor(Descr);
          }
        }
        if (Pci->Flag & EOL) {
          break;
        }
        Pci++;
      }
    }
    return NewData;
  }

  // Save a copy of original value
  PreviousData = Pci->Value;

  //
  // Generate mask to preserve read-only fields
  //
  switch (Size) {

    case BYTE_IO:
      Mask = 0x000000FF;
      break;

    case WORD_IO:
      Mask = 0x0000FFFF;
      break;

    case DWORD_IO:
      Mask = 0xFFFFFFFF;
      break;

    default:
      Mask = 0x00000000;
      break;
  }
  Mask = Pci->Mask & (Mask << Shift);


  //
  // Compute new register value
  // - preserves R/O fields
  // - handles misaligned accesses
  // - handles accesses smaller than dword
  //
  NewData <<= Shift;
  Data = (NewData & Mask) | (PreviousData & ~Mask);


  //
  // Record the new register value
  //
  Pci->Value = Data;


  // Handle Write-to-Clear bits
  if (NewData & Pci->WriteToClear) {
    Pci->Value &= ~(NewData & Pci->WriteToClear);
  }


  //
  // Update the MBus hardware
  //
  switch (AlignedReg) {

    case COMMAND:

      // Handle writes to Command register
      if ((USHORT)PreviousData != (USHORT)Data) {
        // If the Command register has changed, update the h/w
        Update_Command((USHORT)PreviousData, (USHORT)Data);
      }

      // Handle writes to Status[15:11] (write-to-clear);
      if (NewData &= Pci->WriteToClear) {
        // Clear status bits in device's MSR
        Clear_MBus_Error(Pci, NewData);
      }

      // Handle SMI on EHCI COMMAND write
      if (Pci->Flag & PCI_EHCI) {
        Handle_EHCI_Wr(Pci);
        break;
      }
      break;

    case CACHE_LINE:
      // PCI Spec: if an unsupported Cache Line value is written, treat as 0x00
      if ((UCHAR)Data ^ (UCHAR)Pci->Mask) {
        Pci->CacheLineSize = 0x00;
      }
      Set_Latency((UCHAR)(Data >> 8));
      break;



    // Emulation of CS5530 software SMI mechanism
    case SW_SMI:  // 0xD0
      SMINT_Handler((USHORT)Data);
      break;

    case (PCI_PM_REG+4): // 0x44
#if SUPPORT_CAPABILITIES
      if (Pci->Flag & PCI_PM) {
        Handle_PCI_PM_Wr(Pci, (USHORT)PreviousData);
        break;
      }
#endif
    case IDE_CFG:
        // Don't write IDE_CFG if this is a IDE-to-Flash switch
        if (NewData == 0xDEADBEEF && Pci->LBar) {
          return NewData;
        }
    case IDE_DTC:	// 0x48
    case IDE_CAST:  // 0x4C
    case IDE_ETC:   // 0x50
      // Thor ATA vendor-specific registers:
      if (Pci->LBar) {
        ULONG MsrAddr = ATA_Error;

        (USHORT)MsrAddr = (USHORT)Pci->LBar;

        // Sync with the current MSR value
        PreviousData = Read_MSR_LO(MsrAddr);

        // Update Thor ATA MSR with the new value
        Pci->Value = (NewData & Mask) | (PreviousData & ~Mask);
        Write_MSR_LO(MsrAddr, Pci->Value);
        break;
      }


#if SUPPORT_CAPABILITIES
    case BAR0:
    case USBLEGCTLSTS:
    case SRBN_REG:
      if (Pci->Flag & PCI_EHCI) {
        Handle_EHCI_Wr(Pci);
      }
#endif


    default:
      // Is it a BAR ?
      if (Pci->Flag & (MMIO_BAR | MEM_BAR | IO_BAR)) {
        // If value has changed, update the MBus descriptor
        if (PreviousData != Data) {
          // I/O BAR and Memory BAR ?
          EnableFlag  = (Pci->Flag & IO_BAR) ? IO_SPACE : MEM_SPACE;
          EnableFlag &= (UCHAR)CommandPtr->Value;
          Update_BAR(Pci, EnableFlag);
        }
      }
      break;

  } // end switch(AlignedReg)

  return Pci->Value;
}



