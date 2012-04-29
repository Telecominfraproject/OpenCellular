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

//*	 Function:                                                          *
//*    Utility routines for managing MBIUs  


#include "VSA2.H"
#include "PROTOS.H"
#include "GX2.H"
#include "VPCI.H"
#include "DESCR.H"
#include "SYSMGR.H"


extern void InitStatCounters(ULONG, UCHAR);


// External variables:
extern ULONG Mbiu0;

// Local variables:
ULONG ExtendedMemoryDescr0, ExtendedMemoryDescr1 = 0;
MBIU_INFO MbiuInfo[MAX_MBIU];
UCHAR NumMbius=0;





//***********************************************************************
// Called once for each MBIU:
// - Initializes statistic counters
// - Records all available descriptors
//***********************************************************************
void pascal Init_MBIU(UCHAR * CountPtr, ULONG Msr)
{ UCHAR Type, i;
  ULONG Default[2], MsrData[2];


  // Record info about MBIU
  MbiuInfo[NumMbius].Mbiu = Msr;
  MbiuInfo[NumMbius].SubtrPid = Read_MSR_LO(Msr + MBD_MSR_CONFIG) << 29;
  MbiuInfo[NumMbius].NumCounters = ((CAPABILITIES *)CountPtr)->NSTATS;
  MbiuInfo[NumMbius].ActiveCounters = 0x00;
  MbiuInfo[NumMbius].ClockGating = Read_MSR_LO(Msr + MBD_MSR_PM);

  // Clear SMIs on this MBIU & disable all events except HW Emulation
  MsrData[0] = 0x0000000E;
  MsrData[1] = 0x0000000F;
  (USHORT)Msr = MBD_MSR_SMI;
  Write_MSR(Msr, MsrData);

  //*********************************************
  // Initialize Statistics MSRs
  //*********************************************
  InitStatCounters(Msr, ((CAPABILITIES *)CountPtr)->NSTATS);

  //*********************************************
  // Begin with P2D_BM descriptors
  //*********************************************
  Type = P2D_BM;
  (USHORT)Msr = MSR_MEM_DESCR;

  do {

    // Get number of next type of descriptor     
    if (i = *(CountPtr++)) {

      // Get the default value for this descriptor type
      Get_Descriptor_Default(Type, Default);

      // Loop through all descriptors of a given type
      do {

        // If descriptor is already in use by the BIOS, skip it
        Read_MSR(Msr, MsrData);
        if ((MsrData[0] == Default[0]) && (MsrData[1] == Default[1])) {

          // Initialize Descriptor[] entry
          if (Init_Descr(Type, Msr)) {
            // Not enough table entries...abort
            Type = 0x99;
            break;
          }

        } else {
          // Descriptor is in use.
          // If it's an extended memory descriptor, record the routing address.
          if (Type == P2D_R) {
            if ((Msr & ROUTING) == Mbiu0) {
              ExtendedMemoryDescr0 = Msr;
            } else {
              ExtendedMemoryDescr1 = Msr;
            }
          }
        }
        Msr++;
      } while (--i);
    }          

    // Check next descriptor type.
    // If transitioning from P2D to IOD descriptors...
	if (++Type == IOD_BM) {
      // change MSR offset to 0xE0
      (USHORT)Msr = MSR_IO_DESCR;
    }
  } while (Type <= IOD_SC);

  // Increment number of MBIUs
  NumMbius++;
}







