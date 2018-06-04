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

typedef unsigned short EVENT;
typedef unsigned short MSG;
typedef unsigned short PRIORITY;
typedef unsigned long VSM;
typedef void (* GPIO_FUNCTION)(unsigned long, unsigned long, unsigned char);



void pascal out_8(unsigned short, unsigned char);
void pascal out_16(unsigned short, unsigned short);
void pascal out_32(unsigned short, unsigned long);
unsigned char  pascal in_8(unsigned short);
unsigned short pascal in_16(unsigned short);
unsigned long  pascal in_32(unsigned short);

void pascal Hex_8(unsigned char);
void pascal Hex_16(unsigned short);
void pascal Hex_32(unsigned long);



extern void pascal Trap_PCI_IDSEL(unsigned short, unsigned char);
extern unsigned char pascal Init_Descr(unsigned char, unsigned long);
extern unsigned char pascal BitScanReverse(unsigned long);
extern unsigned char pascal BitScanForward(unsigned long);
extern unsigned long pascal read_flat(unsigned long);
extern unsigned long pascal Read_MSR_LO(unsigned long);
extern unsigned long pascal Read_MSR_HI(unsigned long);
extern unsigned long pascal Virtual_PCI_Read_Handler(unsigned short);
extern unsigned long pascal GetFlink(unsigned long);
extern unsigned char pascal Get_VSM_Type(unsigned long);
extern unsigned char pascal IsPowerOfTwo(unsigned long);
extern unsigned short pascal Send_Event(EVENT, VSM);
extern unsigned char pascal Find_Matching_IO_Descriptor(unsigned long *, unsigned short *, unsigned char);
extern unsigned char pascal Setup_IO_Descriptor(unsigned long *, unsigned short *, unsigned char);
extern void pascal Init_MBIU(unsigned char *, unsigned long);
extern void pascal Get_Descriptor_Default(unsigned char, unsigned long *);
extern unsigned long pascal Virtual_PCI_Write_Handler(unsigned short, unsigned char, unsigned long);
extern void pascal write_flat(unsigned long, unsigned long);
extern void pascal Read_MSR(unsigned long, unsigned long *);
extern void pascal Write_MSR(unsigned long, unsigned long *);
extern void pascal Write_MSR_LO(unsigned long, unsigned long);
extern void pascal Write_MSR_HI(unsigned long, unsigned long);
extern void pascal Store_Timestamp(void *);
extern void pascal Schedule_VSM(unsigned long);
extern void pascal Report_Error(unsigned char, unsigned long, unsigned long);
extern void pascal Report_VSM_Error(unsigned char, unsigned long, unsigned long);
extern void pascal Send_Message(VSM, VSM, unsigned long);
extern void pascal Broadcast_Message(MSG, unsigned short, unsigned long);
extern void pascal Register_Event(EVENT, PRIORITY, VSM, unsigned long, unsigned long);
extern void pascal Enable_Event(EVENT, unsigned short, unsigned char);
extern void pascal Disable_Event(EVENT, unsigned short);
extern void pascal Unregister_VSM_Events(VSM);

extern void pascal IRQY_Mapper(unsigned char, unsigned char);
extern void pascal IRQZ_Mapper(unsigned char, unsigned char);


extern void Keep_History(EVENT, EVENT);
extern unsigned short pascal Unregister_Event(EVENT, VSM, unsigned long, unsigned long);
extern unsigned short pascal Allocate_BAR(unsigned char, unsigned short, unsigned long, unsigned short, unsigned short);

extern unsigned long Current_VSM;
extern unsigned long SysMgr_VSM;


extern void Log_Error(const char *format, ...);
extern unsigned long pascal Compute_IOD_SC(unsigned long *, unsigned short *, unsigned char);
extern void pascal MBus_IO_Trap(unsigned long, unsigned long, unsigned char);



