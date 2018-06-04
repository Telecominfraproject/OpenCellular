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

#define VENDOR_ID_COMPAQ		0x0E11
#define VENDOR_ID_CYRIX		0x1078
#define VENDOR_ID_NATIONAL		0x100B
#define VENDOR_ID_AMD			0x1022

#define DEVICE_ID_MEDIAGX		0x0001
#define DEVICE_ID_5530			0x0100

#define DEVICE_ID_GX2			0x0028
#define DEVICE_ID_GFX2			0x0030

#define DEVICE_ID_5535			0x002B
#define DEVICE_ID_FLASH		0x002C
#define DEVICE_ID_ATA			0x002D
#define DEVICE_ID_AUDIO		0x002E
#define DEVICE_ID_OHCI			0x002F

#define DEVICE_ID_LX			0x2080
#define DEVICE_ID_GFX3			0x2081
#define DEVICE_ID_AES			0x2082

#define DEVICE_ID_5536			0x2090
#define DEVICE_ID_AMD_FLASH		0x2091
#define DEVICE_ID_AMD_ATA		0x2092
#define DEVICE_ID_AMD_AUDIO		0x2093
#define DEVICE_ID_AMD_OHCI		0x2094
#define DEVICE_ID_AMD_EHCI		0x2095
#define DEVICE_ID_AMD_UDC		0x2096
#define DEVICE_ID_AMD_OTG		0x2097
#define DEVICE_ID_AMD_THOR		0x209A


#define LEGACY_FUNCTION			0x0000
#define PM_FUNCTION				0x0100
#define IDE_FUNCTION			0x0200
#define AUDIO_FUNCTION			0x0300
#define VIDEO_FUNCTION			0x0400
#define XBUS_FUNCTION			0x0500



// SMI sources relative to Function 1 BAR0
#define SMI_STATUS_RO          0x0000
#define SMI_STATUS				0x0002
#define SMI_SRC_PM		 				(1L << 0)
#define SMI_SRC_AUDIO_INDEX			1
#define SMI_SRC_AUDIO					(1L << SMI_SRC_AUDIO_INDEX)
#define SMI_SRC_ACPI					(1L << 2)
#define SMI_SRC_VG						(1L << 3)
#define SMI_SRC_INT_MEMORY				(1L << 4)
#define SMI_SRC_RETRACE				(1L << 5)
#define SMI_SRC_VGA_TIMER  			(1L << 6)
#define SMI_SRC_A20_INDEX 				7
#define SMI_SRC_A20					(1L << SMI_SRC_A20_INDEX)
#define SMI_SRC_SW_INDEX				8
#define SMI_SRC_SW	 	 				(1L << SMI_SRC_SW_INDEX)
#define GTT_INDEX              		9
#define SMI_SRC_GTT					(1L << GTT_INDEX)
#define SMI_SRC_DEBUG					(1L << 10)
#define SMI_SRC_MFGPT					(1L << 11)
#define SMI_SRC_NMI					(1L << 12)
#define SMI_SRC_RESET		   			(1L << 13)
#define SMI_SRC_USB1	   				(1L << 14)
#define SMI_SRC_USB2	   				(1L << 15)
#define SMI_IGNORE						0x7FFF


#define GTT_STATUS				0x0006
// NOTE: The following source definitions are shifted left 16 bits because
//       Get_SMI_Sources passes them that way in the SMI_Sources variable.
  #define GTT_SRC_GT1		  			(1L << ( 0+16))
  #define GTT_SRC_GT2					(1L << ( 1+16))
  #define GTT_SRC_USR_DEF_TRAP1		(1L << ( 2+16))
  #define GTT_SRC_USR_DEF_TRAP2		(1L << ( 3+16))
  #define GTT_SRC_USR_DEF_TRAP3		(1L << ( 4+16))
  #define SMI_SRC_PCI_TRAP				(1L << ( 5+16))
  #define GTT_SRC_1MS_TMR				(1L << ( 6+16))
  #define GTT_SRC_1SEC_TMR				(1L << ( 7+16))
  #define SMI_SRC_MPCI					(1L << ( 8+16))	 // Virtualized PCI access
  #define SMI_SRC_DESCR_HIT			(1L << ( 9+16))  // Hit on MBus descriptor
  #define SMI_SRC_STAT	 				(1L << (10+16))  // Hit on MBus statistics counter
  #define SMI_SRC_PIC					(1L << (11+16))  // PIC event
  #define SMI_SRC_KEL					(1L << (12+16))  // KEL event
  #define SMI_SRC_PME					(1L << (13+16))  // PME event
  #define SMI_SRC_BLOCKIO				(1L << (14+16))  // BLOCKIO event

#define SMI_SPEEDUP_DISABLE	 	0x0008

#define AUDIO_STATUS            	0x0010		// relative to F3 BAR0



//############################################################################################


#define PCI_CFG_CONTROL			0x40
  #define   LEGACY_CFG_SMI				(1L <<  8)
  #define   PM_CFG_SMI      			(1L << 11)
  #define   IDE_CFG_SMI				(1L << 14)
  #define   AUDIO_CFG_SMI 				(1L << 16)
  #define   VIDEO_CFG_SMI 				(1L << 17)
  #define   USB_PM_SMI	 				(1L << 22)
  #define   USB_CFG_SMI 				(1L << 23)
  #define   USB_ENABLE	 				(1L << 24)

  #define   PM_CFG_SMI_SCxxxx          (1L <<  9)
  #define   IDE_CFG_SMI_SCxxxx         (1L << 10)
  #define   AUDIO_CFG_SMI_SCxxxx       (1L << 11) 
  #define   VIDEO_CFG_SMI_SCxxxx       (1L << 12)
  #define   XBUS_CFG_SMI_SCxxxx        (1L << 13)

#define RESET_CONTROL				0x44
  #define   X_BUS_WARM_START			(1L << 0)
  #define   PCI_RESET					(1L << 1)
  #define   IDE_RESET					(1L << 2)
  #define   IDE_CONTROLLER_RESET		(1L << 3)
  #define   AC97_RESET					(1L << 7)


#define ROM_AT_LOGIC				0x52
  #define   LOWER_ROM_ENABLE			(1 << 0)
  #define   ROM_WRITE_ENABLE			(1 << 1)
  #define   ROM_1MB_WRITE_ENABLE		(1 << 2)
  #define   PORT_92_ENABLE				(1 << 3)
  #define   A20M_DEASSERT				(1 << 4)
  #define   GAMEPORT_CS_ON_READS		(1 << 5)
  #define   GAMEPORT_CS_ON_WRITES		(1 << 6)
  #define   FAST_KEYBOARD_DISABLE		(1 << 7)


#define CPU_SUPPORT				0x53
  #define   A20M_ENABLE				(1 << 0)
  #define   RTC_ENABLE					(1 << 2)
  #define   GAMEPORT_ENABLE			(1 << 3)

#define DECODE_CONTROL				0x5A
  #define   RTC_POSITIVE_DECODE		(1 << 0)
  #define   KBD_POSITIVE_DECODE		(1 << 1)
  #define   COM1_POSITIVE_DECODE		(1 << 2)
  #define   COM2_POSITIVE_DECODE		(1 << 3)
  #define   COM3_POSITIVE_DECODE		(1 << 4)
  #define   COM4_POSITIVE_DECODE		(1 << 5)
  #define   FLPY1_POSITIVE_DECODE		(1 << 6)
  #define   FLPY2_POSITIVE_DECODE		(1 << 7)
  #define   LPT1_POSITIVE_DECODE		(1 << 8)
  #define   LPT2_POSITIVE_DECODE		(1 << 9)
  #define   LPT3_POSITIVE_DECODE		(1 << 10)
  #define   IDE1_POSITIVE_DECODE		(1 << 11)
  #define   IDE2_POSITIVE_DECODE		(1 << 12)
  #define   ROM_POSITIVE_DECODE		(1 << 13)
  #define   KBD_6X_DECODE				(1 << 15)

#define PCI_STEERING				0x5C


// Top-level SMI enables  (registers 0x80-0x83)
#define ENABLE_TRAPS_TIMERS		0x80
  #define   ENABLE_PM					(1L << 0)
  #define   ENABLE_TIMERS				(1L << 1)
  #define   ENABLE_TRAPS		   		(1L << 2)
  #define   ENABLE_IRQ_SPEEDUP			(1L << 3)
  #define   ENABLE_VIDEO_SPEEDUP		(1L << 4)
  #define   ENABLE_CODEC_SMI			(1L << 5)
  #define   ENABLE_RESERVED1			(1L << 6)
  #define   ENABLE_RESERVED2			(1L << 7)
  #define   ENABLE_HDD0_TIMER			(1L << 8)
  #define   ENABLE_FDD_TIMER			(1L << 9)
  #define   ENABLE_PAR_SER_TIMER		(1L << 10)
  #define   ENABLE_KYBD_MOUSE_TIMER	(1L << 11)
  #define   ENABLE_USR_DEF_TIMER1		(1L << 12)
  #define   ENABLE_USR_DEF_TIMER2		(1L << 13)
  #define   ENABLE_USR_DEF_TIMER3		(1L << 14)
  #define   ENABLE_VIDEO_TIMER			(1L << 15)
  #define   ENABLE_HDD0_TRAP	   		(1L << 16) 
  #define   ENABLE_FDD_TRAP			(1L << 17)
  #define   ENABLE_PAR_SER_TRAP		(1L << 18)
  #define   ENABLE_KYBD_MOUSE_TRAP 	(1L << 19)
  #define   ENABLE_USR_DEF_TRAP1		(1L << 20)
  #define   ENABLE_USR_DEF_TRAP2		(1L << 21)
  #define   ENABLE_USR_DEF_TRAP3		(1L << 22)
  #define   ENABLE_VIDEO_TRAP			(1L << 23)
  #define   ENABLE_GT1					(1L << 24)
  #define   ENABLE_GT2					(1L << 25)
  #define   ENABLE_RETRACE				(1L << 26)
  #define   ENABLE_VGA_TIMER	  		(1L << 27)
  #define   ENABLE_THERMAL				(1L << 28)	// SCxxxx only
  #define   ENABLE_ACPI_TIMER			(1L << 29)
  #define   ENABLE_HDD1_TRAP			(1L << 30)
  #define   ENABLE_HDD1_TIMER			(1L << 31)


#define GT1_COUNT					0x88
#define GT1_CONTROL				0x89
  #define   GT1_RESET_HDD				(1 << 0)
  #define   GT1_RESET_FDD				(1 << 1)
  #define   GT1_RESET_PARALLEL_SERIAL	(1 << 2)
  #define   GT1_RESET_KEYBD_MOUSE		(1 << 3)
  #define   GT1_RESET_USR_DEF1			(1 << 4)
  #define   GT1_RESET_USR_DEF2			(1 << 5)
  #define   GT1_RESET_USR_DEF3			(1 << 6)
  #define   GT1_TIMEBASE				(1 << 7)


#define GT2_COUNT					0x8A
#define GT2_CONTROL				0x8B
  #define GT2_RESET_GPIO7				(1 << 2)
  #define GT2_TIMEBASE					(1 << 3)			// 0=second  1=ms
  #define GT1_SHIFT					(1 << 4)			// 0=8 bit   1=16 bit
  #define GT2_SHIFT					(1 << 5)			// 0=8 bit   1=16 bit
  #define VGA_TIMEBASE					(1 << 6)			// 0=ms      1=32 us
  #define GT1_RESET_SEC_HDD			(1 << 7)			// 0=disable 1=enable

#define IRQ_SPEEDUP_COUNT          0x8C
#define VIDEO_SPEEDUP_COUNT        0x8D
#define VGA_TIMER_LOAD_COUNT       0x8E




#define GPIO_DIRECTION          	0x90
#define GPIO_DATA               	0x91
#define   GPIO_CONTROL				0x92
  #define   GPIO0_ENABLE				(1 << 0)
  #define   GPIO1_ENABLE				(1 << 1)
  #define   GPIO2_ENABLE				(1 << 2)
  #define   GPIO0_EDGE					(1 << 3)
  #define   GPIO1_EDGE					(1 << 4)
  #define   GPIO2_EDGE					(1 << 5)
  #define   GPIO6_ENABLE				(1 << 6)
  #define   GPIO6_EDGE					(0 << 6)
  #define   GPIO7_EDGE					(1 << 7)



#define MISCELLANEOUS				0x93
  #define   SERIAL_MOUSE_SELECT		(1 << 0)
  #define   SERIAL_MOUSE				(1 << 1)
  #define   HDD1_PARTIAL_DECODE		(1 << 4)
  #define   HDD0_PARTIAL_DECODE		(1 << 5)
  #define   HDD_SELECT					(1 << 6)
  #define   FDD_SELECT					(1 << 7)

#define SUSPEND_MODULATION_OFF     0x94
#define SUSPEND_MODULATION_ON      0x95
#define SUSPEND_CONFIGURATION      0x96
  #define   SUSPEND_MOD_ENABLE         (1 << 0)
  #define   SMI_SPEEDUP                (1 << 1)
  #define   SUSPEND_MODE               (1 << 2)


#define GPIO_CONTROL2				0x97
  #define   GPIO3_ENABLE				(1 << 0)
  #define   GPIO4_ENABLE				(1 << 1)
  #define   GPIO5_ENABLE				(1 << 2)
  #define   GPIO7_ENABLE				(1 << 3)
  #define   GPIO3_EDGE					(1 << 4)
  #define   GPIO4_EDGE					(1 << 5)
  #define   GPIO5_EDGE					(1 << 6)
  #define   GPIO7_EDGE            		(1 << 7)


#define HDD_IDLE_TIMEOUT           0x98
#define FDD_IDLE_TIMEOUT           0x9A
#define PAR_SER_IDLE_TIMEOUT  		0x9C
#define KYBD_MOUSE_IDLE_TIMEOUT	0x9E
#define USR_DEF1_IDLE_TIMEOUT		0xA0
#define USR_DEF2_IDLE_TIMEOUT		0xA2
#define USR_DEF3_IDLE_TIMEOUT		0xA4
#define VIDEO_IDLE_TIMEOUT			0xA6

#define SEC_HDD_TIMEOUT            0xAC
#define CPU_SUSPEND_COMMAND        0xAE
#define CPU_STOP_CLOCK_COMMAND     0xAF
#define CPU_CACHE_MISS_ACTIVITY    0xB0
#define CPU_CACHE_MISS_INACTIVITY  0xB1
#define CPU_CACHE_MISS_THRESHOLD   0xB2

#define FLOPPY_SHADOW				0xB4
#define FLOPPY_3F2					0xB4
#define FLOPPY_3F7					0xB5
#define FLOPPY_372					0xB6
#define FLOPPY_377					0xB7

#define DMA_SHADOW					0xB8
#define DMA_SHADOW_CNT				10

#define PIC_SHADOW					0xB9
#define PIC_SHADOW_CNT				12

#define PIT_SHADOW					0xBA
#define PIT_SHADOW_CNT				9

#define RTC_SHADOW					0xBB
#define RTC_SHADOW_CNT				1

#define PIC_SHADOW					0xB9
#define PIT_SHADOW					0xBA
#define RTC_SHADOW					0xBB

#define CLOCK_STOP_CONTROL         0xBC

#define USR_DEF_1_BASE				0xC0
#define USR_DEF_2_BASE				0xC4
#define USR_DEF_3_BASE				0xC8
#define USR_DEF_1_CONTROL			0xCC
#define USR_DEF_2_CONTROL			0xCD
#define USR_DEF_3_CONTROL			0xCE
  #define USR_DEF_MEMORY             	(1 << 7)	// Bit 7 = 1
  #define USR_DEF_IO                 	(0 << 7)	// Bit 7 = 0
  #define USR_DEF_WRITE              	(1 << 6)	// Bit 6 = 1 (only for I/O)
  #define USR_DEF_READ               	(1 << 5)	// Bit 5 = 1 (only for I/O)


#define SW_SMI					0xD0



// Relative to PCI Function 0
#define PM_STATUS					0xF4
  #define   PM_SRC_GPIO3	 			(1L <<  0)
  #define   PM_SRC_GPIO4	 			(1L <<  1)
  #define   PM_SRC_GPIO5	 			(1L <<  2)
  #define   PM_SRC_GPIO7	 			(1L <<  3)
  #define   PM_SRC_GAMEPORT	 		(1L <<  4)
  #define   PM_SRC_HDD_IDLE	 		(1L <<  8)	// Primary
  #define   PM_SRC_FDD_IDLE	 		(1L <<  9)
  #define   PM_SRC_PARSER_IDLE 		(1L << 10)
  #define   PM_SRC_KEYBMS_IDLE			(1L << 11)
  #define   PM_SRC_USER_DEF1_IDLE		(1L << 12)
  #define   PM_SRC_USER_DEF2_IDLE		(1L << 13)
  #define   PM_SRC_USER_DEF3_IDLE		(1L << 14)
  #define   PM_SRC_VIDEO_IDLE			(1L << 15)
  #define   PM_SRC_HDD_TRAP	 		(1L << 16)	// Primary
  #define   PM_SRC_FDD_TRAP	 		(1L << 17)
  #define   PM_SRC_PAR_SER_TRAP		(1L << 18)
  #define   PM_SRC_KYBD_MOUSE_TRAP		(1L << 19)
  #define   PM_SRC_SECONDARY_IDLE		(1L << 20)
  #define   PM_SRC_SECONDARY_TRAP 		(1L << 21)
  #define   PM_SRC_reserved			(1L << 22)
  #define   PM_SRC_VIDEO_TRAP			(1L << 23)
  #define   PM_SRC_ACPI_TIMER 			(1L << 24)
  #define   PM_SRC_RTC_ALARM 			(1L << 25)
  #define   PM_SRC_AC97_CODEC 			(1L << 26)
  #define   PM_SRC_LID_SWITCH 			(1L << 27)
  #define   PM_SRC_LID_POSITION		(1L << 28)	// Not an SMI source
  #define   PM_SRC_GPIO0	 			(1L << 29)
  #define   PM_SRC_GPIO1	 			(1L << 30)  // CS55x0 only
  #define   PM_SRC_THERMAL	 			(1L << 30)	// SCxxxx only
  #define   PM_SRC_GPIO2	 			((ULONG)(1L << 31))


#define GPIO_PINS	(PM_SRC_GPIO0 | PM_SRC_GPIO1 | PM_SRC_GPIO2 | PM_SRC_GPIO3 | PM_SRC_GPIO4 | PM_SRC_GPIO5 | PM_SRC_LID_SWITCH | PM_SRC_GPIO7)



