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

//*	Function:                                                           *
//*     This file contains the code for recording event history. 


#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"


extern EVENT_ENTRY Events[];
extern ULONG MsgPacket[];

extern UCHAR VSM_Filter;

#if HISTORY
EVENT NonReportableEvents[] = {
  EVENT_A20,
  EVENT_GRAPHICS,
  EVENT_ACPI_TIMER
};
#define UNREPORT_COUNT (sizeof(NonReportableEvents)/sizeof(EVENT))

//
// Event history
//
EVENT_HISTORY History[HISTORY];
int HistoryWrap  = 0;
int HistoryStart = 0;
int HistoryEnd   = 0;



//*****************************************************************************
//
// Record the event history
//
//*****************************************************************************
void Keep_History(EVENT Event, EVENT EventIndex)
{ int i;
  ULONG ParamMask, Param2;
  UCHAR VSM_Type;
  register EVENT_ENTRY * EventPtr;

  EventPtr = &Events[EventIndex];

  VSM_Type = Get_VSM_Type(EventPtr->Vsm);


  // Filter out VSM(s)
  if (VSM_Filter != VSM_ANY && VSM_Type != VSM_Filter)
    return;

  //
  // Filter events from reporting
  //

  for (i=0; i < UNREPORT_COUNT; i++) {
    if (Event == NonReportableEvents[i])
	  return;
  }

  // Generate mask for Param1
  ParamMask = 0xFFFFFFFF;
  Param2 = EventPtr->Param2;
  switch (Event) {

    case EVENT_PCI_TRAP:
      // Report all PCI events together by PCI function
      ParamMask = ~Param2;
      break;

    case EVENT_GPIO:
      // Get current edge
      Param2 = EventPtr->CurrentEdge;
      break;
 
    case EVENT_SOFTWARE_SMI:
      // Don't report APM calls CPU_Busy or CPU_Idle
      if ((EventPtr->Param1 & 0xFF00) == 0x5300) {
        switch (EventPtr->Param1 & 0x00FF) {
          case 0x05:
          case 0x06:
            return;
        }
        break;
      }
      break;

    case EVENT_VIRTUAL_REGISTER:
      // Report all VR events together by classes
      ParamMask = 0x0000FF00;
      break;

  }

  // If it's the same event for the same VSM...
  if ((History[HistoryEnd].Event == Event)  &&
      (History[HistoryEnd].Vsm   == EventPtr->Vsm) ) {

    switch (Event) {

      case EVENT_IO_TRAP:
        if (History[HistoryEnd].Param1 != (MsgPacket[2] & 0x0000FFFFL)) {
          break;
        }


      // Events to be recorded together if (Param1 & ParamMask) matches
      case EVENT_PCI_TRAP:
      case EVENT_SOFTWARE_SMI:
	  case EVENT_VIRTUAL_REGISTER:
        if (History[HistoryEnd].Param1 != (MsgPacket[1] & ParamMask)) {
          break;
        }


      default:
        // Record together if 1st two parameters match
        if (History[HistoryEnd].Param1 == EventPtr->Param1 &&
            History[HistoryEnd].Param2 == EventPtr->Param2 ) {
          History[HistoryEnd].Count++;
          Store_Timestamp(&History[HistoryEnd].TimeStamp);
          return;   
        }

    } // end switch
  }

  //
  // Report event in a new history entry.
  //
  // Increment HistoryEnd unless it's the 1st event
  if (History[HistoryEnd].Vsm) {
    HistoryEnd++;
    if (HistoryEnd >= HISTORY) {
      HistoryEnd = 0;
      HistoryWrap = 1;
    }

    // Increment HistoryStart index too if HistoryEnd has wrapped
    if (HistoryWrap) {
      HistoryStart++;
      if (HistoryStart >= HISTORY)
        HistoryStart = 0;
    }
  }


  History[HistoryEnd].Vsm    = EventPtr->Vsm;
  History[HistoryEnd].Event  = Event;
  History[HistoryEnd].Param1 = MsgPacket[1] & ParamMask;
  History[HistoryEnd].Param2 = Param2;
  History[HistoryEnd].Count  = 1;
  Store_Timestamp(&History[HistoryEnd].TimeStamp);
}



void Initialize_History(void)
{

  // Initialize timestamp field of history buffer entry "previous" to 1st entry
  Store_Timestamp(&History[HISTORY-1].TimeStamp);
  HistoryWrap  = 0;
  HistoryStart = 0;
  HistoryEnd   = 0;
}

#else

void Initialize_History(void) { }


#endif
