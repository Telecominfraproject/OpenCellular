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


#define UNIMPLEMENTED_FUNCTION  0
#define UNIMPLEMENTED_REGISTER  1


struct vpci_header {
  unsigned char Reg;				   // Register offset (DWORD aligned)
  unsigned char Flag;				   // See flag fields below
  union {
    unsigned long Value;			   // Current value of register
    struct {
      unsigned short Value_LO;
      unsigned short Value_HI;
    };
	struct {
      unsigned short Vendor_ID;		   // Register 0x00
      unsigned short Device_ID;
    };
	struct {						   // Register 0x08
      unsigned char Revision_ID;
      unsigned char Interface;
	  union {
        struct {
          unsigned char SubClass;
          unsigned char BaseClass;
        };
        unsigned short Class;     
      };
    };
	struct {						   // Register 0x0C
      unsigned char CacheLineSize;
      unsigned char LatencyTimer;
      unsigned char HeaderType;
      unsigned char Bist;
    };
	struct {						   // BARs
	  union {
        unsigned short IO_Base;
        unsigned long  Memory_Base;
      };       
    };
	struct {						   // Register 0x3C
      unsigned char Interrupt_Line;
      unsigned char Interrupt_Pin;
      unsigned char Min_Gnt;
      unsigned char Max_Gnt;
    };
	struct {						   // EHCI register 0x54
      unsigned char EHCI_SMI_Enables;
      unsigned char reserved1;
      unsigned char EHCI_Errors;
      unsigned char reserved2;
    };
	struct {						   // EHCI register 0x60
      unsigned char SRBN;
      unsigned char FLADJ;
      unsigned short PORTWAKECAP;
    };
  };
  unsigned long Mask;				   // 0 = R/O   1 = R/W
  unsigned char LBar;				   // MSR offset of LBAR, if required
  unsigned char Link;				   // Index of 1st MSR in chain
  unsigned long WriteToClear;		   // 0 = N/A   1 = W/C

};

typedef struct vpci_header PCI_HEADER_ENTRY ;


//   Device                     MSRs used to implement a BAR
// ------------	   --------------------------------------------------------------
// Northbridge     MBIUx descriptor
// Graphics        MBIU0 descriptor    MBIU1 (for FS2)         RCONF
// Southbridge     LBAR
// IDE             MBIU2 descriptor	   Region config      
// OHCI            MBIU2 to OHCI       MBIU2 to KEL (P2D_BMK)  MDD LBAR

// PCI_HEADER_ENTRY.Flag definitions:
#define EOL         (1 << 7)   // End of list
#define PCI_EHCI    (1 << 6)   // EHCI (NOTE: leverages same bit as PCI_PM)
#define PCI_PM      (1 << 6)   // PCI Power Management
#define USE_BMK     (1 << 5)   // P2D_BMK needs Bizarro bit set
#define IO_BAR      (1 << 4)   // BAR is I/O
#define MEM_BAR     (1 << 3)   // BAR is memory
#define MMIO_BAR    (1 << 2)   // BAR is memory-mapped I/O
#define EPCI_W      (1 << 1)   // Embedded PCI: write to h/w
#define EPCI_R      (1 << 0)   // Embedded PCI: read from h/w
#define EPCI_RW     (EPCI_R | EPCI_W)

typedef PCI_HEADER_ENTRY * VIRTUAL_DEVICE;
typedef VIRTUAL_DEVICE *   VIRTUAL_PTR;

#define MAX_DESCR   110        // Max. # for all descriptor types in all MBIUs
#define DESCRIPTOR_NOT_FOUND 0



// NOTE: A copy of this structure exists in INIT.ASM
typedef struct {
  unsigned char Type;          // Type of MSR
  unsigned char Flag;          // See definitions below
  unsigned char Link;          // Link to next MSR
  unsigned char Split;         // Index of descriptor that was split

  unsigned short Owner;        // PCI Address this descriptor belongs to
  unsigned char Mbiu;          // MBUI on which this descriptor is located
  unsigned char Port;          // Port this descriptor routes to

  unsigned long MsrAddr;       // Routing address of MSR (descriptor/LBAR/RCONF)
  unsigned long Physical;      // Physical memory assigned  (00000000 if none)
  unsigned long Range;         // Actual I/O range for IOD_SC
  unsigned long MsrData[2];
  unsigned short Address;      // Address of I/O Trap or Timeout
} DESCRIPTOR;


// DESCRIPTOR.Flag definitions
#define IO_TIMEOUT  (1 << 2)   // Descriptor is used for I/O timeout
#define IO_TRAP     (1 << 1)   // Descriptor is used for I/O trap
#define AVAILABLE   (1 << 0)   // Descriptor is available





typedef struct {
  unsigned char NP2D_BM;
  unsigned char NP2D_BMO;
  unsigned char NP2D_R;
  unsigned char NP2D_RO;
  unsigned char NP2D_SC;
  unsigned char NP2D_SCO;
  unsigned char NP2D_BMK;
  unsigned char NIOD_BM;
  unsigned char NIOD_SC;
  unsigned char NPORTS;
  unsigned char NSTATS;
} CAPABILITIES;






// External function prototypes:
void pascal Read_MSR(unsigned long, unsigned long *);
void pascal Write_MSR(unsigned long, unsigned long *);
void pascal Write_MSR_LO(unsigned long, unsigned long);
void pascal MergeFields(unsigned long *, unsigned long, unsigned long, unsigned long);
void pascal Parse_Capabilities(unsigned long *, CAPABILITIES *);
void pascal Free_Descriptor(unsigned char Index);
void pascal Update_BAR(PCI_HEADER_ENTRY *, unsigned char);
unsigned char pascal Allocate_Descriptor(unsigned char, unsigned char, unsigned long);
unsigned long pascal Read_MSR_LO(unsigned long);
unsigned long pascal Find_MBus_ID(unsigned short, unsigned char);
