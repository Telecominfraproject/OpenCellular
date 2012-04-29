/*
* Copyright (c) 2007-2008 Advanced Micro Devices,Inc. ("AMD").
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
// This module decodes the SoftVG SMI sources.


#include "lxvg.h"
#include "vsa2.h"
#include "vr.h"
#include "pci.h"

extern void pascal out_8(USHORT, UCHAR);
extern void pascal out_16(USHORT, USHORT);
extern UCHAR pascal in_8(USHORT);
extern USHORT pascal in_16(USHORT);

//---------------------------------------------------------------------------
// virtual_register_event
//
// This routine is called when LXVG receives a VRC_VG virtual register
// access event.
//
//---------------------------------------------------------------------------

void virtual_register_event(unsigned char reg, unsigned long rwFlag, unsigned long vrData)
{

	// Look for virtual register read case first
	if (!(rwFlag & SMM_VR_WRITE))
	{
		// Determine if we are initialized
		if (!(VGState & SF_SECONDARY))
		{
			SET_AX(0xFFFF);
		}else{
			SET_AX(vReg[reg]);
		}
		return;
	}

	// If we get here, we know its a write, so handle the requests
	switch (reg)
	{
		case VG_CONFIG:
			// We only need to initialize once!
			if (!(VGState & SF_SECONDARY))
			{
				// Initialize the secondary controller portion of the engine.
				lxvg_initialize((unsigned short)vrData);

				// REGISTER PCI EVENTS - DO NOT ENABLE PCI EVENTS IF DISABLED.

				if (VGState & SF_SECONDARY)
				{
					// If we've initialized to the secondary state, we need to support
					// PCI config accesses.	 If we are disabled, the SysMgr will handle
					// the support.	 We may be initialized to the primary state, but the
					// minimum level is secondary.
					SYS_REGISTER_EVENT(EVENT_PCI_TRAP, vga_config_addr, 0xFF, NORMAL_PRIORITY);
				}
			}
			break;

		default:
			break;
	}

	// Finally, store the data...
	vReg[reg] = (unsigned short)vrData;

	return;
}

//---------------------------------------------------------------------------
// pci_trap_event
//
// This routine is called when the VSA2 system manager receives an SMI for
// a PCI configuration cycle that we have registered for.
//
// The "flags" parameter indicates the size in bits [3:0] (0x1 = byte,
// 0x3 = word, and 0xF = dword).  Bit 7 indicates a write.
//---------------------------------------------------------------------------

void pci_trap_event(unsigned long address, unsigned long flags, unsigned long data)
{
	unsigned char reg = (unsigned char) address & 0x000000FF;
	unsigned char size = (unsigned char) flags & 0x0000000F;
	unsigned long pciSave;

	if ((address & PCI_CONFIG_MASK) == vga_config_addr)
	{

		// CHECK IF PCI WRITE
		if (flags & PCI_TRAP_WRITE)
		{
			// Handle the trapped register writes
			if (reg == PCI_CMD_REG)
			{
				if ((unsigned char)data & PCI_MEM_SPACE)
					// Change the frame buffer base realated stuff.
					hw_fb_map_init(framebuffer_base);
			}
			else if ((reg >= BAR0) && (reg <= BAR4))
			{
				// NOTE: SysMgr now delivers DWORD aligned values with all reserved bits zeroed.
				if ((data < VGdata.pci_fb_mask) && (data != 0L))
				{
					// We have to assume that the changing agent won't put the framebuffer
					// on top of program memory.  The check for zero above was added when
					// Windows was observed clearing out the BARS by writing 0's to them.
					// This is still susceptible to failure due to bad address choices by
					// the changing agent.
					switch (reg)
					{
						case BAR0:
							if ((data != framebuffer_base) && (data >= 0x1000000))
							{
								// Set the frame buffer base.
								framebuffer_base = data;

								// If the PCI memory is on, change the framebuffer base
								// related stuff.
								pciSave = READ_PCI_DWORD_NO_TRAP(PCI_CMD_REG);
								if (pciSave & PCI_MEM_SPACE)
									// Change the frame buffer base realated stuff.
									hw_fb_map_init(framebuffer_base);
							}
							break;

						case BAR1:
							// Set the GP register base
							GPregister_base = data;
							break;

						case BAR2:
							// Set the VG register base
							VGregister_base = data;
							break;

						case BAR3:
							// Set the DF register base
							DFregister_base = data;
							break;

						case BAR4:
							// Set the VIP register base
							VIPregister_base = data;
							break;
					}
				}
			}
		}
		else
		{
			// Always mask off the invalid bits, but preserve the
			// bottom 4 bits.
			if (size == DWORD_IO)
			{
				pciSave = GET_EAX();
				switch (reg)
				{
					case BAR0:
						pciSave &= VGdata.pci_fb_mask | 0x0000000F;
						break;

					case BAR1:
					case BAR2:
					case BAR3:
					case BAR4:
						pciSave &= MASK16K;
						break;
				}
				SET_EAX(pciSave);
			}
		}
	}
}

// END OF FILE
