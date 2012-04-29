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
// Function:
//	  This module contains the main code to handle the VSA II interface.


#include "vsa2.h"
#include "vr.h"
#include "lxvg.h"

// This is required here because of inclusion problems in LXVG.h
extern Hardware SystemInfo;

// ARRAY TO STORE VSA II MESSAGE PARAMETERS
unsigned long VSAparam[MAX_MSG_PARAM];


//---------------------------------------------------------------------------
// vsa2_message_loop
//
// This is the main routine that handles the VSA II message interface.
//---------------------------------------------------------------------------

void vsa2_message_loop(void)
{
	MSG Msg;

	// SPIN FOREVER ON MESSAGE LOOP
	// The VSA system manager branches here after loading the VSM.	Control
	// is returned to the system manager using an SMI.

	do {

		// GET THE NEXT MESSAGE
		// If a message is available, the macro reads the parameter data
		// from the VSM header and copies it to the VSAparam global array.
		// If a message is not available, control returns to the main VSA
		// dispatcher.
		Msg = SYS_GET_NEXT_MSG(&VSAparam);

		// DECODE THE MESSAGE
		switch(Msg)
		{
			case MSG_INITIALIZE:

				// CHECK IF NORMAL INITIALIZATION
				// Currently there is no "end of post" initialization.
				if (VSAparam[0] == EARLY_INIT)	{

					// Get information about the system I'm executing on.
					SYS_GET_SYSTEM_INFO(&SystemInfo);

					// REGISTER FOR VGA VIRTUAL REGISTER EVENTS
					SYS_REGISTER_EVENT(EVENT_VIRTUAL_REGISTER, VRC_VG, 0, NORMAL_PRIORITY);

				}
				else
				{
					// Now we can handle DPMS and driver active
					VGState |= SF_END_POST;
				}
				break;

			case MSG_EVENT:

				// DECODE THE EVENT
				decode_vsa2_event();
				break;

			case MSG_WARM_BOOT:
				break;


			default:
				break;
		}


	} while(1);
}

//---------------------------------------------------------------------------
// decode_vsa2_event
//
// This routine is called when the message loop receives an event. For
// LXVG, this is either a Virtual Rgister event or a pci event
// (trapping PCI config cycles to our device).
//---------------------------------------------------------------------------

void decode_vsa2_event(void)
{
	// PARSE EVENT

	switch((unsigned short)VSAparam[0])
	{
		case EVENT_VIRTUAL_REGISTER:
			if (VRC_VG == (unsigned char)(VSAparam[1] >> 8))
				virtual_register_event((unsigned char)VSAparam[1], VSAparam[2], VSAparam[3]);

			break;

		case EVENT_PCI_TRAP:
			// If LXVG isn't enabled, just leave
			if (VGState & SF_DISABLED) break;

			// CALL LXVG TO DECODE THE PCI TRAP EVENT
			// address = VSAparam[1]
			// size = VSAparam[2], bit 7 indicates write.
			// data = VSAparam[3]

			pci_trap_event(VSAparam[1], VSAparam[2], VSAparam[3]);
			break;

		default:
			break;
	}
}