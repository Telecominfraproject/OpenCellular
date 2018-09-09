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

/*****************************************************************************
 *    The routines in this file translate requests for hardware
 *     support to Southbridge-specific function calls. 
 *****************************************************************************/


#include "VSA2.H"
#include "CHIPSET.H"
#include "CS5536.H"
#include "ISA.H"
#include "SYSMGR.H"
#include "PROTOS.H"
#include "GX2.H"


// External function prototypes
extern void MillisecondTimer(UCHAR, EVENT_ENTRY *);
extern void InitTimers(void);
extern void Init_CS5536(void);
extern void pascal Enable_USB_5536(UCHAR, USHORT);
extern void pascal Address_Decode_5536(UCHAR, USHORT);
extern void pascal MBus_IO_Trap(ULONG, ULONG, UCHAR);
extern void pascal MBus_IO_Timeout(ULONG, ULONG, UCHAR);
extern void pascal A20_Init(UCHAR);
extern void pascal InactivityTimer(UCHAR, USHORT, UCHAR);
extern void pascal Program_PWM(UCHAR, UCHAR, USHORT, UCHAR);

// External variables
extern EVENT_ENTRY Events[]; 
extern Hardware HardwareInfo;
extern ULONG MPCI_NB;
extern ULONG ATA_Error;

// Local variables:
typedef   void (pascal * CHIPSET_DEPENDENT)(unsigned char, unsigned short);
CHIPSET_DEPENDENT Address_Decode;
CHIPSET_DEPENDENT Enable_USB;
GPIO_FUNCTION GPIO_Control;
ULONG ClocksPerMs;


//*****************************************************************************
// Initializes the Southbridge
//*****************************************************************************
void InitChipset(void)
{

  ClocksPerMs = HardwareInfo.CPU_MHz * 1000L;

  InitTimers();

  switch (HardwareInfo.Chipset_ID) {

    case DEVICE_ID_5536:

      Address_Decode = Address_Decode_5536;
      Enable_USB = Enable_USB_5536;

      // Initialize the CS5536
      Init_CS5536();
      break;

  }
}



//*****************************************************************************
// Enables trapping of a PCI function
// Return value:
//   0 = not supported
//   1 = already enabled
//   2 = previously disabled
//*****************************************************************************
UCHAR pascal Enable_PCI_Trapping(USHORT PCI_Address, UCHAR EnableFlag) 
{ UCHAR ReturnValue = 0;


  // Call the appropriate chipset routine
  switch (HardwareInfo.Chipset_ID) {

    default:
      // Use PBUS MSR to trap this address
      Trap_PCI_IDSEL(PCI_Address, EnableFlag);
      ReturnValue = 1;
      break;
  }

  return ReturnValue;

}


//*****************************************************************************
// Diables an event
//*****************************************************************************
void pascal Disable_Event(EVENT Event, USHORT Index)
{
  Enable_Event(Event, Index, 0);
}

//*****************************************************************************
// Enables an event
//
// EnableFlag:
//   0 = Disable event
//   1 = Enable event
//   2 = Reset event logic (e.g. timer, GPIO)
//*****************************************************************************
void pascal Enable_Event(EVENT Event, USHORT Index, UCHAR EnableFlag)
{ USHORT Device_ID;
  UCHAR Error = 0;
  ULONG Param1, Param2;
  EVENT_ENTRY * EventPtr;

  Device_ID = HardwareInfo.Chipset_ID;

  EventPtr = &Events[Index];
  Param1 = EventPtr->Param1;
  Param2 = EventPtr->Param2;


  switch (Event) {

    case EVENT_USB:
      Enable_USB(EnableFlag, (USHORT)Param1);
    case EVENT_KEL:
      break;

    case EVENT_TIMER:
      // Initialize RemainingInterval field to Interval
      if (EnableFlag == 1) {
        EventPtr->RemainingInterval = EventPtr->Param1;
      }
      MillisecondTimer(EnableFlag, EventPtr);
      break;

    case EVENT_IO_TRAP:
      MBus_IO_Trap(Param1, Param2, EnableFlag);
      break;

    case EVENT_DEVICE_TIMEOUT:
      // If programmer forgot to specify Instance, assume 1st instance
      if ((USHORT)Param2 == 0x0000) {
        (USHORT)Param2 = 1;
      }
      Param2 |= GLIU_ID;
    case EVENT_IO_TIMEOUT:
      MBus_IO_Timeout(Param1, Param2, EnableFlag);
      break;

    case EVENT_PME:
      (USHORT)Param2 |= PME;
    case EVENT_GPIO:
      GPIO_Control(Param1, Param2, EnableFlag);
      break;

    case EVENT_PCI_TRAP:
      if (Enable_PCI_Trapping((USHORT)Param1, EnableFlag) == 0) {
        // Companion I/O can't trap requested PCI function.
        Error = ERR_PCI_TRAP;
      }
      break;

    case EVENT_ACPI:
      switch (Device_ID) {
        case DEVICE_ID_5536:
          Enable_ACPI_5536(EnableFlag);
          break;
      }
      break;

    case EVENT_PWM:
      //                GPIO pin         Duty cycle (%)    Rate (ms)
      Program_PWM((UCHAR)Param1, (UCHAR)(Param2 >> 16), (USHORT)(Param1 >> 16), EnableFlag);

      // 8 LSBs are GPIO pin number
      Param1 &= 0x000000FF;
      // Mask all except valid flags
      Param2 &= OPEN_DRAIN | PULLDOWN | PULLUP | INVERT;
      // Insert flags appropriate to PWM
      Param2 |= OUTPUT | NO_ASMI;
      if (EnableFlag) {
        Param2 |= AUX1;
      }
      GPIO_Control(Param1, Param2, EnableFlag);
      break;

  }	// end switch (Event)

  if (Error) {
    Report_VSM_Error(Error, Event, Param1);
  }
}



void pascal Set_Address_Decode(USHORT Address, UCHAR Decode) 
{
  Address_Decode(Decode, Address);
}




void pascal Allocate_Resource(USHORT Resource, ULONG Param)
{

  switch ((UCHAR)Resource) {

    case RESOURCE_IRQ:
      break;

    default:
      break;
  }
}



