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

//*****************************************************************************
//*    Routines related to timer management.  
//*****************************************************************************


#include "VSA2.H"
#include "CHIPSET.H"
#include "SYSMGR.H"
#include "PROTOS.H"
#include "TIMER.H"

typedef   unsigned char (* TIMER_ON)(ULONG, UCHAR);
typedef   void (* TIMER_OFF)(unsigned short);


// External prototypes:
extern UCHAR EnableMsTimer_5536(ULONG, UCHAR);
extern USHORT DisableMsTimer_5536(USHORT);
extern void pascal MarkTimerAvailable(USHORT);


// External variables:
extern ULONG MsgPacket[];
extern EVENT_ENTRY Events[];
extern ULONG ClocksPerMs;
extern Hardware HardwareInfo;
extern TIMERS TimerInfo[];


// Local variables:
TIMER_ON  EnableTimer;
TIMER_OFF DisableTimer;
USHORT ActiveTimer;
ULONG ActiveInterval;


//***********************************************************************
// Performs initialization related to timers
//***********************************************************************
void InitTimers(void)
{

  switch (HardwareInfo.Chipset_ID) {

    case DEVICE_ID_5536:
      EnableTimer = EnableMsTimer_5536;
      DisableTimer = DisableMsTimer_5536;
      break;
  }
}



//***********************************************************************
// This routine handles timer ticks.
//***********************************************************************
USHORT FilterTimer(EVENT_ENTRY * EventPtr, EVENT EventIndex) 
{ USHORT ReturnValue=0;
  ULONG Vsm;
  static EVENT NextToExpire;

  Vsm = EventPtr->Vsm;


  // If 1st registered timer, initialize 'next interval' variables
  if (EventIndex == EVENT_TIMER) {
    NextToExpire = 0;
  }

  // Only decrement timers associated with the expired h/w timer
  if (EventPtr->Timer == ActiveTimer) {

    // Has this timer expired ?
    if (ActiveInterval >= EventPtr->RemainingInterval) {

      // Yes, reset the VSM's remaining interval
      EventPtr->RemainingInterval = EventPtr->Interval;

      // Fill message packet      
      MsgPacket[1] = EventPtr->Interval;
      MsgPacket[2] = EventPtr->Handle;

      // Was timer due to a SYS_YIELD ?
      if (EventPtr->Param2 & SYS_YIELD) {
        // Yes, schedule the slumbering VSM
        Schedule_VSM(Vsm);
      }

      // Return its Events[] index
      ReturnValue = EventIndex;
    } else {
      // Decrement RemainingInterval by the expired timer interval
      EventPtr->RemainingInterval -= ActiveInterval;
    }

    // Find the next interval on the current h/w timer to expire
    if (EventPtr->RemainingInterval < Events[NextToExpire].RemainingInterval) {
      // Ignore one-shots that just expired
      if (ReturnValue != EventIndex || !(EventPtr->Param2 & ONE_SHOT)) {
        NextToExpire = EventIndex;
      }
    }
  }

  // If no more timers, then set timer h/w to next interval
  if (!EventPtr->Link) {
    // Set the timer h/w to the next interval to expire
    if (Events[NextToExpire].Vsm) {
      Enable_Event(EVENT_TIMER, NextToExpire, 2);
    }
  }

  return ReturnValue;
}





//***********************************************************************
// This routine decrements all software timers.  If a software timer has
// expired, it sends an event message to the VSM.   The time to the next
// scheduled event and invokes Enable_Event() to restart the timer.
//
//***********************************************************************
void pascal Timer_Handler(USHORT TimerNumber)
{
  // Record the active timer
  ActiveTimer = TimerNumber;
  ActiveInterval = TimerInfo[ActiveTimer].Interval;

  // Mark this timer as available
  MarkTimerAvailable(TimerNumber);

  // Send the timer event
  Send_Event(EVENT_TIMER, 0x00000000);
}



//*****************************************************************************
// Enables a millisecond timer to the specified interval
//
// EnableFlag
//   0 = disable
//   1 = new registration
//   2 = reprogram timer to new interval
//*****************************************************************************
void MillisecondTimer(UCHAR EnableFlag, EVENT_ENTRY * EventPtr)
{
  if (EnableFlag == 0) {

    // Disable the h/w timer
    DisableTimer(EventPtr->Timer);

  } else {

    // Program a hardware timer
    EventPtr->Timer = (UCHAR)EnableTimer(EventPtr->RemainingInterval, EventPtr->Attr);
    
  }
}


