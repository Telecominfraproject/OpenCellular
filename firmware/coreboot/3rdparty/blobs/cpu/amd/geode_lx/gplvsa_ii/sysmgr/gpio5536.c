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
//*     This file contains code specific to CS5536 GPIOs
//******************************************************************************



#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"
#include "VPCI.H"
#include "PCI.H"
#include "MDD.H"
#include "MAPPER.H"


#define POWER_BUTTON_PIN     28

// External Functions:
extern void Send_GPIO_Event(USHORT Pin, USHORT Edge);
extern void Enable_PME_Event(UCHAR, UCHAR, UCHAR, USHORT);
extern UCHAR pascal BitScanReverse(ULONG);

// External Variables:
extern PCI_HEADER_ENTRY ISA_Hdr[];
extern GPIO_FUNCTION GPIO_Control;

// Local Variables:
ULONG Enable_Mask;
USHORT Lock_Port, Lock_Data;
USHORT GPIO_Port;
UCHAR  Bank=0;
UCHAR  ShiftCount;



typedef struct {
  UCHAR SelectOffset;  // Offset to filter select register
  UCHAR FilterOffset;  // Offset to filter amount register
  UCHAR Pin;           // Pin this filter is associated to
  UCHAR FilterRestore;
} FILTERS;

FILTERS FilterInfo[8] = {
 {GPIO_FILTER_SELECT0, GPIO_FILTER_AMOUNT+0*8, 0xFF, 0},
 {GPIO_FILTER_SELECT1, GPIO_FILTER_AMOUNT+1*8, 0xFF, 0},
 {GPIO_FILTER_SELECT2, GPIO_FILTER_AMOUNT+2*8, 0xFF, 0},
 {GPIO_FILTER_SELECT3, GPIO_FILTER_AMOUNT+3*8, 0xFF, 0},
 {GPIO_FILTER_SELECT4, GPIO_FILTER_AMOUNT+4*8, 0xFF, 0},
 {GPIO_FILTER_SELECT5, GPIO_FILTER_AMOUNT+5*8, 0xFF, 0},
 {GPIO_FILTER_SELECT6, GPIO6_FILTER_AMOUNT,    28,   1}, // power button pre-defined
 {GPIO_FILTER_SELECT7, GPIO7_FILTER_AMOUNT,    0xFF, 0},
};




//*****************************************************************************
// Sends EVENT_GPIO messages for pins that have the specified edge status pending
//*****************************************************************************
void GPIO_Helper(UCHAR Edge, UCHAR PinBase)
{ ULONG PinMask;
  UCHAR Pin;

  if (Edge == RISING_EDGE) {
    (UCHAR)GPIO_Port = GPIO_POSEDGE_STATUS;
  } else {
    (UCHAR)GPIO_Port = GPIO_NEGEDGE_STATUS;
  }
  if (PinBase) { 
    (UCHAR)GPIO_Port |= GPIO_HIGH_BANK_SELECT;
  }

  // Get edge status
  PinMask = in_32(GPIO_Port);

  // Clear the edge status
  out_32(GPIO_Port, PinMask);

  // Ignore transitions that are not enabled
  GPIO_Port -= GPIO_POSEDGE_STATUS;
  GPIO_Port += GPIO_POSEDGE_ENABLE;
  PinMask &= in_32(GPIO_Port) & 0x0000FFFFL;

  // Loop through all pending GPIO edges
  while (PinMask) {

    // Determine next pending GPIO pin
    Pin = BitScanReverse(PinMask);
    PinMask &= ~(1L << Pin);

    // Send the event to the VSM(s)
    Send_GPIO_Event(PinBase+Pin, Edge);
  }
}

//*****************************************************************************
// Handles GPIO SMIs
//*****************************************************************************
void CS5536_GPIO_Handler(ULONG Active_GPIOs)
{
  // Get the current GPIO address
  GPIO_Port = ISA_Hdr[BAR1/4].Value_LO;
  Bank = GPIO_LOW_BANK_SELECT;
  GPIO_Helper( RISING_EDGE,  0);  // Handle GPIO[15:00]  rising edges
  GPIO_Helper(FALLING_EDGE,  0);  // Handle GPIO[15:00] falling edges
  Bank = GPIO_HIGH_BANK_SELECT;
  GPIO_Helper( RISING_EDGE, 16);  // Handle GPIO[31:16]  rising edges
  GPIO_Helper(FALLING_EDGE, 16);  // Handle GPIO[31:16] falling edges
}




//*****************************************************************************
// Unlocks specified GPIO register(s).
// After GPIO registers are accessed, Relock_GPIO_Regs() should be invoked.
//*****************************************************************************
void Unlock_GPIO_Regs(USHORT UnlockMask)
{
  // Get current Lock register
  Lock_Port = ISA_Hdr[BAR1/4].Value_LO;
  (UCHAR)Lock_Port = Bank + GPIO_LOCK_ENABLE;
  Lock_Data = in_16(Lock_Port);

  // Unlock required registers
  out_16(Lock_Port, Lock_Data & ~UnlockMask);

}

//*****************************************************************************
// Restores GPIO register lock
//*****************************************************************************
void Relock_GPIO_Regs(void)
{
  out_16(Lock_Port, Lock_Data);
}



//*****************************************************************************
// Writes a 32-bit value to a GPIO register
//*****************************************************************************
void Write_GPIO_Register(UCHAR Register, ULONG Data) 
{ 
  // Write register
  (UCHAR)GPIO_Port = Bank + Register;
  out_32(GPIO_Port, Data);

  // Handle registers that are slow due to being in the Standby power domain
  if (Bank == GPIO_HIGH_BANK_SELECT) {
    in_32(GPIO_Port);
  }
}

//*****************************************************************************
// Writes Enable_Mask to a GPIO register
//*****************************************************************************
void Configure_GPIO(UCHAR Reg) 
{
  Write_GPIO_Register(Reg, Enable_Mask);
}


//*****************************************************************************
// Maps the specified GPIO pin to the specified GPIO Interrupt
//*****************************************************************************
void GPIO_Mapper(UCHAR Pin, UCHAR Irq)
{ ULONG Data;

  // Unlock GPIO register(s)
  Unlock_GPIO_Regs(LKIE | LKOE | LKPU | LKPD);

  // Configure pin as Input
  if (Pin != POWER_BUTTON_PIN) {  // Keeps sleep pin from generating SMI
    Configure_GPIO(GPIO_INPUT_ENABLE);
  }

  // Disable Output, Pullup & Pulldown
  Data = Enable_Mask << 16;
  Write_GPIO_Register(GPIO_OUTPUT_ENABLE,   Data);
  Write_GPIO_Register(GPIO_PULLDOWN_ENABLE, Data);
  Write_GPIO_Register(GPIO_PULLUP_ENABLE,   Data);

  // Restore Lock register
  Relock_GPIO_Regs();


  // Program GPIO Mapper
  (UCHAR)GPIO_Port = (UCHAR)(GPIO_MAPPER_X + ((Pin >> 1) & 0x0C));
  ShiftCount = (Pin % 8) * 4;
  Data  = in_32(GPIO_Port);

  Data &= ~(0x0000000FL << ShiftCount);
  Data |=  (ULONG)Irq   << ShiftCount;
  out_32(GPIO_Port, Data);

}




//*****************************************************************************
// Clears the positive & negative edge status registers for the specified GPIOs
//*****************************************************************************
void ClearEdgeStatus(ULONG Mask)
{
  Write_GPIO_Register(GPIO_POSEDGE_STATUS, Mask);
  Write_GPIO_Register(GPIO_NEGEDGE_STATUS, Mask);
}



//*****************************************************************************
// Enables/Disables generation of SMI on GPIO edge(s) for the CS5536 chipset
//*****************************************************************************
void CS5536_GPIO_Control(ULONG Param1, ULONG Param2, UCHAR EnableFlag)
{ ULONG Attributes;
  UCHAR Pin, Pme, Pm1, i, PinToMatch, FilterAmount, filter_to_use;
  ULONG Clear_Mask;
  

  // Unpack parameters
  Pin = (UCHAR)Param1;
  Pme = (UCHAR)(Param1 >> 16);
  Pm1 = (UCHAR)(Param2 >> 16);
  Attributes = Param2 & 0xff00ffff;

  GPIO_Port = ISA_Hdr[BAR1/4].Value_LO;

//  Log_Error("GPIO %d (%d) %d %d 0x%08X 0x%08X 0x%08X\r\n",Pin,EnableFlag,Pme,Pm1,Attributes,Param1,Param2);
  // Validate parameters
  if (Pin >= 32) {
    Log_Error("Invalid GPIO pin # = 0x%02X\r\n", Pin);
    return;
  }
  if ((Pme >  7) && (Attributes & PME)) {
    Log_Error("Invalid PME # = 0x%02X\r\n", Pme);
    return;
  }
  if ((Pm1 > 10) && (Attributes & PM1)) {
    Log_Error("Invalid PM1 # = 0x%02X\r\n", Pm1);
    return;
  }

  // Select Low/High bank
  ShiftCount = Pin;
  Bank = GPIO_LOW_BANK_SELECT;
  if (Pin >= 16) {
    ShiftCount -= 16;
    Bank = GPIO_HIGH_BANK_SELECT;
  }
  // Generate masks
  Clear_Mask = Enable_Mask = 1L << ShiftCount;

  // Clear possible false edge events
  ClearEdgeStatus(Clear_Mask);

  // If disable, set 16 MSBs of Enable_Mask
  if (!EnableFlag) {
    Enable_Mask <<= 16;
  }

  // Set GPIO Interrupt/PME Mapper to generate an ASMI or PME
  if (Attributes & PME) {
    // Map the GPIO as a PME
    GPIO_Mapper(Pin, (UCHAR)(Pme | 0x08));
  } else {
    // Route to ASMI unless PCI interrupt is specified
    if (Pme) {
      // Map the GPIO to a GPIO Interrupt
      GPIO_Mapper(Pin, (UCHAR)(Pme & 0x07));
    } else {
      if (!(Attributes & NO_ASMI)) {
        // Map the GPIO to an ASMI
        GPIO_Mapper(Pin, (Z_IRQ_SMI & 0x07));
      }
    }
  }



  // This will set the MDD MSR_SMI bit for SMI on any PM event
  // and program the appropriate ACPI PM1_EN or GPE0_EN bit.
  // Additionally, it will clear the corresponding ACPI PM1_STS or GPE0_STS bit.
  Enable_PME_Event(EnableFlag, Pm1, Pme, (USHORT)Attributes);

  // If power button, don't touch any GPIO regs!
  if (Pin == POWER_BUTTON_PIN) {
    return;
  }


  // Unlock GPIO registers
  Unlock_GPIO_Regs(LKPE | LKNE | LKFE | LKEE);

  // Configure as Output ?
  if (Attributes & OUTPUT) {

    Configure_GPIO(GPIO_OUTPUT_ENABLE);

    // Configure output pin inversion
    if (Attributes & INVERT) {
      Configure_GPIO(GPIO_OUTPUT_INVERT);
    }

    // Configure open drain
    if (Attributes & OPEN_DRAIN) {
      Configure_GPIO(GPIO_OUTPUT_OPENDRAIN);
    }

    // Configure AUX output selects
    if (Attributes & AUX1) {
      Configure_GPIO(GPIO_OUT_AUX1_SELECT);
    } else {
      if (Attributes & AUX2) {
        Configure_GPIO(GPIO_OUT_AUX2_SELECT);
      }
	}
  }


  // Configure as Input ?
  if ((Attributes & INPUT) || !(Attributes & OUTPUT)) {
    // Configure input pin inversion
    if (Attributes & INVERT) {
      Configure_GPIO(GPIO_INPUT_INVERT);
    }

    if (Attributes & AUX1) {
      Configure_GPIO(GPIO_IN_AUX1_SELECT);
    }



    // Configure input for correct edge(s)
   	switch (Attributes & BOTH_EDGES) {

      case BOTH_EDGES:
   	  case RISING_EDGE:
       	// Enable/Disable the RISING edge
        Configure_GPIO(GPIO_POSEDGE_ENABLE);
   	    if ((Attributes & BOTH_EDGES) == RISING_EDGE) {
       	  break;
        }
   	    // fall through intended

      case FALLING_EDGE:
   	    // Enable/Disable the FALLING edge
       	Configure_GPIO(GPIO_NEGEDGE_ENABLE);
        break;

   	  default:
       	// If neither edge is enabled, an event is 'active' as long
        // as the input is high (or low if inverted).
   	    break;
    }
  }


  // NOTE: If pullup is enabled, pulldown is automatically disabled
  // Configure pullup
  if (Attributes & PULLUP) {
    Configure_GPIO(GPIO_PULLUP_ENABLE);
  }	else {
    // Configure pulldown
    if (Attributes & PULLDOWN) {
      Configure_GPIO(GPIO_PULLDOWN_ENABLE);
    }
  }


  // Configure debounce filter
  if (Attributes & DEBOUNCE) {
    PinToMatch = (UCHAR)Pin;

    if (EnableFlag) {
      FilterAmount = 1;	   // Units = ~30 usec
    } else {
      FilterAmount = 0;
    }

    // Find a filter
    i=0;
    if (Attributes & FOR_STANDBY ) {
        i=6;    // if pme 6/7 set for filter 6/7 (in standby domain)
    }
    filter_to_use=0xff;

    for (; i < 8; i++) {
      if (FilterInfo[i].Pin == PinToMatch) {
        filter_to_use=i;
        break;
      }
      // see if we have found an open filter - use only for enable
      if ((filter_to_use == 0xff) && (FilterInfo[i].Pin == 0xff) && (EnableFlag)) {
        filter_to_use=i;
      } 
    }
//    Log_Error("GPIO %d using filter %d\r\n",Pin,filter_to_use);
    // found an matching pin for filter or an open one 
    if (filter_to_use != 0xff) {    // found one
        i=filter_to_use;  // use this filter
        if (EnableFlag) {
          // Mark filter in-use
          FilterInfo[i].Pin = (UCHAR)Pin;

          // Save the filter select register
          (UCHAR)GPIO_Port = FilterInfo[i].SelectOffset;
          FilterInfo[i].FilterRestore = in_8(GPIO_Port);

          // Select the filter
          out_8(GPIO_Port, (UCHAR)Pin);
        }

        // Program filter Amount & Count
        (UCHAR)GPIO_Port = FilterInfo[i].FilterOffset;
        out_32(GPIO_Port, (ULONG)FilterAmount);

        if (!EnableFlag) {
          // Mark filter as available
          FilterInfo[i].Pin = 0xFF;

          // Restore the filter select register
          (UCHAR)GPIO_Port = FilterInfo[i].SelectOffset;
          out_8(GPIO_Port, FilterInfo[i].FilterRestore);
        }
        // Enable/Disable filter
        Configure_GPIO(GPIO_IN_FILTER_ENABLE);
    }
    else {
      // no filter found
      Log_Error("No filter found for GPIO %d\r\n",Pin);
    }
  }

  // Enable/Disable GPIO as an event unless it is EVENT_PWM
  if ((Attributes & (OUTPUT | NO_ASMI)) != (OUTPUT | NO_ASMI)) {
    Configure_GPIO(GPIO_EVENTS_ENABLE);
  }


  // Clear possible false edge events
  ClearEdgeStatus(Clear_Mask);


  // Restore Lock register
  Relock_GPIO_Regs();

}




//*****************************************************************************
// Initialized CS5536 GPIO logic
//*****************************************************************************
void CS5536_GPIO_Init(void)
{

  // Point GPIO_Control to correct GPIO routine for CS5536
  GPIO_Control = CS5536_GPIO_Control;

  // Clear pending GPIOs
  CS5536_GPIO_Handler(0);

  // Map the GPIO SMI source
  IRQZ_Mapper(Z_IRQ_SMI, 2);

}
