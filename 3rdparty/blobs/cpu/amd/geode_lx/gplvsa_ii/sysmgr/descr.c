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
//*    Utility routines for managing descriptors 
//******************************************************************************



#include "VSA2.H"
#include "PROTOS.H"
#include "GX2.H"
#include "VPCI.H"
#include "DESCR.H"
#include "SYSMGR.H"



// External variables:
extern UCHAR NumMbius;
extern UCHAR MBIU1_SelfReference;

// Local variables:
UCHAR NumDescriptors = 1;
DESCRIPTOR MSRs[MAX_DESCR]={0};
UCHAR DynamicVSALoad=0;


//***********************************************************************
// Computes the 32 LSBs of an IOD_SC descriptor from Address/Range
//***********************************************************************
ULONG pascal Compute_IOD_SC(ULONG * AddressPtr, USHORT * RangePtr, UCHAR Flag)
{ ULONG IO_Mask, Address;
  USHORT Range;
  UCHAR Addr_LSBs, Bit_Mask, Max_Bits;

  Address = * AddressPtr;
  Range = * RangePtr;

  // Set IOD_SC Base field
  IO_Mask = Address & 0x0000FFF8;

  // Compute the R/W attributes
  if (!(Address & WRITES_ONLY)) {
    IO_Mask |= REN;
  }
  if (!(Address & READS_ONLY)) {
    IO_Mask |= WEN;
  }

  // Initialize bit mask  
  Addr_LSBs = (UCHAR)Address & 0x7;
  Max_Bits = 8 - Addr_LSBs;
  if (Range > Max_Bits) {
    Range = Max_Bits;
  }
  Bit_Mask = (UCHAR)(0x01 << Range) - 1;
  Bit_Mask <<= Addr_LSBs;

  // Insert byte enables
  IO_Mask |=  (ULONG)Bit_Mask << 24;


  // Adjust Address & Range parameters by # bytes handled by this descriptor
  if (Flag) {
    * AddressPtr += (ULONG)Range;
    * RangePtr -= (USHORT)Range;
  }
  return IO_Mask;
}


//***********************************************************************
// Returns the default value for the given descriptor Type
//***********************************************************************
void pascal Get_Descriptor_Default(UCHAR Type, ULONG * Msr)
{

  *Msr = *(Msr+1) = 0x00000000;

  switch (Type) {

    case IOD_BM:
    case P2D_BM:
    case P2D_BMO:
    case P2D_BMK:
      *Msr     = 0xFFF00000;
      *(Msr+1) = 0x000000FF;
      break;

    case P2D_R:
    case P2D_RO:
      *(Msr) = 0x000FFFFF;
      break;

    default:
      break;
  }
}


//***********************************************************************
// Initializes a MSRs[] entry
//***********************************************************************
UCHAR pascal Init_Descr(UCHAR Type, ULONG MsrAddr)
{ register DESCRIPTOR * Descr;

  // Keep count of total # descriptors
  if (NumDescriptors >= MAX_DESCR) {
    // Log an error:  Not enough descriptor entries.
    Report_VSM_Error(ERR_NO_MORE_DESCRIPTORS, MAX_DESCR, 0x77 );
    return 0x01;
  }



  Descr = &MSRs[NumDescriptors++];

  // Initialize the descriptor entry
  Descr->Mbiu = NumMbius;
  Descr->Split = 0x00;
  Descr->Flag = AVAILABLE;
  Descr->Type = Type;
  Descr->Link = 0x00;
  Descr->Owner = 0x0000;
  Descr->MsrAddr  = MsrAddr;
  Descr->Physical = 0x00000000;

  return 0x00;
}


//***********************************************************************
// Returns a descriptor to the available pool
//***********************************************************************
void Deallocate_Descriptor(DESCRIPTOR * Descr)
{ 
  // Mark MSR available
  Descr->Flag = AVAILABLE;
  Descr->Link = 0x00;
  Descr->Owner = 0x0000;
  // Set MSR to default value
  Get_Descriptor_Default(Descr->Type, Descr->MsrData);
  Write_MSR(Descr->MsrAddr, Descr->MsrData);

}




//***********************************************************************
// Restores all descriptors owned by VSA to their default values.
// Used when VSA is installed from DOS.
//***********************************************************************
void ReInit_Descriptors(void)
{ register USHORT Index;
  register DESCRIPTOR * Descr;
  ULONG MDD_Msr;

  DynamicVSALoad = 1;
  
  Descr = &MSRs[1];

  MDD_Msr = Find_MBus_ID(ID_MDD, 1) & ROUTING;

  // Restore VSA-owned MSRs to their default values
  for (Index = 1; Index < NumDescriptors; Index++) {
    // Don't zero MDD LBARs
    if ((Descr->MsrAddr & ROUTING) == MDD_Msr) {
      Descr->Flag = AVAILABLE;
    } else {
      if (Descr->MsrAddr == 0x5100002F) {
        continue;
      }
      Deallocate_Descriptor(Descr);
    }
    Descr++;
  }

  NumDescriptors = 1;

}






//***********************************************************************
// Searches for an unused descriptor of the specified type(s) on the
// specified MBIU.
// NOTE: This implementation makes multiple passes through the table, but
//       this removes the requirement that they be in a particular order.
//***********************************************************************
UCHAR pascal Allocate_Descriptor(UCHAR Type, UCHAR EndType, ULONG Msr)
{ register DESCRIPTOR * Descr;
  register UCHAR Index;
  ULONG Mask;
  UCHAR Incr = 1;
  
  if (EndType < Type) {
    Incr = 0xFF;
  }
  EndType += Incr;

  do {
  
    // Find an available descriptor of the right type
    Index = 1;
	do {

      Descr = &MSRs[Index];

      if ((Descr->Flag & AVAILABLE) && (Type == Descr->Type)) {

        switch (Type) {

          case MPCI_RCONF:
          case GX2_RCONF:
            // Any register will do
            Mask = 0x00000000;
            break;
                    
          case USB_LBAR:
          case MDD_LBAR:
            // Entire MSR must match
            Mask = 0xFFFFFFFF;
            break;

          case EPCI:
            // Entire routing field must match
            Mask = ROUTING;
            break;

          default:
            // MSR must match in 1st 3 routing fields
            Mask = 0xFF800000;
            break;

        }
 
        if ((Descr->MsrAddr & Mask) == (Msr & Mask)) {
          // Mark descriptor in-use
          Descr->Flag &= ~AVAILABLE;
          return Index;
        }
      }

      Index++;
    } while (Index != NumDescriptors);
    Type += Incr;	 
  } while (Type != EndType);

  return DESCRIPTOR_NOT_FOUND;
}




//***********************************************************************
// Searches existing descriptors for the specified address range.
// If possible, a descriptor is updated to add/remove the new range.
// Parameter Enable:
// = 0, the range is removed
// = 1, the range is added for I/O timeout
// = 2, the range is added for I/O trap
// = 3, the range is being searched (SYS_IO_DESCRIPTOR)
//***********************************************************************
UCHAR pascal Find_Matching_IO_Descriptor(ULONG * AddressPtr, USHORT * RangePtr, UCHAR Enable)
{ UCHAR Index, HitFlag, DescrFlag;
  ULONG Attributes, Mask;
  USHORT Address, StartRange, EndRange, Length, Range;
  register DESCRIPTOR * Descr;
  static ULONG GLIU_Masks[3] = {NOT_GLIU0, NOT_GLIU1, NOT_GLIU2};

  Range = * RangePtr;
  Address = (USHORT) * AddressPtr;
  Attributes = * AddressPtr;


  // Scan descriptors for those used for I/O trap or timeout
  // NOTE:  We must be careful about mixing I/O traps with I/O address that
  //        are directed to Northbridge devices.  This may cause multiple
  //        MBUI descriptors set to the same address.
  for (Index = 1; Index < NumDescriptors; Index++) {

      Descr = &MSRs[Index];
      DescrFlag = Descr->Flag;

      // Only examine allocated descriptors
      if (DescrFlag & AVAILABLE) {
        continue;
      }

      // Ignore descriptors that route transactions Northbound
      if (Descr->MsrData[1] >> 29 == MBIU1_SelfReference) {
        continue;
      }

      // If an MBIU is to be excluded, don't allow a match on that MBIU
      if (Attributes & GLIU_Masks[Descr->Mbiu]) {
        continue;
      }



      // Check if this descriptor matches the requested address range.
      // Cases:
      //   - Descriptor needs to be split, since I/O ranges are not compatible.
      //   - Subtractive port:  timeout is set for this I/O range
      //   - It is currently routed to an MBus device:  change Port to 0
      //   - Address ranges need to be merged
      switch (Descr->Type)  {

        case IOD_SC:
          HitFlag = 1;
          // Does the I/O range overlap an existing Swiss-cheese descriptor ?
          if ((USHORT)Descr->MsrData[0] == (Address & 0xFFF8)) {
            UCHAR CurrentByteEnables, NewByteEnables, CommonByteEnables;

            // Yes, compute the byte enables
            Mask = Compute_IOD_SC(AddressPtr, RangePtr, 1);
            CurrentByteEnables = (UCHAR)(Descr->MsrData[0] >> 24);
            NewByteEnables = (UCHAR)(Mask >> 24);

            // Compute # bytes of overlap
            CommonByteEnables = CurrentByteEnables & NewByteEnables;
            Length = 0;
            while (CommonByteEnables) {
              if (CommonByteEnables & 1) {
                Length++;
              }
              CommonByteEnables >>= 1;
            }

            switch (Enable) {

              case 0:
                if (CurrentByteEnables == NewByteEnables) {
                  if (Descr->MsrData[0] == Mask) {
                    // Entire range is being disabled
                    Deallocate_Descriptor(Descr);
                  } else {
                    // Only change REN/WEN, not byte enables 
                    Mask &= 0x00FFFFFF;
                  }
                } else {
                  // Only subset of range is being disabled
                  // Don't change REN/WEN
                  Mask &= ~(WEN | REN);
                }
                if (Length) {
                  // Don't change base address
                  (USHORT)Mask = 0x0000;
                  Descr->MsrData[0] &= ~Mask;
                } else {
                  HitFlag = 0;
                }				       
                break;

              case 1:
                if ((Descr->MsrData[0] & (WEN | REN)) != (Mask & (WEN | REN))) {
                  // R/W attributes mismatch; descriptor is not compatible
                  HitFlag = 0;
                } else {
                  // Descriptors are compatible: set additional byte enables
                  Descr->MsrData[0] |= Mask;
                }
                break;

              case 2:
                if ((Descr->MsrData[0] & (WEN | REN)) != (Mask & (WEN | REN))) {
                  // R/W attributes mismatch.
                  // Descriptor is not compatible unless byte enables are the same.
                  if (CurrentByteEnables != NewByteEnables) {
                    HitFlag = 0;
                    break;
                  }
                }

                // Descriptors are compatible: set additional byte enables
                Descr->MsrData[0] |= Mask;

                // If the descriptors overlap but not exactly (disjoint enables)
                // then Address & Range need to be handled according to special cases
                if (CurrentByteEnables != NewByteEnables) {
                  // Restore Address & Range (modified above by Compute_IOD_SC)
                  * (USHORT *)AddressPtr = Address;
                  * RangePtr = Range;

                  if (NewByteEnables > CurrentByteEnables) {
                    // For example:  01110000 - NewByteEnables
                    //               00011100 - CurrentByteEnables
                    // Increment Address by overlapped range
                    * (USHORT *)AddressPtr += Length;
                  }

                  // Decrement Range by overlapped range
                  * RangePtr -= Length;
                }
                break;

              case 3:
                if (CurrentByteEnables & NewByteEnables) {
                  return Index;
                }
                continue;

            } // end switch(Enable)

            Write_MSR(Descr->MsrAddr, Descr->MsrData);

            if (HitFlag == 0) {
              // Restore Address/Range.
              * (USHORT *)AddressPtr = Address;
              * RangePtr = Range;
              continue;
            } else {
              return Index;
            }
          }
          break;

        case IOD_BM:
          HitFlag = 0;
          Mask = ~((USHORT)Descr->MsrData[0])+1;
          Length = 1 << BitScanForward(Mask);
          StartRange = Descr->Address;
          EndRange = StartRange + Length;

          // Check if the requested range is covered by the existing descriptor
          if ((Address >= StartRange) && (Address < EndRange)) {
            if (Enable == 3) {			   
              return Index;
            }
            HitFlag = 1;
            Range -= Address - StartRange;
          }


          Mask = Address ^ StartRange;

          if (Enable) {
            if (HitFlag) {
              Length = Range;
              break;  //??????????????????????????
            }
          } else {

            // Range is being disabled
            if (HitFlag) {
              if (Length <= Range) {
                Deallocate_Descriptor(Descr);
              } else {
                // Descriptor must be trimmed
                if (Address == StartRange) {
                  // I/O Base needs to be adjusted up
                  Descr->MsrData[0] += (ULONG)Range << 20;
                  Mask = (1L << Range) - 1;
                  Length = Range;
                } else {
                  // Adjust IO_MASK to trim range down
                 (USHORT)Descr->MsrData[0] &= ~(Range-1);
                }

                // Adjust IO_MASK
                (USHORT)Descr->MsrData[0] |= Mask;
                HitFlag = 2;
              }
            }
          }

          // Recompute start address of descriptor
          if (HitFlag == 2) {
            Descr->Address  = (USHORT)(Descr->MsrData[0] >> 20);
            Descr->Address |= (USHORT)(Descr->MsrData[1] << 12);
          }

          if (HitFlag) {
            if (Length > Range) {
              Length = Range;
            }
            * AddressPtr += Length;
            * RangePtr -= Length;

            // Write descriptor if matching descriptor was modified
            if (HitFlag == 2) {
              Write_MSR(Descr->MsrAddr, Descr->MsrData);
            }
            return Index;
          }
          break;


      }  // end switch(Descr->Type)
  }

  // If a descriptor gets deallocated, need to clean up any possible links to a BAR
  return DESCRIPTOR_NOT_FOUND;
}





