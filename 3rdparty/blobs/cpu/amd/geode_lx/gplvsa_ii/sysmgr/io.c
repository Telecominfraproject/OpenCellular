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
//*     Handles trapped and virtualized I/O  




#include "VSA2.H"
#include "PROTOS.H" 
#include "SYSMGR.H"
 

// External variables
extern ULONG Saved_EAX, Saved_EDI;
extern ULONG Nested_EAX, Nested_EDI;
extern Descriptor Saved_ES, Flat_Descriptor;
extern SmiHeader SMM_Header;

// External routines
extern void pascal write_flat_size(ULONG, ULONG, UCHAR);
extern ULONG pascal Convert_To_Physical_Addr(ULONG);

#define PAGING_ENABLED 0x80000001

#define ADDR32     1
#define DATA32     2


typedef struct {
  union {
    ULONG Ulong;
    struct {    
      USHORT LowAddr;
      UCHAR MidAddr;
      UCHAR HiAddr;
    };
  };
} DESCR;



//***********************************************************************
// Returns the physical address of the location to return the value 
//***********************************************************************
ULONG pascal Get_Logical_Address(SmiHeader * SmiHdr, ULONG * CR0_Ptr)
{ UCHAR Override = 0, Size, Instruction;
  static UCHAR SizeToIncr[16] = {0,1,0,2,0,0,0,0,0,0,0,0,0,0,0,4};
  ULONG EDI, Logical_Addr, Incr, Phys_Addr, Reg_Ptr;
  register Descriptor * Callers_ES;
  DESCR Mem_Ptr;


  Size = (UCHAR)SmiHdr->data_size;

  if (SmiHdr == &SMM_Header) {
    Reg_Ptr = SysMgr_VSM + (ULONG)((USHORT)&Saved_EAX);
    // Get ES:EDI in case it's INSW
    Callers_ES = &Saved_ES;
    EDI = Saved_EDI;
  } else {
    // Nested_EAX is a flat ptr to EAX on the VSM's stack
	Reg_Ptr = Nested_EAX;
    // Get ES:EDI in case it's INSW
    Callers_ES = &Flat_Descriptor;
	EDI = Nested_EDI;
  }


  // Determine default operand and address size
  if (Callers_ES->attr & D_BIT) {
    Override = DATA32 | ADDR32;
  }


  // Get address of trapped code
  Logical_Addr = SmiHdr->Current_EIP + SmiHdr->_CS.base;

  do {

     // If paging is enabled...
    if ((SmiHdr->r_CR0 & PAGING_ENABLED) == PAGING_ENABLED){
      // translate logical to physical address via the page tables
      Phys_Addr = Convert_To_Physical_Addr(Logical_Addr);
    } else {
      Phys_Addr = Logical_Addr;
    }


    // Get opcode
    Instruction = (UCHAR)read_flat(Phys_Addr);

    switch (Instruction) {

      case 0xE4:    // IN AL, <nn>
      case 0xE5:    // IN AX, <nn>
      case 0xEC:    // IN AL, DX
      case 0xED:    // IN AX, DX
        *CR0_Ptr = 0x00000000;
        return Reg_Ptr;
	 
      case 0x67:   // Address Size
        Override ^= ADDR32;
        break;

      case 0x66:   // Data Size
        // Override ^= DATA32;
      case 0xF2:   // REPNE
      case 0xF3:   // REPE
        break;

      case 0x6C:   // INSB
      case 0x6D:   // INSW/INSD
        Mem_Ptr.LowAddr = Callers_ES->base_15_0;
        Mem_Ptr.MidAddr = Callers_ES->base_23_16;
        Mem_Ptr.HiAddr  = Callers_ES->base_31_24;

        // (E)DI has already been inc'd/dec'd
        Incr = SizeToIncr[Size];
        if (SmiHdr->EFLAGS & EFLAGS_DF) {
          EDI += Incr;
        } else {
          EDI -= Incr;
		}

        // If 16-bit mode, mask 16 MSBs of ptr
        if (!(Override & ADDR32)) {
          EDI &= 0x0000FFFF;
        }
        Mem_Ptr.Ulong += EDI;
        *CR0_Ptr = SmiHdr->r_CR0;
        return Mem_Ptr.Ulong;

      default:
        // Unexpected opcode
        Log_Error("Unexpected opcode at 0x%08X = 0x%08X", Phys_Addr, read_flat(Phys_Addr));
        return 0xFFFFFFFF;
    }

    // Increment code ptr
    Logical_Addr++;

  } while (1);

}




//***********************************************************************
// Returns an I/O value to AL/AX/EAX or ES:(E)DI
//***********************************************************************
void pascal Return_Virtual_Value(SmiHeader * SmiHdr, ULONG Data)
{ ULONG PhysicalAddr, LogicalAddr, CR0;
  UCHAR Size;

  LogicalAddr = Get_Logical_Address(SmiHdr, &CR0);

  if (LogicalAddr == 0xFFFFFFFF)
    return;

  Size = (UCHAR)SmiHdr->data_size;


  if ((CR0 & PAGING_ENABLED) == PAGING_ENABLED){
    // If paging is enabled, translate logical to physical address and
    // write one byte at a time since buffer could cross page boundary.
    do {
      PhysicalAddr = Convert_To_Physical_Addr(LogicalAddr++);
      write_flat_size(PhysicalAddr, Data, BYTE_IO);
      Data >>= 8;
      Size >>= 1;
    } while (Size);

  } else {
    // Write the data to memory
    write_flat_size(LogicalAddr, Data, Size);
  }
}
