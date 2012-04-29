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
//*    Routines related to the MBus Diverse Device 
//******************************************************************************


#include "VSA2.H"
#include "SYSMGR.H"
#include "GX2.H"
#include "MDD.H"
#include "PROTOS.H"
#include "DESCR.H"
#include "ACPI.H"
#include "VPCI.H"
#include "PCI.H"
#include "ACPI.H"
#include "CHIPSET.H"

#define A20_EN	 (A20_P_EN | A20_K_EN)
#define INIT_EN  (INIT_K_EN | INIT_P_EN)


// External function prototypes:
void Init_MFGPT(void);
UCHAR pascal ACPI_Trapping(USHORT);

// Local function prototypes:
void pascal Control_MDD_SMI(USHORT, ULONG);


// Local variables:
ULONG MDD_Base;

// External variables:
extern PCI_HEADER_ENTRY ISA_Hdr[];
extern Hardware HardwareInfo;


//***********************************************************************
// Enables/disables KEL SMIs
//***********************************************************************
void pascal Control_KEL_SMI(USHORT EnableFlag)
{
  Control_MDD_SMI(EnableFlag, KEL_ASMI_EN);
}

//***********************************************************************
// Enable/disable keyboard command snooping by KEL
//***********************************************************************
void pascal Control_KEL_Snoop(USHORT EnableFlag)
{ ULONG MsrAddr, MsrData;

  MsrAddr = MDD_Base;
  (USHORT)MsrAddr = MSR_KEL_CNTRL;
  MsrData = Read_MSR_LO(MsrAddr);

  if (EnableFlag) {
    MsrData |=  KEL_SNOOP;
  } else {
    MsrData &= ~KEL_SNOOP;
  }
  Write_MSR_LO(MsrAddr, MsrData);

}

//***********************************************************************
// Initializes the MBus Diverse Device
//***********************************************************************
void Init_MDD(void)
{ ULONG MsrAddr;
  USHORT ACPI_Bar;
  UCHAR i;


  // Find address of MBus Diverse Device
  MsrAddr = MDD_Base = Find_MBus_ID(ID_MDD, 1) & 0xFFFF0000;

  //*********************************************
  // Record MDD's LBARs in MSRs[]
  //*********************************************
  for (i = MSR_LBAR_IRQ; i <= MSR_LBAR_FLSH3; i++) {

    (UCHAR)MsrAddr = i;

    if (Init_Descr(MDD_LBAR, MsrAddr)) {
      break;
    }
  }

  // Clear PM1_STS
  (UCHAR)MsrAddr = ISA_Hdr[BAR5/4].LBar;
  ACPI_Bar = (USHORT)Read_MSR_LO(MsrAddr);
  out_16(ACPI_Bar, in_16(ACPI_Bar));


  // Initialize KEL

  // Enable keyboard snooping by KEL
  Control_KEL_Snoop(1);

  // Enable SMIs from:
  // - A20 & Init (keyboard and port 92h)
  // - KEL
  // - Extended PIC Mapper
  Control_MDD_SMI(1, A20_EN | INIT_EN | PIC_ASMI_EN | KEL_ASMI_EN);


  // Initialize the MFGPT
  Init_MFGPT();

  // Clear any pending PIC events
  (USHORT)MsrAddr = MBD_MSR_SMI;
  Write_MSR_HI(MsrAddr, PIC_ASMI_EN);

}




//***********************************************************************
// Implements CS5536's F0 Special Cycles.
// Linked to MDD's MSR_LEG_IO[31].
//
// When set, a Shutdown special cycle causes a reset. When the Special
// Cycles bit is cleared, a Shutdown special cycle is ignored.  Before 
// updating MSR_LEG_IO,  VSA will check MSR_ERR[15] and MSR_SMI[1]. If 
// either of these MSR bits are set, then no action is taken.  It will
// be assumed that a debugger is in use and VSA will not interfere.
//***********************************************************************
void pascal Update_Special_Cycles(USHORT EnableFlag)
{ ULONG MSR_Addr, MSR_Data;


  // If either MSR_ERR[15] and MSR_SMI[1] is set, then bail.
  if (Read_MSR_LO(MDD_Base + MBD_MSR_SMI) & 2) {
    return;
  }
  if (Read_MSR_LO(MDD_Base + MBD_MSR_ERROR) & 0x8000) {
    return;
  }

  // Link MSR_LEG_IO[RESET_SHUT_EN] to COMMAND[3]
  MSR_Addr = MDD_Base + MSR_LEG_IO;
  MSR_Data = Read_MSR_LO(MSR_Addr);

  if (EnableFlag) {
    MSR_Data |=  RESET_SHUT_EN;
  } else {
    MSR_Data &= ~RESET_SHUT_EN;
  }

  Write_MSR_LO(MSR_Addr, MSR_Data);
}


//***********************************************************************
// Enable/disable of MDD ASMI(s)
//***********************************************************************
void pascal Control_MDD_SMI(USHORT EnableFlag, ULONG EnableMask)
{ ULONG MsrAddr, MsrData;

  MsrAddr = MDD_Base;
  (USHORT)MsrAddr = MBD_MSR_SMI;

  MsrData = Read_MSR_LO(MsrAddr);
  if (EnableFlag) {
    MsrData |=  EnableMask;
   } else {
    MsrData &= ~EnableMask;
  }
  Write_MSR_LO(MsrAddr, MsrData);
}


//***********************************************************************
// Clears the ACPI Status register
// This routine fills in MsgPacket[0] = ACPI GPE0_STS
// and MsgPacket[1] bits[15:0] = ACPI PM1_STS
//***********************************************************************
USHORT Get_ACPI_Status(ULONG *msgp)
{ USHORT ACPI_Bar, PM1_Status;
  ULONG GPE_Status;
  UCHAR Flag;

  // Disable ACPI trapping
  Flag = ACPI_Trapping(0);

  // Get ACPI Status
  ACPI_Bar = ISA_Hdr[BAR5/4].Value_LO;

  (UCHAR)ACPI_Bar = GPE0_STS_OFS;
  GPE_Status = in_32(ACPI_Bar);

  // Don't clear PIC status
  GPE_Status &= ~1;


  msgp[1] = GPE_Status;


  (UCHAR)ACPI_Bar = PM1_STS_OFS;
  PM1_Status = in_16(ACPI_Bar);


  // Enable ACPI trapping
  if (Flag) {
    ACPI_Trapping(1);
  }

  msgp[2] = (ULONG)PM1_Status;

  return (PM1_Status || GPE_Status);
}

//***********************************************************************
// Enable PM logic
//***********************************************************************
void Enable_PME_Event(UCHAR EnableFlag, UCHAR Pm1Bit, UCHAR PmeBit, USHORT Attributes)
{ ULONG Gpe, Pm1, bmGPE0;
  USHORT ACPI_Bar, bmPM1;
  UCHAR Flag;
  static UCHAR pme_instance = 0;
  static UCHAR PM1_Instance[11] = {0,0,0,0,0,0,0,0,0,0,0};
  static UCHAR PME_Instance[8]  = {0,0,0,0,0,0,0,0};
  static ULONG GPE0_Masks[] = { // Maps PME # to GPE0 bits
    0x00010000,
    0x00020000,
    0x00040000,
    0x00080000,
    0x00100000,
    0x00200000,
    0x40000000,
    0x80000000,
  };

  bmPM1 = 0;
  bmGPE0 = 0L;
  if (Attributes & PM1) {
    bmPM1 = 1 << Pm1Bit;

    if (EnableFlag) {
      PM1_Instance[Pm1Bit]++;
	} else {
      if (PM1_Instance[Pm1Bit]) {
        // If there are still registrations for this PM1 bit...
        if (--PM1_Instance[Pm1Bit]) {
          // then don't turn disable it
          return;
        }
      } else {
        // Error: 
        return;
      }
    }
  }
  if (Attributes & GPE) {
    bmGPE0 = GPE0_Masks[PmeBit];
    if (EnableFlag) {
      PME_Instance[PmeBit]++;
	} else {
      if (PME_Instance[PmeBit]) {
        // If there are still registrations for this GEP0 bit...
        if (--PME_Instance[PmeBit]) {
          // then don't turn disable it
          return;
        }
      } else {
        // Error: 
        return;
      }
    }
  }


  if (!(Attributes & NO_ASMI)) {
    // Keep a count of PME events.
    // Once all have been de-registered, turn off PM_ASMI bit.
    if (EnableFlag) {
      if (Attributes & PME) {
        pme_instance++;
      }
      // Enable ASMIs from Power Management logic
      Control_MDD_SMI(EnableFlag, PM_ASMI_EN);
    } else {
      if (Attributes & PME) {
        if (pme_instance != 0) {
          if (--pme_instance == 0) {
            // Disable ASMIs from Power Management logic
            Control_MDD_SMI(0, PM_ASMI_EN);
          }
        }
      }
    }
  }

  // See if we want to enable the approriate bits in PM1_EN and GPE0_EN
  // if set to NO_ENALE we don't enable (Used by ACPI as the OS controls)
  if (!(Attributes & NO_ENABLE)) {
    // Disable ACPI trapping
    Flag = ACPI_Trapping(0);

    // Get ACPI base address
    ACPI_Bar = ISA_Hdr[BAR5/4].Value_LO;

    if (bmPM1) {
      // Must do 32-bit I/O to avoid shadow register bug (IAeng00003062)
      (UCHAR)ACPI_Bar = 0x00;
      Pm1 = in_32(ACPI_Bar);
      if (EnableFlag) {
        Pm1 |=   (ULONG)bmPM1 << 16;
      } else {
        Pm1 &= ~((ULONG)bmPM1 << 16);
      }
      out_32(ACPI_Bar, Pm1);

      // Serialize the previous I/O write
      in_16(ACPI_Bar);

      // Clear spurious status
      (UCHAR)ACPI_Bar = PM1_STS_OFS;
      out_32(ACPI_Bar, (ULONG)Pm1);
    }

    if (bmGPE0) {
      (UCHAR)ACPI_Bar = GPE0_EN_OFS;
      Gpe = in_32(ACPI_Bar);
      if (EnableFlag) {
        Gpe |=  bmGPE0;
      } else {
        Gpe &= ~bmGPE0;
      }
      out_32(ACPI_Bar, Gpe);
      // Serialize the previous I/O write
      in_32(ACPI_Bar);

      // Clear spurious status
      (UCHAR)ACPI_Bar = GPE0_STS_OFS;
      out_32(ACPI_Bar, Gpe);
    }

    // Serialize the previous I/O write
    in_32(ACPI_Bar);
  
    // Re-enable ACPI trapping
    if (Flag) {
      ACPI_Trapping(1);
    }
  }
}
