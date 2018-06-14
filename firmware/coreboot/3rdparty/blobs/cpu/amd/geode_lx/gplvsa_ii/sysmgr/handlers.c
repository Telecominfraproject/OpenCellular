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
//*     Implements the handlers for top-level SMI source
//*****************************************************************************


#include "VSA2.H"
#include "CHIPSET.H"
#include "PROTOS.H" 
#include "SYSMGR.H"
#include "VPCI.H"
#include "PCI.H"

// External variables
extern SmiHeader SMM_Header;
extern SmiHeader Nested_Header;
extern ULONG Saved_EAX, Saved_EBX, Saved_ECX;
extern ULONG MsgPacket[];
extern ULONG VSM_ListHead;
extern ULONG Virtualized_PCI_Devices;
extern ULONG Stats_Sources;
extern ULONG IRQ_Mask;
extern ULONG MPCI_NB;
extern ULONG VSM_Buffer;
extern ULONG Nested_Flag;
extern ULONG Video_Sources;
extern USHORT Audio_Sources;
extern Hardware HardwareInfo;
extern EVENT_ENTRY Events[]; 
extern PCI_HEADER_ENTRY ISA_Hdr[];
extern PCI_HEADER_ENTRY HostBridge_Hdr[];


// External function prototypes
extern void pascal Timer_Handler(USHORT);
extern void CS5536_GPIO_Handler(ULONG);
extern void INT_Return(void);
extern void Send_OHCI_Event(UCHAR);
extern void VR_Handler(SmiHeader *);
extern void ACPI_Workaround(SmiHeader *, USHORT);
extern void Remove_RTC_Fix(void);
extern void set_reset_state(void);
extern void pascal Unblock_VSM(ULONG);
extern void pascal Return_Virtual_Value(SmiHeader *, ULONG);
extern SmiHeader * pascal Get_Header_Params(ULONG);
extern ULONG Get_ACPI_Status(ULONG *);
extern USHORT Get_Timeout(ULONG, UCHAR *);
extern USHORT CS5536_MFGPT_Handler(void);
extern void ReInit_Descriptors(void);
extern void Init_SysMgr();
extern void A20_Sync(void);
extern void Update_VSMs_CR0(void);


ULONG INT_Vectors[MAX_INT] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xF000F859};
ULONG VSM_Ptrs[VSM_MAX_TYPE+1];
ULONG Audio_IRQ = 0;
ULONG NativeAudioStatus = 0x4F0;   // Defaults to 0040:00F0
UCHAR End_of_POST = 0;








void Broadcast_SysMgr_Msg (MSG Message, UCHAR Param1)
{
  MsgPacket[0] = (ULONG)Param1; 
  MsgPacket[1] = MsgPacket[2] = 0; 
  Broadcast_Message(Message, VSM_ANY, 0x00000000);
}


//***********************************************************************
// Sends a message for an asynchronous event
//***********************************************************************
void pascal Send_Asynchronous_Event(EVENT Event)
{ 
  MsgPacket[1] = MsgPacket[3] = MsgPacket[4] = 00000000;
  if (Event != EVENT_IO_TIMEOUT) {
    MsgPacket[2] = 00000000;
  }
  Send_Event(Event, 0x00000000);
}



//***********************************************************************
// Sends a message for a synchronous event
// Returns TRUE if the event was registered.
//***********************************************************************
USHORT pascal Send_Synchronous_Event(EVENT Event, SmiHeader * SmiHdr)
{ ULONG Vsm;
  USHORT EventRegistered;

  if ((USHORT)SmiHdr == 0x0000 || (USHORT)SmiHdr == (USHORT)&SMM_Header) {
    Vsm = SysMgr_VSM;
  } else {
    Vsm = Current_VSM;
  }
  EventRegistered = Send_Event(Event, Vsm);
  if (!EventRegistered) {

    // No VSM is registered for this event
    // If nested event, change the VSM's state from 'Blocked' to 'Ready'
    if (Nested_Flag) {
      Unblock_VSM(Vsm);
    }
  }

  return EventRegistered;
}



//***********************************************************************
// This routine walks the VSM linked list, recording ptrs to VSMs that
// have special requirements.
//***********************************************************************
void Record_VSM_Locations(void)
{ ULONG VSM_Ptr;
  UCHAR VSM_Type;

  for (VSM_Type=0; VSM_Type<=VSM_MAX_TYPE; VSM_Type++) {
    VSM_Ptrs[VSM_Type] = 0x00000000;
  }
  VSM_Ptr = VSM_ListHead;

  while (VSM_Ptr) {

    VSM_Type = Get_VSM_Type(VSM_Ptr);

    if (VSM_Type < sizeof(VSM_Ptrs)/4) {
      VSM_Ptrs[VSM_Type] = VSM_Ptr;
    }
    if (VSM_Type == VSM_RTC) {
      Remove_RTC_Fix();
    }
    VSM_Ptr = GetFlink(VSM_Ptr);
  }
}



//***********************************************************************
// This routine handles software SMI events.
//***********************************************************************
void pascal SMINT_Handler(USHORT Code)
{ int i;

  // Handle return from INT callback
  if (VSM_Buffer) {
    INT_Return();
    return;
  }

  switch (Code) {

    case SYS_BIOS_INIT:

      // If installing VSA from DOS, restore descriptors to default state
      if (Saved_EBX != 0) {
        ReInit_Descriptors();
      }

      // VSA Initialization
      Init_SysMgr();
      if (Saved_EBX == 0) {
        break;
      }

    case SYS_END_OF_POST:
      Current_VSM = 0;

      End_of_POST = 1;
      //
      // Take a snapshot of the interrupt vectors
      //
      for (i = 0; i < MAX_INT; i++) {
        if (INT_Vectors[i] == 0) {
          INT_Vectors[i] = read_flat((ULONG) i << 2 );
        }
      }

      // Entry point to video ROM
      INT_Vectors[1] = 0xC0000003;

      //
      //  The BIOS may have enabled the changed the cache setting
      // since early init, so update each VSM's CR0 field
      //
      Update_VSMs_CR0();

      // Send a phase 1 initialization message to each VSM
      Broadcast_SysMgr_Msg(MSG_INITIALIZE, END_OF_POST_INIT);
      break;


    case SYS_REMOVE:
      Unregister_VSM_Events(Saved_ECX);
	  break;

    case SYS_VSM_INSTALL:

      // EBX points to the new VSM
      // ECX points to the old VSM
      Unregister_VSM_Events(Saved_ECX);

      Record_VSM_Locations();


      //
   	  // Send both phase 0 & 1 initialization messages to the new VSM
      //
	  MsgPacket[0] = EARLY_INIT;
	  MsgPacket[1] = 1;
      Send_Message(SysMgr_VSM, Saved_EBX, MSG_INITIALIZE);

	  MsgPacket[0] = END_OF_POST_INIT;
	  MsgPacket[1] = 1;
      Send_Message(SysMgr_VSM, Saved_EBX, MSG_INITIALIZE);
     break;

    default:
      //
      // Send event to appropriate VSM
      //
      MsgPacket[1] = (ULONG)Code;
      MsgPacket[2] = Saved_EBX;
      MsgPacket[3] = Saved_ECX;
      Send_Synchronous_Event(EVENT_SOFTWARE_SMI, 0);
      break;

  } // end switch
}


//***********************************************************************
// This routine handles graphics events.
//***********************************************************************
void VG_Handler(void)
{ register SmiHeader * SmiHdr;

  SmiHdr = Get_Header_Params(0);

  // Set bit 24 of MsgPacket[2] if I/O write
  if (SmiHdr->SMI_Flags.IO_Write) {
    MsgPacket[2] |= 0x01000000L;
  }
  MsgPacket[1] = Video_Sources;
  Send_Synchronous_Event(EVENT_GRAPHICS, SmiHdr);

  // Reset video event flags
  Video_Sources = 0;
}



//***********************************************************************
// This routine handles A20
//***********************************************************************
void A20_Handler(void)
{ 

  A20_Sync();

  // Send event so it will be recorded in the history buffer
  // Send_Synchronous_Event(EVENT_A20, SMM_Header);

}

//***********************************************************************
// This routine handles reset
//***********************************************************************
void Reset_Handler(void)
{ 


  // Schedule reset routine after VSMs have processes MSG_SHUTDOWN 
  Schedule_VSM((ULONG)((USHORT)set_reset_state));

  // Tell each VSM to get ready for cold boot
  // No VSMs use this message at this time, so don't broadcast message
  // (to keep overhead down)
  // Broadcast_SysMgr_Msg(MSG_SHUTDOWN, 0);


}

//***********************************************************************
// This routine handles NMI
//***********************************************************************
void NMI_Handler(void)
{
  Send_Asynchronous_Event(EVENT_NMI);
}





//***********************************************************************
// This routine handles trapped PCI events.
// The event may come from an SSMI (CPU) or an external SMI (chipset)
//***********************************************************************
void PCI_Handler(void)
{ ULONG ID_Select, Data;
  USHORT Address;
  UCHAR Size;
  register SmiHeader * SmiHdr;


  SmiHdr = Get_Header_Params(SMI_SRC_PCI_TRAP);
  Address = (USHORT)MsgPacket[2];

  Size = (UCHAR)SmiHdr->data_size;
  ID_Select = 1L << ((Address >> 11) & 0x1F);


  if (SmiHdr->SMI_Flags.IO_Write) {

    //
    // Trapped PCI header WRITE
    //
    Data = MsgPacket[3];

    // Is it is a totally virtual PCI header ?
    if (ID_Select & Virtualized_PCI_Devices) {
      MsgPacket[3] = Virtual_PCI_Write_Handler(Address, Size, Data);
      Address &= 0xFFFC;
      Size = DWORD_IO;
    }

    // Set the write flag
    Size |= IO_WRITE;

  } else {

    //
    // Trapped PCI header READ
    //

    // Is it is a totally virtual PCI header ?
    if (ID_Select & Virtualized_PCI_Devices) {

      Data = Virtual_PCI_Read_Handler(Address);

    } else {
      Trap_PCI_IDSEL(Address, 0);
      out_32(PCI_CONFIG_ADDRESS, 0x80000000 | Address);
      Data = in_32(PCI_CONFIG_DATA);
      Trap_PCI_IDSEL(Address, 1);
    }
    // Return virtualized PCI device value to the right environment
    Return_Virtual_Value(SmiHdr, Data);
    MsgPacket[3] = Data;
  }

  // Repackage the parameters
  MsgPacket[2] = MsgPacket[1] << 16;
  MsgPacket[1] = 0x80000000 + (USHORT)Address;
  (USHORT)MsgPacket[2] = Size;

  // Send EVENT_PCI_TRAP message
  if (!Send_Synchronous_Event(EVENT_PCI_TRAP, SmiHdr)) {
    // This PCI register was not trapped.
    // Re-issue configuration writes to real PCI hardware devices.
    if (Size & IO_WRITE) {
      if (!(ID_Select & Virtualized_PCI_Devices)) {
        USHORT PCI_Data_Reg;
        // Disable trapping for this device
        Trap_PCI_IDSEL(Address, 0);

        // Re-issue the configuration write to the h/w device
        out_32(PCI_CONFIG_ADDRESS, 0x80000000 | Address);
        PCI_Data_Reg = PCI_CONFIG_DATA + (Address & 3);
        switch (Size & ~IO_WRITE) {

          case BYTE_IO:
            out_8(PCI_Data_Reg, (UCHAR)Data);
            break;

          case WORD_IO:
            out_16(PCI_Data_Reg, (USHORT)Data);
            break;

          case DWORD_IO:
            out_32(PCI_Data_Reg, Data);
            break;
        }

        // Re-enable trapping for this device
        Trap_PCI_IDSEL(Address, 1);
      }
    }
  }

}




//***********************************************************************
// This routine handles USB1 events.
//***********************************************************************
void USB1_Handler(void)
{
  Send_OHCI_Event(1);

}

//***********************************************************************
// This routine handles USB2 events.
//***********************************************************************
void USB2_Handler(void)
{
  Send_OHCI_Event(2);
}


//***********************************************************************
// This routine handles the CS5536's KEL events
//***********************************************************************
void KEL_Handler(void)
{
  Send_OHCI_Event(1);
}



//***********************************************************************
// This routine handles a BLOCKIO event (PIO to ATA during UDMA).
//***********************************************************************
void BLOCKIO_Handler(void)
{ SmiHeader * SmiHdr;

  SmiHdr = Get_Header_Params(SMI_SRC_BLOCKIO);
  Send_Synchronous_Event(EVENT_BLOCKIO, SmiHdr);
}

//***********************************************************************
// This routine handles hits on MBus descriptors.
//***********************************************************************
void Descr_Hit_Handler(void)
{ USHORT Address;
  SmiHeader * SmiHdr;

  SmiHdr = Get_Header_Params(SMI_SRC_DESCR_HIT);

  // Ignore if one of the other sources of SSMI_FLAGS
  if (!SmiHdr->SMI_Flags.Ext_IO_Trap && !SmiHdr->SMI_Flags.IO_Trap) {
    return;
  }

  Address = (USHORT)SmiHdr->IO_addr;

  // Handle virtual registers
  if ((Address & 0xFFFC) == (HostBridge_Hdr[BAR0/4].Value_LO)) {
    if (SmiHdr == &SMM_Header) {
      // Handle virtual register
      VR_Handler(SmiHdr);
    } else {
      Report_VSM_Error(ERR_NESTED_ACCESS, 0, 0);
    }
    return;
  }

  // Handle workaround for PM Support registers
  if ((Address & (USHORT)ISA_Hdr[BAR4/4].Mask) == (ISA_Hdr[BAR4/4].Value_LO)) {
    ACPI_Workaround(SmiHdr, 0);
  }

  // Handle workaround for ACPI registers
  if ((Address & 0xFFE0) == (ISA_Hdr[BAR5/4].Value_LO)) {
    ACPI_Workaround(SmiHdr, 1);
  }
  // Send the event
  Send_Synchronous_Event(EVENT_IO_TRAP, SmiHdr);
}



//***********************************************************************
// This routine handles statistic counter ASMIs
//***********************************************************************
void StatCntr_Handler(void)
{ UCHAR StartIndex = 0;
  USHORT Address;
  ULONG SFlag;


  while (Stats_Sources) {
    SFlag = 1L << BitScanForward(Stats_Sources);

    Address = Get_Timeout(SFlag, &StartIndex);
    if (Address) {
      (USHORT)MsgPacket[2] = Address;
      Send_Asynchronous_Event(EVENT_IO_TIMEOUT);
    } else {
      // Clear status bit
      Stats_Sources &= ~SFlag;
    }
  }
}


//***********************************************************************
// This routine handles the Southbridge's PIC events
//***********************************************************************
void PIC_Handler(void)
{ USHORT ExpiredTimerMask, Timer;

  // Need to read PIC registers to determine if one of:
  //   USB1, USB2, S/W Generated, RTC Alarm, Audio, PM, NAND Flash, 
  //   SMB, KEL, UART1, UART2, MFGPT comparator, or GPIO.
  CS5536_GPIO_Handler(0);


  // Check if any MFGPT events occurred
  ExpiredTimerMask = CS5536_MFGPT_Handler();
  Timer = 0;
  while (ExpiredTimerMask) {
    if (ExpiredTimerMask & 1) {
      Timer_Handler(Timer);
    }
	Timer++;
	ExpiredTimerMask >>= 1;
  }
}



//***********************************************************************
// This routine handles the CS5536's ACPI events
//***********************************************************************
void ACPI_Handler(void)
{ SmiHeader * SmiHdr;

  SmiHdr = Get_Header_Params(0);

  // Handle mis-aligned access to the PM1_CNT register
  while ((UCHAR)SmiHdr->IO_addr != 0x08) {
    (UCHAR)SmiHdr->IO_addr++;
    (UCHAR)SmiHdr->data_size >>= 1;
    SmiHdr->write_data >>= 8;
  }
  if ((UCHAR)SmiHdr->data_size == 0x07) {
    (UCHAR)SmiHdr->data_size = WORD_IO;
  }


  Send_Synchronous_Event(EVENT_ACPI, SmiHdr);

}


//***********************************************************************
// This routine handles the CS5536's Power Management Events
//***********************************************************************
void PME_Handler(void)
{ 

  // Handle GPIOs that are routed to PM logic
  CS5536_GPIO_Handler(0);

  // Filter any false event caused by enabling PME
  if (Get_ACPI_Status(MsgPacket)) {
    Send_Synchronous_Event(EVENT_PME, 0);
  }
}


//***********************************************************************
// This routine handles events for which no other handler applies.
//***********************************************************************
void Leftover_Handler(void)
{

  // Report that the event was not handled.
  Log_Error("Unhandled event");

}





//*************************************************************************
//
//     The SMI_Sources table is used for determining the proper handler for
// a top-level SMI source.  Note that a single handler may process multiple
// SMI sources.   The order of entries in this table governs the order that
// the handlers will be invoked.   This is NOT the order that the VSMs will
// be given control.    Therefore, the order of entries is unimportant with
// respect to controlling priority.   However, in terms of finding  a match
// more quickly, the more frequent SMI events should  be  placed earlier in 
// the table.
//
//*************************************************************************

SMI_ENTRY Handler_Table[] = {
	PCI_Handler,		SMI_SRC_PCI_TRAP,
    VG_Handler,			SMI_SRC_VG,
	USB1_Handler,		SMI_SRC_USB1,
	USB2_Handler,		SMI_SRC_USB2,
	A20_Handler,		SMI_SRC_A20,
	Reset_Handler,		SMI_SRC_RESET,
	NMI_Handler,		SMI_SRC_NMI,


	Descr_Hit_Handler,  SMI_SRC_DESCR_HIT,
	PIC_Handler,		SMI_SRC_PIC,
	StatCntr_Handler,	SMI_SRC_STAT,
	KEL_Handler,		SMI_SRC_KEL,
	ACPI_Handler,		SMI_SRC_ACPI,
	PME_Handler,        SMI_SRC_PME,
	BLOCKIO_Handler,    SMI_SRC_BLOCKIO,

	Leftover_Handler,	0xFFFFFFFF,
};




						   