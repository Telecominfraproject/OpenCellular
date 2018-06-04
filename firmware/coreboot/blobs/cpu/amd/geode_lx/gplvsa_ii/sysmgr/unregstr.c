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
//*     This file contains the code that unregisters events
//******************************************************************************



#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"



// Externals:
extern void pascal Clr_MBus_IO_Trap(ULONG Address, USHORT Range);
extern UCHAR * VsmNames[];
extern UCHAR * EventNames[];
extern UCHAR FreeEvent;
extern EVENT_ENTRY Events[MAX_REGISTRATIONS];


//*****************************************************************************
// Copies an Events[] entry
//*****************************************************************************
extern void pascal Copy_Event(USHORT From, USHORT To);

//*****************************************************************************
// Retires an Events[] entry to the free list
//*****************************************************************************
void pascal Retire_Events_Entry(EVENT Event, USHORT match, USHORT previous)
{ USHORT i;

  // Remove Events[match] from the linked list.
  i = Events[match].Link;
  if (previous) {
    Events[previous].Link = (UCHAR)i;
    i = match;
  } else {
    // The first entry in the linked list is being removed.
    Copy_Event(i, Event);
  }

  // Mark the entry as available.
  Events[i].Vsm = 0x00000000;
  if (i) {
    // Put Events[] entry on free list
    Events[i].Link = FreeEvent;
    FreeEvent = (UCHAR)i;
  }
}


//*****************************************************************************
// Unregisters a PCI trap
//*****************************************************************************
USHORT pascal Unregister_PCI_Trap(VSM Vsm, USHORT PCI_Addr, USHORT PCI_Mask)
{ USHORT EventIndex, IDSEL_Count=0, previous=0, match=0;
  register EVENT_ENTRY * EventPtr;


  // The entire event chain must be traversed to find out how
  // many registrations are trapping this IDSEL.  If only this
  // one, then the IDSEL in the MPCI_PBUS MSR will be cleared.
  EventIndex = EVENT_PCI_TRAP;
  while (Events[EventIndex].Vsm) {
    EventPtr = &Events[EventIndex];

    // Check if IDSEL matches
    if ((EventPtr->PCI_Addr & 0xF800) == (PCI_Addr & 0xF800)) {

      IDSEL_Count++;

      // If it is the requested VSM...
      if (EventPtr->Vsm == Vsm) {
        // and the PCI Address/Mask match...
        if (EventPtr->PCI_Addr == PCI_Addr && EventPtr->PCI_Mask == PCI_Mask) {
          // Record the EventIndex to be removed
          match = EventIndex;
        }
      }
    }

    // Record index immediately before the matching Events[] entry
    if (!match) {
      previous = EventIndex;
    }

    // Get the next entry in the linked list
    EventIndex = EventPtr->Link;

  }  // end while


  // If this is the only registration for this IDSEL...
  if (IDSEL_Count == 1) {
    // disable trapping this IDSEL
    Disable_Event(EVENT_PCI_TRAP, match);
  }

  // Retire the Events[] entry
  Retire_Events_Entry(EVENT_PCI_TRAP, match, previous);

  return match;
}


//*****************************************************************************
//   Disassociates an event from a VSM.  This may occur as a result of a
//   VSM performing the UNREGISTER_EVENT() macro, or if a VSM is being
//   removed or replaced.
//*****************************************************************************
// RESOURCE_COUNT:
// 1) Must be power of 2 since the Mod (%) operator is used
// 2) A minimum of 32 (# GPIOs)
// 3) A maximum of 256
#define RESOURCE_COUNT 128
USHORT pascal Unregister_Event(EVENT Event, VSM Vsm, ULONG Param1, ULONG Param2)
{ USHORT EventIndex, i, j=4, k=0, previous=0, match=0;
  static UCHAR HW_Resource[RESOURCE_COUNT];
  UCHAR IO_Base, IO_Range;
  ULONG Mask1=0x00000000, Mask2=0x00000000;
  register EVENT_ENTRY * EventPtr;



  // Determine which parameters must match
  switch (Event) {

    case EVENT_TIMER:
      // Only require Handle to match
      Mask2 = 0x0000FFFF;
      j = 8;      // # h/w timers
      break;

    case EVENT_PWM:
    case EVENT_PME:
    case EVENT_GPIO:
      // Only require pin to match
      Mask1 = 0x0000FFFF;
      j = 32;     // max # GPIO pins
      break;

    case EVENT_IO_TRAP:
      j = RESOURCE_COUNT;
      // Only require I/O addresses within range to match
      Mask1 = 0x00010000 - RESOURCE_COUNT;
      break;

    case EVENT_PCI_TRAP:
      match = Unregister_PCI_Trap(Vsm, (USHORT)Param1, (USHORT)Param2);
      return match;

    case EVENT_IO_TIMEOUT:
      // Require Param1 to match
      Mask1 = 0xFFFFFFFF;
      break;
  }

  // Zero the h/w resource usage counters
  for (i=0; i<j; i++) {
    HW_Resource[i] = 0;
  }
  j = 0;

  // The entire event chain must be traversed to find out how
  // many registrations are using the resource.  If more than
  // one, then the hardware will not be disabled.
  EventIndex = Event;
  while (Events[EventIndex].Vsm) {
    EventPtr = &Events[EventIndex];
    switch (Event) {
      case EVENT_TIMER:
        j = (USHORT)EventPtr->Timer;
        break;

      case EVENT_PWM:
      case EVENT_PME:
      case EVENT_GPIO:
        j = (USHORT)EventPtr->Pin;
        break;
    }

    // Check if parameters match
    if ((EventPtr->Param1 & Mask1) == (Param1 & Mask1) &&
        (EventPtr->Param2 & Mask2) == (Param2 & Mask2)) {

      if (Event == EVENT_IO_TRAP) {
        // Accumulate usage count for each I/O location in range
        IO_Base  = (UCHAR)EventPtr->IO_Base;
        IO_Range = (UCHAR)EventPtr->IO_Range;
        for (i = 0; i < IO_Range; i++) {
          k = (UCHAR)(i + IO_Base) % RESOURCE_COUNT;
          HW_Resource[k]++;
        }
      }

      // If it is the requested VSM...
      if (EventPtr->Vsm == Vsm) {

        // Record which h/w resource is being disabled
        switch (Event) {
          case EVENT_IO_TRAP:
            if (match) {
              // There are overlapping I/O ranges in the same VSM
              if ((EventPtr->IO_Base != (USHORT)Param1) ||
                  (EventPtr->IO_Range != (USHORT)Param2)) {
                // This is not the one being removed
                break;
              }
            }
          case EVENT_PWM:
          case EVENT_PME:
          case EVENT_GPIO:
          case EVENT_TIMER:
            k = j;
          default:
            // Record the EventIndex to be removed
            if (!match) {
              match = EventIndex;
            }
            break;
        }
      }
    }

    // Record entry previous to 'match'
    if (!match) {
      previous = EventIndex;
    }

    // Increment resource usage count
    if (Event != EVENT_IO_TRAP) {
      HW_Resource[j]++;
    }

    // Link to the next entry in the list.
    EventIndex = EventPtr->Link;

  }  // end while


  if (match) {

    switch (Event) {
      USHORT Range;

      case EVENT_IO_TRAP:
        Range=0x0000;
        EventPtr = &Events[match];
        Param1 |= Param2 & 0xFFFF0000;
        for (j = 0; j < (UCHAR)Param2; j++) {
          k = (UCHAR)((j + EventPtr->Param1) % RESOURCE_COUNT);
          if (HW_Resource[k] > 1) {
            if (Range) {
              // Found end of a range used exclusively by this VSM
              Clr_MBus_IO_Trap(Param1, Range);
              Param1 += Range;
              Range = 0;
            } else {
              (USHORT)Param1++;
            }
          } else {
            Range++;
          }
        }
        if (Range) {
          Clr_MBus_IO_Trap(Param1, Range);
        }
        break;

      default:
        // If only one registration is using the h/w resource, disable it
        if (HW_Resource[k] == 1) {
      case EVENT_IO_TIMEOUT:          // Statistic counter logic keeps track of usage
          Disable_Event(Event, match);
        }
      case EVENT_SOFTWARE_SMI:
        break;
    }

    // Retire the Events[] entry
    Retire_Events_Entry(Event, match, previous);
  
  } else {

    // A VSM attempted to unregister a event that it is not registered for
    Log_Error("Attempt to unregister EVENT_%s[0x%08X;0x%08X] by the %s VSM", EventNames[Event], Param1, Param2, VsmNames[Get_VSM_Type(Vsm)]);
  }

  return match;
}





//*****************************************************************************
// Removes all events registered to a VSM.
//*****************************************************************************
void pascal Unregister_VSM_Events(VSM Vsm)
{ register EVENT Event;
  register EVENT_ENTRY * EventPtr;
   
  // Unregister all events registered to this VSM
  for (Event=1; Event <= MAX_EVENT; Event++) {
    EventPtr = &Events[Event];
    while (EventPtr->Vsm) {

	  if (EventPtr->Vsm == Vsm) {
        if (Unregister_Event(Event, Vsm, EventPtr->Param1, EventPtr->Param2)) {
          // Start over on this event since there might be others for this VSM.
          Event--;
          break;
        }        
      }
      EventPtr = &Events[EventPtr->Link];
    }
  }
}

