; 
; Copyright (c) 2006-2008 Advanced Micro Devices,Inc. ("AMD").
; 
; This library is free software; you can redistribute it and/or modify
; it under the terms of the GNU Lesser General Public License as
; published by the Free Software Foundation; either version 2.1 of the
; License, or (at your option) any later version.
; 
; This code is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
; Lesser General Public License for more details.
; 
; You should have received a copy of the GNU Lesser General
; Public License along with this library; if not, write to the
; Free Software Foundation, Inc., 59 Temple Place, Suite 330,
; Boston, MA 02111-1307 USA 
; 
;*   Function:                                                         *
;*     This file implements the SYS_SW_INTERRUPT macro.  

include sysmgr.inc
.model tiny,c
.586p
.CODE



externdef sys_system_call:proc



;***********************************************************************
; void sys_software_interrupt(USHORT Interrupt, Regs * RegsBuffer);
;
; This routine performs the equivalent of a software INT.            
;
; Input:
;   Interrupt = interrupt to execute
;   RegBuffer = ptr to buffer containing general-purpose registers (16 bit)
;
;***********************************************************************

sys_software_interrupt proc pascal \
	Interrupt:	WORD,	\
	RegsBuffer:	PTR Regs

	movzx   ecx, [RegsBuffer]		; Get ptr to Regs[]
	movzx   ebx, [Interrupt]		; Get 4 * INT#
	shl	bx, 2
	mov     ax, SYS_CODE_SW_INT
	call    sys_system_call
	ret

sys_software_interrupt endp



;***********************************************************************
; void sys_state(USHORT Flag, UCHAR * Buffer);
;
; This routine saves/restores the non-SMM state. 
; It's intended purpose is to facilitate SaveToRAM and SaveToDisk VSMs.
;
; Input:
;   Flag:      0 = Save   1 = Restore
;   RegBuffer: Ptr to state buffer.  Must be STATE_SIZE bytes
;
;***********************************************************************
sys_state proc pascal uses di \
	Flag:	WORD,	\
	Buffer:	PTR byte

	movzx   edi, [Buffer]
	mov     cx, [Flag]

	mov     ax, SYS_CODE_STATE
	call    sys_system_call

	ret

sys_state endp



	END 

