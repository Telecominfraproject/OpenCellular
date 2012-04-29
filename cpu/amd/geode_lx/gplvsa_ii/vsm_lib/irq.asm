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
;*     Implements the SYS_GENERATE_IRQ & SYS_MAP_IRQ macros. 


include vsa2.inc
include sysmgr.inc

.model tiny,c
.586p
.CODE

externdef sys_system_call:proc

;***********************************************************************
;***********************************************************************
sys_map_irq proc pascal \
	Y_Source: byte, \
	IRQ:	byte
	
	mov	bl, [Y_Source]
	mov	cl, [IRQ]
	mov	ax, SYS_CODE_IRQ_MAPPER
	call	sys_system_call
	ret

sys_map_irq endp


;***********************************************************************
; void sys_generate_IRQ(USHORT Irq);
;
; Generates an IRQ upon exit from SMM
; NOTE:  If 8 MSBs of Irq are non-zero, then IRQ is set to external
;***********************************************************************
sys_generate_IRQ proc pascal  \
	Irq: word

	xor	bx, bx				; Get ptr to SysMgr's IRQ_Mask
	mov	ebx, (VSM_Header PTR [bx]).SysStuff.SysMgr_Ptr

	mov	cx, [Irq]			; Get IRQ number
	mov	eax, 00010001h			; Generate bit mask
	shl	eax, cl
	or	ch, ch				; Disable internal IRQ external ?
	jz	short AssertIRQ

	not	ax				; Yes
	mov	ebx, fs:(InfoStuff PTR [ebx]).IRQ_Base
	and	fs:[ebx], ax			; Mark IRQ as external
	jmp	short Exit


AssertIRQ:
	lea	ebx, (InfoStuff PTR [ebx]).IRQ_Mask
	or	fs:[ebx], eax			; Record IRQ to be generated

Exit:	ret

sys_generate_IRQ endp



	END 

