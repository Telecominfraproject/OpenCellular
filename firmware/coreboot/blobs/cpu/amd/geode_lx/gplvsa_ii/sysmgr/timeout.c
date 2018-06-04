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
//*    - Handles I/O inactivity counters   
//******************************************************************************



#include "VSA2.H"
#include "PROTOS.H"
#include "GX2.H"
#include "VPCI.H"
#include "DESCR.H"
#include "CS5536.H"

// External prototypes:
void Deallocate_Descriptor(DESCRIPTOR *);


// External variables:
extern ULONG LookupMbiu;
extern ULONG ClocksPerMs;
extern DESCRIPTOR MSRs[];
extern UCHAR NumDescriptors;
extern ULONG Mbiu2;
extern UCHAR NumMbius;
extern MBIU_INFO MbiuInfo[MAX_MBIU];
extern ULONG MbiuSkipFlags[];
extern Hardware HardwareInfo;

#define NUM_PHYSICAL_COUNTERS   7		// Number of physical statistic counters
#define NUM_LOGICAL_COUNTERS   20		// Number of logical statistic counters
#define NUM_LINKED_DESCRIPTORS  3		// Number of descriptors per logical counter

// Local variables:
UCHAR NumPhysCounters= 0;

typedef struct {
  ULONG  MsrAddr;			// MSR address of STATISTICS_CNT
  ULONG  Prescaler;
  ULONG  Mask;				// IOD_MASK field
  ULONG  SFlags;
  USHORT StandbyFlag;
  USHORT Timeout;
  USHORT MbiuNumber;
} PHYSICAL_COUNTERS;


typedef struct {
  ULONG  Mask;
  USHORT Address;
  USHORT DeviceID;
  USHORT Timeout;
  UCHAR  PhysIndex;         // Index to physical counter
  UCHAR  DescrIndex[NUM_LINKED_DESCRIPTORS];     // Indices to descriptors
} LOGICAL_COUNTERS;

PHYSICAL_COUNTERS PhysCounter[NUM_PHYSICAL_COUNTERS];
LOGICAL_COUNTERS  LogCounter[NUM_LOGICAL_COUNTERS];


//***********************************************************************
// Initializes a logical timeout counter
//***********************************************************************
void InitLogicalCounter(LOGICAL_COUNTERS * LogicalPtr)
{ int i;

  LogicalPtr->Mask = 0x00000000;
  for (i = 0; i < NUM_LINKED_DESCRIPTORS; i++) {
    LogicalPtr->DescrIndex[i] = 0xFF;
  }  
}


//***********************************************************************
// Deallocates a logical counter.
// Deallocates all descriptors associated with a logical counter,
// unless the descriptor is also being used for an I/O trap.
//***********************************************************************
void DeallocateLogicalCounter(LOGICAL_COUNTERS * LogicalPtr)
{ int i, j;


  for (i = 0; i < NUM_LINKED_DESCRIPTORS; i++) {
    // Get index of I/O descriptor used for this logical counter
    j = LogicalPtr->DescrIndex[i];
	if (j == 0xFF) {
      // No more linked descriptors
      break;
    }

    // If not used for I/O trap or virtualized PCI BAR
    if (!(MSRs[j].Flag & IO_TRAP) && !(MSRs[j].Owner)) {
      Deallocate_Descriptor(&MSRs[j]);
    }
  }

  // Deallocate logical counter  
  InitLogicalCounter(LogicalPtr);
}


//***********************************************************************
// Initializes the logical timeout counter structures
//***********************************************************************
void InitLogicalCounters(void) 
{ int i;
  register LOGICAL_COUNTERS * LogicalPtr;

  // Initialize logical counters
  LogicalPtr = &LogCounter[0];
  for (i=0; i < NUM_LOGICAL_COUNTERS; i++) {
    InitLogicalCounter(LogicalPtr++);
  }
}

//***********************************************************************
// Initializes the physical timeout counter structures & MSRs
// Called from Init_MBIU().
//***********************************************************************
void InitStatCounters(ULONG Msr, UCHAR NumStatCntrs) 
{ int i;
  ULONG MsrData[2], SFlags;
  register PHYSICAL_COUNTERS * PhysPtr;
  static UCHAR Shift=0;

  // Initialize physical counters
  MsrData[0] = MsrData[1] = 0x00000000;
  (USHORT)Msr = MSR_STATISTICS_CNT;
  SFlags = 1L << Shift;

  // Prepare shift count for next MBIU 
  Shift += 8;
  for (i = 0; i < NumStatCntrs; i++) {

    // Initialize the statistic MSR
    Write_MSR(Msr+0, MsrData);  // MSR_STATISTICS_CNT
    Write_MSR(Msr+1, MsrData);  // MSR_STATISTICS_MASK
    Write_MSR(Msr+2, MsrData);  // MSR_STATISTICS_ACTION

    // Initialize next physical counter
    if (NumPhysCounters < NUM_PHYSICAL_COUNTERS) {

      PhysPtr = &PhysCounter[NumPhysCounters];
      PhysPtr->MbiuNumber = NumMbius;
      PhysPtr->MsrAddr = Msr;
      PhysPtr->Mask = 0x00000000;
      PhysPtr->StandbyFlag = 0;
      PhysPtr->SFlags = SFlags;
      SFlags <<= 1;

      // Determine 16-bit prescaler
      if ((Msr & ROUTING) == Mbiu2) {
        // Clock to Southbridge statistic counters is 66 MHz.
	    PhysPtr->Prescaler = 66000L/8;
      } else {
        // Clock to Northbridge statistic counters is DRAM clock.
	    PhysPtr->Prescaler = HardwareInfo.DRAM_MHz * 1000L/8;
        // If memory is DDR, DRAM clock is running at 1/2 MBUS frequency
        if (!(Read_MSR_LO(0x4C000014) & 0x400)) {
          // so adjust the prescaler
          PhysPtr->Prescaler >>= 1;
        }
      }
      // Shift into PREDIV field
      PhysPtr->Prescaler <<= 8;
      // Set counter attributes
      (UCHAR)PhysPtr->Prescaler = (ALWAYS_DEC | HIT_LDEN | ZERO_SMI);

      // Increment number of statistics counters.
      NumPhysCounters++;   

    } else {
      Log_Error("The PhysCounter structure has fewer entries than # of h/w counters");
      // Continue initializing, but don't record MSR in array
    }

    // Advance to next statistic MSR
    Msr += 4;
  }
}







//***********************************************************************
// Finds the Address corresponding to a bit set in MBD_MSR_SMI.
// Clears the next set bit in the EventMask variable.
//***********************************************************************
USHORT Get_Timeout(ULONG SFlag, UCHAR * StartIndex)
{ UCHAR i, j;
  USHORT Address = 0x0000;
  register PHYSICAL_COUNTERS * PhysPtr;
  register LOGICAL_COUNTERS * LogicalPtr;

  // Find the correct logical counter & return the associated address
  LogicalPtr = &LogCounter[* StartIndex];
  for (i = * StartIndex; i < NUM_LOGICAL_COUNTERS; i++) {
    // Is logical counter in use ?
    if (LogicalPtr->Mask != 0x00000000) {

      // Get physical counter to which it is linked
      j = LogicalPtr->PhysIndex;
	  PhysPtr = &PhysCounter[j];

      // Is this counter generating an event ?
      if (PhysPtr->SFlags & SFlag) {

        // Yes, return either the DeviceID or the Inactive Address
        Address = LogicalPtr->DeviceID;
        if (!Address) {
          Address = LogicalPtr->Address;
        }
        break;
      }
    }
    LogicalPtr++;
  }

  // Return logical index
  * StartIndex = i+1;

  return Address;
}



//***********************************************************************
// Enables/Disables SMIs for a statistic counter
//***********************************************************************
void pascal StatCntrSMI(ULONG MsrAddr, UCHAR EnableFlag)
{ ULONG MsrData[2], Mask;
  int j;

  MsrData[0] = MsrData[1] = 0x00000000;
  if (!EnableFlag) {
    Write_MSR(MsrAddr, MsrData);
    Write_MSR(MsrAddr+2, MsrData);
  }  

  j = ((UCHAR)MsrAddr - MSR_STATISTICS_CNT) / 4;
  j++;  // HW Emulation is bit 0


  (USHORT)MsrAddr = MBD_MSR_SMI;

  Read_MSR(MsrAddr, MsrData);
  Mask = 1L << j;
  if (EnableFlag) {
    MsrData[0]  &= ~Mask;
  } else {
    MsrData[0]  |=  Mask;
  }
  Write_MSR(MsrAddr, MsrData);

}



//***********************************************************************
// Sets the IOD_MASK field of STATISTIC_MASK
//***********************************************************************
void pascal Set_IOD_MASK(PHYSICAL_COUNTERS * PhysPtr) 
{
  Write_MSR_HI(PhysPtr->MsrAddr | 1, PhysPtr->Mask);
}


//***********************************************************************
// Disables an inactivity timer for the specified parameters.
// Param1:
//    31:16 = Timeout in seconds
//    15:00 = I/O Base
// Param2:
//    31:16 = Flags (ONE_SHOT, WRITES_ONLY, READS_ONLY)
//    15:00 = I/O Range
//***********************************************************************
void pascal Clr_MBus_IO_Timeout(ULONG Param1, ULONG Param2)
{ int i;
  USHORT Address, Range, Timeout;
  MBIU_INFO * MbiuPtr;
  register PHYSICAL_COUNTERS * PhysPtr;
  register LOGICAL_COUNTERS * LogicalPtr;

  // Unpack parameters
  Range = (USHORT)Param2;
  Address = (USHORT)Param1;
  Timeout = (USHORT)(Param1 >> 16);

  // Find the corresponding logical counter
  LogicalPtr = &LogCounter[0];
  for (i = 0; i < NUM_LOGICAL_COUNTERS; i++) {
    if (LogicalPtr->Address == Address) {
	  break;
	}
    LogicalPtr++;
  }

  if (i >= NUM_LOGICAL_COUNTERS) {
    // ERROR: VSM is unregistering a non-existent timeout.
    Report_VSM_Error(ERR_UNREGISTRATION, Address, 0);
    return;
  }

  // Get physical counter corresponding to this logical counter
  PhysPtr = &PhysCounter[LogicalPtr->PhysIndex];

  // Clear IOD_MASK in the corresponding physical counter
  PhysPtr->Mask &= ~LogicalPtr->Mask;
  Set_IOD_MASK(PhysPtr);

  // Decrement timer count on this MBIU
  MbiuPtr = &MbiuInfo[PhysPtr->MbiuNumber];
  if (MbiuPtr->ActiveCounters) {
    MbiuPtr->ActiveCounters--;
  }


  // If no more timeouts for this physical counter...
  if (PhysPtr->Mask == 0x00000000) {
    PhysPtr->StandbyFlag = 0;

    // Disable the statistics counter
    StatCntrSMI(PhysPtr->MsrAddr, 0);


    if (MbiuPtr->ActiveCounters == 0) {
      // Restore clock gating on this MBIU
      if (MbiuPtr->ActiveCounters == 0) {
        Write_MSR_LO(MbiuPtr->Mbiu + MBD_MSR_PM, MbiuPtr->ClockGating);
      }
	}
  }	else {
    if (MbiuPtr->ActiveCounters == 0) {
      Log_Error("MbiuInfo->ActiveCounters is out of sync");
    }
  }

  // Deallocate descriptor(s)
  DeallocateLogicalCounter(LogicalPtr);

}

//***********************************************************************
// Enables an inactivity timer for the specified I/O Range
// Param1:
//    31:16 = Timeout in seconds
//    15:00 = I/O Base
// Param2:
//    31:16 = Flags (NOT_GLIUx, ONE_SHOT, WRITES_ONLY, READS_ONLY)
//    15:00 = I/O Range
//***********************************************************************
void pascal Set_MBus_IO_Timeout(USHORT Address, USHORT Timeout, USHORT Range, USHORT Attr, USHORT DeviceID)
{ ULONG MsrAddr, MsrData[2], Addr, Candidate, Flags;
  UCHAR i, j, k;
  MBIU_INFO * MbiuPtr;
  register DESCRIPTOR * Descr;
  register PHYSICAL_COUNTERS * PhysPtr;
  register LOGICAL_COUNTERS * LogicalPtr;

  Flags = (ULONG)Attr << 16;

  // Find an available logical counter
  LogicalPtr = &LogCounter[0];
  for (i=0; i < NUM_LOGICAL_COUNTERS; i++) {
    if (LogicalPtr->Mask == 0x00000000) {
	  break;
	}
    LogicalPtr++;
  }

  if (i >= NUM_LOGICAL_COUNTERS) {
    Log_Error("The LogCounter structure is not large enough");
    return;
  }


  // Record info about this timeout
  LogicalPtr->Address = Address;
  LogicalPtr->DeviceID = DeviceID;
  LogicalPtr->Timeout = Timeout;


  // Set Flags:NOT_GLIUx for GLIUs with no available counters
  for (k=0; k < NumMbius; k++) {

    // Skip this MBIU ?
    if (Flags & MbiuSkipFlags[k]) {
      continue;
    }
    MbiuPtr = &MbiuInfo[k];
    j = MbiuPtr->NumCounters;

    //  
    PhysPtr = &PhysCounter[0];
    for (i=0; i < NumPhysCounters; i++) {
      // Is this counter on the current GLIU ?
      if ((MbiuPtr->Mbiu & ROUTING) == (PhysPtr->MsrAddr & ROUTING)) {
        // Is it available ?
        if (PhysPtr->Mask) {
          // No, is it the last counter on this GLIU ?
          if (--j == 0) {
            // Yes, then exclude from search
            Flags |= MbiuSkipFlags[k];
            break;
          }
        }
      }
      PhysPtr++;
    }
  }

  // Allocate descriptor(s) for address range
  Addr = Flags;		   // Set Addr = Flags::Address
  (USHORT)Addr = Address;
  k = 0;
  while (Range) {

    // Find existing descriptor(s) that match Address
    j = Find_Matching_IO_Descriptor(&Addr, &Range, 1);
    if (j == DESCRIPTOR_NOT_FOUND) {
      // No compatible descriptor exists, so create one
      // and route it to the subtractive port.
      j = Setup_IO_Descriptor(&Addr, &Range, 1);
      if (j == DESCRIPTOR_NOT_FOUND) {
        // Error: No free descriptors
        k = 0xFF;
        break;
      }
    }

    Descr = &MSRs[j];

    // Record the descriptor index
    LogicalPtr->DescrIndex[k] = j;

    if (k++ == 0) {
      // Record 1st descriptor
      MsrAddr = Descr->MsrAddr;
    } else {
      // Check that all descriptors are on same MBIU
	  if ((MsrAddr & ROUTING) != (Descr->MsrAddr & ROUTING)) {
        // No free descriptors on same MBIU
        k = 0xFF;
        break;
      }
    }
    // Add the new descriptor to IOD_MASK
    LogicalPtr->Mask |= 1L << ((UCHAR)Descr->MsrAddr - MSR_IO_DESCR);
  }


  // If no descriptor found...
  if (k == 0xFF) {
    // ERROR: Not enough descriptors to handle request
    Report_VSM_Error(ERR_NO_MORE_DESCRIPTORS, Address, 0);

    // Deallocate descriptor(s) and logical counter
    DeallocateLogicalCounter(LogicalPtr);
    return;
  }

  // Find an appropriate hardware statistic counter
  Candidate = 0;
  for (i=0; i < NumPhysCounters; i++) {

    PhysPtr = &PhysCounter[i];

    // Physical counter must be on the same MBIU as the I/O descriptor
    if ((PhysPtr->MsrAddr & ROUTING) != (Descr->MsrAddr & ROUTING)) {
      continue;
    }

    if (PhysPtr->Timeout == Timeout) {
      // If same timeout, the leverage same counter
      break;
    }

    // Is this counter being used for Standby inactivity detection ?
    if (PhysPtr->StandbyFlag) {
      // Yes, is the request for Standby ?
      if (Flags & FOR_STANDBY) {
        // Yes, then must use the same physical counter
        break;
      }
    }

    // Mark physical counter as a candidate
    Candidate |= 1L << i;
  }

  // If an existing counter can't be leveraged, find a new one
  if (i >= NumPhysCounters) {
    // Among the candidates, find an unused counter
    for (i=0; i < NumPhysCounters; i++) {
      if (Candidate & (1L << i)) {
        PhysPtr = &PhysCounter[i];
        if (!(PhysPtr->Mask)) {
          break;
        }
      }
    }
  }

  // If physical counter is found, link it to the descriptor
  if (i < NumPhysCounters) {

    // Increment timer count on this MBIU
    MbiuPtr = &MbiuInfo[PhysPtr->MbiuNumber];
    MbiuPtr->ActiveCounters++;
    // Turn off clock gating on this MBIU
    Write_MSR_LO(MbiuPtr->Mbiu + MBD_MSR_PM, Read_MSR_LO(MbiuPtr->Mbiu + MBD_MSR_PM) & 0xFFFFFFF0);

    // Record physical counter used for the logical counter
    LogicalPtr->PhysIndex = i;

    // Mark descriptor as used for I/O timeout
    Descr->Flag |= IO_TIMEOUT;

    // Record info about this timer
    PhysPtr->Timeout = Timeout;
    PhysPtr->Mask |= LogicalPtr->Mask;

    // Set flag if counter is being used for Standby inactivity detection
    if (Flags & FOR_STANDBY) {
      PhysPtr->StandbyFlag = 1;
    }

    // Set STATISTIC_CNT[LOAD_VAL]
    MsrAddr = PhysPtr->MsrAddr;
    MsrData[0] = MsrData[1] = Timeout * 8*1000L;
    Write_MSR(MsrAddr, MsrData);

    // Link counter to the descriptor
    Set_IOD_MASK(PhysPtr);

    // Set STATISTIC_ACTION[PREDIV]
    // Set Prescaler to decrement Count every ms
    Write_MSR_LO(MsrAddr+2, PhysPtr->Prescaler);


    // Enable the statistics counter in MBD_MSR_SMI
    StatCntrSMI(MsrAddr, 1);

    return;
  }


  // ERROR: Not enough statistics counter to handle request
  Report_VSM_Error(ERR_NO_MORE_DESCRIPTORS, (ULONG)Address, NumPhysCounters);

}


typedef struct {
  union {
    ULONG dword;
    struct {
      USHORT IO_Base;
      USHORT Timeout;
    };
  };
} TIMEOUT_P1;

typedef struct {
  union {
    ULONG Flags;
    struct {
      union {
        USHORT Range;
        UCHAR Instance;
      };
      USHORT Attributes;
    };
  };
} TIMEOUT_P2;


//***********************************************************************
// Enables/disables an inactivity timer for an I/O Range or GLIU device.
// Param1:
//    31:16 = Timeout in seconds
//    15:00 = I/O Base
//          = GLIU DeviceID if Flags[GLIU_ID] is set
// Param2:
//    31:16 = Flags
//    15:00 = Length of I/O range
//          = Instance if Flags[GLIU_ID] is set
// EnableFlag:
//        0 = disable
//        1 = enable
//***********************************************************************
void pascal MBus_IO_Timeout(ULONG Param1, ULONG Param2, UCHAR EnableFlag) 
{ UCHAR Port, ByteEnables, i, Hit;
  USHORT Address, DeviceID = 0x0000;
  ULONG MsrAddr;
  register DESCRIPTOR * Descr;
  TIMEOUT_P1 p1;
  TIMEOUT_P2 p2;


  // Force use of descriptors in Southbridge
  if (!(Param2 & FOR_STANDBY)) {
    Param2 |= NOT_GLIU0 | NOT_GLIU1;
  }

  p1.dword = Param1;
  p2.Flags = Param2;

  if (p2.Flags & GLIU_ID) {
    p2.Flags &= ~GLIU_ID;

    DeviceID = p1.IO_Base;

    // Don't allow stupid requests
    switch (DeviceID) {
      case ID_MBIU:
      case ID_MDD:
      case ID_MCP:
      case ID_VAIL:
        // Log an error
        Report_VSM_Error(ERR_BAD_PARAMETER, EVENT_IO_TIMEOUT, Param1);
        return;
    }


    // Find specified instance of the requested device
    MsrAddr = Find_MBus_ID(DeviceID, p2.Instance);
    Port = (UCHAR)MsrAddr;

    if (!MsrAddr) {
      // Log an error
      Report_VSM_Error(ERR_HW_MISMATCH, EVENT_IO_TIMEOUT, Param2);
      return;
    }

    MsrAddr = LookupMbiu & ROUTING;   // LookupMbiu was set by Find_MBus_ID()

    // Find all descriptors on this MBIU/Port
    Hit = 0;
    for (i = 1; i < NumDescriptors; i++) {

      Descr = &MSRs[i];

      // Is descriptor routing to a device ?
      Address = Descr->Address;
      if (Address == 0x0000) {
        continue;
      }
      // Is descriptor on correct MBIU ?
      if (MsrAddr != (Descr->MsrAddr & ROUTING)) {
        continue;
      }
      // Is descriptor routing to the requested device ?
      if (Port == (Descr->MsrData[1] >> 29)) {

        // Yes, determine start address of descriptor
        switch (Descr->Type) {
          case IOD_SC:
            // Determine start address of IOD_SC descriptor
            ByteEnables = (UCHAR)(Descr->MsrData[0] >> 24);
            while (ByteEnables) {
              if (ByteEnables & 1) {
                break;
              } else {
                Address++;
              }
              ByteEnables >>= 1;
            }
            // Fall through intended

          case IOD_BM:
             // Recursive call
            p1.IO_Base = Address;
            p2.Range = 1;
            MBus_IO_Timeout(p1.dword, p2.Flags, EnableFlag);
            Hit = 1;
            break;

          default:
            break;
        }
      }
    }

    if (Hit || DeviceID != ID_VG) {
      DeviceID = 0x0000;
      return;
    }
    // Monitor external video card activity
    p1.IO_Base = 0x03C0;
    p2.Range = 32;
  }

  // Check for illegal combination of flags
  if ((p2.Flags & ALL_GLIUS) == ALL_GLIUS) {
    // ERROR: Illegal combination of EVENT_IO_TIMEOUT flags
    Report_VSM_Error(ERR_BAD_PARAMETER, EVENT_IO_TIMEOUT, p2.Flags);
    return;
  }

  if (EnableFlag) {
    Set_MBus_IO_Timeout(p1.IO_Base, p1.Timeout, p2.Range, p2.Attributes, DeviceID);
  } else {
    Clr_MBus_IO_Timeout(p1.dword, p2.Flags);
  }
}


