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
//*     Implements the virtual register handler 
//******************************************************************************

#include "VSA2.H"
#include "PROTOS.H" 
#include "SYSMGR.H"
#include "VR.H"
#include "PCI.H" 
#include "CHIPSET.H"

// External variables
extern ULONG Saved_EAX;
extern USHORT Saved_AX;
extern SmiHeader SMM_Header;
extern ULONG IRQ_Mask;
extern ULONG Audio_IRQ;
extern ULONG NativeAudioStatus;
extern ULONG MsgPacket[];
extern ULONG VSM_Ptrs[];
extern Hardware	HardwareInfo;


// External function prototypes
extern void pascal Send_Synchronous_Event(EVENT, SmiHeader *);
extern void pascal VR_Miscellaneous_Write(UCHAR);
extern void pascal Return_Virtual_Value(SmiHeader *, ULONG);
extern ULONG pascal VR_Miscellaneous_Read(UCHAR);




//***********************************************************************
// Initialize virtual register trapping
//***********************************************************************
void Init_Virtual_Regs(void) 
{ USHORT PCI_Addr;
  ULONG Enable;


  // Allocate a 4-byte I/O BAR0 associated with Northbridge header
  PCI_Addr = Allocate_BAR(RESOURCE_SCIO, BAR0, 4, 0x0000, HardwareInfo.CPU_ID);

  // Set the base address  
  Virtual_PCI_Write_Handler(PCI_Addr, DWORD_IO, VRC_INDEX);

  // Enable the I/O space
  PCI_Addr = (PCI_Addr & 0xFF00) + COMMAND;
  Enable = Virtual_PCI_Read_Handler(PCI_Addr);   
  Enable |= IO_SPACE;
  Virtual_PCI_Write_Handler(PCI_Addr, DWORD_IO, Enable);


}

//***********************************************************************
// Handles audio virtual registers when there is no audio VSM.
//***********************************************************************
void Handle_Audio_VRC(UCHAR Index, ULONG Trapped_Data, USHORT Trapped_Flags, UCHAR Size)
{ static USHORT AudioVRs[MAX_AUDIO+1] = {
    0x201,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xAC97,0xFFFF,0xFFFF,0xFFFF,0x04F0
  };



  // If illegal register, ignore it
  if (Index > MAX_AUDIO) {
    return;
  }

  if (Trapped_Flags & SMI_FLAGS_OUTPUT) {

    switch (Index) {

      // Read-only registers
      case CODEC_TYPE:
      case AUDIO_VERSION:
        return;

      // Native audio IRQ                
      case AUDIO_IRQ:

        // Set previous audio IRQ to external
        IRQ_Mask |= Audio_IRQ << 16;

        Audio_IRQ = 0;
        if ((UCHAR)Trapped_Data) {
          Audio_IRQ = 0x00000001L << (UCHAR)Trapped_Data;
        }
        break;

      // Native audio status pointer
      case STATUS_PTR:
        switch (SMM_Header.data_size) {

          case WORD_IO:
            (USHORT)NativeAudioStatus = (USHORT)Trapped_Data;
            break;

          case DWORD_IO:
            NativeAudioStatus = Trapped_Data;
            break;
        }
        break;
    }

    AudioVRs[Index] = (USHORT)Trapped_Data; 

  } else {
    // Return virtual register value
    Saved_AX = AudioVRs[Index];
    // STATUS_PTR may be read as a DWORD
    if (Index == STATUS_PTR && DWORD_IO == Size) {
      Saved_EAX = NativeAudioStatus;
    }
  }
}


typedef struct {
  union {
    USHORT ClassIndex;
    struct {
      UCHAR Index;
      UCHAR Class;
    };
  };
} CLASS_INDEX;


typedef struct {
  union {
    ULONG Dword;
    struct {
      USHORT Word0;
      USHORT Word1;
    };
    struct {
      UCHAR Byte0;
      UCHAR Byte1;
      UCHAR Byte2;
      UCHAR Byte3;
    };
  };
} DWORD;

#define VR_LOCKED   0
#define VR_UNLOCKED 1
#define VR_INDEXED  2

//***********************************************************************
// Handles accesses to Virtual Registers
//***********************************************************************
void VR_Handler(SmiHeader * SmiHdr)
{ USHORT Trapped_Addr;
  ULONG Data, SegOffset;
  UCHAR Trapped_Flags, IO_Write, Addr_2LSBs, Size;
  DWORD Trapped_Data;
  static UCHAR VR_LockState = 0;
  static CLASS_INDEX VirtReg;

  // Get info from SMM header
  Trapped_Addr  = (USHORT)SmiHdr->IO_addr;
  Trapped_Flags = (UCHAR)SmiHdr->SMI_Flags_Ushort;
  IO_Write      = (UCHAR)Trapped_Flags & SMI_FLAGS_OUTPUT ? 1 : 0;
  Size          = (UCHAR)SmiHdr->data_size;
  Addr_2LSBs    = (UCHAR)Trapped_Addr & 3;
  Trapped_Data.Dword  = SmiHdr->write_data;

  SegOffset  = (ULONG) SmiHdr->_CS.selector << 16;
  SegOffset += (USHORT)SmiHdr->Current_EIP;


  // Validate the I/O as a valid virtual register access
  switch (Size) {

    case DWORD_IO:
      // Allow unlock & index to occur in a single SMI
      if (VR_LockState == VR_LOCKED) {
        if (IO_Write && (VR_UNLOCK == Trapped_Data.Word1)) {
          VirtReg.ClassIndex = Trapped_Data.Word0;
          VR_LockState = VR_INDEXED;
          return;
        }
      }

    case BYTE_IO:
      // Byte I/O is illegal for virtual registers.  Start over.
      VR_LockState = VR_LOCKED;
      return;

  } // end switch(Size)


  switch(Addr_2LSBs) {

    // Virtual register INDEX
    case 0:

      switch (VR_LockState) {

        // Virtual registers have been unlocked & Class:Index has been written,
        // but VR class::index is being re-written.
        case VR_INDEXED:
          if (!IO_Write) {
            goto ReturnClassIndex;
          }
          VR_LockState = VR_LOCKED;

        // Virtual registers have not been unlocked yet
        case VR_LOCKED:
          if (IO_Write) {
            if (VR_UNLOCK == Trapped_Data.Word0) {
              // Virtual registers have been unlocked.
              // Advance to the next VR_LockState.
              VR_LockState = VR_UNLOCKED;
            }
          }
          return;
 
        // Virtual registers have been unlocked.
        // Next VR write should be Class::Index
        case VR_UNLOCKED:
          if (IO_Write) {

            // Record the VR index & advance to the next VR_LockState
            VirtReg.ClassIndex = Trapped_Data.Word0;


            // Report error if illegal VR class
            if (VirtReg.Class > MAX_VR_CLASS && VirtReg.Class < 0x80) {
              Log_Error("Invalid virtual register class 0x%02X", VirtReg.Class);
              goto VR_Error;
            }

            VR_LockState = VR_INDEXED;

          } else {
ReturnClassIndex:
            // Reading back Class::Index
            Return_Virtual_Value(SmiHdr, (ULONG)VirtReg.ClassIndex);
          }
          return;


      } // end switch (VR_LockState)

      Log_Error("Incorrect virtual register sequence");
      goto VR_Error;


    // Virtual register DATA
    case 2:
      if (VR_INDEXED == VR_LockState) {
        switch (VirtReg.Class) {

          case VRC_MISCELLANEOUS:

            if (IO_Write) {

              VR_Miscellaneous_Write(VirtReg.Index);

            } else {

              Data = VR_Miscellaneous_Read(VirtReg.Index);

              switch (VirtReg.Index) {
                case SIGNATURE:
                case HIGH_MEM_ACCESS:
                  (UCHAR)SmiHdr->data_size = DWORD_IO;

                case VSA_VERSION_NUM:
                case VSM_VERSION:
                  // Return virtualized PCI device value to the right environment                     
                  Return_Virtual_Value(SmiHdr, Data);
                  break;
              }
            }
            // If access was to an invalid MSR, ignore the SSMI_FLAG
            if (VirtReg.Index == MSR_ACCESS) {
              SmiHdr->SMI_Flags.IO_Trap = 0;
              SmiHdr->SMI_Flags.Ext_IO_Trap = 0;
            }
            break;


          default:

            // Prepare message packet
            MsgPacket[1] = (ULONG)VirtReg.ClassIndex;
            MsgPacket[2] = 0;        		// Assume I/O read
            MsgPacket[3] = 0;
            if (IO_Write) {
              (UCHAR) MsgPacket[2] = 1;		// I/O write
              (USHORT)MsgPacket[3] = Trapped_Data.Word0;
            }
            // Send EVENT_VIRTUAL_REGISTER to registered VSM(s)
            Send_Synchronous_Event(EVENT_VIRTUAL_REGISTER, SmiHdr);
            break;

        } // switch (Class)

      } else {
        static UCHAR * RdWr[] = {"Read", "Write"};

        Log_Error("%s of locked VR %02X:%02X at %04X:%08X ", RdWr[IO_Write], VirtReg.Class, VirtReg.Index, SmiHdr->_CS.selector, SmiHdr->Current_EIP);
VR_Error:


        // Ignore writes. Return 0's for reads.
        if (!IO_Write) {
          // NOTE: We know it is a WORD_IO from above validation.
          Saved_AX = 0;
        }
      }
      break;

    default:
      // Report mis-aligned access to virtual register
      Log_Error("Mis-aligned access to virtual register 0x04X", Trapped_Addr);
      break;
  }	// end switch(Addr_2LSBs)

  // Reset the VR state machine
  VR_LockState = VR_LOCKED;

}









						   