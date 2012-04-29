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
//*     Utility routines related to virtualized PCI config headers
//******************************************************************************



#include "VSA2.H"
#include "PCI.H"
#include "GX2.H"
#include "VPCI.H"
#include "SYSMGR.H"
#include "CHIPSET.H"
#include "PROTOS.H"
#include "DESCR.H"
#include "MDD.H"


// External function declarations:
extern void pascal Parse_Descriptor(UCHAR, ULONG *, ULONG *);
extern void pascal Trim_P2D_R(ULONG, ULONG, ULONG *);


// External variable declarations:
extern UCHAR DynamicVSALoad;
extern UCHAR MC_Port, VG_Port;
extern UCHAR MBIU1_SelfReference;
extern UCHAR End_of_POST;
extern ULONG MDD_Base;
extern ULONG Mbiu0, Mbiu1, Mbiu2;
extern ULONG MPCI_NB, MPCI_SB;
extern DESCRIPTOR MSRs[];
extern ULONG ExtendedMemoryDescr0, ExtendedMemoryDescr1;
extern Hardware HardwareInfo;
extern VIRTUAL_DEVICE   * Virtual_Devices[];
extern VIRTUAL_DEVICE   * SouthBridge;
extern PCI_HEADER_ENTRY * NorthBridge[];
extern PCI_HEADER_ENTRY * Virtual_5536[];
extern PCI_HEADER_ENTRY Graphics_Hdr[];
extern PCI_HEADER_ENTRY HostBridge_Hdr[];
extern PCI_HEADER_ENTRY AES_Hdr[];
extern PCI_HEADER_ENTRY ISA_Hdr[];
extern PCI_HEADER_ENTRY Thor_Hdr[];
extern PCI_HEADER_ENTRY Audio_Hdr[];
extern PCI_HEADER_ENTRY Flash_Hdr[];


// Local variable declarations:
PCI_HEADER_ENTRY * CommandPtr, * HdrPtr;
PCI_HEADER_ENTRY Dummy_Hdr[] = {
  {0x00, 0x00, 0xFFFFFFFF, 0x00000000},
  {0x00, 0x00, 0xFFFFFFFF, 0x00000000},
  {0x00, 0x00, 0xFFFFFFFF, 0x00000000},
  {0x00,  EOL, 0xFFFFFFFF, 0x00000000},
};
VIRTUAL_DEVICE * IDSELs;
VIRTUAL_PTR * VirtDevPtr;
USHORT DeviceID;
USHORT Class;
UCHAR BaseClass;
UCHAR Shift, AlignedReg, Function;
ULONG Virtualized_PCI_Devices=0;


//***********************************************************************
// Given a ptr to a virtual PCI header, finds Register
//***********************************************************************
PCI_HEADER_ENTRY * pascal Find_Register(PCI_HEADER_ENTRY * Pci, UCHAR Register)
{
  // Keep a ptr to the Vendor ID register
  HdrPtr = Pci;
  DeviceID = HdrPtr->Device_ID;

  // Keep a ptr to the Command register
  CommandPtr = Pci + COMMAND/4;

  // Record the device Class
  BaseClass = (HdrPtr+REVISION_ID/4)->BaseClass;
  Class = (HdrPtr+REVISION_ID/4)->Class;

  // Scan the table for the specified register entry
  do {
    if (Pci->Reg == Register) {
      return Pci;
    }
  } while (!((Pci++)->Flag & EOL));


  // If CommandPtr is used, avoid generating an exception
  CommandPtr = Dummy_Hdr;

  // Register is not in the table
  return (PCI_HEADER_ENTRY *) UNIMPLEMENTED_REGISTER;
}

//***********************************************************************
// Parses PCI_Address & returns a pointer to the corresponding entry in a
// virtual header table.
// Computes global variables:
//   AlignedReg, Function, Shift, HdrPtr, CommandPtr, & DeviceID
//***********************************************************************
PCI_HEADER_ENTRY * pascal Get_Structure(USHORT PCI_Address)
{ register PCI_HEADER_ENTRY * Pci;
  UCHAR Reg;

  // Compute Function #
  Function = (UCHAR)(PCI_Address >> 8) & 0x07;

  // Get register offset
  Reg = (UCHAR) PCI_Address;

  // Compute DWORD aligned register offset
  AlignedReg = Reg & ~3;

  // Compute shift count
  Shift = (Reg & 3) << 3;

  // Subsystem Vendor ID & Subsystem ID are same as Vendor ID & Device ID
  if (AlignedReg == SUBSYSTEM_VENDOR_ID) {
    AlignedReg = VENDOR_ID;
  }


  // Return value if function not implemented
  Pci = (PCI_HEADER_ENTRY *) UNIMPLEMENTED_FUNCTION;

  // Is this IDSEL virtualized ?
  if (IDSELs = *(VirtDevPtr+(PCI_Address >> 11))) {

    // Get ptr to this function (0000 if not implemented)
    if (Pci = IDSELs[Function]) {
      Pci = Find_Register(Pci, AlignedReg);
    }
  }

  return Pci;
}





//***********************************************************************
// Trims the MSRs that map extended memory by RangeRequest bytes
//***********************************************************************
ULONG Trim_Extended_Memory(ULONG RangeRequest)
{ ULONG Fields[3], Ext_Mem[2], Msr;

  // Trim the MPCI Region 1 configuration
  Msr = MPCI_NB + MPCI_R1;
  Read_MSR(Msr, Ext_Mem);
  if (DynamicVSALoad) {
    RangeRequest = 0;
  }
  Ext_Mem[1] -= RangeRequest;
  Write_MSR(Msr, Ext_Mem);

  // Adjust RCONF_DEFAULT[SYSTOP]
  Read_MSR(MSR_RCONF_DEFAULT, Ext_Mem);
  Ext_Mem[0] -= RangeRequest >> 4;
  Write_MSR(MSR_RCONF_DEFAULT, Ext_Mem);


  // Trim extended memory in GLIUs 0 & 1
  Trim_P2D_R(ExtendedMemoryDescr0, RangeRequest, Ext_Mem);
  Trim_P2D_R(ExtendedMemoryDescr1, RangeRequest, Ext_Mem);

  // Copy Start/End to new descriptor
  Parse_Descriptor(P2D_R, Ext_Mem, Fields);
  return (Fields[1] + 1);
}


typedef struct {
  UCHAR StartType;
  UCHAR EndType;
  UCHAR Port;
  ULONG Msr;
} ROUTING_INFO;
#define MAX_ROUTE   5
ROUTING_INFO Routing[MAX_ROUTE];


//***********************************************************************
// Allocates a Region Configuration Register
//***********************************************************************
ROUTING_INFO * pascal Allocate_RCONF(ULONG DevAddress, ROUTING_INFO * RoutingPtr)
{

  // Allocate a region configuration register
  // If in Southbridge...
  if ((DevAddress & MPCI_SB) == MPCI_SB) {
    // then use SB MPCI R0-R15
    RoutingPtr->StartType = MPCI_RCONF;
    RoutingPtr->Msr = MPCI_SB;
  } else {
    // else use GX2 RCONF0-RCONF7
    RoutingPtr->StartType = GX2_RCONF;
  }
  RoutingPtr++;
  return RoutingPtr;
}

//***********************************************************************
//   This routines creates an association between the virtualized PCI BAR
// (specified by BaseAddr and Device_ID) and an MBus device (specified by
// Geode_ID).  The Resource parameter specifies the type  of BAR (Memory,
// Memory-mapped I/O, or I/O) and the size of the region by RangeRequest.
//***********************************************************************
USHORT pascal Allocate_BAR(UCHAR Resource, USHORT BaseAddr, ULONG RangeRequest, \
                           USHORT Geode_ID, USHORT Device_ID)
{ ULONG DevAddress, Mbiu, Physical=0;
  UCHAR LSB, MSB;
  UCHAR Instance=1;
  UCHAR Index, StartType, EndType;
  UCHAR * LinkPtr;
  USHORT DevNum, Function, PCI_Address;
  register ROUTING_INFO * RoutingPtr;
  register PCI_HEADER_ENTRY * Pci;
  register VIRTUAL_DEVICE * VirtDev;
  register DESCRIPTOR * Descr;
  int i;

  // Validate parameters
  switch (Resource) {
    case RESOURCE_MEMORY:
    case RESOURCE_MMIO:
    case RESOURCE_SCIO:
    case RESOURCE_IO:
      break;
    default:
      Log_Error("Invalid value for parameter Resource: 0x%02X", Resource);
	  return 0x0000;
  }

  // Ensure RangeRequest meets PCI & MBus requirements
  LSB = BitScanForward(RangeRequest);
  MSB = BitScanReverse(RangeRequest);

  if (Resource == RESOURCE_MEMORY || Resource == RESOURCE_MMIO) {
    // Memory BARs must be at least 4 KB because of granularity of descriptors
    if (RangeRequest < 4096) {
      MSB = LSB = 12;
    }
  }
  // If RangeRequest is not a power of 2...
  if (LSB != MSB) {
    // Round size of BAR to next higher power of 2
    MSB++;
  }


  // Do some massaging based on Device ID
  switch (Device_ID) {

    // Graphics header is invisible until SoftVG is enabled.
    case DEVICE_ID_GFX2:
    case DEVICE_ID_GFX3:
      NorthBridge[1] = Graphics_Hdr;
      break;
  }

  //
  // Determine the PCI Address by scanning the PCI tables for Device_ID
  //
  for (DevNum = 1; DevNum <= 21; DevNum++) {
    if (VirtDev = *(VirtDevPtr+DevNum)) {
      // For each defined function...
      for (Function = 0; Function <= 7; Function++) {
        if (Pci = *VirtDev++) {
          // Do the Device IDs match ?
          if (Pci->Device_ID == Device_ID) {
            // Handle the 2nd instance of 5536's OHCI
            if ((Geode_ID == ID_OHCI) && ((Pci+BAR0/4)->Flag & (IO_BAR | MEM_BAR | MMIO_BAR))) {
              continue;
            }
            break;
          }
        }
      }
      // If Device ID match is found...
      if (Function < 8) {
        // Compute the PCI address
        PCI_Address = (DevNum << 11) + (Function << 8) + (UCHAR)BaseAddr;

        Pci = Find_Register(Pci, (UCHAR)BaseAddr);

        // Check for errors
        if ((USHORT)Pci > UNIMPLEMENTED_REGISTER) {
          // Has this BAR already been allocated ?
          if (Pci->Flag & (IO_BAR | MEM_BAR | MMIO_BAR)) {
            // Yes, log an error
            Log_Error("Resource has already been allocated for PCI address 0x%X", PCI_Address);
          }
        }
        break;
      }
    }
  }


  // Device_ID was not found in virtual PCI tables
  if (DevNum > 21) {
    Log_Error("DeviceID 0x%04X was not found", Device_ID);
    return 0x0000;
  }

  // Record Mask for BAR
  Pci->Mask = ~((1L << MSB)-1);


  // Initialize routing structure
  RoutingPtr = &Routing[0];
  for (i=0; i<MAX_ROUTE; i++) {
    RoutingPtr->StartType = 0x00;
    RoutingPtr->EndType   = 0x00;
    RoutingPtr->Port = 0x00;
    RoutingPtr->Msr  = 0x00000000;
    RoutingPtr++;
  }
  RoutingPtr = &Routing[0];


  // Handle special cases
  switch (Geode_ID) {

    // SMI generation on access (no physical device, e.g. virtual registers)
    case 0x0000:
      DevAddress = 0x00000000;
      break;

    // Routing address of some devices has already been determined
    case ID_MDD:
      DevAddress = MDD_Base;
      break;

    // Handle the 2nd instance of 5536's OHCI
    case ID_OHCI:
      if (Function == 4) {
        Instance = 2;
      }

    default:
      // Search for the requested Geode_ID
      DevAddress = Find_MBus_ID(Geode_ID, Instance);
      if (!DevAddress) {
        Log_Error("Geode ID 0x%04X was not found for Device ID 0x%04X", Geode_ID, Device_ID);
        return 0x0000;
      }
      break;

  }

  // If an LBAR is required, allocate it
  if (Pci->LBar) {
    RoutingPtr->StartType = MDD_LBAR;
    RoutingPtr->Msr = MDD_Base;
    switch (Geode_ID) {
      case ID_USB_20:
        // OHCI needs an LBAR in the MDD
        if ((HdrPtr+REVISION_ID/4)->Interface == 0x10) {
          (UCHAR)RoutingPtr->Msr = MSR_LBAR_KEL1;
          RoutingPtr++;
        }
        RoutingPtr->StartType = USB_LBAR;

      case ID_ATA:
        RoutingPtr->Msr = DevAddress;
        break;
    }
    (UCHAR)RoutingPtr->Msr = Pci->LBar;
    RoutingPtr++;
  }

  // Get an available descriptor appropriate to the type of BAR
  switch (Resource) {

    // Physical Memory
    case RESOURCE_MEMORY:

      // Mark it as a memory BAR
      Pci->Flag |= MEM_BAR;

      RoutingPtr->StartType = P2D_BMO;
      RoutingPtr->EndType = P2D_RO;
      RoutingPtr->Msr = Mbiu0;

      // Legacy VGA frame buffers don't require physical memory
      if ((UCHAR)BaseAddr > BAR5) {
        ULONG RegionEnables, RegionProperties[2];

#define OFF_PROPS  (REGION_WS)	// Frame buffer properties to be disabled
#define OFF_PROPERTIES ((OFF_PROPS<<24) | (OFF_PROPS<<16) | (OFF_PROPS<<8) | (OFF_PROPS))
#define ON_PROPS   (0)	        // Frame buffer properties to be enabled
#define ON_PROPERTIES  (( ON_PROPS<<24) | ( ON_PROPS<<16) | ( ON_PROPS<<8) | ( ON_PROPS))

        // Set MPCI Fixed Region Enables and disable write-serialize
        Read_MSR(MSR_RCONF_A0_BF, RegionProperties);

        Physical = 0xA0000;
        RegionEnables = 0x00;

        switch (RangeRequest) {

          case 128L*1024:
            RegionEnables = 0xFF;	// A0000-BFFFF
            RegionProperties[1] &= ~OFF_PROPERTIES;
            RegionProperties[1] |=   ON_PROPERTIES;

          case  64L*1024:
            RegionEnables |= 0x0F;	// A0000-AFFFF
            RegionProperties[0] &= ~OFF_PROPERTIES;
            RegionProperties[0] |=   ON_PROPERTIES;
            break;

          case  32L*1024:
            RegionEnables = 0xC0;	// B8000-BFFFF
            RegionProperties[1] &= ~(OFF_PROPERTIES & 0xFFFF0000);
            RegionProperties[1] |=  (ON_PROPERTIES  & 0xFFFF0000);
            Physical = 0xB8000;
            break;

        }

        // Set Vail Region Properties (0x180B)
        Write_MSR(MSR_RCONF_A0_BF, RegionProperties);

        // Set MPCI Fixed Region Enables (0x2014)
        Write_MSR_LO(MPCI_NB + MPCI_REN, Read_MSR_LO(MPCI_NB + MPCI_REN) | RegionEnables);

        // Allocate P2D_BM
        switch (Device_ID) {
          case DEVICE_ID_GFX2:
          case DEVICE_ID_GFX3:
            RoutingPtr->StartType = P2D_BM;
            break;
        }
        RoutingPtr++;

      } else {

        // Trim required memory from the end of extended memory
        Physical = Trim_Extended_Memory(RangeRequest);
        DevAddress = Find_MBus_ID(ID_MC, 1);

        // Allocate a Region Configuration Register
        RoutingPtr++;
        RoutingPtr = Allocate_RCONF(DevAddress, RoutingPtr);
      }
      break;


    // Memory-mapped I/O
    case RESOURCE_MMIO:
      // Mark it as a memory-mapped BAR
      Pci->Flag |= MMIO_BAR;

      switch (Geode_ID) {

        // Subtractive ports; no MBIU descriptor is necessary
        case ID_MPCI:
        case ID_MDD:
          break;

        case ID_USB_20:
        case ID_OHCI:
          // Allocate a P2D_BMK descriptor in MBIU2
          RoutingPtr->StartType = P2D_BMK;
          if (!(Pci->Flag & USE_BMK)) {
            RoutingPtr->EndType = P2D_BM;
          }
          RoutingPtr->Msr = Mbiu2;
          RoutingPtr->Port = (UCHAR)DevAddress;
          RoutingPtr++;

          if (Pci->Flag & EPCI_RW) {
            // Allocate the appropriate mailbox address
            RoutingPtr->StartType = EPCI;
            RoutingPtr->Msr = DevAddress & ROUTING;
            RoutingPtr++;
          }
          break;

        case ID_VG:
          RoutingPtr->StartType = P2D_RO;
          RoutingPtr++;
          // Fix for VG alias bug: POFFSET field should be set to -PBASE
          // See Compute_Msr_Value()
          Physical = 1;
          break;

        case ID_AES:
          RoutingPtr->StartType = P2D_R;
          RoutingPtr++;
          break;

        default:
          RoutingPtr->StartType = P2D_BM;
          RoutingPtr->EndType = P2D_RO;
          RoutingPtr++;
          break;
      }

      // Allocate a Region Configuration Register
      RoutingPtr = Allocate_RCONF(DevAddress, RoutingPtr);
      break;


    // I/O Space
    case RESOURCE_SCIO:
      RoutingPtr->StartType = IOD_SC;
      RoutingPtr++;

    case RESOURCE_IO:
      // Mark it as an I/O BAR
      Pci->Flag |= IO_BAR;

      switch (Geode_ID) {
        // Subtractive ports; no MBIU descriptor is necessary
        case ID_MPCI:
        case ID_MDD:
          break;

        default:
          if (Resource != RESOURCE_SCIO) {
            RoutingPtr->StartType = IOD_BM;
            RoutingPtr->EndType = IOD_SC;
            RoutingPtr++;
          }
          break;
      }

      // If device is in the Southbridge...
      if ((DevAddress & MPCI_SB) == MPCI_SB) {
        // allocate a Region Configuration Register
        RoutingPtr = Allocate_RCONF(DevAddress, RoutingPtr);
      }

      // PCI Spec requires I/O BAR a minimum of 4 bytes
      Pci->Mask &= 0xFFFFFFFC;
      break;

    default:
      Log_Error("Unknown resource requested: 0x%X", Resource);
      return 0x0000;
  }




  //
  // Get device's PCI Revision ID
  //
  switch (Device_ID) {

    // Revision of some devices is already determined
    case DEVICE_ID_OHCI:
    case DEVICE_ID_5536:
    case DEVICE_ID_GX2:
    case DEVICE_ID_LX:
      break;

    // Graphic's Revision ID comes from GP, not VG or DF
    case DEVICE_ID_GFX2:
    case DEVICE_ID_GFX3:
      if (Geode_ID != ID_GP) {
        break;
      }

    default:
      // Read MBD_MSR_CAP
      Mbiu = DevAddress;
      (USHORT)Mbiu = MBD_MSR_CAP;

      // Insert Revision ID into header
      (HdrPtr+REVISION_ID/4)->Revision_ID = (UCHAR)Read_MSR_LO(Mbiu);
      break;
  }

  // Compute number of MSRs needed to support this BAR
  i = (UCHAR)(RoutingPtr - &Routing[0]);

  if (i >= MAX_ROUTE) {
    Log_Error("Allocate_BAR- routing failed on PCI address 0x%X", PCI_Address);
  }

  //
  // Allocate the required MSRs
  //
  RoutingPtr = &Routing[0];
  LinkPtr = &Pci->Link;

  while (i) {

    // Force use of P2D_RO to prevent ROVER from complaining about overlapped
    // physical addresses when the frame buffer length is not a power of 2.
    if (Pci == &Graphics_Hdr[BAR0/4]) {
      if(RoutingPtr->Msr == Mbiu0) {
        RoutingPtr->StartType = P2D_RO;
      }
    }
    StartType = RoutingPtr->StartType;
    EndType = RoutingPtr->EndType;

    // If 2nd choice is not specified, force it to 1st choice
    if (EndType == 0x00) {
      EndType = StartType;
    }
    // Has the correct MBIU been determined ?
    if (RoutingPtr->Msr == 0x00000000) {
      // Determine the MBIU as follows:
      // - If the 2nd routing field == 0, then it's MBIU0
      // - If the 1st three routing fields match MBIU2, then it's MBIU2
      // - Otherwise it's MBIU1
      if ((DevAddress & (7L << 26)) == 0) {
        Mbiu = Mbiu0;
      } else {
        if ((DevAddress & 0xFF800000) == (Mbiu2 & 0xFF800000)) {
          Mbiu = Mbiu2;
        } else {
          Mbiu = Mbiu1;
        }
      }
      RoutingPtr->Msr = Mbiu;
    }


    if (StartType) {

      Index = Allocate_Descriptor(StartType, EndType, RoutingPtr->Msr);

      if (Index == DESCRIPTOR_NOT_FOUND) {

        // If SMI generation, then any MBIU may be used
        if (!Geode_ID) {
          // Try MBIU1
          Mbiu = Mbiu1;
          Index = Allocate_Descriptor(StartType, EndType, Mbiu);
          if (Index == DESCRIPTOR_NOT_FOUND) {
            if (Mbiu = Mbiu2) {
              Index = Allocate_Descriptor(StartType, EndType, Mbiu);
            }
          }
        }
      }
    }

    if (Index == DESCRIPTOR_NOT_FOUND) {
      UCHAR Mbiu=9;

      if (RoutingPtr->Msr == Mbiu0) Mbiu = 0;
      else if (RoutingPtr->Msr == Mbiu1) Mbiu = 1;
      else if (RoutingPtr->Msr == Mbiu2) Mbiu = 2;
      Log_Error("MSR allocation failed for GeodeID=%02X/DeviceID=%04X/BAR%x/MBIU%1x", \
                 (UCHAR)Geode_ID, Device_ID, (USHORT)((BaseAddr-BAR0)/4), Mbiu);
      goto NextMSR;
    }

    // Record info about the descriptor
    Descr = &MSRs[Index];
    Descr->Owner = PCI_Address;
    Descr->Range = RangeRequest;
    Descr->Physical = Physical;


    if (RoutingPtr->Port == 0) {
      RoutingPtr->Port = (UCHAR)DevAddress;
    }
    Descr->Port = RoutingPtr->Port;

    // If embedded PCI device, record PCI mailbox MSR to Command register
    if (StartType == EPCI) {
      CommandPtr->Link = Index;
    }

    // Link to previous MSRs[] (or Pci->Link)
    * LinkPtr = Index;

    // Update link pointer
    LinkPtr = &Descr->Link;


    // Is an additional swiss-cheese descriptor necessary ?
    if (Descr->Type == IOD_SC && RangeRequest > 8) {
      RangeRequest -= 8;
      continue;
    }

    // If the device is on GLIU0, then allocate a descriptor on GLIU1
    // to route FS2 transactions to GLIU0.
    if (RoutingPtr->Msr == Mbiu0 && RoutingPtr->StartType < GX2_RCONF ) {
      // Don't allocate MBUI1 descriptor for virtual register BAR
      if (Geode_ID) {
        switch (RoutingPtr->StartType) {

          case IOD_SC:
            RoutingPtr->StartType = IOD_BM;
            RoutingPtr->EndType = IOD_SC;
            RangeRequest = 1L << MSB;
            break;

          case P2D_RO:
            if (Pci == &Graphics_Hdr[BAR0/4]) {
              RoutingPtr->StartType = P2D_R;
              break;
            } else {
              RoutingPtr->StartType = P2D_BM;
            }

          default:
            RangeRequest = 1L << MSB;
            break;
        }

        // Use only 1 descriptor for northbound VG transactions
        if (RangeRequest == 16 && Geode_ID == ID_VG) {
          static UCHAR VG_FS2_Flag=0;

          if (VG_FS2_Flag) {
            break;
          } else {
            VG_FS2_Flag = 1;
            RangeRequest = 32;
          }
        }

        RoutingPtr->Msr = Mbiu1;
        RoutingPtr->Port = MBIU1_SelfReference;
        continue;
      }
    }

    // Advance ptr to next MSR
NextMSR:
    RoutingPtr++;
    i--;
  }	 // end while



  // Make appropriate Command register bit R/W
  if (Pci->Flag & IO_BAR) {
    CommandPtr->Mask |= IO_SPACE;
  } else {
    CommandPtr->Mask |= MEM_SPACE;
  }

  return PCI_Address;

}





//***********************************************************************
//
// Perform early POST initialization of virtualized PCI config space
//
//***********************************************************************
void VirtualPCI_EarlyInit(void)
{ USHORT i;
  ULONG PCI_Address;
  ULONG MsrAddr;
  VIRTUAL_PTR DevicePtr;
  PCI_HEADER_ENTRY * Pci;


  // Initialize ptr to virtualized PCI topology
  VirtDevPtr = &Virtual_Devices[0];


  // Virtualize CS5536's configuration space, if present,
  // at the same device number as the h/w header.
  i = (UCHAR)((USHORT)HardwareInfo.Chipset_Base >> 11);
  switch (HardwareInfo.Chipset_ID) {

    case DEVICE_ID_5536:
      Virtual_Devices[i] = Virtual_5536;
      break;

    default:
      return;
  }

  SouthBridge = Virtual_Devices[i];

  // Compute mask of IDSELs that are to be virtualized
  for (i = 1; i <= 21; i++) {
    // If IDSEL is virtualized, set enable mask
    DevicePtr = * (VirtDevPtr+i);
    if (DevicePtr) {

      // Record IDSEL to trap
      Virtualized_PCI_Devices |= (1L << i);

      // Register System Manager for this IDSEL
      PCI_Address = ((ULONG)i << 11);
      Register_Event(EVENT_PCI_TRAP, MAX_PRIORITY, SysMgr_VSM, PCI_Address, 0x07FF);
    }
  }

  // Enable virtual PCI headers
  MsrAddr = MPCI_NB + MBD_MSR_SMI;
  Write_MSR_LO(MsrAddr, Read_MSR_LO(MsrAddr) & ~VPHM);


  // Store CPU's Device & Revision IDs in Host Bridge's PCI header
  HostBridge_Hdr[0].Device_ID = HardwareInfo.CPU_ID;
  HostBridge_Hdr[2].Revision_ID = (UCHAR)HardwareInfo.CPU_Revision;


  // AES header (F2) BAR
  Allocate_BAR(RESOURCE_MMIO, BAR0, 16*1024, ID_AES, AES_Hdr[0].Device_ID);

  // Implement Interrupt Pin/Line registers in LX graphic header
  if (Pci = Find_Register(Graphics_Hdr, INTERRUPT_LINE)) {
    // Make Interrupt Line read-write
    (UCHAR)Pci->Mask = 0xFF;
    // Set Interrupt Pin to INTA#
    Pci->Interrupt_Pin = 0x01;
  }
}


//***********************************************************************
// Routine to support INFO getting PCI<->MSR linkages
//***********************************************************************
ULONG pascal Get_MSR_Linkage(USHORT PCI_Address)
{ PCI_HEADER_ENTRY * Pci;
  ULONG MsrAddr;
  static UCHAR Link=0x00;
  static DESCRIPTOR * Descr;
  static USHORT LastPCI = 0x0000;

  if (PCI_Address != LastPCI) {
    LastPCI = PCI_Address;
    Pci = Get_Structure(PCI_Address);

    switch ((USHORT)Pci) {

      case UNIMPLEMENTED_FUNCTION:
      case UNIMPLEMENTED_REGISTER:
        Link = 0x00;
        break;

      default:
        Link = Pci->Link;
        break;
    }
  }

  Descr = &MSRs[Link];

  // Get link to next descriptor
  Link = Descr->Link;

  if (!(MsrAddr = Descr->MsrAddr)) {
    LastPCI = 0x0000;
  }

  return MsrAddr;
}




