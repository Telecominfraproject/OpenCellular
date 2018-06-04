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

//*    Routines related to PCI power management. 

#include "VSA2.H"
#include "SYSMGR.H"
#include "VPCI.H"
#include "PROTOS.H"
#include "PCI.H"
#include "CHIPSET.H"
#include "GX2.H"
#include "ACPI.H"
#include "DESCR.H"



#if SUPPORT_CAPABILITIES

extern DESCRIPTOR MSRs[];
extern PCI_HEADER_ENTRY * HdrPtr;
extern PCI_HEADER_ENTRY * CommandPtr;
extern UCHAR End_of_POST;
extern ULONG OHCI1_Smi;
extern ULONG MCP_SB;

#define OHC_IN_D3   1
#define EHC_IN_D3   2
#define UDC_IN_D3   4
#define UOC_IN_D3   8
#define ALL_IN_D3   (OHC_IN_D3 | EHC_IN_D3 | UDC_IN_D3 | UOC_IN_D3)

typedef struct {	            // All bits are Read-Only
  union {
    ULONG AsDword;
    struct {
      ULONG CompatibilityID: 8;
      ULONG NextItemPtr:     8;
      ULONG Version:         3;
      ULONG PME_Clock:       1;
      ULONG Reserved:        1;
      ULONG DSI:             1;
      ULONG Aux_Current:     3;
      ULONG D1_Support:      1;
      ULONG D2_Support:      1;
      ULONG PME_D0:          1;
      ULONG PME_D1:          1;
      ULONG PME_D2:          1;
      ULONG PME_D3_Hot:      1;
      ULONG PME_D3_Cold:     1;
    };
  };
} PMC;

typedef struct {
  union {
    ULONG AsDword;
    struct {
      union {
        USHORT AsWord;
        struct {
          USHORT PowerState:  2;  // Read-write
          USHORT Reserved:    6;  // Read-only
          USHORT PME_En:      1;  // Read-write
          USHORT Data_Select: 4;  // Read-write
          USHORT Data_Scale:  2;  // Read-only
          USHORT PME_Status:  1;  // Read/Write-Clear
        };
      };
      UCHAR PMCSR_BSE;            // Bridge Support Extensions
      UCHAR Data;
    };
  };
} PMCR;
// R/W Mask = 1001_1111_0000_0011
#define PMCR_MASK           0x9F03



// Fields in USBMSROHCB, USBMSREHCB, USBMSRUDCB, USBMSRUOCB:
typedef struct {
  union {
    struct {
      ULONG Reserved:  1;
      ULONG MEMEN:     1;
      ULONG BMEN:      1;
      ULONG PMEEN:     1;
      ULONG PMESTS:    1;
    };
    struct {
      UCHAR Enables;
      // NOTE: the next 3 fields are actually 6 bits, but the compiler
      //       generates crappy code if defined as such
      UCHAR FLADJ;
      UCHAR LEGSMIEN;
      UCHAR LEGSMISTS;
    };
  };
} USBMSR;


// Fields in 32-MSBs of GLCP's PMCLKACTIVE, PMCLKOFF, PMCLKDISABLE, PMCLK4ACK, PMCLKDISABLE:
typedef struct {
  union {
    ULONG AsDword;
    struct {
      ULONG GL0_0:           1;
      ULONG GL0_1:           1;
      ULONG GLPCI_GLIU:      1;
      ULONG GLPCI_PCI:       1;
      ULONG GLPCI_PCIF:      1;
      ULONG RSVD:            6;
      ULONG ATAC_GLIU:       1;
      ULONG ATAC_LB:         1;
      ULONG ACC_GLIU:        1;
      ULONG ACC_LB:          1;
      ULONG ACC_BIT:         1;
      ULONG DIVIL_GLIU:      1;
      ULONG DIVIL_LB:        1;
      ULONG DIVIL_LPC:       1;
      ULONG DIVIL_DMA:       1;
      ULONG DIVIL_SMB:       1;
      ULONG DIVIL_PIT:       1;
      ULONG DIVIL_UART1:     1;
      ULONG DIVIL_UART2:     1;
      ULONG DIVIL_PMC:       1;
      ULONG DIVIL_PMC_STD:   1;
      ULONG DIVIL_GPIO:      1;
      ULONG DIVIL_GPIO_STD:  1;
      ULONG DIVIL_MFGPT_32K: 1;
      ULONG DIVIL_MFGPT_14M: 1;
      ULONG DIVIL_32K_STD:   1;
      ULONG GLCP_GLIU:       1;
    };
  };
  union {
    ULONG AsDword_HI;
    struct {
      ULONG GLCP_DBG:        1;
      ULONG GLCP_PCI:        1;
      ULONG OHC_CLK48:       1;
      ULONG UDC_HCLK:        1;
      ULONG EHC_HCLK:        1;
      ULONG OHC_HCLK:        1;
      ULONG EHC_CLK60:       1;
      ULONG UDC_CLK60:       1;
      ULONG USBP1_CLK60:     1;
      ULONG USBP2_CLK60:     1;
      ULONG USBP3_CLK60:     1;
      ULONG USBP4_CLK60:     1;
      ULONG OTC_HCLK:        1;
      ULONG USB_GLIU:        1;    // aka M2A_HCLK in USB 2.0 IDS
      ULONG USBPHYPLLEN:     1;
    };
  };
} GLCP_CLKS;

// Local variables:
ULONG MsrAddr, MsrData[2];
USBMSR * UsbMsr = (USBMSR *)&MsrData[1];
GLCP_CLKS Clocks;
USHORT D3_Flag = 0;
ULONG EHCI_BAR;

//***********************************************************************
// Returns TRUE if the specified EHCI port is suspended
// NOTE: Port is 1-based
//***********************************************************************
UCHAR pascal EHCI_Port_Suspended(UCHAR Port)
{
  if ((UCHAR)READ_MEMORY(EHCI_BAR + 0x50 + Port*4) & 0x80) {
    return 1;
  } else {
    return 0;
  }
}

//***********************************************************************
// Reads the MSR corresponding to the current USB 2.0 function
//***********************************************************************
void Get_USB_MSR(void)
{
  MsrAddr = OHCI1_Smi;
  (UCHAR)MsrAddr = (HdrPtr+BAR0/4)->LBar;

  // Get current MSR value 
  Read_MSR(MsrAddr, MsrData);
}


//***********************************************************************
// Returns value of the EHCI Power Management register USBLEGCTLSTS
//***********************************************************************
ULONG pascal Handle_EHCI_Rd(PCI_HEADER_ENTRY * Pci)
{

  // Filter out non-EHCI functions
  if (((HdrPtr+REVISION_ID/4)->Value & 0xFFFFFF00) == 0x0C032000) {

    // Read USBMSREHCB MSR
    Get_USB_MSR();

    Pci->EHCI_Errors = (UCHAR)UsbMsr->LEGSMISTS;

    // Sync PCI register with MSR (optional):
    Pci->EHCI_SMI_Enables = (UCHAR)UsbMsr->LEGSMIEN;

  }
  return Pci->Value;
}

//***********************************************************************
// Handles writes to the EHCI PCI registers:
// COMMAND
// BAR0
// USBLEGSUP
// USBLEGCTLSTS 
// FLADJ
//***********************************************************************
void pascal Handle_EHCI_Wr(PCI_HEADER_ENTRY * Pci)
{ ULONG SMI_Status = SMI_ON_COMMAND;

  // Read USBMSREHCB MSR
  Get_USB_MSR();

  switch (Pci->Reg & 0xFC) {
    case BAR0:
      EHCI_BAR = Pci->Value;
      SMI_Status = SMI_ON_BAR;
    case COMMAND:
      if (End_of_POST) {
        // Set USBLEGCTLSTS[31/30]
        (HdrPtr+9)->Value |= SMI_Status;
      }
      return;

    case USBLEGSUP:		// EECP
      // If ownership changing, set USBLEGCTLSTS[29]
      if (Pci->Value & (OS_OWNED_SEMAPHORE | BIOS_OWNED_SEMAPHORE)) {
        // Set USBLEGCTLSTS[29]
        (Pci+1)->Value |= SMI_ON_OC;
      }
      // If system software is requesting ownership...
      if (Pci->Value & OS_OWNED_SEMAPHORE) {
        // Clear BIOS Owned	Semaphore
        Pci->Value &= ~BIOS_OWNED_SEMAPHORE;
        // Clear SMI enables
        UsbMsr->LEGSMIEN = 0x00;
      }
      // If BIOS is requesting ownership...
      if (Pci->Value & BIOS_OWNED_SEMAPHORE) {
        // Clear system software Owned Semaphore
        Pci->Value &= ~OS_OWNED_SEMAPHORE;
      }
      break;

    case USBLEGCTLSTS:  // EECP+4
      UsbMsr->LEGSMIEN = Pci->EHCI_SMI_Enables;
      break;

    case SRBN_REG:
      UsbMsr->FLADJ = Pci->FLADJ;
      break;
  }

  // Update USBMSREHCB
  Write_MSR(MsrAddr, MsrData);
}




//***********************************************************************
// Handles reads from a PCI Power Management register
// Returns value of PCI register
//***********************************************************************
ULONG pascal Handle_PCI_PM_Rd(PCI_HEADER_ENTRY * Pci)
{ PMCR * PMCR_Ptr;

  // Get PME status from MSR
  Get_USB_MSR();

  // Cast register ptr
  PMCR_Ptr = (PMCR *)&Pci->Value;
  PMCR_Ptr->PME_Status = UsbMsr->PMESTS ? 1: 0; 

  return Pci->Value;
}

//***********************************************************************
// Handles writes to the PCI Power Management register
//***********************************************************************
void pascal Handle_PCI_PM_Wr(PCI_HEADER_ENTRY * Pci, USHORT PreviousData)
{ PMCR Delta;
  PMCR * PMCR_Ptr;
  PMC * PMC_Ptr;
  USHORT Value;

  // Cast register ptrs
  PMCR_Ptr = (PMCR *)&Pci->Value;
  PMC_Ptr  = (PMC *)&(Pci-1)->Value;

  Value = (USHORT)Pci->Value;

  // Mask read-only bits
  Value &= PMCR_MASK;


  // Compute changes
  Delta.AsWord = (USHORT)PreviousData ^ PMCR_Ptr->AsWord;


  // Select data to be reported through the Data register
  // Return as "not implemented"
  PMCR_Ptr->Data       = 0;  //Power[PMCR_Ptr->Data_Select].Data;
  PMCR_Ptr->Data_Scale = 0;  //Power[PMCR_Ptr->Data_Select].Scale;

  Get_USB_MSR();

  // PME# status write-to-clear
  if (PMCR_Ptr->PME_Status) {
    UsbMsr->PMESTS = 1;
  }

  // PME# enable
  if (Delta.PME_En) {
    // Check if PME# is supported 
    if (PMC_Ptr->PME_D3_Cold || PMC_Ptr->PME_D3_Hot || 
        PMC_Ptr->PME_D0      || PMC_Ptr->PME_D1     || PMC_Ptr->PME_D2) {
      if (PMCR_Ptr->PME_En) {
        // Enable PME# assertion
        UsbMsr->PMEEN = 1;
      } else {
        // Disable PME# assertion
        UsbMsr->PMEEN = 0;
      }
    } else {
      // PME# not supported
      PMCR_Ptr->PME_En = 0;
    }
  }
  // Update USBMSRxxCB
  Write_MSR(MsrAddr, MsrData);

  // Power state
  if (Delta.PowerState) {
    UCHAR SupportedState=0;
	ULONG ClocksMsr;
    register USHORT USB_20_D3_Flag = D3_Flag;

    ClocksMsr = MCP_SB + MCP_PMCLKOFF;
    Read_MSR(ClocksMsr, &Clocks.AsDword);

    switch (PMCR_Ptr->PowerState) {
      case D0_STATE:
        // If D0 is supported...
        if (PMC_Ptr->PME_D0) {
          switch (HdrPtr->Device_ID) {
            case DEVICE_ID_AMD_OHCI:
              Clocks.OHC_HCLK  = 0;
              Clocks.OHC_CLK48 = 0;
              USB_20_D3_Flag &= ~OHC_IN_D3;
              break;

            case DEVICE_ID_AMD_EHCI:
              Clocks.EHC_HCLK  = 0;
              Clocks.EHC_CLK60 = 0;
              Clocks.USBP1_CLK60 = 0;
              Clocks.USBP2_CLK60 = 0;
              Clocks.USBP3_CLK60 = 0;
              Clocks.USBP4_CLK60 = 0;
              Clocks.USBPHYPLLEN = 0;
              USB_20_D3_Flag &= ~EHC_IN_D3;
              break;

            case DEVICE_ID_AMD_UDC:
              Clocks.UDC_HCLK  = 0;
              Clocks.UDC_CLK60 = 0;
              USB_20_D3_Flag &= ~UDC_IN_D3;
              break;

            case DEVICE_ID_AMD_OTG:
              Clocks.OTC_HCLK  = 0;
              USB_20_D3_Flag &= ~UOC_IN_D3;
              break;
          }
          SupportedState = 1;
        }
        break;

      case D1_STATE:
      case D2_STATE:
        // D1 & D2 are not supported
        switch (HdrPtr->Device_ID) {
          case DEVICE_ID_AMD_OHCI:
            USB_20_D3_Flag &= ~OHC_IN_D3;
            break;

          case DEVICE_ID_AMD_EHCI:
            USB_20_D3_Flag &= ~EHC_IN_D3;
            break;

          case DEVICE_ID_AMD_UDC:
            USB_20_D3_Flag &= ~UDC_IN_D3;
            break;

          case DEVICE_ID_AMD_OTG:
            USB_20_D3_Flag &= ~UOC_IN_D3;
            break;
        }
        break;

      case D3_STATE:   // D3hot
        // If D3hot is supported...
        if (PMC_Ptr->PME_D3_Hot) {
          switch (HdrPtr->Device_ID) {
            case DEVICE_ID_AMD_OHCI:
              Clocks.OHC_HCLK  = 0;
              Clocks.OHC_CLK48 = 0;
              USB_20_D3_Flag |= OHC_IN_D3;
              break;

            case DEVICE_ID_AMD_EHCI:
              Clocks.EHC_HCLK  = 0;
              Clocks.EHC_CLK60 = 0;
              Clocks.USBP1_CLK60 = 0;
              Clocks.USBP2_CLK60 = 0;
              Clocks.USBP3_CLK60 = 0;
              Clocks.USBP4_CLK60 = 0;
              Clocks.USBPHYPLLEN = 1;

              // In D3 *only* USBPHYPLLEN should be gated (Phy Clock switched off).
              // This guarantees max. power saving during S1 and proper USB Remote WakeUp functionality and Resume from S1
              // If all EHC ports are suspended USBPHYPLLEN should not be gated, because USBPHYPLL is switched off by Phy-HW 
              if (EHCI_Port_Suspended(1) & EHCI_Port_Suspended(2) & EHCI_Port_Suspended(3) & EHCI_Port_Suspended(4)) {
                Clocks.USBPHYPLLEN = 0;
              }
              USB_20_D3_Flag |= EHC_IN_D3;
              break;

            case DEVICE_ID_AMD_UDC:
              USB_20_D3_Flag |= UDC_IN_D3;
              break;

            case DEVICE_ID_AMD_OTG:
              USB_20_D3_Flag |= UOC_IN_D3;
              break;
          }

          SupportedState = 1;
        }
        break;
    } // end switch (PMCR_Ptr->PowerState)

    if (SupportedState) {
      PCI_HEADER_ENTRY * Bar;
      DESCRIPTOR * Descr;
      ULONG MsrData[2];
      UCHAR Index;

      // If going to D3, turn off P2D_BM descriptor so accesses will master-abort.
      // Otherwise, the system hangs when the device registers are accessed since
      // the clocks have been turned off.  Restore the descriptor when going to D0.
      Bar = HdrPtr+BAR0/4;
      while ((Bar->Reg >= BAR0) && (Bar->Reg <= BAR5)) {
        if (Bar->Flag & (MMIO_BAR | MEM_BAR)) {

          // For each linked item, update the associated MSR
          // Get the P2D_BM descriptor linked to this BAR
          Index = Bar->Link;
          do {
            Descr = &MSRs[Index];
            if (Descr->Type == P2D_BM || Descr->Type == P2D_BMK) {
              if (PMCR_Ptr->PowerState == D3_STATE) {
                Get_Descriptor_Default(Descr->Type, MsrData);
              } else {
                MsrData[0] = Descr->MsrData[0];
                MsrData[1] = Descr->MsrData[1];
              }
              Write_MSR(Descr->MsrAddr, MsrData);
              break;
            }
          } while (Index = Descr->Link);
        }
        if (Pci->Flag & EOL) {
          break;
        }
        Bar++;
      }



// PBZ 2292 - Can't turn off USB_GLIU else MSR read of USB 2.0 hangs
#if 0
      // Are all USB 2.0 functions are in D3?
      if (USB_20_D3_Flag == ALL_IN_D3) {
        // Yes, then turn off USB_GLIU
        Clocks.USB_GLIU = 1;
      } else {
        Clocks.USB_GLIU = 0;
      }
#endif
      // Update D3 state for each function
      D3_Flag = USB_20_D3_Flag;
      // Update clock settings
      Write_MSR(ClocksMsr, &Clocks.AsDword);

    } else {
      PMCR Previous;

      // PCI PM Specification (page 29; Table 7):
      // Writes of an unsupported state should discard the data
      Previous.AsWord = PreviousData;
      PMCR_Ptr->PowerState = Previous.PowerState;
	}
  } // end if (Delta.PowerState)

  // Record current value
  Pci->Value = PMCR_Ptr->AsDword;
}


#endif
