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
;*     Debug routines for GX2-based systems   


include vsa2.inc
include vr.inc
include sysmgr.inc

.model tiny,c
.586p
.CODE


CR	equ	0Dh
LF	equ	0Ah

externdef SMM_Header: SmiHeader

;***********************************************************************
; Hex dump routines
;***********************************************************************
Hex_32	proc	pascal Num:dword

	pushad
	mov	ebx, Num
	mov	cx, 8
@@:	rol	ebx, 4
	call	Hex_4
	loop	@b
	call	Space
	popad
	ret

Hex_32	endp



Hex_16	proc	pascal Num:word

	pusha
	mov	cx, 4
	mov	bx, Num
@@:	rol	bx, 4
	call	Hex_4
	loop	@b
	call	Space
	popa
	ret

Hex_16	endp


Hex_8	proc	pascal Num:byte

	pusha
	mov	cx, 2
	mov	bl, Num
@@:	rol	bl, 4
	call	Hex_4
	loop	@b
	call	Space
	popa
	ret

Hex_8	endp


Hex_4:	mov	al, bl
	and	al, 0Fh
	add	al, '0'			; Convert to ASCII
	cmp	al, '9'
	jbe	@f
	add	al, 7			; 'A'-'F'
@@:	mov	dx, DBG_PORT
	jmp	Char
	
Space:	mov	al, ' '
Char:	out	dx, al
	in	al, 80h	
	ret





;***********************************************************************
; Displays SMI source(s)
;
; On Entry:
;  ECX = SMI source(s)
;***********************************************************************
Show_SMI_Source proc

	pushad
	cld

	mov	dx, DBG_PORT
	lea	bx, SMI_Source_Strings-2
MsgLoop:jecxz	Exit
	add	bx, 2
	shr	ecx, 1
	jnc	MsgLoop

	mov	si, [bx]		; Get message ptr
	cmp	si, OFFSET Msg_DescrHit
	jne	short CharLoop
	mov	ax, [SMM_Header].IO_addr
	cmp	ax, VRC_INDEX
	je	IsVirtualReg
	cmp	ax, VRC_DATA
	jne	CharLoop
IsVirtualReg:
	lea	si, [Msg_VirtReg]	
CharLoop:
	lodsb
	or	al, al			; End of string ?
	jz	Blank
	out	dx, al			; No, display next character
	jmp	CharLoop

Blank:	mov	al, ' '			; Yes, display trailing blank
	out	dx, al
	jmp	MsgLoop

Exit:	popad
	ret


Show_SMI_Source endp


SMI_Source_Strings:
	dw	OFFSET Msg_PM
	dw	OFFSET Msg_Audio
	dw	OFFSET Msg_ACPI
	dw	OFFSET Msg_VG
	dw	OFFSET Msg_Reserved
	dw	OFFSET Msg_Retrace
	dw	OFFSET Msg_VGA_Timer
	dw	OFFSET Msg_A20
	dw	OFFSET Msg_SW_SMI
	dw	OFFSET Msg_GTT
	dw	OFFSET Msg_Reserved
	dw	OFFSET Msg_MFGPT
	dw	OFFSET Msg_NMI
	dw	OFFSET Msg_Reset
	dw	OFFSET Msg_USB
	dw	OFFSET Msg_Graphics

	dw	OFFSET Msg_GT1
	dw	OFFSET Msg_GT2
	dw	OFFSET Msg_USR_DEF_1
	dw	OFFSET Msg_VirtReg
	dw	OFFSET Msg_USR_DEF_3
	dw	OFFSET Msg_PCI_Trap
	dw	OFFSET Msg_Reserved
	dw	OFFSET Msg_Reserved
	dw	OFFSET Msg_MPCI
	dw	OFFSET Msg_DescrHit
	dw	OFFSET Msg_Stat_Hit
	dw	OFFSET Msg_PIC
	dw	OFFSET Msg_KEL
	dw	OFFSET Msg_PME
	dw	OFFSET Msg_BlockIO
	dw	OFFSET Msg_Reserved


Msg_Reserved	db	'???',0
Msg_VG		db	'VG',0
Msg_PM		db	'PM',0
Msg_Audio	db	'Audio',0
Msg_ACPI	db	'ACPI',0
Msg_Retrace	db	'Vsync',0
Msg_VGA_Timer	db	'Vga',0
Msg_SW_SMI	db	'S/W',0
Msg_A20		db	'A20',0
Msg_GTT		db	'GTT',0
Msg_MFGPT	db	'MFGPT',0
Msg_NMI		db	'NMI',0
Msg_GT1		db	'GT1',0
Msg_GT2		db	'Timer',0
Msg_USB		db	'USB',0
Msg_Reset	db	'Reset',0
Msg_Graphics	db	'VGA',0
Msg_USR_DEF_1	db	'PIC',0
Msg_VirtReg	db	'VR',0
Msg_USR_DEF_3	db	'UDef3',0
Msg_PCI_Trap	db	'PCI',0
Msg_MPCI	db	'MPCI',0
Msg_DescrHit	db	'I/O',0
Msg_Stat_Hit	db	'StatHit',0
Msg_PIC		db	'PIC',0
Msg_KEL		db	'KEL',0
Msg_PME		db	'PME',0
Msg_BlockIO	db	'BlockIO',0





	end

