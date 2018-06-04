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
//*     This file contains code specific to CS5536 MFGPTs 
//*****************************************************************************


#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"
#include "MDD.H"
#include "MAPPER.H"
#include "TIMER.H"

// External variables:
extern ULONG MDD_Base;

// Local variables:
USHORT MFGPT_Base=0;
ULONG MFGPT_LBAR[2];
 


// NOTE: Timers that use the 32KHz clock interfere with LPC DMA (see PBZ 709)
  
                                                                      // Clock    Prescalar  Period
#define TIMER_SETUP (MFGPT_SCALE_1K | MFGPT_CMP1MODE | MFGPT_CLK_SEL) // 14 MHz     1024      71 us
#define STDBY_SETUP (MFGPT_SCALE_32 | MFGPT_CMP1MODE)                 // 32 KHz       32       1 ms
#define PWM_SETUP   (MFGPT_SCALE_32 | MFGPT_CMP1GE)                   // 32 KHz       32       1 ms
TIMERS TimerInfo[] = {
//  #       Z Mapper        Setup           
// Save for OS (Linux) use   { 0,   Z_IRQ_MFGPT_04,   PWM_SETUP},
  { 1,   Z_IRQ_MFGPT_15,   PWM_SETUP},
  { 2,   Z_IRQ_MFGPT_26,   PWM_SETUP},
  { 3,   Z_IRQ_MFGPT_37, TIMER_SETUP},
// Save for OS (Linux) use  { 4,   Z_IRQ_MFGPT_04, TIMER_SETUP},
  { 5,   Z_IRQ_MFGPT_15, TIMER_SETUP},
  { 6,   Z_IRQ_MFGPT_26, STDBY_SETUP},
  { 7,   Z_IRQ_MFGPT_37,   PWM_SETUP},
};
#define NUM_MFGPTS     (sizeof(TimerInfo)/sizeof(TIMERS))  // Number of MFGPTs available

//************************************************************************************
// Saves the MFGPT LBAR setting and enables the LBAR
//************************************************************************************
void Enable_MFGPT_LBAR()
{ ULONG Tmp;
  int i;

  // Get MFGPT LBAR
  Read_MSR(MDD_Base + MSR_LBAR_MFGPT, MFGPT_LBAR);

  // Has the MFGPT base address changed?
  if (MFGPT_Base != (USHORT)MFGPT_LBAR[0]) {
    // Yes, record the new base address of MFGPT
    MFGPT_Base = (USHORT)MFGPT_LBAR[0];

    // Update the individual timer base addresses
    for (i = 0; i < NUM_MFGPTS; i++) {
      TimerInfo[i].TimerBase = MFGPT_Base + TimerInfo[i].Timer*MFGPT_OFFSET;
    }
  }

  // Save the current LBAR enable
  Tmp = MFGPT_LBAR[1];

  // Enable the LBAR
  MFGPT_LBAR[1] |= LBAR_EN;
  Write_MSR(MDD_Base + MSR_LBAR_MFGPT, MFGPT_LBAR);
  MFGPT_LBAR[1] = Tmp;

}


//************************************************************************************
// Restores the MFGPT LBAR
//************************************************************************************
void Restore_MFGPT_LBAR()
{
  // Restore MFGPT LBAR
  Write_MSR(MDD_Base + MSR_LBAR_MFGPT, MFGPT_LBAR);
}


//************************************************************************************
// Checks for MFGPT events
// Returns a mask of which timer(s) have expired
//************************************************************************************
USHORT CS5536_MFGPT_Handler(void)
{ USHORT Setup, Status, Accum_Status=0, i;
  register TIMERS * TimerPtr;

  Enable_MFGPT_LBAR();

  for (i = 0; i < NUM_MFGPTS; i++) {

    TimerPtr = &TimerInfo[i];

    // Get timer status
    Setup = TimerPtr->TimerBase + MFGPT_SETUP;
    Status = in_16(Setup);
    if (Status & MFGPT_INITED) {
      out_16(Setup, Status);
    }

    // Check if MFGPT is reserved for EVENT_PWM
    if(TimerPtr->Setup == PWM_SETUP) {
      continue;
    }

    // If timer is enabled and expired...
    Status &= MFGPT_ENABLE | MFGPT_COMPARE1;
    if (Status == (MFGPT_ENABLE | MFGPT_COMPARE1)) {
      // disable the timer
      out_16(Setup, Status & (~MFGPT_ENABLE));
      // record timer event
      Accum_Status |= TimerPtr->Mask;
    }
  }

  // Restore the MFGPT LBAR
  Restore_MFGPT_LBAR();

  return Accum_Status;
}

//************************************************************************************
// Initializes a single MFGPT
//************************************************************************************
void Init_Timer(TIMERS * TimerPtr)
{ USHORT Setup, TimerKHz;
  ULONG MicrosPerCount;

  TimerPtr->TimerBase = MFGPT_Base + TimerPtr->Timer*MFGPT_OFFSET;

  TimerPtr->Mask = 1 << TimerPtr->Timer;

  Setup = TimerPtr->Setup;

  // Compute microseconds / tick
  MicrosPerCount = 1000L << (Setup & 0xF);
  TimerKHz = 32;
  if (Setup & MFGPT_CLK_SEL) {
    TimerKHz = 14318;
  }
  TimerPtr->Period = (USHORT)(MicrosPerCount / TimerKHz);

  // Initialize the timer
  out_16(TimerPtr->TimerBase + MFGPT_SETUP, Setup);

  // NOTE: Counter resets to 0 when Counter == Comparator2 register
  out_32(TimerPtr->TimerBase + MFGPT_CMP1,  0xFFFFFFFF);

}

//***********************************************************************
// Marks a MFGPT as available
//***********************************************************************
void pascal MarkTimerAvailable(USHORT Timer)
{ USHORT i;

  for (i = 0; i < NUM_MFGPTS; i++) {
    if (TimerInfo[i].Timer == Timer) {
      TimerInfo[i].Interval = 0x00000000;
      return;
    }
  }

  Log_Error("Invalid timer: %d.", Timer);
}


//************************************************************************************
// Initializes the MFGPT timers to be used for EVENT_TIMER
//************************************************************************************
void Init_MFGPT(void)
{ ULONG MsrAddr, IRQ_Enables;
  USHORT i;
  register TIMERS * TimerPtr;

  Enable_MFGPT_LBAR();

  // Get current MFGPT IRQ mask
  MsrAddr = MDD_Base;
  (USHORT)MsrAddr = MSR_MFGPT_IRQ;
  IRQ_Enables = Read_MSR_LO(MsrAddr);

  for (i = 0; i < NUM_MFGPTS; i++) {

    TimerPtr = &TimerInfo[i];

    // Initialize a MFGPT
    Init_Timer(TimerPtr);

    // Mark timer as available
    MarkTimerAvailable(TimerPtr->Timer);

    // Route Compare 1 events to SMI
    IRQZ_Mapper(TimerPtr->Mapper, 2);

    // Enable timer event
    IRQ_Enables |= TimerPtr->Mask;

  }


  // Restore the MFGPT LBAR
  Restore_MFGPT_LBAR();


  // Enable MFGPT SMIs
  Write_MSR_LO(MsrAddr, IRQ_Enables);

} 



//***********************************************************************
// Disables a millisecond timer
//***********************************************************************
void DisableMsTimer_5536(USHORT Timer)
{ USHORT Setup, i;

  Enable_MFGPT_LBAR();


  for (i = 0; i < NUM_MFGPTS; i++) {
    if (TimerInfo[i].Timer == Timer) {
      // Mark timer as available
      MarkTimerAvailable(Timer);

      // Clear timer event and disable timer
      Setup = TimerInfo[i].TimerBase + MFGPT_SETUP;
      out_16(Setup, MFGPT_COMPARE1 | MFGPT_COMPARE2);
      break;
    }
  }

  // Restore the MFGPT LBAR
  Restore_MFGPT_LBAR();

}



//***********************************************************************
// Allocates a h/w timer for the specified interval
// Returns the timer # allocated else 0xFFFF
//***********************************************************************
USHORT AllocateTimer_5536(ULONG Interval, UCHAR Attributes)
{ UCHAR i, Exact = 0, Unused=0, Compat=0, Mask;
  register TIMERS * TimerPtr;

  // Find the MFGPT whose timebase is the best-match
  for (i = 0; i < NUM_MFGPTS; i++) {
    TimerPtr = &TimerInfo[i];

    // Don't use timers reserved for EVENT_PWM
    if (TimerPtr->Setup == PWM_SETUP) {
      continue;
    }

    if (Attributes & (FOR_STANDBY >> 24)) {
      // Timers used for PM must be in the 32 KHz domain
      if (TimerPtr->Setup & MFGPT_CLK_SEL) {
        continue;
      }
    } else {
      if (TimerPtr->Timer >= 6) {
        // Timers 6 & 7 are reserved for PM
        continue;
      }
    }
    Mask = (UCHAR)TimerPtr->Mask;

    // Is timer unused?
    if (TimerPtr->Interval == 0x00000000) {
      Unused |= Mask;
	}

    // Is timebase a perfect match?
    if ((Interval*1000 % TimerPtr->Period) == 0) {
      Exact |= Mask;
    }

    // Is timebase compatible?
    if (Interval*1000 >= TimerPtr->Period) {
      Compat |= Mask;
    }
  }

  // Is there an UNUSED timer whose timebase is an exact match?
  if (!(Mask = (Unused & Exact))) {
    // No, is there an UNUSED timer that is compatible?
    if (!(Mask = (Unused & Compat))) {
      // No, is there ANY timer whose timebase is an exact match?
      if (!(Mask = Exact)) {
        // No, is there ANY timer that is compatible?
        if (!(Mask = Compat)) {
          // No timers are available
          return 0xFFFF;
        }
      }
    }
  }

  // Return the MFGPT index
  Mask = 1 << BitScanReverse(Mask);
  for (i = 0; i < NUM_MFGPTS; i++) {
    if (TimerInfo[i].Mask == Mask) {
      return i;
    }
  }
  return 0xFFFF;
}


//***********************************************************************
// Enables a millisecond timer
//***********************************************************************
UCHAR EnableMsTimer_5536(ULONG Interval, UCHAR Attributes)
{ USHORT i, Count, TimerBase;
  ULONG MsrAddr, IRQ_Enables, TotalCounts, Roundoff;
  register TIMERS * TimerPtr;

  // Get current MFGPT IRQ mask
  MsrAddr = MDD_Base;
  (USHORT)MsrAddr = MSR_MFGPT_IRQ;
  IRQ_Enables = Read_MSR_LO(MsrAddr);

  // Allocate a h/w timer
  i = AllocateTimer_5536(Interval, Attributes);
  if (i == 0xFFFF) {
    Log_Error("Attempt to enable %d ms timer interval failed", Interval);
    return 0;
  }

  TimerPtr = &TimerInfo[i];

  // A timer was allocated, is it already in use?
  if (TimerPtr->Interval) {
    // Yes, does it need to be reprogrammed?
    if (Interval > TimerPtr->Interval) {
      // No, but since the timer has already been running for an
      // indeterminate period, the 1st interval will be shortened.
      return (UCHAR)TimerPtr->Timer;
    }
  }


  if (Interval) {

    Enable_MFGPT_LBAR();

    // Get I/O address of this timer
    TimerBase = TimerPtr->TimerBase;

    // The MFGPT document specifies the following sequence in order
    // to prevent any spurious resets, interrupt, or output events:
    // - Set Counter Enable bit to 0.
    // - Clear SMI enable in MSR_MFGPT_IRQ
    // - Update Count as desired
    // - When updates are completed, clear event bits
    // - Set SMI enable in MSR_MFGPT_IRQ
    // - Set Counter Enable bit to 1

    // Does this timer need to be initialized again (e.g. after S3) ?
    if (!(in_16(TimerBase + MFGPT_SETUP) & MFGPT_INITED)) {
      // Initialize the timer
      Init_Timer(TimerPtr);
    }

    // Set Counter Enable to 0
    out_16(TimerBase + MFGPT_SETUP, 0);

    // Disable SMIs
    Write_MSR_LO(MsrAddr, 0x00000000);

    // Scale the interval appropriately
    Roundoff = 0;
    if (TimerPtr->Period < 1000) {
      Roundoff = TimerPtr->Period;
    }
    TotalCounts = (Interval*1000 + Roundoff)/TimerPtr->Period;
    Count = (USHORT)TotalCounts;
    if (TotalCounts & 0xFFFF0000) {
	  Count = 0xFFFF;
    }

    // Record the timer's interval
    TimerPtr->Interval = ((ULONG)Count * TimerPtr->Period)/1000;

    // Zero the counter
    out_16(TimerBase + MFGPT_COUNTER, 0x0000);

    // Set Comparator1 to Count and Comparator2 to 0xFFFF
    out_32(TimerBase + MFGPT_CMP1, 0xFFFF0000L | Count);

    // Clear timer event(s)
    out_16(TimerBase + MFGPT_SETUP, MFGPT_COMPARE1 | MFGPT_COMPARE2);

    // Re-enable SMIs
    Write_MSR_LO(MsrAddr, IRQ_Enables);

    // Set Counter Enable to 1
    out_16(TimerBase + MFGPT_SETUP, MFGPT_ENABLE);

    // Restore the MFGPT LBAR
    Restore_MFGPT_LBAR();
  }

  return (UCHAR)TimerPtr->Timer;
}


//***********************************************************************
// Configures a MFGPT as a PWM signal generator
//***********************************************************************
void pascal Program_PWM(UCHAR Gpio, UCHAR Duty, USHORT Rate, UCHAR EnableFlag)
{ USHORT i, Timer;
  register TIMERS * TimerPtr;

  // Determine the MFGPT associated with the specified GPIO
  switch (Gpio) {
    case 5:
      Timer = 0;
      break;
    case 6:
      Timer = 1;
      break;
    case 7:
      Timer = 2;
      break;
    case 27:
      Timer = 7;
      break;
    default:
      Log_Error("EVENT_PWM is not supported on GPIO%d", Gpio);
      return;
  }

  // Program the MFGPT to the specified rate and duty cycle
  for (i = 0; i < NUM_MFGPTS; i++) {

    TimerPtr = &TimerInfo[i];

    if (TimerPtr->Timer == Timer) {

      // Check if MFGPT is reserved for EVENT_PWM
      if(TimerPtr->Setup != PWM_SETUP) {
        continue;
      }

      // Enable the MFGPT LBAR
      Enable_MFGPT_LBAR();

      // Clear this timer's SETUP register in case it has been used previously as a timer
      // Write_MSR_LO(MDD_Base + MSR_MFGPT_CLR_SETUP, TimerPtr->Mask);
      // out_16(TimerPtr->TimerBase + MFGPT_SETUP, PWM_SETUP);


      // Clamp duty cycle to 100%
      if (Duty > 100) {
        Duty = 100;
      }

      // Program the Count registers
      out_16(TimerPtr->TimerBase + MFGPT_COUNTER, 0x0000);
      out_16(TimerPtr->TimerBase + MFGPT_CMP1, (USHORT)(((ULONG)Rate * Duty)/100));
      out_16(TimerPtr->TimerBase + MFGPT_CMP2,  Rate);

      // Enable the counter
      out_16(TimerPtr->TimerBase + MFGPT_SETUP, EnableFlag ? MFGPT_ENABLE: 0);

      Restore_MFGPT_LBAR();

      return;
	}
  }

  Log_Error("EVENT_PWM on GPIO%d failed because MFGPT%d is reserved.", Gpio, Timer);
}

