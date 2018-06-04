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
//*	  Function:                                                         *
//*     This file contains routines specific to the CS5536 UARTs. 

#include "vsa2.h"
#include "vr.h"
#include "mdd.h"
#include "protos.h"


// Function prototypes:
void Handle_5536_UART(UCHAR, UCHAR, USHORT);
USHORT Get_5536_UART(USHORT);
void Prog_5536_UART(USHORT, USHORT);

// External variables:
extern ULONG MDD_Base;
extern USHORT gpio_base;


// 5536 implements two UART's. The resources for both are controlled
// by MDD MSR_LEG_IO, MSR_IRQM_YHIGH.
// IRQ Enable, Clock freeze and soft reset controlled by MSR_UARTx_CONF
//
// One Virtual Register is implemented for each UART.
// bit15 = 1 Soft-Reset
// bits[14:13] = unused
// bit12 = 1 UART GPIO's have been setup by BIOS, RO bit
// bit11 = 1 enable UART I/O space extended register banks
// bit10 = 1 enable test mode
// bit9 = 1 Freeze device clocks, IRQ still enabled
// bit8 = 1 enable interrupt & functionality, doesn't affect I/O
// bits[7:4] = IRQ number
// bit3 = unused
// bit2 = 1 I/O decode enabled
// bits[1:0] = I/O location
//
void Handle_5536_UART(UCHAR vrIndex, UCHAR wFlag, USHORT wData)
{
	if (wFlag) {	// write
		Prog_5536_UART(vrIndex, wData);
	} else {
		SYS_RETURN_RESULT((ULONG)Get_5536_UART(vrIndex));
	}
}

USHORT Get_5536_UART(USHORT vri)
{
	ULONG d;
	USHORT u;
	
	u = 0;
		
	(USHORT)MDD_Base = MSR_LEG_IO;
	d = Read_MSR_LO(MDD_Base);
	switch (vri) {
		case VRC_CS_UART1:
			d >>= UART1_SHIFT;
			break;

		case VRC_CS_UART2:
			d >>= UART2_SHIFT;
			break;

		default:
			return 0;
	}

	u = (USHORT)d & UART_MASK;

	// get the IRQ number
	(USHORT)MDD_Base = MSR_IRQM_YHIGH;
	d = Read_MSR_LO(MDD_Base);
	if (vri == VRC_CS_UART1) {
		d >>= 20;
	} else {
		d >>= 24;
	}
	u |= ((USHORT)d & 0x00F0);

	// fill in the other flags from MSR_UARTx_CONF
	if (vri == VRC_CS_UART1) {
		(USHORT)MDD_Base = MSR_UART1_CONF;
	} else {
		(USHORT)MDD_Base = MSR_UART2_CONF;
	}
	d = Read_MSR_LO(MDD_Base);
	// MSR bit0 = soft-reset -> bit15
	// MSR bit1 = deven -> bit8
	// MSR bit2 = freeze -> bit9
	// MSR bit3 = test mode -> bit10
	// MSR bit4 = upper banks -> bit11
	if ((USHORT)d & 0x0001) {
		u |= 0x8000;
	}
	d <<= 7;
	u |= ((USHORT)d & 0x0F00);

	// check if GPIO's for this UART have been setup
	// This is used as an indicator the UART is 'hidden' from OS
	d = in_32(gpio_base + GPIO_IN_AUX1_SELECT);
	if (vri == VRC_CS_UART1) {
		// UART1 GPIO9 should be IN_AUX
		if (d & 0x0200L) {
			u |= 0x1000;	// set bit12
		}
	} else {
		// UART2 GPIO3 should be IN_AUX
		if (d & 0x0008L) {
			u |= 0x1000;	// set bit12
		}
	}

	return u;	
}


void Prog_5536_UART(USHORT vri, USHORT vrval)
{
	ULONG d, r;
	USHORT curval, uartmsr, shift, irqshift;

	r = 0L;

	// clear reserved/unused bits
	vrval &= 0x8FF7;

	switch (vri) {
		case VRC_CS_UART1:
			uartmsr = MSR_UART1_CONF;
			shift = UART1_SHIFT;
			irqshift = 24;
			break;

		case VRC_CS_UART2:
			uartmsr = MSR_UART2_CONF;
			shift = UART2_SHIFT;
			irqshift = 28;
			break;

		default:
			return;
	}

	// get current settings
	curval = Get_5536_UART(vri);

	// change?
	if (curval ^ vrval) {	// Yes
		// I/O change?
		if ((curval & 0x0007) ^ (vrval & 0x0007)) {
			// Program I/O
			(USHORT)MDD_Base = MSR_LEG_IO;
			d = Read_MSR_LO(MDD_Base);
			// don't trust C compiler when expanding a 'define' to
			// long unless 'define' is a long constant.
			(USHORT)r = UART_MASK;
			d &= ~(r << shift);
			d |= ((ULONG)(vrval & 0x0007)) << shift;
			Write_MSR_LO(MDD_Base, d);
		}

		// IRQ change?
		if ((curval & 0x00F0) ^ (vrval & 0x00F0)) {				 
			// Program IRQ
			(USHORT)MDD_Base = MSR_IRQM_YHIGH;
			d = Read_MSR_LO(MDD_Base);
			d &= ~(0x0FL << irqshift);
			d |= ((ULONG)(vrval & 0x00F0)) << (irqshift-4);
			Write_MSR_LO(MDD_Base, d);
		}

		// control bit(s) changed?
		if ((curval & 0x8F00) ^ (vrval & 0x8F00)) {
			// Program control bits in MSR_UARTx_CONF
			// bits[11:8] of vr map to bits[4:1] of MSR
			// bit[15] of vr maps to bit[0] of MSR
			(USHORT)MDD_Base = uartmsr;
			d = Read_MSR_LO(MDD_Base);
			// clear bits[4:0]
			d &= ~(0x1FL);
			(USHORT)d |= ((vrval & 0x0F00) >> 7);
			if (vrval & 0x8000) {
				(USHORT)d |= 0x0001;
			}
			Write_MSR_LO(MDD_Base, d);
		}
	}
	
}



