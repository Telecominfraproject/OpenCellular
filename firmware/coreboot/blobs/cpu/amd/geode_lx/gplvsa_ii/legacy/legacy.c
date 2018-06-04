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

//*  Function:                                                          *
//*     This file implements miscellaneous VSA functionality:           *
//*                                                                     *
//*     All chipsets:                                                   *
//*       1) SYSINFO virtual registers                                  *
//*                                                                     *
//*     CS5536 :                                                        *
//*       1) Emulation of CS5530 PCI interrupt steering registers.      *
//*       2) Flash-IDE switching                                        *
//*       3) Power management                                           * 


#include "vsa2.h"
#include "chipset.h"
#include "protos.h"

// Function prototypes
extern void Legacy_Early_Init(void);
extern void Legacy_Late_Init(void);
extern void Handle_Events(ULONG *);


// Local variables
Hardware SystemInfo;
ULONG ChipsetBase;
ULONG Param[MAX_MSG_PARAM];



//***********************************************************************
// Message handler for the Legacy VSM
//***********************************************************************
void VSM_msg_loop()
{ MSG Msg;

  // Get information about the system I'm executing on.
  SYS_GET_SYSTEM_INFO(&SystemInfo);
  ChipsetBase = SystemInfo.Chipset_Base;

  //
  // Message Handling Loop
  //
  do {

    // Get the next message
    Msg = SYS_GET_NEXT_MSG(&Param);

    switch (Msg) {

      case MSG_INITIALIZE:
        switch (Param[0]) {
          case EARLY_INIT:
            Legacy_Early_Init();
            break;

          case END_OF_POST_INIT:
            Legacy_Late_Init();
            break;
        }
        break;

      case MSG_EVENT:
        Handle_Events(Param);
        break;

      case MSG_SET_POWER_MODE:
      case MSG_SET_POWER_STATE:
      case MSG_SAVE_STATE:
      case MSG_RESTORE_STATE:
        break;
    } // end switch(Msg)
  } while (1);
}