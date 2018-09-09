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

#define VSM_SIGNATURE		0x204D5356	// 'VSM '
#define VSA_VERSION		0x03B0		// SysMgr Version


#define BYTE_IO                0x01
#define WORD_IO                0x03
#define DWORD_IO               0x0F
#define IO_WRITE               0x80



typedef  unsigned char UCHAR;
typedef  unsigned long ULONG;
typedef  unsigned short USHORT;



#define HIWORD(p)  ((USHORT) ((p) >> 16))

typedef struct {
	unsigned short Alignment: 5;		// 2^(n+5)  (e.g. 00000 = 32-byte boundary)
	unsigned short LoadHi: 1;			// 1 = must load VSM above top of memory
	unsigned short LoadLo: 1;			// 1 = must load VSM below 1 MB
	unsigned short SkipMe: 1;			// 1 = Skip this VSM
	unsigned short Reserved: 8;
} Requirements;






//*********************************************************************
// 			Structures
//*********************************************************************

typedef struct {
  union {
    ULONG  Reg_EAX;
    USHORT Reg_AX;
	struct {
	  UCHAR Reg_AL;
	  UCHAR Reg_AH;
	};
  };
  union {
    ULONG  Reg_EBX;
    USHORT Reg_BX;
	struct {
	  UCHAR Reg_BL;
	  UCHAR Reg_BH;
	};
  };
  union {
    ULONG  Reg_ECX;
    USHORT Reg_CX;
	struct {
	  UCHAR Reg_CL;
	  UCHAR Reg_CH;
	};
  };
  union {
    ULONG  Reg_EDX;
    USHORT Reg_DX;
	struct {
	  UCHAR Reg_DL;
	  UCHAR Reg_DH;
	};
  };
  union {
    ULONG  Reg_EBP;
    USHORT Reg_BP;
  };
  union {
    ULONG  Reg_ESI;
    USHORT Reg_SI;
  };
  union {
    ULONG  Reg_EDI;
    USHORT Reg_DI;
  };
  USHORT Reg_DS;
  USHORT Reg_ES;
  USHORT Flags;
  UCHAR PIC0_Mask;
  UCHAR PIC1_Mask;			
} INT_REGS;



typedef struct {			// Used by SVDC & RSDC instructions
	USHORT	limit_15_0;
	USHORT	base_15_0;
	UCHAR	base_23_16;
	UCHAR	attr;
#define G_BIT				0x80
#define D_BIT				0x40

	UCHAR	limit_19_16;
	UCHAR	base_31_24;
	USHORT	selector;
} Descriptor;


typedef struct {
	ULONG	Reserved;
	ULONG	SMM_CTL_MSR;
	ULONG	write_data;
	USHORT	IO_addr;
	USHORT	data_size;
    union {
  	  struct {
	    USHORT CS_Writable: 1;
	    USHORT IO_Write:    1;
	    USHORT REP_Prefix:  1;
	    USHORT SMINT:       1;

	    USHORT HALT:        1;
	    USHORT Ext_IO_Trap: 1;
	    USHORT External:    1;
	    USHORT IO_Trap:     1;

	    USHORT Nested:      1;
	    USHORT Reserved:    6;
	    USHORT CS_Readable: 1;
	  } SMI_Flags;
	  USHORT SMI_Flags_Ushort;
	};
  
	USHORT SS_Flags;

    struct {
      ULONG limit;
      ULONG	 base;
      USHORT selector;
      USHORT attr;
	} _CS;

    union {
      USHORT Next_IP;
	  ULONG	Next_EIP;
    };
    union {
      USHORT Current_IP;
	  ULONG	Current_EIP;
    };
	ULONG	r_CR0;
	ULONG	EFLAGS;
	ULONG	r_DR7;
} SmiHeader;






typedef USHORT MSG;
typedef USHORT PRIORITY;
typedef USHORT EVENT;
typedef ULONG  VSM;





#define MAX_MSG_PARAM       4	// Parameter count
#define MAX_MSG_CNT	 	10 	// # entries in message queue

typedef struct {
   MSG Msg;				// Message code
   PRIORITY Priority;			// Priority
   ULONG From_VSM;				// VSM that sent the message
   ULONG Param[MAX_MSG_PARAM];	// Parameters
   ULONG Timestamp;			// Timestamp when message was entered
} Message;


typedef struct {

  // VSM's state
  SmiHeader State;				// SMM header for this VSM

  // NOTE: Flink field must be immediately after State structure
  ULONG Flink;				// Forward link to next VSM
  ULONG Blink;	 			// Backward link to previous VSM
  ULONG SavedESP;				// VSM's stack pointer
  ULONG SysMgr_Ptr;			// Ptr to SysMgr's InfoStuff structure
  ULONG Southbridge;			// PCI address of Southbridge


  // Statistics
  ULONG Adjustment;			// Clocks used by SMM entry/exit
  ULONG Clocks[2];				// Total clocks used by this VSM
  ULONG NumSMIs[2];			// Total SMI count for this VSM
  ULONG FrozenClocks[2]; 		// Copied from Clocks[]
  ULONG FrozenNumSMIs[2];		// Copied from NumSMIs[]
  ULONG StartTime[2];			// Timestamp @ start of timeslice
  ULONG StartClocks[2];		// Timestamp of last INFO /S

  // Floating Point
  UCHAR FPU_State[108];   		// Saved FPU state
  UCHAR FPU_Flag;              // Non-zero if FPU in use

  UCHAR RunFlag;				// VSM's scheduler state

  USHORT ResumeVector;			// Used by SaveToRAM
  USHORT Pad[5];

  // Message Queue
  USHORT EndMsgQ;	 	 		// Offset of *end* of message queue
  USHORT Qhead;				// Message queue head offset
  USHORT Qtail; 				// Message queue tail offset

  // MsgQueue is variable length, so it must be the last field
  Message MsgQueue[MAX_MSG_CNT];	// The VSM's message queue

} System;



//*********************************************************************
// 			VSM Header
//*********************************************************************

typedef struct  {
	ULONG  Signature;			// 'VSM '
	UCHAR  VSM_Type;   			// Type of VSM
	UCHAR  ForCPU;				// Required CPU  (FFFFh for any)
	USHORT ForChipset;			// Required companion I/O  (FFFFh for any)
	USHORT VSM_Version;			// Version of VSM
	ULONG  VSM_Length; 			// Length of VSM module in bytes
	USHORT EntryPoint;	 		// Offset of entry point
	ULONG  DS_Limit;
	Requirements Flag;			// Special requirements/capabilities
	USHORT VSA_Version;
	Descriptor _SS;				// SS: descriptor for this VSM
	Descriptor _DS;			 	// DS: descriptor for this VSM
	Descriptor _ES;				// ES: descriptor for this VSM
	USHORT AlignSystem;			// SysStuff must be DWORD aligned
	System SysStuff;			// Reserved for use by System Manager

} VSM_Header;






//*********************************************************************
// 			VSM Types
//*********************************************************************
#define VSM_SYS_MGR        0x00		// System Manager
#define VSM_AUDIO          0x01		// Xpress Audio
#define VSM_VGA            0x02		// SoftVGA
#define VSM_LEGACY         0x03		// Standard AT peripherals
#define VSM_PM             0x04		// Legacy Power Management
#define VSM_OHCI           0x05		// OHCI
#define VSM_i8042          0x06		// 8042 emulator
#define VSM_DEBUGGER       0x07		// SMI based debugger
#define VSM_ACPI           0x08		// Virtual ACPI
#define VSM_APM            0x09		// APM 1.2
#define VSM_OEM_ACPI       0x0A		// OEM ACPI customizations
#define VSM_SMB            0x0B		// System Management Bus
#define VSM_BATTERY        0x0C		// Battery controller
#define VSM_RTC            0x0D		// Virtual RTC
#define VSM_S2D            0x0E		// SaveToDisk
#define VSM_EXT_AMP        0x0F		// External audio amplifier
#define VSM_PCMCIA         0x10		// PCMCIA
#define VSM_SPY            0x11		// Spy. Receives ALL messages first.
#define VSM_NETWORK        0x12		// Network
#define VSM_GPIO           0x13		// GPIO handler
#define VSM_KEYBOARD       0x14		// USB keyboard to PC/AT emulation
#define VSM_MOUSE          0x15		// USB mouse to PS/2 emulation
#define VSM_USB            0x16		// Universal Serial Bus
#define VSM_FLASH          0x17		// FLASH
#define VSM_INFRARED       0x18		// Infrared
#define VSM_THERMAL        0x19		// Thermal monitor
#define VSM_NULL           0x1A		// Unspecified
#define VSM_MPEG           0x1B		// MPEG video decoder (EMMA)
#define VSM_VIP            0x1C		// Video processor (VIDEC)
#define VSM_LPC            0x1D		// Low Pin Count bus
#define VSM_VUART          0x1E		// Virtual UART
#define VSM_MICRO          0x1F		// MicroController
#define VSM_USER1          0x20		// USER 1
#define VSM_USER2          0x21		// USER 2
#define VSM_USER3          0x22		// USER 3
#define VSM_SYSINFO        0x23		// System Information
#define VSM_SUPERIO        0x24		// PM for SuperIO
#define VSM_EHCI           0x25		// EHCI
#define VSM_MAX_TYPE       VSM_EHCI	// Highest valid VSM type

#define VSM_ANY            0xFF		// Wildcard for SYS_BROADCAST
#define VSM_NOT_SELF       0x4000	// Flag used by SYS_BROADCAST
#define VSM_ALL_EXCEPT     0x8000	// Flag used by SYS_BROADCAST


//*********************************************************************
// SMINT codes used by non-VSA components (BIOS, INIT, etc.)
//*********************************************************************
#define SYS_BIOS_INIT          0x00F0   // VSA installation	from POST
#define SYS_DOS_INSTALL        0x00F2   // VSA installation from DOS prompt
#define SYS_VSM_INSTALL        0x00F3   // Install VSM dynamically
#define SYS_REMOVE             0x00F4   // Unregister events belonging to VSM
#define SYS_INT_RETURN         0x00F5   // Return from call to INT vector
#define SYS_RESUME_FROM_RAM    0x6789   // Resume from RAM
#define SYS_END_OF_POST        0x5000   // Issued by GeodeROM at end of POST
#define SYS_INT13_SMI          0x5001   // Issued by Int 13 module (USB floppy)
#define SYS_INT13CDR_SMI       0x5003   // Issued by Int 13 module (USB CD-ROM)
#define SYS_USB_DEVICE_SMI     0x7777   // Issued by USBBOOT.ROM to access device table

//*********************************************************************
//			Event Priorities
//*********************************************************************
#define NORMAL_PRIORITY        0x0000
#define MAX_PRIORITY           0x7FFF
#define BROADCAST_PRIORITY     0x9000
#define UNREGISTER_PRIORITY    0xFFF0

//*********************************************************************
//			Messages
//*********************************************************************

#define MSG_INITIALIZE		   0		// Perform VSM initialization
  #define EARLY_INIT           0
  #define END_OF_POST_INIT     1

#define MSG_SHUTDOWN           1		// Prepare for system shutdown (cold boot)
#define MSG_SAVE_STATE         2		// Save entire state of device(s) controlled by VSM
#define MSG_RESTORE_STATE      3		// Restore saved state of device(s) controlled by VSM
#define MSG_SET_POWER_STATE    4		// Set device(s) to specified power state
#define MSG_EVENT              5		// A registered  event has occurred
#define MSG_QUEUE_OVERFLOW     6		// The message queue is full.
#define MSG_WARM_BOOT          7		// Prepare for a warm boot
#define MSG_SET_POWER_MODE     9		// Restore saved state of device(s) controlled by VSM
#define MSG_ABORT_POWER_STATE  10		// Power state is to be aborted

//*********************************************************************
//			Events
//*********************************************************************

#define EVENT_GRAPHICS         1		// Video event
#define EVENT_AUDIO            2		// Audio event
#define EVENT_USB              3		// USB event
#define EVENT_ACPI             4		// ACPI register access
#define EVENT_ACPI_TIMER       5		// The ACPI timer expired
#define EVENT_IO_TRAP          6		// I/O trap
#define EVENT_IO_TIMEOUT       7		// I/O timeout
#define EVENT_PME              8		// Power Management
#define EVENT_KEL              9		// KEL
#define EVENT_VIDEO_INACTIVITY 0x0A	// Not supported in GX2
#define EVENT_GPIO             0x0B	// GPIO transition
  #define FALLING_EDGE          (1 << 0)
  #define RISING_EDGE           (1 << 1)
  #define BOTH_EDGES            (FALLING_EDGE | RISING_EDGE) 
  #define PME                   (1 << 2)
  #define DEBOUNCE              (1 << 3)
  #define PULLDOWN              (1 << 4)
  #define PULLUP                (1 << 5)
  #define INVERT                (1 << 6)
  #define OUTPUT                (1 << 7)
  #define OPEN_DRAIN            (1 << 8)
  // INPUT may be removed as if it is not OUTPUT it is assumed to be INPUT
  // this will have almost no impact on the code
  // Do Not use INPUT
  #define INPUT                 (1 << 9) 
  #define AUX1                  (1 << 10)
  #define AUX2                  (1 << 11)
  #define NO_ASMI               (1 << 12)
  #define PM1                   (1 << 13)
  #define GPE                   (1 << 14)
  #define NO_ENABLE             (1L << 15) // do not enable in GPE0_EN or PM1_EN
                                          // when using EVENT_PME
#define EVENT_SOFTWARE_SMI      0x0C	// Software SMI
#define EVENT_PCI_TRAP          0x0D	// PCI trap
#define EVENT_VIRTUAL_REGISTER  0x0E	// Virtual register access
#define EVENT_NMI               0x0F	// NMI
#define EVENT_TIMER             0x10	// Millisecond timer
#define EVENT_DEVICE_TIMEOUT    0x11	// Device timeout
#define EVENT_SEMAPHORE         0x12	// ACPI global lock
#define EVENT_VBLANK            0x13    // Vertical blank
#define EVENT_A20               0x14	// A20 mask toggled
#define EVENT_SMB               0x15	// SMB Controller
#define EVENT_RTC               0x16	// RTC Alarm
#define EVENT_THERMAL           0x17    // THRM pin
#define EVENT_LPC               0x18    // Low Pin Count bus
#define EVENT_UART              0x19
#define EVENT_BLOCKIO           0x1A
#define EVENT_PWM               0x1B
#define MAX_EVENT               EVENT_PWM

// Flags for event registration
#define WRITES_ONLY            (1L << 16)
#define READS_ONLY             (1L << 17)
#define GLIU_ID                (1L << 19)
#define NOT_GLIU0              (1L << 20)
#define NOT_GLIU1              (1L << 21)
#define NOT_GLIU2              (1L << 22)
#define ALL_GLIUS              (NOT_GLIU0 | NOT_GLIU1 | NOT_GLIU2)
// Flags used for EVENT_TIMER must be >= bit 24
#define ONE_SHOT               (1L << 24)
#define FOR_STANDBY            (1L << 25)


//*********************************************************************
//			Resources
//*********************************************************************
#define RESOURCE_MEMORY        0		// Physical Memory
#define RESOURCE_MMIO          1		// Memory mapped I/O
#define RESOURCE_IO            2		// I/O space
#define RESOURCE_SCIO          3		// Swiss-cheese I/O
#define RESOURCE_GPIO          4		// General-purpose I/O pin
#define RESOURCE_IRQ           5		// IRQ


  
//*********************************************************************
//			Macros
//*********************************************************************

#define SYS_GET_NEXT_MSG(p)                 sys_get_next_msg(p)
#define SYS_QUERY_MSG_QUEUE(p)              sys_query_msg_queue(p)
#define SYS_UNREGISTER_EVENT(e, p1, p2)     sys_register_event(e, p1, p2, UNREGISTER_PRIORITY)
#define SYS_REGISTER_EVENT(e, p1, p2, p)    sys_register_event(e, p1, p2, p)
#define SYS_PASS_EVENT(e, p1, p2, p3) 		 // no longer needed
#define SYS_VSM_PRESENT(vsm)                sys_vsm_present(vsm)
#define SYS_YIELD_CONTROL(p1)               sys_yield_control(p1)
#define SYS_SW_INTERRUPT(interrupt, regs)   sys_software_interrupt(interrupt, regs)
#define SYS_BROADCAST_MSG(msg, p, vsm)      sys_broadcast_msg(msg, p, vsm)
#define SYS_GET_SYSTEM_INFO(buffer)         sys_get_system_info(buffer)
#define SYS_REPORT_ERROR(err, info1, info2) sys_report_error(err, info1, info2)
#define SYS_GENERATE_IRQ(irq)               sys_generate_IRQ(irq)
#define SYS_UNLOAD_VSM()                    sys_unload_vsm()
#define SYS_LOGICAL_TO_PHYSICAL(addr)       sys_logical_to_physical(addr)
#define SYS_ALLOCATE_RESOURCE(f,p,q,r,s)    sys_resource(f, p, q, r, s)
#define SYS_DEALLOCATE_RESOURCE(f,p,q,r,s)  sys_resource((UCHAR)(f|0x80), p, q, r, s)
#define SYS_MBUS_DESCRIPTOR(addr, p)        sys_mbus_descriptor(addr, p, 0)
#define SYS_IO_DESCRIPTOR(addr, p)          sys_mbus_descriptor(addr, p, 3)
#define SYS_LOOKUP_DEVICE(id, i)            sys_lookup_device(id, i)
#define SYS_SAVE_STATE(buffer)              sys_state(0, buffer)
#define SYS_RESTORE_STATE(buffer)           sys_state(1, buffer)
#define SYS_SET_DECODE(addr, flag)          sys_address_decode(addr, flag)
#define SYS_MAP_IRQ(Source, Irq)            sys_map_irq(Source, Irq)
#define SYS_RETURN_RESULT(Data)             sys_return_result(Data)
#define SYS_DUPLICATE_VSM(MemModel)         sys_duplicate_vsm(MemModel)

#define READ_PCI_BYTE(addr)                 read_PCI_byte(addr)
#define READ_PCI_WORD(addr)                 read_PCI_word(addr)
#define READ_PCI_DWORD(addr)                read_PCI_dword(addr)
#define WRITE_PCI_BYTE(addr, data)          write_PCI_byte(addr, data)
#define WRITE_PCI_WORD(addr, data)          write_PCI_word(addr, data)
#define WRITE_PCI_DWORD(addr, data)         write_PCI_dword(addr, data)

#define WRITE_PCI_BYTE_NO_TRAP(addr, data)  write_PCI_no_trap(addr, (ULONG)data, BYTE_IO)
#define WRITE_PCI_WORD_NO_TRAP(addr, data)  write_PCI_no_trap(addr, (ULONG)data, WORD_IO)
#define WRITE_PCI_DWORD_NO_TRAP(addr, data) write_PCI_no_trap(addr, data, DWORD_IO)
#define READ_PCI_BYTE_NO_TRAP(addr)         ((UCHAR)read_PCI_no_trap(addr, BYTE_IO))
#define READ_PCI_WORD_NO_TRAP(addr)         ((USHORT)read_PCI_no_trap(addr, WORD_IO))
#define READ_PCI_DWORD_NO_TRAP(addr)        read_PCI_no_trap(addr, DWORD_IO)

#define WRITE_MEMORY(addr, data)            write_flat(addr, data)
#define READ_MEMORY(addr)                   read_flat(addr)

#define ENTER_CRITICAL_SECTION              EnterCriticalSection();
#define EXIT_CRITICAL_SECTION               ExitCriticalSection();



void __pascal sys_register_event(EVENT, ULONG, ULONG, USHORT);
void __pascal sys_software_interrupt(USHORT, INT_REGS *);
void __pascal sys_broadcast_msg(MSG, void *, USHORT);
void __pascal sys_yield_control(ULONG);
void __pascal sys_get_system_info(void *);
void __pascal sys_generate_IRQ(USHORT);
void __pascal sys_state(USHORT, void *);
void __pascal sys_address_decode(USHORT, USHORT);
void __pascal sys_map_irq(UCHAR, UCHAR); 
void __pascal sys_return_result(ULONG);
void __pascal sys_get_descriptor(USHORT, void *);
void __pascal sys_set_descriptor(USHORT, void *);
void __pascal sys_set_header_data(USHORT, ULONG);
void __pascal sys_set_register(USHORT, ULONG);
void __pascal sys_set_virtual_register(USHORT, USHORT);
void __pascal sys_report_error(USHORT, ULONG, ULONG);

void __pascal write_PCI_byte(ULONG, UCHAR);
void __pascal write_PCI_word(ULONG, USHORT);
void __pascal write_PCI_dword(ULONG, ULONG);
void __pascal write_PCI_no_trap(ULONG, ULONG, USHORT);

void EnterCriticalSection(void);
void ExitCriticalSection(void);
void sys_fast_path_return(void);
void sys_unload_vsm(void);
void __pascal sys_duplicate_vsm(USHORT);

void __pascal write_flat(ULONG, ULONG);
ULONG __pascal read_flat(ULONG);
UCHAR  __pascal sys_vsm_present(UCHAR);
USHORT __pascal sys_get_virtual_register(USHORT);
ULONG  __pascal sys_get_header_data(USHORT);
ULONG  __pascal sys_get_register(USHORT);
UCHAR  __pascal read_PCI_byte(ULONG);
USHORT __pascal read_PCI_word(ULONG);
ULONG  __pascal read_PCI_dword(ULONG);
ULONG  __pascal read_PCI_no_trap(ULONG, USHORT);
ULONG  __pascal sys_logical_to_physical(void *);
ULONG  __pascal sys_resource(UCHAR, USHORT, ULONG, USHORT, USHORT);
ULONG  __pascal sys_mbus_descriptor(USHORT, ULONG *, USHORT);
ULONG  __pascal sys_lookup_device(USHORT, USHORT);
MSG    sys_get_next_msg(void *);
MSG    sys_query_msg_queue(void *);

// Macros for use by VSMs to access the non-SMM context
#define SET_HEADER_DATA(reg, data)   sys_set_header_data(reg, data)
#define GET_HEADER_DATA(reg)         (ULONG)sys_get_header_data(reg)
#define GET_DESCRIPTOR(reg, buffer)  sys_get_descriptor(reg, buffer);
#define SET_DESCRIPTOR(reg, buffer)  sys_set_descriptor(reg, buffer);
#define GET_REGISTER(reg)            (ULONG)sys_get_register(reg)
#define SET_REGISTER(reg, data)      sys_set_register(reg, data)
#define SET_EAX(data)                SET_REGISTER(R_EAX, (ULONG) (data))
#define SET_AX(data)                 SET_REGISTER(R_AX,  (USHORT)(data))
#define SET_AL(data)                 SET_REGISTER(R_AL,  (UCHAR) (data))
#define SET_AH(data)                 SET_REGISTER(R_AH,  (UCHAR) (data))
#define SET_EFLAGS(data)             SET_HEADER_DATA(R_EFLAGS, data)
#define GET_EAX()                    ((ULONG) GET_REGISTER(R_EAX))
#define GET_AX()                     ((USHORT)GET_REGISTER(R_AX))
#define GET_AL()                     ((UCHAR) GET_REGISTER(R_AL))
#define GET_AH()                     ((UCHAR) GET_REGISTER(R_AH))
#define GET_EFLAGS()                 (GET_HEADER_DATA(R_EFLAGS))


// Macros for accessing virtual registers
#define GET_VIRTUAL_REGISTER(reg)          (USHORT)sys_get_virtual_register(reg)
#define SET_VIRTUAL_REGISTER(reg, data)    sys_set_virtual_register(reg, data)


//*********************************************************************
//			Error Codes
//*********************************************************************

#define ERR_UNDEF_EVENT		1		// Attempt to register an undefined event
//#define ERR_SCHEDULER		2		// Scheduler error
#define ERR_BAD_PARAMETER		3		// Illegal system call parameter
#define ERR_RESOURCE_CONFLICT	4		// Multiple VSMs requested conflicting resources
#define ERR_UNHANDLED_EVENT	5		// An event occurred that no VSM handled
#define ERR_INVALID_EVENT		6		// SysMgr attempted to send an invalid event
//#define ERR_TIME_LIMIT		7		// A VSM exceeded its allotted runtime
#define ERR_REGISTRATION_LOST	8		// A registered event is lost due to too many event registrations
#define ERR_HW_MISMATCH		9		// A VSM could not find the correct hardware (e.g. OHCI)
#define ERR_BAD_DESCRIPTOR		0x0A	// A descriptor in a VSM header is not valid
//#define ERR_MSG_QUEUE_FULL	0x0B	// A VSM's message queue is full
//#define ERR_MULTIPLE_EVENT   0x0C	// A VSM attempted to register the same event twice
#define ERR_UNREGISTRATION     0x0D	// A VSM attempted to unregister an event for which it was not registered
#define ERR_UNREGISTERED_EVENT 0x0E	// An event occurred for which no VSM is registered
#define ERR_MISALIGNED_IO		0x0F	// An misaligned I/O access occurred
//#define ERR_UNEXPECTED_EVENT 0x10    // A handler routine was passed an unexpected SMI event
//#define ERR_BAD_VSM			0x11	// A VSM header doesn't look right (no signature, etc.)
#define ERR_NESTED_ACCESS		0x12	// A VSM directly accessed a ACPI or virtual register
//#define ERR_BAD_MSG			0x13	// An attempt was made to send an illegal message code
//#define ERR_UNHANDLED_VIRTUAL 0x14	// An access was made by a VSM to an unhandled virtual register
#define ERR_BAD_INTERRUPT      0x15    // A VSM attempted to call an illegal INT vector
#define ERR_ILLEGAL_MACRO      0x16    // Illegal use of GET/SET_REGISTER or GET/SET_HEADER_DATA macros
#define ERR_UNDEF_SYS_CALL		0x17	// Undefined system call
//#define ERR_BAD_POWER_STATE	0x18	// Invalid power state
#define ERR_BAD_VR_ACCESS      0x19	// Access to undefined VR class by an application
#define ERR_UNDEF_VIRTUAL_REG	0x1A	// Access to undefined VR class by a VSM
//#define ERR_UNSUPPORTED_CHIPSET 0x1B // This chipset is not supported
#define ERR_PCI_TRAP           0x1C    // A VSM requested an unsupported PCI trap
#define ERR_RESOURCE_NOT_FOUND 0x1D    // A VSM requested an unsupported resource
#define ERR_NO_MORE_DESCRIPTORS 0x1E   // Out of MBIU descriptors
//#define ERR_INTERNAL_ERROR    0x1F   // System error, e.g. inconsistent data structure
#define ERR_DATA_STRUCTURE      0x20	// A system structure is too small






// Used as 2nd parameter to SYS_SET_DECODE macro:
#define POSITIVE_DECODE			1
#define SUBTRACTIVE_DECODE		0




//****************************************************************************************************
#define FROM_HEADER            0x2000
#define WORD_SIZE              0x4000
#define DWORD_SIZE             0x8000




// Field names for GET_REGISTER and SET_REGISTER macros
// These offsets must match the register order on the stack (PUSHAD)

#define R_DI			 0   + WORD_SIZE 
#define R_EDI			 0   + DWORD_SIZE
				
#define R_SI			 4   + WORD_SIZE
#define R_ESI			 4   + DWORD_SIZE

#define R_BP			 8   + WORD_SIZE
#define R_EBP			 8   + DWORD_SIZE
				
#define R_BL			16
#define R_BH			R_BL + 1
#define R_BX			R_BL + WORD_SIZE
#define R_EBX			R_BL + DWORD_SIZE

#define R_DL			20
#define R_DH			R_DL + 1
#define R_DX			R_DL + WORD_SIZE
#define R_EDX			R_DL + DWORD_SIZE
		
#define R_CL			24
#define R_CH			R_CL + 1
#define R_CX			R_CL + WORD_SIZE
#define R_ECX			R_CL + DWORD_SIZE

#define R_AL			28
#define R_AH			R_AL + 1
#define R_AX			R_AL + WORD_SIZE
#define R_EAX			R_AL + DWORD_SIZE

#define R_SP			32   + WORD_SIZE
#define R_ESP			32   + DWORD_SIZE


// Segment registers
// NOTE: offset points to .selector field
#define DESCRIPTOR_SIZE 10 // sizeof(Descriptor)
#define R_SS			36-2 + DESCRIPTOR_SIZE + WORD_SIZE
#define R_DS			R_SS + DESCRIPTOR_SIZE
#define R_ES			R_DS + DESCRIPTOR_SIZE
#define R_FS			R_ES + DESCRIPTOR_SIZE
#define R_GS			R_FS + DESCRIPTOR_SIZE

// Fields from SMM header
#define WRITE_DATA		0x08 + DWORD_SIZE + FROM_HEADER
#define IO_ADDRESS		0x0C + WORD_SIZE  + FROM_HEADER
#define DATA_SIZE		0x0E              + FROM_HEADER
#define SMM_FLAGS		0x10 + WORD_SIZE  + FROM_HEADER
#define CS_LIMIT		0x14 + DWORD_SIZE + FROM_HEADER
#define CS_BASE	   	0x18 + DWORD_SIZE + FROM_HEADER
#define CS_SELECTOR	0x1C + WORD_SIZE  + FROM_HEADER
#define CS_ATTR		0x1E + WORD_SIZE  + FROM_HEADER 
#define R_CS		0x1C + WORD_SIZE  + FROM_HEADER
#define R_IP		0x20 + WORD_SIZE  + FROM_HEADER
#define R_EIP		0x20 + DWORD_SIZE + FROM_HEADER
#define CURRENT_EIP  	0x24 + DWORD_SIZE + FROM_HEADER
#define R_CR0		0x28 + DWORD_SIZE + FROM_HEADER
#define R_EFLAGS   	0x2C + DWORD_SIZE + FROM_HEADER
  #define EFLAGS_CF    0x0001
  #define EFLAGS_ZF    0x0040
  #define EFLAGS_IF	0x0200
  #define EFLAGS_DF    0x0400
#define R_DR7 	   	0x30 + DWORD_SIZE + FROM_HEADER






typedef struct {
  ULONG  Chipset_Base;
  USHORT Chipset_ID;
  USHORT Chipset_Rev;
  USHORT CPU_ID;
  USHORT CPU_Revision;
  USHORT CPU_MHz;
  ULONG  SystemMemory;		// Units = bytes
  ULONG  VSA_Location;		// Physical location
  USHORT VSA_Size;			// Units = KB
  USHORT PCI_MHz;
  USHORT DRAM_MHz;
} Hardware;




