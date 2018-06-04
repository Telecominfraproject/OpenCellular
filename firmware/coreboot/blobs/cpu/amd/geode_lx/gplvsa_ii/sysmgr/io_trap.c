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
//*   Routines for setting I/O traps    
//******************************************************************************



#include "VSA2.H"
#include "PROTOS.H"
#include "GX2.H"
#include "VPCI.H"
#include "DESCR.H"
#include "CHIPSET.H"

ULONG pascal Compute_IOD_SC(ULONG *, USHORT *, UCHAR);

// External variables:
extern DESCRIPTOR MSRs[];
extern UCHAR NumMbius;
extern MBIU_INFO MbiuInfo[MAX_MBIU];
extern Hardware HardwareInfo;
extern UCHAR * VsmNames[];
extern ULONG Current_VSM;


// Local variables:
UCHAR MbiuSearchOrder[MAX_MBIU] = {2,1,0};
ULONG MbiuSkipFlags[] = {NOT_GLIU0, NOT_GLIU1, NOT_GLIU2};

//***********************************************************************
// Creates an I/O descriptor for the specified Address/Range.
// UsePID:
//  0 = set PID to zero (generate SMI)
//  1 = set PID to MBIU's subtractive port
//***********************************************************************
UCHAR pascal Setup_IO_Descriptor(ULONG * AddressPtr, USHORT * RangePtr, UCHAR UsePID)
{ UCHAR i, j, Index, FirstChoice, SecondChoice;
  USHORT Range, MaxRange;
  ULONG Address, Flags;
  MBIU_INFO * MbiuPtr;
  register DESCRIPTOR * Descr;


  Range = * RangePtr;
  Flags = Address = * AddressPtr;

  // Determine type(s) of I/O descriptor to use
  FirstChoice  = IOD_BM;
  SecondChoice = IOD_SC;
  if (Flags & (READS_ONLY | WRITES_ONLY)) {
    FirstChoice = IOD_SC;
  } else {
    // Check if we should try to use a swiss-cheese descriptor
    if (Address & 7 || Range < 8) {
      FirstChoice = IOD_SC;
      SecondChoice = IOD_BM;
    }
  }


  // Find an appropriate descriptor
  for (i = 0; i < NumMbius; i++) {
    j = MbiuSearchOrder[i];

    // Skip this MBIU ?
    if (Flags & MbiuSkipFlags[j]) {
      continue;
    }


    MbiuPtr = &MbiuInfo[j];
    Index = Allocate_Descriptor(FirstChoice, SecondChoice, MbiuPtr->Mbiu);
    if (Index != DESCRIPTOR_NOT_FOUND) {
      break;
    }
  }
  if (Index == DESCRIPTOR_NOT_FOUND) {
    return Index;
  }

  Descr = &MSRs[Index];
  Descr->Address = (USHORT)Address;

  // Set appropriate port
  if (UsePID) {
    Descr->MsrData[1] = MbiuPtr->SubtrPid;
  }

  switch (Descr->Type) {

    case IOD_BM:
      // Mask must not include any valid address bits
      MaxRange = 1 << BitScanForward(Address);
      if (Range > MaxRange) {
        Range = MaxRange;
      } else {
        while (!IsPowerOfTwo(Range)) {
          Range--;       
        }
      }
      (USHORT)Descr->MsrData[1] = (USHORT)Address >> 12;
      Descr->MsrData[0]  = ~(Range-1) | 0x000F0000;
      Descr->MsrData[0] |= Address << 20;
      Descr->Range = Range;
      * AddressPtr += Range;
      * RangePtr -= Range;
      break;

    case IOD_SC:
      Descr->MsrData[0] = Compute_IOD_SC(AddressPtr, RangePtr, 1);
      Descr->Range = Range - * RangePtr;
      break;

    default:
      return DESCRIPTOR_NOT_FOUND;

  }

  Write_MSR(Descr->MsrAddr, Descr->MsrData);
  return Index;
}



//***********************************************************************
// Clears an I/O trap on the specified I/O Range
//***********************************************************************
void pascal Clr_MBus_IO_Trap(ULONG Address, USHORT Range)
{ UCHAR Index;

  while (Range) {

    // Find an existing descriptor that matches Address
    Index = Find_Matching_IO_Descriptor(&Address, &Range, 0);

    if (Index == DESCRIPTOR_NOT_FOUND) {
      // Report error
      Log_Error("Unregistration of I/O 0x%04X by the %s VSM", Address, VsmNames[Get_VSM_Type(Current_VSM)]);
      return;
    }
  }
}


//***********************************************************************
// Sets an I/O trap on the specified I/O Range
//***********************************************************************
void pascal Set_MBus_IO_Trap(ULONG Address, USHORT Range)
{ UCHAR Index;


  // If the range is > 8 bytes and/or is not a power of 2,
  // multiple descriptors may be required.
  while (Range) {
    // Find an existing descriptor that matches Address
    Index = Find_Matching_IO_Descriptor(&Address, &Range, 2);

    if (Index == DESCRIPTOR_NOT_FOUND) {
      // An existing descriptor does not match the range.
      // Allocate a new descriptor
      Index = Setup_IO_Descriptor(&Address, &Range, 0);
      if (Index == DESCRIPTOR_NOT_FOUND) {
        // Report resource error
        Log_Error("No descriptors for I/O trap of 0x%04X by the %s VSM", Address, VsmNames[Get_VSM_Type(Current_VSM)]);
        break;
      }
    } else {
      // An existing descriptor was modified for the new Addr/Range
    }
    MSRs[Index].Flag |= IO_TRAP;
  }
}


//***********************************************************************
// Enable/Disable I/O trap
//***********************************************************************
void pascal MBus_IO_Trap(ULONG Param1, ULONG Param2, UCHAR EnableFlag)
{ ULONG Address;
  USHORT Range;

  // Repackage parameters
  Address = Param2;
  (USHORT)Address = (USHORT)Param1;
  Range = (USHORT)Param2;


  // Check for illegal combination of flags
  if ((Address & ALL_GLIUS) == ALL_GLIUS) {
    // Report error
    Log_Error("Illegal combination of flags for I/O 0x%04X by the %s VSM", Address, VsmNames[Get_VSM_Type(Current_VSM)]);
    return;
  }

  if (EnableFlag) {
    Set_MBus_IO_Trap(Address, Range);
  } else {
    Clr_MBus_IO_Trap(Address, Range);
  }
}
