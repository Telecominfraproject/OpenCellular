/*
 * This file is part of the coreboot project.
 *
 * Copyright (c) 2011 Sven Schnelle <svens@stackframe.org>
 * Copyright (c) 2013 Vladimir Serbinenko <phcoder@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

Device(EC)
{
	Name (_HID, EISAID("PNP0C09"))
	Name (_UID, 0)

	Name (_GPE, THINKPAD_EC_GPE)
	Mutex (ECLK, 0)

	OperationRegion(ERAM, EmbeddedControl, 0x00, 0x100)
	Field (ERAM, ByteAcc, NoLock, Preserve)
	{
		Offset (0x02),
				DKR1, 1,	/* Dock register 1 */
		Offset (0x05),
				HSPA, 1,
		Offset (0x0C),
				LEDS, 8,	/* LED state */
		Offset (0x0F),
				    , 7,
				TBSW, 1,	/* Tablet mode switch */
		Offset (0x1a),
				DKR2, 1,	/* Dock register 2 */
		Offset (0x2a),
				EVNT, 8,	/* write will trigger EC event */
		Offset (0x2f),
				    , 6,
				FAND, 1,	/* Fan disengage */
				FANA, 1,	/* Fan automatic mode enable */
		Offset (0x30),
				    , 6,
				ALMT, 1,	/* Audio Mute + LED */
		Offset (0x3a),
				AMUT, 1,	/* Audio Mute (internal use) */
				    , 3,
				BTEB, 1,
				WLEB, 1,
				WWEB, 1,
		Offset (0x3B),
				    , 1,
				KBLT, 1,	/* Keyboard Light */
				    , 2,
				USPW, 1,	/* USB Power enable */
		Offset (0x48),
				HPPI, 1,	/* Headphone plugged in */
				GSTS, 1,	/* Master wireless switch */
		Offset (0x4e),
				WAKE, 16,
		Offset (0x78),
				TMP0, 8,	/* Thermal Zone 0 temperature */
				TMP1, 8,	/* Thermal Zone 1 temperature */
		Offset (0x81),
				PAGE, 8,	/* Information Page Selector */
		Offset (0xfe),
				    , 4,
				DKR3, 1		/* Dock register 3 */
	}

	Method (_CRS, 0, Serialized)
	{
		Name (ECMD, ResourceTemplate()
		{
			IO (Decode16, 0x62, 0x62, 1, 1)
			IO (Decode16, 0x66, 0x66, 1, 1)
		})
		Return (ECMD)
	}

	Method (TLED, 1, NotSerialized)
	{
		Store(Arg0, LEDS)
	}

	/* Not used for coreboot. Provided for compatibility with thinkpad-acpi.  */
	Method (LED, 2, NotSerialized)
	{
		TLED(Or(Arg0, Arg1))
	}

	Method (_INI, 0, NotSerialized)
	{
	}

	Method (MUTE, 1, NotSerialized)
	{
		Store(Arg0, AMUT)
	}

	Method (RADI, 1, NotSerialized)
	{
		Store(Arg0, WLEB)
		Store(Arg0, WWEB)
		Store(Arg0, BTEB)
	}

	Method (USBP, 1, NotSerialized)
	{
		Store(Arg0, USPW)
	}

	Method (LGHT, 1, NotSerialized)
	{
		Store(Arg0, KBLT)
	}


	/* Sleep Button pressed */
	Method(_Q13, 0, NotSerialized)
	{
		Notify(^SLPB, 0x80)
	}

	/* Brightness up GPE */
	Method(_Q14, 0, NotSerialized)
	{
		BRIGHTNESS_UP()
	}

	/* Brightness down GPE */
	Method(_Q15, 0, NotSerialized)
	{
		BRIGHTNESS_DOWN()
	}

#ifdef ACPI_VIDEO_DEVICE
	/* Next display GPE */
	Method(_Q16, 0, NotSerialized)
	{
		Notify (ACPI_VIDEO_DEVICE, 0x82)
	}
#endif
	/* AC status change: present */
	Method(_Q26, 0, NotSerialized)
	{
		Notify (AC, 0x80)
 		\PNOT()
	}

	/* AC status change: not present */
	Method(_Q27, 0, NotSerialized)
	{
		Notify (AC, 0x80)
		Store(0x50, EVNT)
 		\PNOT()
	}

	Method(_Q2A, 0, NotSerialized)
	{
		Notify(^LID, 0x80)
	}

	Method(_Q2B, 0, NotSerialized)
	{
		Notify(^LID, 0x80)
	}


	/* IBM proprietary buttons.  */

	Method (_Q10, 0, NotSerialized)
	{
		^HKEY.RHK (0x01)
	}

	Method (_Q11, 0, NotSerialized)
	{
		^HKEY.RHK (0x02)
	}

	Method (_Q12, 0, NotSerialized)
	{
		^HKEY.RHK (0x03)
	}

	Method (_Q64, 0, NotSerialized)
	{
		^HKEY.RHK (0x05)
	}

	Method (_Q65, 0, NotSerialized)
	{
		^HKEY.RHK (0x06)
	}

	Method (_Q17, 0, NotSerialized)
	{
		^HKEY.RHK (0x08)
	}

	Method (_Q66, 0, NotSerialized)
	{
		^HKEY.RHK (0x0A)
	}

	Method (_Q6A, 0, NotSerialized)
	{
		^HKEY.RHK (0x1B)
	}

	Method (_Q1A, 0, NotSerialized)
	{
		^HKEY.RHK (0x0B)
	}

	Method (_Q1B, 0, NotSerialized)
	{
		^HKEY.RHK (0x0C)
	}

	Method (_Q62, 0, NotSerialized)
	{
		^HKEY.RHK (0x0D)
	}

	Method (_Q60, 0, NotSerialized)
	{
		^HKEY.RHK (0x0E)
	}

	Method (_Q61, 0, NotSerialized)
	{
		^HKEY.RHK (0x0F)
	}

	Method (_Q1F, 0, NotSerialized)
	{
		^HKEY.RHK (0x12)
	}

	Method (_Q67, 0, NotSerialized)
	{
		^HKEY.RHK (0x13)
	}

	Method (_Q63, 0, NotSerialized)
	{
		^HKEY.RHK (0x14)
	}

	Method (_Q19, 0, NotSerialized)
	{
		^HKEY.RHK (0x18)
	}

	Method (_Q1C, 0, NotSerialized)
	{
		^HKEY.RHK (0x19)
	}

	Method (_Q1D, 0, NotSerialized)
	{
		^HKEY.RHK (0x1A)
	}

	Method (_Q5C, 0, NotSerialized)
	{
		^HKEY.RTAB (0xB)
	}

	Method (_Q5D, 0, NotSerialized)
	{
		^HKEY.RTAB (0xC)
	}

	Method (_Q5E, 0, NotSerialized)
	{
		^HKEY.RTAB (0x9)
	}

	Method (_Q5F, 0, NotSerialized)
	{
		^HKEY.RTAB (0xA)
	}

	/*
	 * Set FAN disengage:
	 * Arg0: 1: Run at full speed
	 *       0: Automatic fan control
	 */
	Method (FANE, 1, Serialized)
	{
		If (Arg0) {
			Store (One, FAND)
			Store (Zero, FANA)
		} Else {
			Store (Zero, FAND)
			Store (One, FANA)
		}
	}

	Device (HKEY)
	{
		Name (_HID, EisaId ("IBM0068"))
		Name (BTN, 0)
		Name (BTAB, 0)

		/* MASK */
		Name (DHKN, 0x080C)

		/* Effective Mask */
		Name (EMSK, 0)

		/* Effective Mask for tablet */
		Name (ETAB, 0)

		/* Device enabled. */
		Name (EN, 0)

		Method (_STA, 0, NotSerialized)
		{
			Return (0x0F)
		}

		/* Retrieve event. */
		Method (MHKP, 0, NotSerialized)
		{
			Store (BTN, Local0)
			If (LNotEqual (Local0, Zero)) {
				Store (Zero, BTN)
				Add (Local0, 0x1000, Local0)
				Return (Local0)
			}
			Store (BTAB, Local0)
			If (LNotEqual (Local0, Zero)) {
				Store (Zero, BTAB)
				Add (Local0, 0x5000, Local0)
				Return (Local0)
			}
			Return (Zero)
		}

		/* Report event  */
		Method (RHK, 1, NotSerialized) {
			ShiftLeft (One, Subtract (Arg0, 1), Local0)
			If (And (EMSK, Local0)) {
				Store (Arg0, BTN)
				Notify (HKEY, 0x80)
			}
		}

		/* Report tablet  */
		Method (RTAB, 1, NotSerialized) {
			ShiftLeft (One, Subtract (Arg0, 1), Local0)
			If (And (ETAB, Local0)) {
				Store (Arg0, BTAB)
				Notify (HKEY, 0x80)
			}
		}

		/* Enable/disable all events.  */
		Method (MHKC, 1, NotSerialized) {
			If (Arg0) {
				Store (DHKN, EMSK)
				Store (Ones, ETAB)
			}
			Else
			{
				Store (Zero, EMSK)
				Store (Zero, ETAB)
			}
			Store (Arg0, EN)
		}

		/* Enable/disable event.  */
		Method (MHKM, 2, NotSerialized) {
			If (LLessEqual (Arg0, 0x20)) {
				ShiftLeft (One, Subtract (Arg0, 1), Local0)
				If (Arg1)
				{
					Or (DHKN, Local0, DHKN)
				}
				Else
				{
					And (DHKN, Not (Local0), DHKN)
				}
				If (EN)
				{
					Store (DHKN, EMSK)
				}
			}
		}

		/* Mask hotkey all. */
		Method (MHKA, 0, NotSerialized)
		{
			Return (0x07FFFFFF)
		}

		/* Report tablet mode switch state */
		Method (MHKG, 0, NotSerialized)
		{
			Return (ShiftLeft(TBSW, 3))
		}

		/* Mute audio */
		Method (SSMS, 1, NotSerialized)
		{
			Store(Arg0, ALMT)
		}

		/* Control mute microphone LED */
		Method (MMTS, 1, NotSerialized)
		{
			If (Arg0)
			{
				TLED(0x8E)
			}
			Else
			{
				TLED(0x0E)
			}
		}

		/* Version */
		Method (MHKV, 0, NotSerialized)
		{
			Return (0x0100)
		}

		/* Master wireless switch state */
		Method (WLSW, 0, NotSerialized)
		{
			Return (\_SB.PCI0.LPCB.EC.GSTS)
		}
	}

#include "ac.asl"
#include "battery.asl"
#include "sleepbutton.asl"
#include "lid.asl"
#include "beep.asl"
#include "thermal.asl"
#include "systemstatus.asl"
}
