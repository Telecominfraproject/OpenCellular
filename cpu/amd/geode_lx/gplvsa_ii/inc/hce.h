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

typedef struct {
  union {
    unsigned long  HceUlong;
    unsigned short HceUshort;
	struct {
     unsigned short EmulationEnable: 1;
     unsigned short EmulationInterrupt: 1;
     unsigned short CharacterPending: 1;
     unsigned short IRQEn: 1;
     unsigned short ExternalIRQEn: 1;
     unsigned short GateA20Sequence: 1;
     unsigned short IRQ1Active: 1;
     unsigned short IRQ12Active: 1;
     unsigned short A20State: 1;
    };
  };
} HCE_CONTROL;


// Host Controller Operational Registers
typedef struct {
  unsigned long HcRevision;
  unsigned long HcControl;
  unsigned long HcCommandStatus;
  unsigned long HcInterruptStatus;
  unsigned long HcInterruptEnable;
  unsigned long HcInterruptDisable;
  unsigned long HcHCCA;
  unsigned long HcPeriodCurrentED;
  unsigned long HcControlHeadED;
  unsigned long HcControlCurrentED;
  unsigned long HcBulkHeadED;
  unsigned long HcBulkCurrentED;
  unsigned long HcDoneHead;
  unsigned long HcFmInterval;
  unsigned long HcFmRemaining;
  unsigned long HcFmNumber;
  unsigned long HcPeriodicStart;
  unsigned long HcLSThreshold;
  unsigned long HcRhDescriptorA;
  unsigned long HcRhDescriptorB;
  unsigned long HcRhStatus;
  unsigned long HcRhPortStatus[2];

  unsigned char  Reserved[0x100-0x54-2*4];	// Reserved for use by HC
  HCE_CONTROL HceControl;	// 0x100
  unsigned long  HceInput;			// 0x104
  unsigned long  HceOutput;			// 0x108
  unsigned long  HceStatus;			// 0x10C
} HCOR;


// HcInterruptStatus & HcInterruptDisable fields:
#define SO		0x00000001L		// Scheduling Overrun
#define WDH		0x00000002L		// HcDoneHead Writeback
#define SF		0x00000004L		// Start of Frame
#define RD		0x00000008L		// Resume Detect
#define UE		0x00000010L		// Unrecoverable Error
#define FNO		0x00000020L		// Frame Number Overflow
#define RHSC	0x00000040L		// Root Hub Status Change
#define OC		0x40000000L		// Ownership Change
#define MIE		0x80000000L		// Master Interrupt Enable


// HceControl fields:
#define EMULATION_ENABLE    0x01
#define EMULATION_INTERRUPT 0x02
#define CHARACTER_PENDING   0x04
#define IRQ_ENABLE          0x08
#define EXTERNAL_IRQ_ENABLE 0x10
#define GATE_A20_SEQUENCE   0x20
#define IRQ1_ACTIVE         0x40
#define IRQ12_ACTIVE        0x80
#define A20_STATE		   0x100

// HceStatus fields:
#define CMD_DATA			0x08

