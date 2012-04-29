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
//*     Implements tables that determine the virtualized PCI topology
//*     and register definitions. 
//*****************************************************************************

#include "VSA2.H"
#include "VPCI.H"
#include "PCI.H"
#include "CHIPSET.H"
#include "CS5536.H"
#include "SYSMGR.H"
#include "MDD.H"
#include "MAPPER.H"

//***********************************************************************
//                  Notes on Virtualized PCI Header Tables
//
// 1) The PCI_HEADER_ENTRY structure must contain at a minimum:
//    - Vendor & Device IDs
//    - Command/Status
//    - RevisionID/ClassCode
//    - CacheLine/LatencyTimer/HeaderType/BIST
// 2) The Flag field of the last (and only last) entry must have EOL set.
// 3) The Mask field contains a 1 in each bit position that is R/W,
//    except for bits 11-15 of the Status register.  In this case, a set
//    bit means that this feature is reportable.
// 4) Memory BARs require a minimum of 4 KB alignment.
// 5) Registers up to and including BAR0 must be present and in ascending order
//***********************************************************************

// Write-to-Clear Status bits
#define WC_STATUS_BITS	((ULONG)(SIGNALED_TARGET_ABORT | RECEIVED_TARGET_ABORT | RECEIVED_MASTER_ABORT | \
							 SIGNALED_SYSTEM_ERROR | DETECTED_PARITY_ERROR ))


#define DEVSEL_TIMING        (DEVSEL_MEDIUM)
// Defaults
#define DEF_STATUS           (PCI_66MHZ_CAPABLE | DEVSEL_TIMING | BACK2BACK_CAPABLE)
#define DEF_MASK             (FAST_BACK_TO_BACK | PARITY_RESPONSE)

// Northbridge
#define NB_STATUS            (DEF_STATUS  | BUS_MASTER) & ~(BACK2BACK_CAPABLE)
#define NB_MASK              (DEF_MASK    | BUS_MASTER | IO_SPACE) & \
						    ~(SERR_ENABLE | PARITY_RESPONSE | SIGNALED_SYSTEM_ERROR | FAST_BACK_TO_BACK)

// Southbridge
#define SB_MASK              (DEF_MASK    | SPECIAL_CYCLES)
#define SB_STATUS            (DEF_STATUS)

// Graphics
#define GFX_STATUS           (DEF_STATUS) & ~(BACK2BACK_CAPABLE)
#define GFX_MASK             (DEF_MASK | BUS_MASTER) & \
                            ~(SERR_ENABLE | PARITY_RESPONSE | SIGNALED_SYSTEM_ERROR | FAST_BACK_TO_BACK)

// AES
#define AES_STATUS           (DEF_STATUS)
#define AES_MASK             (GFX_MASK)

// Bus Masters
#define BM_STATUS            (DEF_STATUS) 
#define BM_MASK              (DEF_MASK | BUS_MASTER)

// OHCI Controllers
#define OHCI_MASK            (BM_MASK | IO_SPACE | PARITY_RESPONSE)



//***********************************************************************
//                Virtualized Northbridge PCI Device
//***********************************************************************
PCI_HEADER_ENTRY HostBridge_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20801022,   0x00000000},
  {   COMMAND,    0x00,  NB_STATUS,      NB_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x06000000,   0x00000000},  // Bridge: Host
  {   0x0C,       0x00, 0x00800008,   0x0000F808},
  {   BAR0,       0x00, 0x00000000,   0x00000000},  // Virtual registers 
  {   BAR1,       0x00, 0x00000000,   0x00000000},  // CPU PM functionality
  {   0x58,        EOL, 0x00000000,   0xFFFFFFFF},  // Regression testing
};


PCI_HEADER_ENTRY Graphics_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20811022,   0x00000000},
  {   COMMAND,    0x00, GFX_STATUS,     GFX_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x03000000,   0x00000000},  // Display: VGA-compatible
  {   0x0C,       0x00, 0x00000008,   0x00000008},
  {   BAR0,       0x00, 0x00000000,   0x00000000},  // Graphics memory
  {   BAR1,       0x00, 0x00000000,   0x00000000},  // GP
  {   BAR2,       0x00, 0x00000000,   0x00000000},  // VG
  {   BAR3,       0x00, 0x00000000,   0x00000000},  // DF
  {   BAR4,       0x00, 0x00000000,   0x00000000},  // VIP  (LX only)
  {   0x3C,       0x00, 0x00000000,   0x00000000},  // LX only
  {   OEM_BAR0,   0x00, 0x00000000,   0x00000000},  // VG
  {   OEM_BAR1,   0x00, 0x00000000,   0x00000000},  // VG
  {   OEM_BAR2,   0x00, 0x00000000,   0x00000000},  // A0000-AFFFF or A0000-BFFFF if no MONO
  {   OEM_BAR3,    EOL, 0x00000000,   0x00000000},  // B8000-BFFFF only if MONO present
};

PCI_HEADER_ENTRY AES_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20821022,   0x00000000},
  {   COMMAND,    0x00, AES_STATUS,     AES_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x10100000,   0x00000000},  // Encryption: entertainment
  {   0x0C,       0x00, 0x00000008,   0x00000008},
  {   BAR0,       0x00, 0x00000000,   0x00000000},  // 
  {   0x3C,        EOL, 0x00000100,   0x000000FF},  // INTA
};



//***********************************************************************
//                Virtualized Southbridge PCI Device
//***********************************************************************

PCI_HEADER_ENTRY ISA_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20901022,   0x00000000},
  {   COMMAND,    0x00,  SB_STATUS,      SB_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x06010000,   0x00000000},  // Bridge: ISA
  {   0x0C,       0x00, 0x00800008,   0x0000F808},
  {   BAR0,       0x00, 0x00000000,   0x00000008, MSR_LBAR_SMB  },  //   8 byte I/O BAR	(SMB)
  {   BAR1,       0x00, 0x00000000,   0x00000100, MSR_LBAR_GPIO },  // 256 byte I/O BAR (GPIO)
  {   BAR2,       0x00, 0x00000000,   0x00000040, MSR_LBAR_MFGPT},  //  64 byte I/O BAR (MFGPT)
  {   BAR3,       0x00, 0x00000000,   0x00000020, MSR_LBAR_IRQ  },  //  32 byte I/O BAR (IRQ)
  {   BAR4,       0x00, 0x00000000,   0x00000080, MSR_LBAR_PMS  },  // 128 byte I/O BAR (PMS)
  {   BAR5,       0x00, 0x00000000,   0x00000040, MSR_LBAR_ACPI },  //  64 byte I/O BAR (ACPI)
  {   0xD0,        EOL, 0x00000000,   0x0000FFFF},  // Software SMI
};

PCI_HEADER_ENTRY Flash_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20911022,   0x00000000},
  {   COMMAND,    0x00, DEF_STATUS,     DEF_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x05010000,   0x00000000},  // Memory controller: Flash
  {   0x0C,       0x00, 0x00000008,   0x00000008},
  {   BAR0,       0x00, 0x00000000,   0x00000000, MSR_LBAR_FLSH0},  // Flash0
  {   BAR1,       0x00, 0x00000000,   0x00000000, MSR_LBAR_FLSH1},  // Flash1
  {   BAR2,       0x00, 0x00000000,   0x00000000, MSR_LBAR_FLSH2},  // Flash2
  {   BAR3,       0x00, 0x00000000,   0x00000000, MSR_LBAR_FLSH3},  // Flash3
  {   0x3C,       0x00, 0x00000100 | Y_IRQ_FLASH,     0x000000FF},  // INTA
  {   0x40,        EOL, 0x00000000,   0xFFFFFFFF},  // IDE-Flash switch
};



PCI_HEADER_ENTRY Audio_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20931022,   0x00000000},
  {   COMMAND,    0x00,  BM_STATUS,      BM_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x04010000,   0x00000000},  // Multimedia: Audio
  {   0x0C,       0x00, 0x00000008,   0x00000008},
  {   BAR0,       0x00, 0x00000000,   0x00000000},  // 128 byte I/O BAR
  {   0x3C,        EOL, 0x00000200 | Y_IRQ_AUDIO, 0x000000FF}	// INTB
};




#define USB20_INT           (0x00000400 | Y_IRQ_USB2)   // INTD
#if SUPPORT_CAPABILITIES  
  #define USB20_CMD         (DEVSEL_TIMING | PCI_66MHZ_CAPABLE | CAPABILITIES_LIST)
#else
  #define USB20_CMD         (DEVSEL_TIMING | PCI_66MHZ_CAPABLE)
#endif
#define USB20_MASK          (BM_MASK  & ~(SERR_ENABLE | PARITY_RESPONSE | SIGNALED_SYSTEM_ERROR | FAST_BACK_TO_BACK | BACK2BACK_CAPABLE))

#define OTG_CMD             USB20_CMD
#define OTG_MASK            (USB20_MASK & ~BUS_MASTER)



PCI_HEADER_ENTRY OHC_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20941022,   0x00000000},
  {   COMMAND,    0x00,  USB20_CMD,   USB20_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x0C031000,   0x00000000},  // Serial Bus: USB : OHCI
  {   0x0C,       0x00, 0x00000008,   0x00000008},
  {   BAR0,    USE_BMK, 0x00000000,   0x00001000, USBMSROHCB},
#if SUPPORT_CAPABILITIES  
  {   0x34,       0x00, PCI_PM_REG,   0x00000000},  // Capabilities pointer
  {  PCI_PM_REG,  0x00, 0xC8020001,   0x00000000},  // PCI Power Management
  {  PCI_PM_REG+4,PCI_PM, 0x00000000,   0x00008103},
#endif
  {   0x3C,        EOL,  USB20_INT,   0x000000FF},
};


PCI_HEADER_ENTRY EHC_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20951022,   0x00000000},
  {   COMMAND,PCI_EHCI,  USB20_CMD,   USB20_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x0C032000,   0x00000000},  // Serial Bus: USB : EHCI
  {   0x0C,       0x00, 0x00000008,   0x00000008},
  {   BAR0,   PCI_EHCI, 0x00000000,   0x00001000, USBMSREHCB},
#if SUPPORT_CAPABILITIES  
  {   0x34,       0x00, PCI_PM_REG,   0x00000000},  // Capabilities pointer
  {PCI_PM_REG,    0x00, 0xC8020001,   0x00000000},  // PCI Power Management
  {PCI_PM_REG+4,PCI_PM, 0x00000000,   0x00008103},
#endif
  {   EECP,   PCI_EHCI, 0x00000001,   0x01010000},  // USBLEGSUP     section 2.1.7 of EHCI spec
  {   EECP+4, PCI_EHCI, 0x00000000,   0x0000E03F,0,0,0xE0000000},  // USBLEGCTLSTS  section 2.1.8 of EHCI spec
  {  SRBN_REG,PCI_EHCI, 0x00000020,   0x00003F00},  // FLADJ/SBRN
  {   0x3C,        EOL,  USB20_INT,   0x000000FF},
};


PCI_HEADER_ENTRY UDC_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20961022,   0x00000000},
  {   COMMAND,    0x00,  USB20_CMD,   USB20_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x0C03FE00,   0x00000000},  // Serial Bus: USB : device
  {   0x0C,       0x00, 0x00000008,   0x00000008},
  {   BAR0,       0x00, 0x00000000,   0x00002000, USBMSRUDCB},
#if SUPPORT_CAPABILITIES  
  {   0x34,       0x00, PCI_PM_REG,   0x00000000},  // Capabilities pointer
  {PCI_PM_REG,    0x00, 0xC8020001,   0x00000000},  // PCI Power Management
  {PCI_PM_REG+4,PCI_PM, 0x00000000,   0x00008103},
#endif
  {   0x3C,        EOL,  USB20_INT,   0x000000FF},
};


PCI_HEADER_ENTRY OTG_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x20971022,   0x00000000},
  {   COMMAND,    0x00,    OTG_CMD,     OTG_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x0C038000,   0x00000000},  // Serial Bus: USB : No specific interface
  {   0x0C,       0x00, 0x00000008,   0x00000008},
  {   BAR0,       0x00, 0x00000000,   0x00001000, USBMSRUOCB},
#if SUPPORT_CAPABILITIES  
  {   0x34,       0x00, PCI_PM_REG,   0x00000000},  // Capabilities pointer
  {PCI_PM_REG,    0x00, 0xC8020001,   0x00000000},  // PCI Power Management
  {PCI_PM_REG+4,PCI_PM, 0x00000000,   0x00008103},
#endif
  {   0x3C,        EOL,  USB20_INT,   0x000000FF},
};


#define THOR_MASK   (BM_MASK | IO_SPACE)
PCI_HEADER_ENTRY Thor_Hdr[] = {
  //  Reg         Flag     Value         Mask
  {   0x00,       0x00, 0x209A1022,   0x00000000},
  {   COMMAND,    0x00,  BM_STATUS,    THOR_MASK,0,0,WC_STATUS_BITS},
  {   0x08,       0x00, 0x01018000,   0x00000000},  // Mass Storage: IDE
  {   0x0C,       0x00, 0x0000F808,   0x00000008},
  {   BAR4,       0x00, 0x00000000,   0x00000000, MSR_LBAR_ATA},  // 16 byte I/O BAR
// The following 4 registers must be contiguous:
  {   IDE_CFG,    0x00, 0x00000000,   0x0003FFFF, 0x10}, 
  {   IDE_DTC,    0x00, 0xA8A80000,   0xFFFF0000, 0x12}, 
  {   IDE_CAST,   0x00, 0xFF0000F0,   0xFF0000F0, 0x13}, 
  {   IDE_ETC,    0x00, 0x03030000,   0xC7C70000, 0x14},
  {   IDE_PM,      EOL, 0x00000000,   0x00000003, 0x15},
};


PCI_HEADER_ENTRY * Virtual_5536[] = {
   ISA_Hdr,          // F0  ISA bridge
   Flash_Hdr,        // F1  Flash
   Thor_Hdr,         // F2  ATA
   Audio_Hdr,        // F3  AC97
   OHC_Hdr,          // F4  OHCI
   EHC_Hdr,          // F5  EHCI
   UDC_Hdr,          // F6  UDC
   OTG_Hdr,          // F7  OTG
};


//***********************************************************************
// The following tables determine the virtualized PCI topology
//***********************************************************************
PCI_HEADER_ENTRY * NorthBridge[] = {
   HostBridge_Hdr,   // F0  Host bridge
   0,                // F1  Graphics (enabled via softvg)
   AES_Hdr,          // F2  Encryption
   0,                // F3
   0,                // F4
   0,                // F5
   0,                // F6
   0                 // F7
};


// Pointer to the virtualized Southbridge table
VIRTUAL_DEVICE * SouthBridge;



// NOTE: SouthBridge will be inserted into the following table 
//       at the same DEVSEL as the hardware header.      
VIRTUAL_DEVICE * Virtual_Devices[32] = {
                    // IDSEL Dev#    Address          Description
                    // ----- -----  ----------   ---------------------
   0,               //  N/A   0
   NorthBridge,     //  11    1     0x80000800   HostBridge + Graphics
   0,               //  12    2
   0,               //  13    3
   0,               //  14    4
   0,               //  15    5
   0,               //  16    6
   0,               //  17    7
   0,               //  18    8
   0,               //  19    9
   0,               //  20   10
   0,               //  21   11
   0,               //  22   12
   0,               //  23   13
   0,               //  24   14
   0,               //  25   15     0x80007800   CS5535 & CS5536
   0,               //  26   16
   0,               //  27   17
   0,               //  28   18     0x80009000   CS5530
   0,               //  29   19     0x80009800   CS5530 OHCI
   0,               //  30   20
   0,               //  31   21
   0,               //  N/A  22
   0,               //  N/A  23
   0,               //  N/A  24
   0,               //  N/A  25
   0,               //  N/A  26
   0,               //  N/A  27
   0,               //  N/A  28
   0,               //  N/A  29
   0,               //  N/A  30
   0,               //  N/A  31
};


