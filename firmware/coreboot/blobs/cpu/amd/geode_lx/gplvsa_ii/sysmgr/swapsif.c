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
//*    SWAPSiFs routines
//*****************************************************************************



#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"
#include "VPCI.H"
#include "PCI.H"


// External Functions:
extern void pascal Return_Virtual_Value(SmiHeader *, ULONG);


// External Variables:
extern PCI_HEADER_ENTRY ISA_Hdr[];
extern DESCRIPTOR MSRs[];



#define ACPI_FLAGS (0)
#define ACPI_RANGE 0x20
#define PMS_RANGE  0x80
#define PMS_FLAGS  (NOT_GLIU1)

ULONG ACPI_Timer_MSR;


//***********************************************************************
// Enables/disables trapping of PMS or ACPI registers
//***********************************************************************
UCHAR pascal PM_Trapping(USHORT EnableFlag, USHORT Bar)
{ ULONG DescrDefault[2];
  UCHAR Link, i=0;
  register DESCRIPTOR * Descr;

  // For each linked item, update the associated MSR
  Link = ISA_Hdr[Bar/4].Link;
  while (Link) {  
    Descr = &MSRs[Link];

    // Get link to next MSR
    Link = Descr->Link;

    // Ignore this MSR if not an I/O trap
    if ((Descr->Flag & IO_TRAP) != IO_TRAP) {
      continue;
    }

    if (EnableFlag) {
      // Restore descriptor to original value (re-enable trapping)
	  Write_MSR(Descr->MsrAddr, Descr->MsrData);
	} else {
      i++;
      // Set descriptor to default (disable trapping)
      Get_Descriptor_Default(Descr->Type, DescrDefault);
	  Write_MSR(Descr->MsrAddr, DescrDefault);
    }
  }

  return i;
}


//***********************************************************************
// Enables/disables trapping of PM Support registers
//***********************************************************************
UCHAR pascal PMS_Trapping(USHORT EnableFlag)
{
  return PM_Trapping(EnableFlag, BAR4);
}



//***********************************************************************
// Enables/disables trapping of ACPI registers
//***********************************************************************
UCHAR pascal ACPI_Trapping(USHORT EnableFlag)
{
  return PM_Trapping(EnableFlag, BAR5);
}




//***********************************************************************
// Workaround for ACPI issues with A3 parts:
// 1) Hang on byte accesses to ACPI registers 0x00-0x03 
// 2) Improper sharing of shadow register
// Flag:
//  = 0 if PM Support registers
//  = 1 if ACPI registers
//***********************************************************************
void ACPI_Workaround(SmiHeader * SmiHdr, USHORT Flag)
{ USHORT Address;
  UCHAR Alignment, Size;
  ULONG WriteToClearMask, Mask, Data, SrcData;
  static ULONG WriteToClearMasks[] = {
    0xFFFF0000,		// 00 PM1_STS
    0xFFFFFFFF,		// 04 PM1_EN
    0xFFFFFDFF,		// 08 PM1_CNT
    0xFFFFFFFF,		// 0C PM2_CNT
    0xFFFFFFFF,		// 10 PM_TMR
    0xFFFFFFFF,		// 14 Reserved
    0x00000000,		// 18 GPE0_STS
    0xFFFFFFFF,		// 1C GPE0_EN
  };


  // Get info about the I/O from the SMM header
  Data = SmiHdr->write_data;
  Address = SmiHdr->IO_addr;
  Size = (UCHAR)SmiHdr->data_size;

  Alignment = (UCHAR)((Address & 3) << 3);
  // Make Address dword-aligned
  Address &= ~3;

  if (Flag) {

    // Get write-to-clear mask for ACPI registers
    WriteToClearMask = WriteToClearMasks[(Address & 0x1F) >> 2];

    // Disable I/O trapping
    ACPI_Trapping(0);

  } else {

    // Get write-to-clear mask for PM Support registers
	switch ((UCHAR)Address) {

      case 0x00:
        WriteToClearMask = 0xFFFF7FFF;
        break;

      case 0x54:
        WriteToClearMask = 0xFFFF0000;
        break;

      default:
        WriteToClearMask = 0xFFFFFFFF;
        break;
	}
    // Disable I/O trapping
    PMS_Trapping(0);

  }


  // Read initial value
  SrcData = in_32(Address);
 
  if (SmiHdr->SMI_Flags.IO_Write) {  
    // I/O WRITE

    // Get mask appropriate to I/O size
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
    }


    Data  &= Mask;
    Data <<= Alignment;
    SrcData &= ~(Mask << Alignment);

    // Write final data out; Don't rewrite write-to-clear bits
    Data |= (SrcData & WriteToClearMask);
    out_32(Address, Data);

  } else {

    // I/O READ
	Data = (SrcData >> Alignment);

    // Return value to the right environment                     
    Return_Virtual_Value(SmiHdr, Data);
  }

  // Re-enable I/O trapping
  if (Flag) {
    ACPI_Trapping(1);
  } else {
    PMS_Trapping(1);
  }
}


//***********************************************************************
// An I/O BAR for devices in the MDD (e.g. PMS and ACPI) doesn't require
// a GLIU descriptor since the MDD is the subtractive decode. Therefore,
// if these registers are trapped, they are not linked to the BARs.
// This function adds the linkages so when the BAR is changed, the 
// descriptor(s) generating the I/O trap are also changed.
// NOTES:
// 1) The descriptors must be linked in order of increasing address.
//    Increasing address does not imply increasing MSRs[] index, so the
//    search index starts over at 1 on an address hit. 
// 2) This function must handle the case where the BAR is of one size but
//    the trapped range is another (either longer or shorter). 
//***********************************************************************
void pascal FixupLinkages(USHORT Bar)
{ USHORT IO_Address;
  UCHAR First = 0, Previous=0, Index;
  register DESCRIPTOR * Descr;
 
  IO_Address = ISA_Hdr[Bar/4].Value_LO;

  // Walk through all the descriptors 
  for (Index=1; Index<MAX_DESCR ; Index++ ) {
    Descr = &MSRs[Index];

    // Only examine descriptors used for I/O traps
    if ((Descr->Flag & (AVAILABLE | IO_TRAP)) != IO_TRAP) {
      continue;
    }
    // Does this descriptor map the region covered by the BAR?
    if (Descr->Address == IO_Address) {
      // Yes
      if (Previous) {
        // Link it
        MSRs[Previous].Link = Index;
      } else {
        First = Index;
      }
      Previous = Index;

      // Mark it unavailable so we don't 'hit' again
      Descr->Flag &= ~AVAILABLE;

      // Advance the address
      IO_Address += (USHORT)Descr->Range;

      // Skip the ACPI timer register
      if (Bar == BAR5) {
        switch (IO_Address & (ACPI_RANGE-1)) {
          case 0x10:
            (UCHAR)IO_Address += 4;
            break;
          case 0x18:
            // Record descriptor that skips ACPI timer
            ACPI_Timer_MSR = Descr->MsrAddr;
            break;
        }
      }
      // Re-start search at beginning of MSR array
      Index = 1;
    }
  }

  if (First) {
    // Re-link the 'expanded' list to the BAR's linkages  
    MSRs[Previous].Link = ISA_Hdr[Bar/4].Link;
    ISA_Hdr[Bar/4].Link = First;
  }	else {
    Log_Error("No links found by FixupLinkages()");
  }
}



//***********************************************************************
// Installs the trapping for ACPI and PMS register SWAPSiF
//***********************************************************************
void ACPI_PMS_SWAPSiF(void)
{  USHORT IO_Address;

  // Trap PMC registers (F0 BAR4)
  IO_Address = ISA_Hdr[BAR4/4].IO_Base;
  if (IO_Address) {
    Register_Event(EVENT_IO_TRAP, 0, SysMgr_VSM, IO_Address, PMS_FLAGS | PMS_RANGE);
    // Link the descriptors used for trapping the PMS registers
    FixupLinkages(BAR4);
  }

  // Trap ACPI registers (F0 BAR5)
  IO_Address = ISA_Hdr[BAR5/4].IO_Base;
  if (IO_Address) {
    Register_Event(EVENT_IO_TRAP, 0, SysMgr_VSM, IO_Address,      ACPI_FLAGS | 0x10);
    // Skip the ACPI Timer register
    Register_Event(EVENT_IO_TRAP, 0, SysMgr_VSM, IO_Address+0x14, ACPI_FLAGS | 0x0C);
    // Link the descriptors used for trapping the ACPI registers
    FixupLinkages(BAR5);
  }
}