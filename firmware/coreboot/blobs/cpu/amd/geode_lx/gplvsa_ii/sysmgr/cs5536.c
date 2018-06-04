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
//*    Routines related to the CS5536   
//*****************************************************************************

#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"
#include "GX2.H"
#include "VPCI.H"
#include "DESCR.H"
#include "PCI.H"
#include "CHIPSET.H"
#include "CS5536.H"
#include "MDD.H"

ULONG Mbiu2   = MPCI_SOUTH + 0x00020000;  // 2.4.2.0.1
ULONG MPCI_SB = MPCI_SOUTH;               // 2.4.2.0.0
ULONG MCP_SB  = MPCI_SOUTH + 0x00700000;  // 2.4.2.7.0
ULONG OHCI1_Smi=0, OHCI2_Smi=0;
CAPABILITIES Southbridge_MBIU;
ULONG ATA_Error;

// External Functions:
extern void CS5536_GPIO_Init(void);
extern void Init_MDD(void);
extern ULONG Write_EPCI(UCHAR, ULONG);
extern void ACPI_PMS_SWAPSiF(void);
extern PCI_HEADER_ENTRY * Get_Structure(USHORT);
extern void pascal Handle_EHCI_Wr(PCI_HEADER_ENTRY *);
extern PCI_HEADER_ENTRY * pascal Find_Register(PCI_HEADER_ENTRY *, UCHAR);

// External Variables:
extern ULONG MPCI_NB, MDD_Base;
extern Hardware HardwareInfo;
extern PCI_HEADER_ENTRY ISA_Hdr[];
extern PCI_HEADER_ENTRY Audio_Hdr[];
extern VIRTUAL_DEVICE * SouthBridge;
extern PCI_HEADER_ENTRY * Virtual_5536[];

//***********************************************************************
// Enables USB SMIs on CS5536
//***********************************************************************
void pascal Enable_USB_5536(UCHAR EnableFlag, USHORT Instance)
{ ULONG MsrData;
  UCHAR Mask;

  // OHCI is the only valid setting for now
  if (Instance == 1) {

    MsrData = Read_MSR_LO(OHCI1_Smi);
    Mask = 0x10 << (Instance-1);
    if (EnableFlag) {
      (UCHAR)MsrData &= ~Mask;
    } else {
      (UCHAR)MsrData |=  Mask;
    }
    Write_MSR_LO(OHCI1_Smi, MsrData);

  } else {
    static UCHAR * Msg_Enable  = "enable";
    static UCHAR * Msg_Disable = "disable";
    UCHAR * OnOff;

    if (EnableFlag) {
      OnOff = Msg_Enable;
    } else {
      OnOff = Msg_Disable;
    }
    Log_Error("Attempt to %s invalid USB instance: 0x%04X", OnOff, Instance);
  }
}


//***********************************************************************
// Enables/disables writes to PM1_CNT register
//***********************************************************************
void Enable_ACPI_5536(UCHAR EnableFlag)
{ ULONG MsrAddr, MsrData;

  // Get address to MDDs SMI MSR
  MsrAddr = MDD_Base;
  (USHORT)MsrAddr = MBD_MSR_SMI;

  // Get current value
  MsrData = Read_MSR_LO(MsrAddr);

  // Set/clear enable for trapping of writes to PM1_CNT
  if (EnableFlag) {
    MsrData |=  PM1_CNT_SSMI_EN;

  } else {
    MsrData &= ~PM1_CNT_SSMI_EN;
  }

  // Update the MSR
  Write_MSR_LO(MsrAddr, MsrData);

}


//***********************************************************************
// Sets positive/subtractive decode
//***********************************************************************
void pascal Address_Decode_5536(UCHAR Decode, USHORT Address)
{
  // Not yet implemented
}

//***********************************************************************
// Initializes BARs in the Southbridge virtual headers
//***********************************************************************
void Init_SB_Headers(void)
{ ULONG MsrAddr, BAR_Value;
  USHORT PCI_Addr, Bar;


  //*********************************************
  // Allocate BARs for CS5536's F0 header
  //*********************************************
  MsrAddr = MDD_Base;
  for (Bar = BAR0; Bar <= BAR5; Bar += 4) {
    ULONG BAR_Length;

    (UCHAR)MsrAddr = ISA_Hdr[Bar/4].LBar;

    // Get current LBAR value
    BAR_Value = Read_MSR_LO(MsrAddr);

    // Determine the length of the I/O range
    BAR_Length = ISA_Hdr[Bar/4].Mask;

    // Allocate a PCI BAR corresponding to the LBAR
    PCI_Addr = Allocate_BAR(RESOURCE_IO, Bar, BAR_Length, ID_MDD, ISA_Hdr[0].Device_ID);

    // Synchronize with any LBARs that have already been initialized by the BIOS
    Virtual_PCI_Write_Handler(PCI_Addr, DWORD_IO, BAR_Value);

  }

  // Initialize Southbridge's COMMAND register
  (UCHAR)PCI_Addr = COMMAND;
  Virtual_PCI_Write_Handler(PCI_Addr, BYTE_IO, SPECIAL_CYCLES | IO_SPACE);


  //*********************************************
  // Allocate Audio BAR
  //*********************************************
  Allocate_BAR(RESOURCE_IO, BAR0,  128,  ID_AC97, Audio_Hdr[0].Device_ID);
}


//***********************************************************************
// Initializes the CS5536 chipset
//***********************************************************************
void Init_CS5536(void)
{ register PCI_HEADER_ENTRY * Header;
  ULONG MsrAddr, USB20_Msr, MsrData[2];
  UCHAR i, CS5536_DevNum;



  //*********************************************
  // Get routing address of CS5536's MPCI
  //*********************************************
  MPCI_SB = MPCI_NB;
  CS5536_DevNum = (UCHAR)((USHORT)HardwareInfo.Chipset_Base >> 11);
  MsrData[0] = Read_MSR_LO(MPCI_NB + MPCI_ExtMSR);
  while (MsrData[0]) {
    MPCI_SB += 1L << 23;
    if ((UCHAR)MsrData[0] == CS5536_DevNum) {
      break;
    }
    MsrData[0] >>= 8;
  }

  //*********************************************
  // Get routing address to Southbridge's GLIU
  //*********************************************
  Mbiu2 = MPCI_SB + 0x00020000;


  // Read Southbridge's GLIU capabilities
  Read_MSR(Mbiu2 + MBIU_CAP, &MsrData[0]);


  //*********************************************
  // Initialize GLIU2
  //*********************************************
  Parse_Capabilities(MsrData, &Southbridge_MBIU);
  // The NP2D_SCO field is actually the NP2D_BMK field
  Southbridge_MBIU.NP2D_BMK = Southbridge_MBIU.NP2D_SCO;
  Southbridge_MBIU.NP2D_SCO = 0;

  // SWAPSiF for erroneous P2D_BM/BMK numbering
  if (HardwareInfo.Chipset_ID == DEVICE_ID_5536) {
    Southbridge_MBIU.NP2D_BM = 3;
  }

  // Find address of MCP in Southbridge
  MCP_SB = Find_MBus_ID(ID_MCP, 2) & 0xFFFF0000;

  // Default range for IDE P2D_BM descriptor is 16 bytes.
  // Change range to 8 bytes.
  MsrAddr = Mbiu2 + MSR_IO_DESCR;
  Write_MSR_LO(MsrAddr, Read_MSR_LO(MsrAddr) | 0x00000008);
  	
  // Initialize the descriptor structure for Southbridge's GLIU
  Init_MBIU((UCHAR *)&Southbridge_MBIU, Mbiu2);

  // Get Southbridge's Revision ID from MCP
  ISA_Hdr[REVISION_ID/4].Revision_ID = (UCHAR)Read_MSR_LO(MCP_SB + 0x0017);


  //*********************************************
  // Initialize the MDD
  //*********************************************
  Init_MDD();

  //*********************************************
  // Add MPCI R0-R15 to available descriptor list
  //*********************************************
  MsrAddr = MPCI_SB;
  // REGION_R15 is reserved for port 84h 
  for ((USHORT)MsrAddr = REGION_R0; (UCHAR)MsrAddr <= REGION_R15-1; MsrAddr++) {
    if (Init_Descr(MPCI_RCONF, MsrAddr)) {
      break;
	}
  }

  // Enable SSMI Received event
  (USHORT)MsrAddr = MBD_MSR_SMI;
   Write_MSR_LO(MsrAddr, Read_MSR_LO(MsrAddr) | SSMM);

  // Add ATA LBAR to available descriptor list
  // NOTE: Not really part of MDD (just leveraging MDD logic)
  MsrAddr = Find_MBus_ID(ID_ATA, 1);
  ATA_Error = MsrAddr;
  (USHORT)ATA_Error = MBD_MSR_ERROR;

#if 1
  // Enable SSMI Received event
  (USHORT)MsrAddr = MBD_MSR_SMI;
  Write_MSR_LO(MsrAddr, Read_MSR_LO(MsrAddr) | SSMM);
#endif

  (USHORT)MsrAddr = MSR_LBAR_ATA;
  Init_Descr(MDD_LBAR, MsrAddr);


  //*********************************************
  // Initialize the virtual Southbridge headers
  //*********************************************
  Init_SB_Headers();

  //*********************************************
  // Initialize the GPIO logic
  //*********************************************
  CS5536_GPIO_Init();

  //*********************************************
  // Install workaround for ACPI & PMS bugs
  //*********************************************
  ACPI_PMS_SWAPSiF();


  // Find address of USB 2.0
  OHCI1_Smi = USB20_Msr = Find_MBus_ID(ID_USB_20, 1);

  if (OHCI1_Smi == 0) {
    return;
  }
  (USHORT)OHCI1_Smi = MBD_MSR_SMI;

  // Set PMEEN for each USB 2.0 controller
  for (i=USBMSROHCB; i<=USBMSRUOCB; i++) {
    (UCHAR)USB20_Msr = i;
    Read_MSR(USB20_Msr, MsrData);
    MsrData[1] |= PMEEN;
    Write_MSR(USB20_Msr, MsrData);
  }
  


  // Initialize the USB 2.0 devices
  for (i=0; i <= 7; i++) {

    // If the header exists...
    if (Header = Virtual_5536[i]) {
      // Read Class Code to see it is USB class
      if ((Header+REVISION_ID/4)->Class == 0x0C03) {
        // Initialize USB LBAR
        (UCHAR)USB20_Msr = (Header+BAR0/4)->LBar;
        Init_Descr(USB_LBAR, USB20_Msr);

        // Allocate the PCI BAR
        Allocate_BAR(RESOURCE_MMIO, BAR0, (Header+BAR0/4)->Mask, ID_USB_20, Header->Device_ID);

        // If EHCI, initialize FLADJ
        if (Header->Device_ID == DEVICE_ID_AMD_EHCI) {
          Header = Find_Register(Header, SRBN_REG);
          if ((USHORT)Header != UNIMPLEMENTED_REGISTER) {
            Header->FLADJ = 0x20;
            Handle_EHCI_Wr(Header);
          }
        }
      }
    }
  }
}
