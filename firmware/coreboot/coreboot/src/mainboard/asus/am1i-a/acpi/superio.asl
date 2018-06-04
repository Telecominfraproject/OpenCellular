/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 The ChromiumOS Authors.  All rights reserved.
 * Copyright (C) 2015 Sergej Ivanov <getinaks@gmail.com>
 * Copyright (C) 2018 Gergely Kiss <mail.gery@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// Scope is \_SB.PCI0.LPCB

// Values, defined here, must match settings in devicetree.cb

Device (PS2M) {
	Name (_HID, EisaId ("PNP0F13"))
	Name (_CRS, ResourceTemplate () {
		IRQNoFlags () {12}
	})
}

Device (PS2K) {
	Name (_HID, EisaId ("PNP0303"))
	Name (_CRS, ResourceTemplate () {
		IO (Decode16, 0x0060, 0x0060, 0x00, 0x01)
		IO (Decode16, 0x0064, 0x0064, 0x00, 0x01)
		IRQNoFlags () {1}
	})
}

Device (COM1) {
	Name (_HID, EISAID ("PNP0501"))
	Name (_UID, 1)
	Name (_CRS, ResourceTemplate ()
	{
		IO (Decode16, 0x03F8, 0x03F8, 0x08, 0x08)
		IRQNoFlags () {4}
	})
	Name (_PRS, ResourceTemplate ()
	{
		IO (Decode16, 0x03F8, 0x03F8, 0x08, 0x08)
		IRQNoFlags () {4}
	})
}

Device (COM2) {
	Name (_HID, EISAID ("PNP0501"))
	Name (_UID, 2)
	Name (_CRS, ResourceTemplate ()
	{
		IO (Decode16, 0x02F8, 0x02F8, 0x08, 0x08)
		IRQNoFlags () {3}
	})
	Name (_PRS, ResourceTemplate ()
	{
		IO (Decode16, 0x02F8, 0x02F8, 0x08, 0x08)
		IRQNoFlags () {3}
	})
}

Device (LPT1) {
	Name (_HID, EISAID ("PNP0401"))
	Name (_UID, 1)
	Name (_CRS, ResourceTemplate ()
	{
		IO (Decode16, 0x0378, 0x0378, 0x04, 0x08)
		IO (Decode16, 0x0778, 0x0778, 0x04, 0x08)
		IRQNoFlags () {5}
	})
	Name (_PRS, ResourceTemplate ()
	{
		IO (Decode16, 0x0378, 0x0378, 0x04, 0x08)
		IO (Decode16, 0x0778, 0x0778, 0x04, 0x08)
		IRQNoFlags () {5}
	})
}

Device (ENVC) {
	Name (_HID, EISAID ("PNP0C02"))
	Name (_UID, 1)
	Name (_CRS, ResourceTemplate ()
	{
		IO (Decode16, 0x0230, 0x0230, 0x04, 0x10)
		IO (Decode16, 0x0290, 0x0290, 0x04, 0x10)
	})
	Name (_PRS, ResourceTemplate ()
	{
		IO (Decode16, 0x0230, 0x0230, 0x04, 0x10)
		IO (Decode16, 0x0290, 0x0290, 0x04, 0x10)
	})
}

Device (GPIC) {
	Name (_HID, EISAID ("PNP0C02"))
	Name (_UID, 2)
	Name (_CRS, ResourceTemplate ()
	{
		IO (Decode16, 0x0300, 0x0300, 0x04, 0x20)
	})
	Name (_PRS, ResourceTemplate ()
	{
		IO (Decode16, 0x0300, 0x0300, 0x04, 0x20)
	})
}
