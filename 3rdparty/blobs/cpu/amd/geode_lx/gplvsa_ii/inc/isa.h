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

// Real Time Clock (RTC) definitions
#define CMOS_INDEX			0x70
#define CMOS_DATA			CMOS_INDEX+1
#define	CMOS_SECONDS		0x00
#define	CMOS_MINUTES		0x02
#define	CMOS_HOURS			0x04
#define	CMOS_DAY			0x07
#define	CMOS_MONTH			0x08
#define	CMOS_YEAR			0x09
#define	CMOS_STATUS_A		0x0A
  #define UIP				0x80
#define	CMOS_STATUS_B		0x0B
  #define SET				0x80
  #define PI 				0x40
  #define AI 				0x20
  #define UI 				0x10
  #define SQUARE			0x08
  #define DM 				0x04
  #define HOUR24			0x02
  #define DLS				0x01
#define	CMOS_STATUS_C		0x0C
  #define IRQ				0x80
  #define PS 				0x40
  #define AS 				0x20
  #define US 				0x10
#define	CMOS_CENTURY		0x32



// Programmable Interrupt Controller (PIC) definitions
#define PIC1_BASE			0x20
#define PIC1_MASK			PIC1_BASE+1
#define PIC2_BASE			0xA0
#define PIC2_MASK			PIC2_BASE+1
#define NONSPECIFIC_EOI		0x20
#define SPECIFIC_EOI		0x60
#define PIC1_EDGE			0x4D0
#define PIC2_EDGE			0x4D1




#define         PIC1_ICW1       0x20
#define         PIC1_ICW2       0x21
#define         PIC1_ICW3       0x21
#define         PIC1_ICW4       0x21
#define         PIC1_OCW1       0x21
#define         PIC1_OCW2       0x20
#define         PIC1_OCW3       0x20
#define         PIC2_ICW1       0xA0
#define         PIC2_ICW2       0xA1
#define         PIC2_ICW3       0xA1
#define         PIC2_ICW4       0xA1
#define         PIC2_OCW1       0xA1
#define         PIC2_OCW2       0xA0
#define         PIC2_OCW3       0xA0


// DMA definitions
#define TRANSFER_MASK  (0x0C)
  #define DMA_VERIFY   (0x00)
  #define DMA_WRITE    (0x04)
  #define DMA_READ     (0x08)
#define MODE_MASK      (0xC0)
  #define MODE_DEMAND  (0x00)
  #define MODE_SINGLE  (0x40)
  #define MODE_BLOCK   (0x80)
  #define MODE_CASCADE (0xC0)

#define         DMA1_ADDR0      0x00
#define         DMA1_CNT0       0x01
#define         DMA1_ADDR1      0x02
#define         DMA1_CNT1       0x03
#define         DMA1_ADDR2      0x04
#define         DMA1_CNT2       0x05
#define         DMA1_ADDR3      0x06
#define         DMA1_CNT3       0x07
#define         DMA1_MODE       0x0B
#define         DMA1_CPTR       0x0C
#define         DMA1_MASK       0x0F
#define         DMA2_ADDR0      0xC0
#define         DMA2_CNT0       0xC2
#define         DMA2_ADDR1      0xC4
#define         DMA2_CNT1       0xC6
#define         DMA2_ADDR2      0xC8
#define         DMA2_CNT2       0xCA
#define         DMA2_ADDR3      0xCC
#define         DMA2_CNT3       0xCE
#define         DMA2_MODE       0xD6
#define         DMA2_CPTR       0xD8
#define         DMA2_MASK       0xDE
#define         DMA_PAGE        0x80
#define         DMA_HPAGE       0x480



// Programmable Interval Timer (PIT) definitions
#define         PIT_CTR0        0x40
#define         PIT_CTR1        0x41
#define         PIT_CTR2        0x42
#define         PIT_CMD         0x43
#define         PIT_CMD_BOTH_BYTES      0x30 // Sets CMD word to read/write both bytes





#define	READ_IRR			0x0A
#define	READ_ISR			0x0B

// Keyboard controller registers
#define KYBD_DATA			0x60
#define KYBD_STATUS			0x64
  #define STAT_OBF			  0x01
  #define STAT_IBF			  0x02
  #define STAT_FLAG			  0x04
  #define STAT_CMD			  0x08
  #define STAT_INHIBIT 		  0x10
  #define STAT_AUX_OBF		  0x20
  #define STAT_TIMEOUT		  0x40
  #define STAT_PARITY		  0x80
#define KYBD_COMMAND		0x64

// Keyboard controller output port bits
#define KYBD_SYSR			0x01			// Processor reset
#define KYBD_GA20			0x02			// Gate A20 (1=on)
#define KYBD_ADAT			0x04			// AUX data    
#define KYBD_ACLK			0x08			// AUX clock
#define KYBD_KOBF			0x10			// Keyboard OBF
#define KYBD_AOBF			0x20			// AUX OBF
#define KYBD_KCLK			0x40			// Keyboard clock
#define KYBD_KDAT			0x80			// Keyboard data

// Keyboard controller commands
#define KYBD_RD_OUT			0xD0
#define KYBD_WR_OUT			0xD1

// Keyboard controller command byte
#define OBF_INTERRUPT		0x01
#define AUX_INTERRUPT		0x02
#define SYSTEM_STATUS		0x04
#define KBD_DISABLED		0x10
#define AUX_DISABLED		0x20
#define XT_SCANSET			0x40


#define PORT_B				0x92
#define GAMEPORT			0x200
#define PRIMARY_IDE			0x1F6
#define SECONDARY_IDE		0x176
#define PRIMARY_FLOPPY		0x3F5
#define SECONDARY_FLOPPY	0x375

#define COM1				0x3F8
#define COM2				0x2F8
#define COM3				0x3E8
#define COM4				0x2E8

#define LPT1				0x378
#define LPT2				0x278




