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
//*     This file contains routines specific to the CS5536. 

#include "vsa2.h"
#include "chipset.h"
#include "pci.h"
#include "vr.h"
#include "mapper.h"
#include "gx2.h"
#include "legacy.h"
#include "protos.h"
#include "isa.h"



// External Function prototypes
extern void GetDrivesPresent(void);
extern void Allocate_Flash_BARs(void);
extern void pascal Flash_IDE_Switch(USHORT, ULONG);
extern void BlockIO(UCHAR);
extern void Register_DMA_Fix(void);
extern UCHAR FlashIsEnabled(void);

// Local function prototypes:
void Handle_5536_PCI_Traps(USHORT, USHORT, ULONG);
void Register_PCI_Trap(USHORT, UCHAR);

// External variables:
extern Hardware SystemInfo;
extern ULONG ChipsetBase;
extern ULONG Param[];
extern USHORT UDMA_IO_Base;
extern ULONG OHCI_Address[];

// Local variables:
typedef void (* PCI_HANDLER)(USHORT, USHORT, ULONG);
extern PCI_HANDLER Handle_PCI_Traps;

USHORT uart1 = 0;
USHORT uart2 = 0;
USHORT Flash_Function=0x00FF, IDE_Function = 0x00FF;
USHORT PCI_Int_AB = ((USHORT)((INTB_PIN << 8) | INTA_PIN));
USHORT PCI_Int_CD = ((USHORT)((INTD_PIN << 8) | INTC_PIN));
USHORT Y_Sources[8];
USHORT Flash_PME;
USHORT Steering = 0x0000;
USHORT Hidden_Function = 0x0000;
UCHAR IDE_Allocated = 0;
UCHAR Flash_Allocated = 0;
ULONG MDD_Base;
USHORT pmc_base = 0;
USHORT gpio_base = 0;
USHORT acpi_base = 0;

typedef struct {
  UCHAR Pin;
  UCHAR Z_Source;
  UCHAR Lbar;
} PCI_INTERRUPT;

PCI_INTERRUPT PCI_Interrupt[] = {
//    Pin     Z_Source
  {INTA_PIN, Z_IRQ_INTA},
  {INTB_PIN, Z_IRQ_INTB},
  {INTC_PIN, Z_IRQ_INTC},
  {INTD_PIN, Z_IRQ_INTD}
};


//***********************************************************************
// Hides the PCI header of the specified function
//***********************************************************************
void Hide_Hdr(USHORT Function)
{

  // If a function is currently being hidden, un-hide it
  if (Hidden_Function) {
    SYS_UNREGISTER_EVENT(EVENT_PCI_TRAP, ChipsetBase+Hidden_Function, 0x000000FF);
    SYS_UNREGISTER_EVENT(EVENT_PCI_TRAP, ChipsetBase+Function+0x40,  WRITES_ONLY);
  }

  // Register for IDE/Flash config space (to make invisible)
  Register_PCI_Trap(Function, 0xFF);

  // Record which function is hidden
  Hidden_Function = Function;

  // Get the 'other' function
  if (Function == IDE_Function) {
    Function = Flash_Function;
  } else {
    Function = IDE_Function;
  }

  // Trap IDE/Flash register 0x40 (used for Flash<->IDE switch)
  SYS_REGISTER_EVENT(EVENT_PCI_TRAP, ChipsetBase+Function+0x40, WRITES_ONLY, 0);

}


//***********************************************************************
// Maps the NAND Flash PMEs to the specified IRQ
//***********************************************************************
void Map_Flash_IRQ(UCHAR Irq)
{
  SYS_MAP_IRQ((UCHAR)(Flash_PME+0), Irq);
  SYS_MAP_IRQ((UCHAR)(Flash_PME+1), Irq);
}



//***********************************************************************
// Hides the IDE function and un-hides the Flash function
//***********************************************************************
void Hide_IDE_Hdr(void)
{ UCHAR Shift, Irq;

  // Map NAND Flash interrupts
  Shift = (UCHAR)(((Flash_PME >> 8)-1) * 4);
  Irq = (UCHAR)(Steering >> Shift) & 0x0F;
  Map_Flash_IRQ(Irq);

  // Hide the IDE header
  Hide_Hdr(IDE_Function);
} 
 


//***********************************************************************
// Hides the Flash function and un-hides the IDE function
//***********************************************************************
void Hide_Flash_Hdr(void)
{ USHORT Bar;

  // Unmap NAND Flash interrupts
  Map_Flash_IRQ(0);

  // If Flash BARs have been allocated, then zero them to disable linked MSRs
  if (Flash_Allocated) {
    for (Bar = BAR0; Bar <= BAR3; Bar += 0x10) {
      WRITE_PCI_DWORD(ChipsetBase+IDE_Function+Bar, 0x00000000);
    }
  }
  Hide_Hdr(Flash_Function);

  if (!IDE_Allocated) {

    // Allocate UDMA BAR
    SYS_ALLOCATE_RESOURCE(RESOURCE_IO, BAR4, 16, DEVICE_ID_AMD_THOR,  ID_ATA);

    IDE_Allocated = 1;
  }
} 
 


//***********************************************************************
// Performs early initialization for CS5536
//***********************************************************************
void CS5536_Early_Init(void)
{ USHORT Function;
  ULONG PciAddr, PciData;

  Handle_PCI_Traps = Handle_5536_PCI_Traps;

  MDD_Base = SYS_LOOKUP_DEVICE(ID_MDD, 1);
  // Get I/O base of PMC, ACPI & GPIO
  pmc_base  = READ_PCI_WORD(ChipsetBase + BAR4) & 0xFFFE;
  gpio_base = READ_PCI_WORD(ChipsetBase + BAR1) & 0xFFFE;
  acpi_base = READ_PCI_WORD(ChipsetBase + BAR5) & 0xFFFE;

					 
  // Scan Southbridge functions to:
  // - Get Unrestricted Sources Y IRQ.
  // - Find Flash & IDE functions.
  for (Function = 0; Function < 8; Function++) {

    // Generate PCI configuration address
    PciAddr = ChipsetBase | (Function << 8);
    
    // For functions that have PCI interrupts, Interrupt Line 
    // contains Y Sources field number.
    (UCHAR)PciAddr = INTERRUPT_LINE;

    // If PCI interrupt is defined, record linked Unrestricted Y Source
    Y_Sources[Function] = READ_PCI_WORD(PciAddr);
    if (Y_Sources[Function]) {
      // clear Interrupt Line
      WRITE_PCI_WORD(PciAddr, 0x0000);
    }

    // Read Class Code
    (UCHAR)PciAddr = REVISION_ID;
    PciData = READ_PCI_DWORD(PciAddr);
    // Ignore Revision ID
    PciData &= 0xFFFFFF00;

    // Record function # of Flash header
    if (PciData == 0x05010000) {
      Flash_Function = (USHORT)PciAddr & 0x0700;
      Flash_PME = Y_Sources[Function];
    }
    // Record function # of IDE header
    if (PciData == 0x01018000) {
      IDE_Function   = (USHORT)PciAddr & 0x0700;
      Y_Sources[Function] = Flash_PME+1;
    }
  }

  switch (SystemInfo.Chipset_ID) {
    case DEVICE_ID_5536:
      SYS_REGISTER_EVENT(EVENT_PCI_TRAP, ChipsetBase+IDE_Function+BAR4, WRITES_ONLY, 0);
      break;
  }


  // Is Flash controller enabled ?
  if (FlashIsEnabled()) {

    // Allocate Flash BARs
    Allocate_Flash_BARs();

    // Hide the IDE header
    Hide_IDE_Hdr();

  } else {

    // Hide the Flash header
    Hide_Flash_Hdr();
  }

  // Register for virtual registers VRC_MISCELLANEOUS::PCI_INT_AB->WATCHDOG
  SYS_REGISTER_EVENT(EVENT_VIRTUAL_REGISTER, VRC_MISCELLANEOUS, (PCI_INT_AB<<8) | WATCHDOG, MAX_PRIORITY);

  // Register for SB PCI register 0x5C-0x5D (emulation of 5530 PCI steering register)
  Register_PCI_Trap(0x005C, 0x01);

/*MEJ
  // If Power Management VSM is present...
  if (SYS_VSM_PRESENT(VSM_PM)) {
    // Register for virtual register timeouts on legacy devices
    SYS_REGISTER_EVENT(EVENT_VIRTUAL_REGISTER, VRC_PM, (DISK_TIMEOUT<<8) | PARALLEL_TIMEOUT, 0);
  }
*/

  // Register for virtual register class VRC_CHIPSET
  SYS_REGISTER_EVENT(EVENT_VIRTUAL_REGISTER, VRC_CHIPSET, 0, NORMAL_PRIORITY);


}



//***********************************************************************
// Performs End-of-POST initialization for CS5536
//***********************************************************************
void CS5536_Late_Init(void) 
{
  // Determine how many ATA drives are present
  GetDrivesPresent();

  // Set ISA bridge Latency Timer to 0x40
  WRITE_PCI_BYTE(ChipsetBase + LATENCY_TIMER, 0x40);

}


//***********************************************************************
// Handler for writes to VRC_MISCELLANEOUS PCI_INT_AB & PCI_INT_CD
// These registers define what GPIOs are to be used for PCI interrupts.
//***********************************************************************
void Handle_Misc_VR(UCHAR Index, USHORT Data)
{ int i, j;
  ULONG Param2;
  static UCHAR Flag = 0x00;


  if (Index == PCI_INT_AB) {
    // PCI interrupt GPIOs can only be allocated once
    if (Flag & 1) {
      return;
    }
    // Record data for readback
    PCI_Int_AB = Data;
    Flag |= 1;
    i = 0;
  } else {
    // PCI interrupt GPIOs can only be allocated once
    if (Flag & 2) {
      return;
    }
    // Record data for readback
    PCI_Int_CD = Data;
    Flag |= 2;
    i = 2;
  }

  // Set GPIOs as level-sensitive (no ASMI!), inverted inputs
  j = i + 2;
  while (i < j) {

    // Record the GPIO pin
    PCI_Interrupt[i].Pin = (UCHAR)Data;

    if (PCI_Interrupt[i].Pin < 32) {
      Param2 = ((ULONG)PCI_Interrupt[i].Z_Source << 16) | PCI_Interrupt[i].Pin;
      SYS_REGISTER_EVENT(EVENT_GPIO, Param2, INVERT, MAX_PRIORITY);
    }

    // Shift the next pin # into the 8 LSBs
    Data >>= 8;

    i++;
  }
}




//***********************************************************************
// Emulates CS5530's PCI Interrupt steering registers 0x5C & 0x5D
// Register 0x5C:
//   3:0	INTA#
//   7:4	INTB#
// Register 0x5D:
//   3:0	INTC#
//   7:4	INTD#
// Maps PCI INT pins to Unrestricted Z sources
//***********************************************************************
void PCI_Interrupt_Steering(USHORT Data)
{ UCHAR i, j, Irq;

  // Register for PCI interrupt GPIOs in case BIOS never wrote to the VR
  Handle_Misc_VR(PCI_INT_AB, PCI_Int_AB);
  Handle_Misc_VR(PCI_INT_CD, PCI_Int_CD);

  for (i=0; i < 4; i++) {

    // Extract IRQ from next nibble
    Irq = (UCHAR)(Data & 0x0F);
 
    // Don't allow IRQ2 (SMI)
    if (Irq != 2) {
      // Map the Unrestricted Z Source to the requested IRQ
      SYS_MAP_IRQ((UCHAR)(PCI_Interrupt[i].Z_Source+16), Irq);

      // Map the Unrestricted Y Source (if any) to the requested IRQ
      for (j=0; j < 8; j++) {
        UCHAR InterruptPin;

        InterruptPin = (UCHAR)(Y_Sources[j] >> 8);
        if (InterruptPin == i+1) {
          SYS_MAP_IRQ((UCHAR)Y_Sources[j], Irq);
        }
      }
    }

    // Shift the next nibble into the 4 LSBs
    Data >>= 4;
  }
}

//***********************************************************************
// Handler for emulation of 5530 PCI steering registers on a 5536 system
//***********************************************************************
void Handle_5536_PCI_Traps(USHORT PCI_Addr, USHORT IO_Params, ULONG Data)
{ UCHAR PCI_Reg, Shift, IO_Size;
  USHORT Function;


  PCI_Reg  = (UCHAR) PCI_Addr;
  IO_Size  = (UCHAR) IO_Params;
  Function = PCI_Addr & 0x0700;
  Shift    = (PCI_Reg & 3) << 3;

  // Record OHCI BAR values
  if (PCI_Addr == 0x7C10 || PCI_Addr == 0x7D10) {
    if (IO_Params & IO_WRITE) {
      Function = (Function >> 8) - 4;
      OHCI_Address[Function] = Data & 0xFFFFF000;
    }
    return;
  }

  // PCI Interrupt Steering register
  if (Function == 0x0000) {

    if (IO_Params & IO_WRITE) {
      // Handle mis-aligned accesses
      if (Shift) {
        Steering &= 0x00FF;
        Steering |= (USHORT)Data << 8;
      } else {
        Steering = (USHORT)Data;
      }
      PCI_Interrupt_Steering(Steering);
      return;

    } else {
      if (PCI_Reg < 0x5C) {
        Data = (ULONG)Steering << (32-Shift);
        Shift = 0;
      } else {
        Data = Steering;
      }
    }

  } else {

    // Flash/IDE configuration space
    if (Function == Flash_Function || Function == IDE_Function) {

      if (IO_Params & IO_WRITE) {

        // PCI writes to IDE/Flash function
        switch (PCI_Reg) {
          case BAR4:
            if (Function == IDE_Function && (USHORT)Data != 0xFFFF) {
              // Record UDMA I/O base for BLOCK_IO logic
              UDMA_IO_Base = (USHORT)Data & 0xFFF0;
            }
            break;

          // Write to IDE<->Flash switch
          case 0x40:
            if (Data == 0xDEADBEEF || Data == 0xBEEFDEAD) {
              // Switch Flash<->IDE 
              Flash_IDE_Switch(Function, Data);
            }
        }
        return;

      } else {

        // PCI reads of hidden function
        if (Function == Hidden_Function) {
          Data = 0xFFFFFFFF;
        }
      }
    }
  }


  // Handle non-dword aligned accesses
  Data >>= Shift;
  Data  |= 0xFFFFFFFFL << (32-Shift);

  // Return the PCI register value
  SYS_RETURN_RESULT(Data);

}

//***********************************************************************
// Registers a PCI trap for the specified Addr/Mask
//***********************************************************************
void Register_PCI_Trap(USHORT Addr, UCHAR Mask)
{
  SYS_REGISTER_EVENT(EVENT_PCI_TRAP, ChipsetBase+Addr, (ULONG)Mask, 0);
}

