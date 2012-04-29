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

// ATA
#define MSR_LBAR_ATA			0x0008


// Diverse Integration Logic

#define MSR_MAST_CONF			0x0001
  #define NON_COH_RD  (1L << 12)
  #define NON_COH_WR  (1L << 13)

#define LBAR_EN             (1 << 0)
#define NOR_NAND            (1 << 1)
#define MEM_IO              (1 << 2)
#define LBAR_IO_MASK		0x0001FFF0
#define LBAR_MEM_MASK		0xFFFFF000


#define MSR_LBAR_IRQ			0x0008
#define MSR_LBAR_KEL1			0x0009
#define MSR_LBAR_KEL2			0x000A
#define MSR_LBAR_SMB			0x000B

#define MSR_LBAR_GPIO			0x000C
  #define GPIO_LOW_BANK_SELECT	0x00
  #define GPIO_HIGH_BANK_SELECT	0x80
  #define GPIO_OUTPUT_VALUE		0x00
  #define GPIO_OUTPUT_ENABLE	0x04
  #define GPIO_OUTPUT_OPENDRAIN	0x08
  #define GPIO_OUTPUT_INVERT	0x0C
  #define GPIO_OUT_AUX1_SELECT	0x10
  #define GPIO_OUT_AUX2_SELECT  0x14
  #define GPIO_PULLUP_ENABLE	0x18
  #define GPIO_PULLDOWN_ENABLE	0x1C
  #define GPIO_INPUT_ENABLE		0x20
  #define GPIO_INPUT_INVERT		0x24
  #define GPIO_IN_FILTER_ENABLE	0x28
  #define GPIO_IN_EVENTCOUNT	0x2C
  #define GPIO_READ_BACK		0x30
  #define GPIO_IN_AUX1_SELECT	0x34
  #define GPIO_EVENTS_ENABLE	0x38
  #define GPIO_LOCK_ENABLE		0x3C
    #define LKOV				(1 <  0)
    #define LKOE				(1 <  1)
    #define LKOD				(1 <  2)
    #define LKOI				(1 <  3)
    #define LKA1				(1 <  4)
    #define LKA2				(1 <  5)
    #define LKPU				(1 <  6)
    #define LKPD				(1 <  7)
    #define LKIE				(1 <  8)
    #define LKII				(1 <  9)
    #define LKFE				(1 < 10)
    #define LKEE				(1 < 11)
    #define LKIA				(1 < 12)
    #define LKIP				(1 < 13)
    #define LKPE				(1 < 14)
    #define LKNE				(1 < 15)
  #define GPIO_POSEDGE_ENABLE	0x40
  #define GPIO_NEGEDGE_ENABLE	0x44
  #define GPIO_POSEDGE_STATUS	0x48
  #define GPIO_NEGEDGE_STATUS	0x4C

  // GPIO IRQ Mapper
  #define GPIO_MAPPER_X			0xE0
  #define GPIO_MAPPER_Y			0xE4
  #define GPIO_MAPPER_Z			0xE8
  #define GPIO_MAPPER_W			0xEC

  // Digital Filter
  #define GPIO_FILTER_AMOUNT    0x50
  #define GPIO_FILTER_COUNT     0x52
  #define GPIO_EVENT_COUNT      0x54
  #define GPIO_EVENT_COMPARE    0x56

  #define GPIO6_FILTER_AMOUNT	0xD0
  #define GPIO7_FILTER_AMOUNT	0xD8

  #define GPIO_FILTER_SELECT0	0xF0
  #define GPIO_FILTER_SELECT1	0xF1
  #define GPIO_FILTER_SELECT2	0xF2
  #define GPIO_FILTER_SELECT3	0xF3
  #define GPIO_FILTER_SELECT4	0xF4
  #define GPIO_FILTER_SELECT5	0xF5
  #define GPIO_FILTER_SELECT6	0xF6
  #define GPIO_FILTER_SELECT7	0xF7




#define MSR_LBAR_MFGPT			0x000D
  // I/O offsets relative to MFGPT LBAR
  #define MFGPT_CMP1			0x00
  #define MFGPT_CMP2			0x02
  #define MFGPT_COUNTER			0x04
  #define MFGPT_SETUP			0x06

  #define MFGPT_OFFSET          8

#define MSR_LBAR_ACPI			0x000E
#define MSR_LBAR_PMS			0x000F
#define MSR_LBAR_FLSH0 			0x0010
#define MSR_LBAR_FLSH1 			0x0011
#define MSR_LBAR_FLSH2 			0x0012
#define MSR_LBAR_FLSH3 			0x0013
#define MSR_LEG_IO	 			0x0014
  #define RESET_SHUT_EN  (0x80000000L)
  #define UART1_SHIFT		(16)
  #define UART2_SHIFT		(20)
  #define UART_MASK			(0x07)
  #define UART_IO_MASK		(0x03)
  #define UART_EN			(0x04)
#define MSR_PIN_OPTS			0x0015
  #define PIN_OPT_IDE		(1 << 0)
#define MSR_SOFT_IRQ			0x0016
#define MSR_SOFT_RESET			0x0017
#define MSR_AC_DMA				0x0019

#define MSR_KEL_CNTRL			0x001F
  #define KEL_SNOOP			(1 << 0)
  #define KEL_EER			(1 << 1)
  #define KEL_PRTA_EN		(1 << 4)


// IRQ Mask & Mapper (from MDD Specification)
#define MSR_IRQM_YLOW			0x0020
#define MSR_IRQM_YHIGH			0x0021
#define MSR_IRQM_ZLOW			0x0022
#define MSR_IRQM_ZHIGH			0x0023
#define MSR_IRQM_PRIM			0x0024
#define MSR_IRQM_LPC			0x0025
#define MSR_IRQM_LXIRR			0x0026
#define MSR_IRQM_HXIRR			0x0027
#define MSR_MFGPT_IRQ			0x0028
#define MSR_MFGPT_NR			0x0029
#define MSR_MFGPT_CLR_SETUP		0x002B

#define MSR_FLOP_S3F2			0x0030
#define MSR_FLOP_S3F7			0x0031
#define MSR_FLOP_S372			0x0032
#define MSR_FLOP_S377			0x0033
#define MSR_PIC_SHADOW			0x0034
#define MSR_PIT_SHADOW			0x0036

// UART's
#define MSR_UART1_MOD			0x0038
#define MSR_UART1_DONG			0x0039
#define MSR_UART1_CONF			0x003A
	#define UART_SOFT_RESET			(0x01L)
	#define UART_DEVEN				(0x02L)
	#define UART_FREEZE				(0x04L)
	#define UART_TEST				(0x08L)
	#define UART_EXT_BANKS			(0x10L)
#define MSR_UART2_MOD			0x003C
#define MSR_UART2_DONG			0x003D
#define MSR_UART2_CONF			0x003E

#define MSR_DMA_MAP				0x0040
#define MSR_DMA_SHAD0			0x0041
#define MSR_DMA_SHAD1			0x0042
#define MSR_DMA_SHAD2			0x0043
#define MSR_DMA_SHAD3			0x0044
#define MSR_DMA_SHAD4			0x0045
#define MSR_DMA_SHAD5			0x0046
#define MSR_DMA_SHAD6			0x0047
#define MSR_DMA_SHAD7			0x0048
#define MSR_DMA_MSK_SHAD		0x0049

// LPC
#define MSR_LPC_SIRQ			0x004E



// MSR_SMI
#define HLT_ASMI_EN			(1L << 0)
#define SHUTDOWN_ASMI_EN	(1L << 1)
#define KEL_ASMI_EN			(1L << 2)
#define PIC_ASMI_EN			(1L << 3)
#define PM_ASMI_EN			(1L << 4)
#define INIT_K_EN			(1L << 5)
#define A20_P_EN			(1L << 6)
#define INIT_P_EN			(1L << 7)
#define UART1_SSMI_EN		(1L << 8)
#define UART2_SSMI_EN		(1L << 9)
#define RESERVED_EN    		(1L << 10)
#define LPC_SSMI_EN			(1L << 11)
#define DMA_SSMI_EN			(1L << 12)
#define A20_K_EN			(1L << 13)
#define PM2_CNT_SSMI_EN		(1L << 14)
#define PM1_CNT_SSMI_EN		(1L << 15)

#define HLT_ASMI_STAT		(1L << 32)
#define SHUTDOWN_ASMI_STAT	(1L << 33)
#define KEL_ASMI_STAT		(1L << 34)
#define PIC_ASMI_STAT		(1L << 35)
#define PM_ASMI_STAT		(1L << 36)
#define INIT_K_STAT			(1L << 37)
#define A20_P_STAT			(1L << 38)
#define INIT_P_STAT			(1L << 39)
#define UART1_SSMI_STAT		(1L << 40)
#define UART2_SSMI_STAT		(1L << 41)
#define RESERVED_STAT  		(1L << 42)
#define LPC_SSMI_STAT		(1L << 43)
#define DMA_SSMI_STAT		(1L << 44)
#define A20_K_STAT			(1L << 45)
#define PM2_CNT_SSMI_STAT	(1L << 46)
#define PM1_CNT_SSMI_STAT	(1L << 47)

