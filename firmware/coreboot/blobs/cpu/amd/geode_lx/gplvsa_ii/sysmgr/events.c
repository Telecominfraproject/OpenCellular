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

//******************************************************************************
//*     This file contains code for:
//*       1) Initialization of the Events[] array
//*       2) Registering events
//*       3) Sending event messages 
//******************************************************************************



#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"


#define MAX_HIT 10		// Max. # of hits on any single event

// External function prototypes:
extern USHORT FilterTimer(EVENT_ENTRY *, EVENT);
extern UCHAR VSM_Is_Yielded(VSM);

// External variables:
extern ULONG VSM_Ptrs[];

// Local variables:
//*****************************************************************************
// The first MAX_EVENT entries are indexed by the EVENT code.
// The Link field contains the index of the next VSM registered for that event.
// A link of 00h terminates the list.
//*****************************************************************************
EVENT_ENTRY Events[MAX_REGISTRATIONS];
UCHAR FreeEvent;  // Head of available Events[] entries
ULONG MsgPacket[MAX_MSG_PARAM+1];
ULONG MsgParams[MAX_HIT][4];

UCHAR * VsmNames[] = {
  "SYS_MGR",
  "AUDIO",
  "VGA",
  "LEGACY",
  "PM",
  "OHCI",
  "i8042",
  "DEBUGGER",
  "ACPI",
  "APM",
  "OEM_ACPI",
  "SMB",
  "BATTERY",
  "RTC",
  "S2D",
  "EXT_AMP",
  "PCMCIA",
  "SPY",
  "NETWORK",
  "GPIO",
  "KEYBOARD",
  "MOUSE",
  "USB",
  "FLASH",
  "INFRARED",
  "THERMAL",
  "NULL",
  "MPEG",
  "VIP",
  "LPC",
  "VUART",
  "MICRO",
  "USER1",
  "USER2",
  "USER3",
  "SYSINFO",
  "SUPERIO",
  "EHCI",
};

UCHAR * EventNames[] = {
  "????",
  "GRAPHICS",
  "AUDIO",
  "USB",
  "ACPI",
  "ACPI_TIMER",
  "IO_TRAP",
  "IO_TIMEOUT",
  "PME",
  "KEL",
  "VIDEO_INACTIVITY",
  "GPIO",
  "SOFTWARE_SMI",
  "PCI_TRAP",
  "VIRTUAL_REGISTER",
  "NMI",
  "TIMER",
  "DEVICE_TIMEOUT",
  "SEMAPHORE",
  "VBLANK",
  "A20",
  "SMB",
  "RTC",
  "THERMAL",
  "LPC",
  "UART",
  "BLOCKIO",
};


//*****************************************************************************
// Copies an Events[] entry
//*****************************************************************************
void pascal Copy_Event(USHORT From, USHORT To)
{  
  Events[To].Vsm    = Events[From].Vsm;
  Events[To].Param1 = Events[From].Param1;
  Events[To].Param2 = Events[From].Param2;
  Events[To].Param3 = Events[From].Param3;
  Events[To].Link   = Events[From].Link;
  Events[To].Priority = Events[From].Priority;
}



//*****************************************************************************
// Initializes the Events[] array
//*****************************************************************************
void Initialize_Events(void) 
{ int i;

  Events[0].RemainingInterval = 0xFFFFFFFF;

  for (i = 0; i < MAX_REGISTRATIONS ; i++) {
    Events[i].Priority = 0xffff;
    Events[i].Index = i;
  }

  // Initialize the list of free event entries
  for (i = MAX_REGISTRATIONS-2; i > MAX_EVENT ; i--) {
    Events[i].Link = i+1;
  }
  FreeEvent = MAX_EVENT+1;
}

//*****************************************************************************
// Checks Events[EventIndex] for parameters matching those in MsgPacket[].
// Return value is EventIndex if the parameters match else 0x00.
//*****************************************************************************
USHORT pascal FilterEvent(EVENT Event, EVENT EventIndex)
{ USHORT IO_Hit, IO_Base, IO_Range, ClassIndex, RdWrAttribute;
  UCHAR Size;
  ULONG Vsm;
  register EVENT_ENTRY * EventPtr;
  USHORT PCI_Hit;
  static ULONG PCI_Data;
  static USHORT PCI_Addr;
  static UCHAR PCI_Size;

  EventPtr = &Events[EventIndex];
  Vsm = EventPtr->Vsm;

  switch (Event) {

    case EVENT_USB:
      if ((UCHAR)MsgPacket[2] == (UCHAR)EventPtr->Param1) {
        return EventIndex;
      }
      break;

    case EVENT_TIMER:
      // Check if the registered interval has expired
      return FilterTimer(EventPtr, EventIndex);

    case EVENT_GPIO:
      // Verify the pin & edge match the event registration
      if (MsgPacket[1] == EventPtr->Pin && (MsgPacket[2] & EventPtr->Attributes)) {
        // If GPIO is Sleep button being used to wake system, don't send event
          return EventIndex;
      }
      break;

	case EVENT_PME:
      // MsgPacket[1] = ACPI GPE0_STS
      // MsgPacket[2] = ACPI PM1_STS
      if (EventPtr->Attributes & GPE) {
        if ((1L << EventPtr->Pme) & MsgPacket[1]) {
          return EventIndex;
        }
      }
      if (EventPtr->Attributes & PM1) {
        if ((1L << EventPtr->Pm1) & MsgPacket[2]) {
          return EventIndex;
        }
      }
      break;

    case EVENT_IO_TRAP:
    case EVENT_IO_TIMEOUT:
      IO_Base  = EventPtr->IO_Base;
      IO_Range = EventPtr->IO_Range;
      IO_Hit   = (USHORT)MsgPacket[2];
      // Check address range
      if ((IO_Hit >= IO_Base) && (IO_Hit <= (IO_Base+IO_Range-1))) {

        if (Event == EVENT_IO_TIMEOUT) {
          return EventIndex;
        }

        // Filter reads/writes
        if (MsgPacket[1] & 2) {
          RdWrAttribute = READS_ONLY >> 16;
        } else {
          RdWrAttribute = WRITES_ONLY >> 16;
        }
        if (!(EventPtr->Flags & RdWrAttribute)) {
          return EventIndex;
        }
      }
      break;

    case EVENT_VIRTUAL_REGISTER:
      // Check for correct Class::Index
      ClassIndex = (USHORT)MsgPacket[1];
      if ((ClassIndex >= EventPtr->ClassLow) && (ClassIndex <= EventPtr->ClassHigh)) {
        return EventIndex;
      }
      break;

    case EVENT_PCI_TRAP:
      if (EventIndex == EVENT_PCI_TRAP) {
        // Save original parameters
        PCI_Addr = (USHORT)MsgPacket[1];
        PCI_Size = (UCHAR)MsgPacket[2];
        PCI_Data = MsgPacket[3];
      } else {
        // Restore original parameters
        (USHORT)MsgPacket[1] = PCI_Addr;   
        (UCHAR)MsgPacket[2] = PCI_Size;   
        MsgPacket[3] = PCI_Data;
      }

      // Ignore SysMgr's virtualization
      if (Vsm == SysMgr_VSM) {
        break;         
      }

      if (MsgPacket[2] & 0x20000) {
        RdWrAttribute = READS_ONLY >> 16;
      } else {
        RdWrAttribute = WRITES_ONLY >> 16;
      }


      PCI_Hit = PCI_Addr;
      Size = PCI_Size & DWORD_IO;

      while (Size) {

        // Check for PCI address match after applying mask
        if (EventPtr->PCI_Addr == (PCI_Hit & ~(EventPtr->PCI_Mask))) {

          // Filter reads/writes
          if (!(EventPtr->Flags & RdWrAttribute)) {
            // Limit Size to correct boundaries
            Size = PCI_Size & DWORD_IO;
            switch (PCI_Hit & 0x3) {
              case 2:
               if (Size == DWORD_IO) {
                 Size = WORD_IO;
               }
               break;
              case 3:
               Size = BYTE_IO;
               break;
            }
            MsgPacket[2] &= ~DWORD_IO;
            MsgPacket[2] |=  Size;
            (USHORT)MsgPacket[1] = PCI_Hit;
            return EventIndex;
          }
        }

        // Don't cross DWORD boundaries
        if ((++PCI_Hit & 0x3) == 0) {
          break;
        }
        MsgPacket[3] >>= 8;
        if (!(PCI_Size & IO_WRITE)) { 
          MsgPacket[3] |= 0xFF000000;
        }
        Size >>= 1;
      }
      break;

    case EVENT_SOFTWARE_SMI:
      // Check for code match after applying mask
      if (EventPtr->Param1 == (MsgPacket[1] & ~EventPtr->Param2)) {
        return EventIndex;
      }
      break;

    default:
      return EventIndex;
  }

  return 0;
}


//*****************************************************************************
//
// Sends a MSG_EVENT to all VSMs registered for the specified event.
//
//*****************************************************************************
EVENT pascal Send_Event(EVENT Event, VSM From_VSM)
{ ULONG To_VSM, ErrorParam = Event;
  UCHAR HitCount = 0;
  EVENT Hits[MAX_HIT], EventIndex;
  USHORT i;
  register EVENT_ENTRY * EventPtr;

  // Perform sanity checks
  if (Event == 0 || Event > MAX_EVENT) {
    Log_Error("Invalid event 0x%04X", Event);
	return 0;
  }

  if (Events[Event].Vsm == 0) {
    // No VSM has registered this event.
    Log_Error("EVENT_%s[0x%08X;0x%08X] is not registered to a VSM", EventNames[Event], MsgPacket[1], MsgPacket[2]);
    return 0;
  }

  EventIndex = Event;

#if HISTORY
  Keep_History(Event, EventIndex);
#endif

  // Store Event code as message parameter[0]
  MsgPacket[0] = Event;

  // Walk the entire event chain looking for parameter matches
  while (EventIndex) {

    EventPtr = &Events[EventIndex];

    // Check if the parameters for the current event match this event registration
    if (FilterEvent(Event, EventIndex)) {
      if (HitCount < MAX_HIT-1) {
        // Record the hit
        Hits[++HitCount] = EventIndex;
        MsgParams[HitCount][1] = MsgPacket[1];
        MsgParams[HitCount][2] = MsgPacket[2];
        MsgParams[HitCount][3] = MsgPacket[3];
      } else {
        // ERROR: MAX_HIT is too small
        Log_Error("Hits[] array is too small");
        break;
      }
    }

    // Point to the next registered VSM
    EventIndex = EventPtr->Link;
  }

  // Send messages in priority order
  for (i = HitCount; i > 0; i--) {
    EventPtr = &Events[Hits[i]];
    To_VSM = EventPtr->Vsm;

    // Is the VSM being awakened prematurely from SYS_YIELD_CONTROL?
    if (VSM_Is_Yielded(To_VSM)) {
      EVENT_ENTRY * TimerPtr;

      // Find the wakeup timer being used
      EventIndex = EVENT_TIMER;
      while (EventIndex) {
        TimerPtr = &Events[EventIndex];

        if ((TimerPtr->Vsm == To_VSM) && (TimerPtr->Param2 & SYS_YIELD)) {
          Unregister_Event(EVENT_TIMER, To_VSM, TimerPtr->Param1, TimerPtr->Param2);
          break;
        }
        EventIndex = TimerPtr->Link;
      }       
    }

    // Send a message to the VSM
    if (!(EventPtr->Param2 & SYS_YIELD)) {
      MsgPacket[1] = MsgParams[i][1];
      MsgPacket[2] = MsgParams[i][2];
      MsgPacket[3] = MsgParams[i][3];
      Send_Message(From_VSM, To_VSM, MSG_EVENT);
    }

    // Unregister one-shot events
    if (EventPtr->Param2 & ONE_SHOT) {
      Unregister_Event(Event, To_VSM, EventPtr->Param1, EventPtr->Param2);
    }
  }

  // Send message to Spy VSM, if present
  if (HitCount && VSM_Ptrs[VSM_SPY]) {
    Send_Message(From_VSM, VSM_Ptrs[VSM_SPY], MSG_EVENT);
  }


  return HitCount;
}





//*****************************************************************************
// Associates a VSM with an Event and associated Parameters.
// The System Manager maintains a table indexed by the Event.  Each entry
// contains a Link (array index) to the next VSM registered as a handler for
// that Event.  Entries are ordered by Priority (high to low).  Unused array
// elements are denoted by the Vsm field == 0000.
//*****************************************************************************
void pascal Register_Event(EVENT Event, PRIORITY Priority, VSM Vsm, ULONG Param1, ULONG Param2)
{ UCHAR index, previous, next, Match=0;
  register EVENT_ENTRY * EventPtr;

  if (Event > MAX_EVENT) {
    // Illegal Event
    Log_Error("Attempt to register invalid event 0x%04X by the %s VSM", Event, VsmNames[Get_VSM_Type(Vsm)]);
	return;
  }

  // Handle special cases
  switch (Event) {

    case EVENT_VIRTUAL_REGISTER:
      // Param2[15:8] = low index  Param2[7:0] = high index
	  // If index == 0x00, then any index is valid
      Param1 <<= 8;                             // Shift class to bits 15:8
      index = 0xFF;                             // Default upper index
      if (Param2) {
        index = (UCHAR)Param2;                  // Get upper index
        (UCHAR)Param1 = (UCHAR)(Param2 >> 8);   // Get lower index
      }
      Param2 = Param1;
      (UCHAR)Param2 = index;
      break;

    case EVENT_IO_TRAP:
      // 16 MSBs of Param1 are reserved
      Param1 &= 0x0000FFFF;	
      // Don't allow PCI config cycles to be trapped
      if (Param1 >= 0xCF8 && Param1 <= 0xCFF) {
        Log_Error("Attempt to trap I/O 0xCF8-0xCFF by the %s VSM", VsmNames[Get_VSM_Type(Vsm)]);
        return;
      }
      break;

    case EVENT_TIMER:
      // Mask all except Handle and valid flags
      Param2 &= FOR_STANDBY | ONE_SHOT | SYS_YIELD | 0xFFFF;
      if (Param1 == 0) {
        Priority = UNREGISTER_PRIORITY;
      }
      break;
  }
    
  // If unregistering event(s)...
  if (Priority >= UNREGISTER_PRIORITY) {
     Unregister_Event(Event, Vsm, Param1, Param2);
     return;
  }

  index = (UCHAR)Event;
  next = 0;

  if (Events[Event].Vsm) {

    // Ensure a VSM doesn't register for the same event twice
	do {
      EventPtr = &Events[index];

      // If it is the same VSM...
      if (EventPtr->Vsm == Vsm) {

        switch (Event) {

          // If existing timer handle, assign new interval and/or priority
          case EVENT_TIMER:
            if (EventPtr->Handle == (USHORT)Param2) {
              Match = 2;
            }
            break;

          case EVENT_PWM:
            // If existing pin, change rate and/or duty cycle
            if (EventPtr->Pin == (USHORT)Param1) {
              Match = 3;
            }
            break;

          // If existing pin, accumulate attributes
          case EVENT_GPIO:
            if (EventPtr->Pin == Param1 && EventPtr->Priority == Priority) {
              EventPtr->Attributes |= Param2;
              Match = 1;
            }
            break;


          default:
            // If params match, then just update the event priority
            if (EventPtr->Param1 == Param1 && EventPtr->Param2 == Param2) {
#if SUPPORT_PRIORITY
              // Sort list again according to priority
#endif
              Match = 1;
              break;
            }
        } 
      }

      // Early out
      if (Match) {
        break;
      }
      // Get link to next event
	  index = EventPtr->Link;

	} while (index);



    if (Match == 0) {

      // Report error if no more Events[] entries
      if (FreeEvent == 0) {
        Log_Error("Failed attempt to register EVENT_%s[0x%08X;0x%08X] by the %s VSM", EventNames[Event], Param1, Param2, VsmNames[Get_VSM_Type(Vsm)]);
	    return;
      }

      // Get an unused Events[] entry
      index = FreeEvent;
      FreeEvent = Events[index].Link;

      //
      // Find the insertion point.  There are two cases:
      // 1) The new entry is higher priority than the current 1st entry
      // 2) The new entry belongs somewhere down the chain.
	  //
      previous = next = (UCHAR)Event;
      while (next = Events[next].Link) {

        if (Priority > Events[next].Priority) {
          break;
        }
        previous = next;
      }

      if (Priority > Events[Event].Priority) {
        // case 1:  Need to copy 1st entry to Events[index]
	    Copy_Event(Event, index);
        next = index;
        index = (UCHAR)Event;

      } else {
        // case 2:
        // Insert new event into linked list
        Events[previous].Link = index;
      }
    }
  }

  // Create new Events[] entry
  EventPtr = &Events[index];


  switch (Match) {

    // New registration
    case 0:
      EventPtr->Link     = next;
      EventPtr->Vsm      = Vsm;
    case 3:
      EventPtr->Param2   = Param2;

    // Update of Priority
    case 1:
      EventPtr->Priority = Priority;

    // Update of Parameter (e.g. timer interval)
    case 2:
      EventPtr->Param1   = Param1;

  }

  // Enable the appropriate hardware
  Enable_Event(Event, index, 1);

}