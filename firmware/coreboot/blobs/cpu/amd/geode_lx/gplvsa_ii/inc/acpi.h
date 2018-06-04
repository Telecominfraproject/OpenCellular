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



//*********************************************************
// Private messages used between ACPI VSM and PMCORE VSM **
//*********************************************************
#define PMSG_GOTO_SLEEP         0x70
#define PMSG_BLINK_LED		    0x71


/* device power states */
#define D0_STATE	(0)
#define D1_STATE	(1)
#define D2_STATE	(2)
#define D3_STATE	(3)

/**************************************************************
* smi_cmd Definitions
* These values must match the corresponding values in ACPI
* FADT.  OS writes these values to SMI_CMD I/O location.
****************************************************************/
#define ACPI_ENABLE     (0xA1)
#define ACPI_DISABLE    (0xA2)
#define S4BIOS_REQ      (0xA3)


// Bitmap of IRQ's PM VSM can map SCI to
#define ALLOWED_SCI_IRQ	(0x0E20) // 11,10,9,5
#define DEFAULT_SCI_IRQ	(9)

/**************************************************************
* PM1_STS bit definitions
****************************************************************/
#define WAKE_STS        (0x8000)
#define RTC_STS         (0x0400)
#define SLPBTN_STS      (0x0200)
#define PWRBTN_STS      (0x0100)
#define GBL_STS         (0x0020)
#define BM_STATUS       (0x0010)
#define TMR_STS         (0x0001)
#define PM1_STS_CLR		(0x8731)

/**************************************************************
* PM1_EN bit definitions
****************************************************************/
#define RTC_EN          (0x0400)
#define SLPBTN_EN       (0x0200)
#define PWRBTN_EN       (0x0100)
#define GBL_EN          (0x0020)
#define TMR_EN          (0x0001)

/**************************************************************
* PM1_CNT bit definitions
****************************************************************/
#define SLP_EN          (0x2000)
#define SLP_ENB			(0x20)
#define SLP_TYPx_MASK   (0x1C00)            // mask for setting SLP_TYPx
#define SLP_TYPx_MASKB	(0x1C)
#define SLP_TYPx_SHFT	(10)
#define SLP_TYPx_SHFTB	(2)
#define GBL_RLS         (0x0004)
#define BM_RLD          (0x0002)
#define SCI_EN          (0x0001)
#define ACPI_S0			(0)
#define ACPI_S1			(1)
#define ACPI_S1_CLKOFF	(0x81)
#define ACPI_S2			(2)
#define ACPI_S3			(3)
#define ACPI_S4			(4)
#define ACPI_S5			(5)

/**************************************************************
* PM2_CNT bit definitions
**************************************************************/
#define ARB_DIS			(0x0001)


/**************************************************************
* P_CNT bit definitions
****************************************************************/
#define CLK_VAL_MASK    (0x0000000F)
#define CLK_VAL_OFFSET  (0)
#define CLK_VAL_WIDTH   (4)
#define THT_EN          (0x00000010)


/********************************************************************
* FACS
********************************************************************/
#define FACS_SIG_OFS	(0x00)
#define FACS_LEN_OFS	(0x04)
#define FACS_HWSIG_OFS	(0x08)
#define FACS_OSWV_OFS	(0x0C)
#define FACS_GBLOCK_OFS	(0x10)
	#define FACS_GBL_OWNED		(0x02)
	#define FACS_GBL_PENDING	(0x01)
#define FACS_FLAGS_OFS	(0x14)


/********************************************************************
* CS5536 PM stuff
********************************************************************/
#define YIG_SCI			(5)			// Y Interrupt Group 5 is all SCI sources
#define MSR_SYS_RESET	(0x0014)	// 5536 GLCP SYS_RESET MSR

// default location of PMC regs
#define PMC5536_BASE		(0x9D00)

// default location of ACPI regs (32 bytes)
#define ACPI5536_BASE	(0x9C00)
#define PM1_STS_OFS		(0x00)
#define PM1_EN_OFS		(0x02)
#define PM1_CNT_OFS		(0x08)
#define PM2_CNT_OFS		(0x0C)
#define PM_TMR_OFS		(0x10)
#define GPE0_STS_OFS	(0x18)
#define GPE0_EN_OFS		(0x1C)

// Virtualized ACPI registers, these offsets were picked to provide
// minimal overlap of misaligned accesses.
// Note: PM1_STS/EN and GPE0_STS/EN must be back-to-back AND 
// PM1_STS and PM1_EN must be 16-bit only.
#define	VACPI_TRAP_BASE	0x9C20
#define VACPI_TRAP_LEN	(32)
#define VPM1_STS_OFS	(0x00)
#define VPM1_EN_OFS		(0x02)
#define VPM1_CNT_OFS	(0x08)
#define VGPE0_STS_OFS	(0x10)
#define VGPE0_EN_OFS	(0x14)
#define VACPI_ENABLE	(0x1C)

// 5536 GPIO13 AUX1_IN is dedicated to Sleep Button. It's controlled
// by SLPB_STS in PM1_STS and SLPB_EN in PM1_EN.
// Unfortunately, GPIO13 is in Working Power Domain so it is useless as
// a wake event for anything other than S1.
#define DFLT_5536_SLPB_GPIO	(13)

// Bit[9] of PM1_CNT, ignored bit in ACPI spec. On 5536 this bit indicates
// software has written a 1 to GBL_RLS(bit[2]) of PM1_CNT. Bit[9] can be cleared
// by writing a 1 to it.
#define GBL_RLS_FLAG	(0x0200)

// Bit[11] of PM1_STS, ignored bit in ACPI spec. On 5536 writing this bit=1 causes
// GBL_STS(bit[5]) to be set to 1.  Bit[11] always reads as 0.
#define SET_GBL_STS		(0x0800)

// 5536 specific bits in ACPI GPE0_STS & GPE0_EN
#define GPE0_PIC_INT	(0x00000001L)
#define GPE0_PIC_ASMI	(0x00000002L)
#define GPE0_SMB		(0x00000004L)
#define GPE0_UART1		(0x00000008L)
#define GPE0_UART2		(0x00000010L)
#define GPE0_USB1		(0x00000020L)
#define GPE0_USB2		(0x00000040L)
#define GPE0_PME0		(0x00010000L)
#define GPE0_PME1		(0x00020000L)
#define GPE0_PME2		(0x00040000L)
#define GPE0_PME3		(0x00080000L)
#define GPE0_PME4		(0x00100000L)
#define GPE0_PME5		(0x00200000L)
#define GPE0_PME6		(0x40000000L)
#define GPE0_PME7		(0x80000000L)



/*##
 *## ACPI Indicator Designations
 *##
 */
#define LED_OFF				0x00		// LED turned off
#define LED_SLOW			0x01		// 1/4Hz rate (4 second cycle time), 50% duty cycle
#define LED_FAST			0x02		// 1Hz rate (1 second cycle time), 50% duty cycle
#define LED_ON				0x03		// LED always on

#define	NO_LED				0x00		// No LEDs here
//#define	MB_LED0				0x01		// The motherboard LED 0 bit mask
//#define	MB_LED1				0x02		// The motherboard LED 1 bit mask
//#define	MB_LED2				0x04		// The motherboard LED 2 bit mask
//#define	MB_LED3				0x08		// The motherboard LED 3 bit mask
#define	MB_LEDALL			0x0F		// The all motherboard LEDs bit mask
//#define	SIO_LED0			0x10		// The SIO LED 0 bit mask
//#define	SIO_LED1			0x20		// The SIO LED 1 bit mask
//#define	SIO_LED2			0x40		// The SIO LED 2 bit mask
//#define	SIO_LED3			0x80		// The SIO LED 3 bit mask
#define SIO_LEDALL			0xF0		// The SIO LEDs are in the upper nibble
//The LEDS used to indicate sleep/wake
#define	INDICATOR_SLEEP		0x11


/*
* table indexes for gx2/lx msrs
*   (This may be moved *)
*/
// Northbridge
#define IDX_GLIU0	(0)
#define IDX_MC		(1)
#define IDX_GLIU1	(2)
#define IDX_VG		(3)
#define IDX_GP		(4)
#define IDX_DF		(5)
#define IDX_GLCP	(6)
#define IDX_GLPCI	(7)
#define IDX_FG		(8)
#define IDX_CPU		(9)
#define IDX_VIP		(10)
#define IDX_AES		(11)

// Southbridge
#define IDX_SB_GLPCI (12)
#define IDX_SB_GLIU	(13)
#define IDX_SB_USB2	(14)
#define IDX_SB_ATA	(15)
#define IDX_SB_MDD	(16)
#define IDX_SB_AC97	(17)
#define IDX_SB_USB1	(18)
#define IDX_SB_GLCP	(19)


#define NUM_DEVS		(IDX_SB_GLCP+1)

// definitions for callbacks for PM functions
#define PM_CALLBACK 0xBD50
#define  PM_CB_LED  0x00
#define  PM_CB_PME  0x01
#define     PM_CB_PME_DISARM    0x00
#define     PM_CB_PME_ARM       0x01
#define  PM_CB_SLEEP 0x02
#define     PM_CB_ENTER_S3    0x00
#define     PM_CB_ENTER_SLEEP 0x01
#define     PM_CB_LEAVE_SLEEP 0x02
#define     PM_CB_DONE_S3     0x03
