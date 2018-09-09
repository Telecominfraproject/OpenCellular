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



// Build flags for various optional features
#define HISTORY		0	// History support (value determines size of history buffer; 0=disabled)
#define CHECKED_BUILD		1	// Perform internal sanity checks

#define SUPPORT_CAPABILITIES	1	// Enables support for PCI capabilities list
#define SUPPORT_PRIORITY	0	// Enables message priority logic
#define SUPPORT_FS2		1	// Enables MBIU1 descriptors so FS2 sees same map as GX2

#define USB_FIX		0	// 0=none 1=old 2=new
#define MAX_INT		0x1B	// Maximum INT vector supported for BIOS callbacks


#define SYS_YIELD		0x40000000L	 // Must be >= bit 24

#define DBG_PORT		0x84
#define VSA_POST		0x84		// I/O port for VSA POST codes

#define EXTRA_SAVE		4		// State saved other than registers
#define VSM_STACK_FRAME	(8*4+EXTRA_SAVE)// PUSHAD + EXTRA_SAVE

#define SPECIAL_LOC		0xA80		// Determines depth of SysMgr's stack
#define STACK_OFFSET		0x94	// Should be: EndSaveArea - StartSaveArea
#define SYSMGRS_STACK		(SPECIAL_LOC-STACK_OFFSET)

#define MAX_REGISTRATIONS	100		// # entries in Events[]

// VSM specific definitions
#define CODE_ATTR		0x9B
#define DATA_ATTR		0x93
#define VSM_CR0		0x00000014
#define VSM_EFLAGS		0x00000002
#define VSM_DR7		0x00000400
#define VSM_STACK_SIZE		0x300		// Size of allocated stack in bytes
#define BIOS_STACK_SIZE	0x100

// VSM States
#define RUN_FLAG_INACTIVE	0x00		// VSM is idle
#define RUN_FLAG_SLEEPING	0x55		// VSM is in Standby/Suspend
#define RUN_FLAG_ACTIVE	0xAA		// VSM is running or scheduled to run
#define RUN_FLAG_WAITING	0xEE		// VSM has yielded control
#define RUN_FLAG_BLOCKED	0xBB		// VSM is blocked
#define RUN_FLAG_READY		0x77		// VSM is ready to execute


// System calls
#define SYS_CODE_EVENT		0x0000		// Event registration
#define SYS_CODE_YIELD		0x0001		// VSM is yielding control
#define SYS_CODE_SW_INT	0x0002		// Software Interrupt (INT xx)
#define SYS_CODE_PASS_EVENT	0x0003		// VSM did not handle an event
#define SYS_CODE_UNLOAD	0x0004		// Unload VSM
#define SYS_CODE_REGISTER	0x0005		// Get/Set special registers
#define SYS_CODE_PCI_ACCESS	0x0006		// Access a PCI dword with trapping disabled
#define SYS_CODE_SET_VIRTUAL	0x0007		// Set virtual register
#define SYS_CODE_GET_VIRTUAL	0x0008		// Get virtual register
#define SYS_CODE_BROADCAST	0x0009		// Broadcast a message to one or more VSMs
#define SYS_CODE_STATE		0x000A		// Save/Restore non-SMM state
#define SYS_CODE_ERROR		0x000B		// Report error
#define SYS_CODE_RESOURCE	0x000C		// Reserve resource
#define SYS_CODE_DECODE	0x000D		// Set resource to be subtractive/positive decode
#define SYS_CODE_DESCRIPTOR	0x000E		// Get descriptor of virtualized PCI BAR
#define SYS_CODE_LOOKUP	0x000F		// Lookup routing for MBus device
#define SYS_CODE_IRQ_MAPPER	0x0010		// Set IRQ mapping (CS5535 only)
#define SYS_CODE_RESULT	0x0011		// Return virtualized result
#define SYS_CODE_DUPLICATE	0x0012		// Duplicate a VSM
#define SYS_CODE_EXIT		0x0013		// Exit to System Manager

#define GET_REG		0x80
#define SET_REG		0x81
#define GET_HDR		0x82
#define SET_HDR		0x83
#define GET_DESCR		0x84
#define SET_DESCR		0x85


// Fields in SMM header flag
#define SMI_FLAGS_CS_WRITABLE	0x0001		// "Cw" bit  CS is writable
#define SMI_FLAGS_OUTPUT       0x0002		// "I" bit   I/O indicator
#define SMI_FLAGS_REP          0x0004		// "P" bit   REP indicator
#define SMI_FLAGS_SMINT        0x0008		// "S" bit	 SMI occured due to a SMINT
#define SMI_FLAGS_HALT         0x0010		// "H" bit   SMI occured during CPU halt
#define SMI_FLAGS_MEMORY       0x0020		// "M" bit   0=I/O, 1=memory
#define SMI_FLAGS_EXT	        0x0040		// "X" bit   External SMI source
#define SMI_FLAGS_VGA          0x0080		// "V" bit   VGA emulation source
#define SMI_FLAGS_NESTED       0x0100		// "N" bit   Nested SMI
#define SMI_FLAGS_CS_READABLE  0x8000		// "Cr" bit  CS is writable




typedef void (* SMI_Handler)(void);

typedef struct {
  SMI_Handler Handler;
  unsigned long SMI_Mask;
} SMI_ENTRY;



typedef struct {
  unsigned long   Vsm;

  union {
    struct {
      unsigned long Param1;
      unsigned long Param2;
      unsigned long Param3;
	};
	struct {		// Timers
      unsigned long Interval;
      unsigned short Handle;
      unsigned char Timer;
      unsigned char Attr;
      unsigned long RemainingInterval;
    };
	struct {		// GPIOs
	  unsigned short Pin;
	  unsigned short Pme;
	  unsigned short Attributes;
	  unsigned short Pm1;
	  unsigned long CurrentEdge;
    };
	struct {		// PCI header
	  unsigned short PCI_Addr;
	  unsigned short Unused;
	  unsigned short PCI_Mask;
	  unsigned short Flags;
    };
	struct {		// I/O trap & timeout
	  unsigned short IO_Base;
	  unsigned short IO_Timeout;
	  unsigned short IO_Range;
    };
	struct {		// Virtual Register
      unsigned long ClassLow;
      unsigned long ClassHigh;
    };

  };

  unsigned long Timestamp[2];
  unsigned char Index;
  unsigned char Link;
  unsigned short Priority;
} EVENT_ENTRY;

typedef struct {
  unsigned long Vsm;
  unsigned long Event;
  unsigned long Param1;
  unsigned long Param2;
  unsigned long Count;
  unsigned long TimeStamp[2];
} EVENT_HISTORY;


typedef struct {
  unsigned short History_Array;
  int * History_Start;
  int * History_End;
  int * History_Wrap;
  unsigned short HistoryEntries;
} HISTORY_INFO;
  






//
// An instance of this structure is found in the System Manager at offset SPECIAL_LOC
//
typedef struct {
  unsigned short Events;		// Events array
  unsigned short Descriptors;		// MBus Descriptors array
  unsigned short Vectors;		// INT vectors
  unsigned short HardwareInfo;		// Hardware structure
  unsigned long  IRQ_Base;		// Used by SYS_GENERATE_IRQ
  unsigned long  IRQ_Mask;		// Used by SYS_GENERATE_IRQ
  unsigned long  SysMgr_VSM;		// Used in SysMgr only
  unsigned long  SMI_Base;		// Used in SysMgr only
  unsigned short Header_Addr;		// Used in SysMgr only
  unsigned short SysMgr_Stack;		// Used in SysMgr only
  unsigned short MSRs;			// Used for DOS_BUILD
  unsigned short NumDescrs;		// Used for DOS_BUILD
} InfoStuff;
