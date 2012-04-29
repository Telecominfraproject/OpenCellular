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
//*     This file contains code specific to the IRQ Mapper  
//******************************************************************************

#include "VSA2.H"
#include "SYSMGR.H"
#include "PROTOS.H"
#include "VPCI.H"
#include "PCI.H"
#include "MDD.H"
#include "MAPPER.H"
#include "ISA.H"

// Local Variables:
UCHAR PCI_INTs[4] = {0,0,0,0};   // Current PCI INT mappings
UCHAR Max_PCI_Interrupt=4;

// External Variables:
extern ULONG MDD_Base;
extern USHORT Saved_AX;




// External Functions:
void PCI_Interrupts(void);



//************************************************************************************
// Maps an Unrestricted Source to a PIC IRQ via the IRQ Mapper logic
//************************************************************************************
void pascal IRQ_Mapper(UCHAR Reg, UCHAR Source, UCHAR Irq)
{ ULONG MsrAddr, MsrData;
  UCHAR ShiftCount;

  // 8 sources per MSR 
  Reg += Source/8;

  MsrAddr = MDD_Base + Reg;

  // 4 bits per field
  ShiftCount = (Source % 8) * 4;

  // Map Unrestricted Source[Source] to Irq via IRQ Mapper
  MsrData = Read_MSR_LO(MsrAddr);
  MsrData &= ~(0x0000000FL << ShiftCount);
  MsrData |=  (ULONG)Irq   << ShiftCount;
  Write_MSR_LO(MsrAddr, MsrData);
}

//************************************************************************************
// Maps an Unrestricted Source Y to a PIC IRQ
//************************************************************************************
void pascal IRQY_Mapper(UCHAR Y_Source, UCHAR Irq)
{ 
  IRQ_Mapper(MSR_IRQM_YLOW, Y_Source, Irq);
}


//************************************************************************************
// Maps an Unrestricted Source Z to a PIC IRQ
//************************************************************************************
void pascal IRQZ_Mapper(UCHAR Z_Source, UCHAR Irq)
{
  IRQ_Mapper(MSR_IRQM_ZLOW, Z_Source, Irq);
}



//************************************************************************************
// Implements the SYS_GENERATE_IRQ macro for CS5536
//************************************************************************************
void pascal Generate_IRQ_5536(USHORT IRQ_Mask) 
{ UCHAR Irq=0;
  USHORT Level;
  
  Level = in_16(PIC1_EDGE);

  // Don't attempt to generate a level-sensitive interrupt
  if (IRQ_Mask & Level) {
    Log_Error("Attempt to generate a level-sensitive IRQ. Mask= 0x%04X", IRQ_Mask);
    return;
  }       

  while (IRQ_Mask) {
    if (IRQ_Mask & 1) {
      // Map the software IRQ to the requested IRQ
      IRQY_Mapper(Y_IRQ_SW, Irq);

      // Generate an edge to the PIC
      Write_MSR_LO(MDD_Base + MSR_SOFT_IRQ, 0L);
      Write_MSR_LO(MDD_Base + MSR_SOFT_IRQ, 1L);
    }
    IRQ_Mask >>= 1;
	Irq++;
  }
}

