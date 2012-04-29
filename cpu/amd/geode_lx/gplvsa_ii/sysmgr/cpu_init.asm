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
;*     This file contains code specific to the processor core.

include vsa2.inc
include gx2.inc
include sysmgr.inc
include init.inc
include chipset.inc
  	
.model small,c
.586

.CODE


externdef BIOS_ECX: dword
externdef BIOS_EDX: dword
externdef VSA_Image:byte

SMM_Size	dd	0
FooGlue		dd	54000000h

;***********************************************************************
; Sets CPU dependent fields in the SMM header
; On entry:
;   SI = pointer to VSM header
;***********************************************************************
Set_CPU_Fields proc

SMM_CONTROL  equ EXTL_SMI_EN + INTL_SMI_EN + SMM_INST_EN + NEST_SMI_EN

	mov	(VSM_Header PTR [si]).SysStuff.State.SMM_CTL_MSR, SMM_CONTROL
	ret
	
Set_CPU_Fields endp





;***********************************************************************
; - Sets the SMM entry point to the location in EBX
; - Sets the SMM header location
; - Write protects the SMM regions
;***********************************************************************
Init_SMM_Region proc

	; Set the SMM entry point
	mov	ecx, MSR_SMM_LOC
	mov	eax, ebx
	mov	edx, [SMM_Size]		; SMM Code Limit
	wrmsr

	; Set the SMM header location
	lea	eax, (VSM_Header PTR [eax]).SysStuff.State + sizeof(SmiHeader)
	mov	ecx, MSR_SMM_HDR
	xor	edx, edx
	wrmsr


	; Write protect the SMM memory
	mov	ecx, MSR_RCONF_SMM
	rdmsr	
	or	al, REGION_WP
	wrmsr

	ret
	
Init_SMM_Region endp






;***********************************************************************
; Returns information about the LX CPU
; On exit:
;  AX = CPU Revision
;  SI = CPU ID
;  BX = PCI MHz
;  CX = CPU MHz
;  DX = DRAM MHz
;***********************************************************************
Get_CPU_Info proc

	mov	ecx, 4C000014h 					; GLCP_SYS_RSTPLL
	rdmsr

	; PCI Speed
	mov	bx, 33
	test al, 1 SHL 7					; RSTPPL_LOWER_PCISPEED_SHIFT
	jz	not66
	mov	bx, 66
not66:	
	push bx								; save PCI speed

	; CPU Speed
	mov	ax, 333							; 33.3MHZ * 10

	ror	dx, 1							; RSTPLL_UPPER_CPUMULT_SHIFT
	mov	bx, dx
	rol	dx, 1							; RSTPLL_UPPER_CPUMULT_SHIFT
	push dx								; save RSTPLL
	and	bx, 1Fh
	inc	bx								; 0 = multiply by 1....
	mul	bx								; ax=PCI * bl=Mul

	; Rounding divide
	mov	bx, 10
	xor	dx, dx
	div	bx								; ax= quotent and dx=remainder
	cmp	dx, 5
	jb	NoRound							; can round because of /10
	inc	ax								; round up
NoRound:
	pop dx								; restore RSTPLL
	pop bx								; restore PCI * 10
	push ax								; save CPU speed


	; DRAM speed
	mov	ax, 333							; 33.3MHZ * 10
	
	ror	dx, 7							; RSTPLL_UPPER_GLMULT_SHIFT
	mov	bx, dx
	rol	dx, 7							; RSTPLL_UPPER_GLMULT_SHIFT
	push dx								; save RSTPLL
	and	bx, 1Fh
	inc	bx								; 0 = multiply by 1....
	mul	bx								; ax=PCI * bl=Mul

	; Rounding divide
	mov	bx, 10
	xor	dx, dx
	div	bx								; ax= quotent and dx=remainder
	cmp	dx, 5
	jb	NoRoundmem						; can round because of /10
	inc	ax								; round up
NoRoundmem:

	; DDR is 1/2 GeodeLink speed.
	xor	dx, dx
	mov bx, 2
	div	bx
	push ax								; save mem speed

	; Get CPU Revision
	mov	cx, 0017h
	rdmsr
	push	ax
	
	mov	ecx, 10002000h	; Read MBIU0 Capabilities MSR
	rdmsr
	and	ah, 0Fh			; Extract 4 LSBs of DEVID
	cmp	ah, 04h			; Is it LX ?
	mov	si, DEVICE_ID_GX2
	jne	RestoreInfo
	mov	si, DEVICE_ID_LX	; Yes
	mov	[FooGlue], 4C000020h	; FooGlue is at Northbridge MCP + 20h
RestoreInfo:
	pop	ax			; Restore CPU Revision
	pop	dx			; Restore DRAM MHz	
	pop	cx			; Restore CPU MHz
	pop	bx			; Restore PCI MHz
	ret

Get_CPU_Info endp



;***********************************************************************
; Returns the SMM information
; On Exit:
;    EAX = SMM entry point
;     BX = Size of SMM memory in KB
;***********************************************************************
Get_SMM_Region proc 
	Local Attributes: byte

	mov	ecx, MSR_RCONF_DEFAULT
	rdmsr
	mov	[Attributes], al


	mov	ecx, [BIOS_ECX]		; Descriptor for SMM memory
	rdmsr
	cmp	cl, 27h			; P2D_BMO or P2D_RO ?
	jbe	BaseMaskOffset
	mov	ebx, eax	      	; P2D_RO
	shl	eax, 12			; EAX = SMM base
	shrd	ebx, edx, 8		; EBX = end of SMM range
	and	bx, 0F000h
	sub	ebx, eax		; Compute length of range
	add	ebx, 1000h		; Adjust for 4KB granularity
	shr	ebx, 10			; Convert to KB
	jmp	short Save_SMM_Base

BaseMaskOffset:       
	mov	ebx, eax		; BX = length of SMM memory in KB
	shrd	eax, edx, 8		; EAX = Base
	and	ax, 0F000h

	or	ebx, 0FFF00000h
	neg	ebx
	shl	ebx, 2			; Adjust for 4KB granularity

Save_SMM_Base:
	push	eax			; Save SMM base
	push	bx			; Save SMM size in KB

	shl	ebx, 10			; Convert KB to bytes
	dec	ebx
	mov	[SMM_Size], ebx		; Save size for MSR_SMM_LOC

	; Set SMM RCONF
	and	bx, 0F000h
	mov	edx, ebx		; SMM_TOP = SMM_BASE + sizeof(SMM region)
	add	edx, eax
	mov	dl, [Attributes]	; SMM active properties mirror RCONF_DEFAULT
	or	ah, 1 			; Set Enable
	mov	al, dl			; SMM inactive properties (R/W for now)
	and	al, NOT REGION_WP
	mov	ecx, MSR_RCONF_SMM
	wrmsr

	pop	bx			; BX = size of SMM region in KB
	pop	eax			; EAX = base of SMM region

	ret
	
Get_SMM_Region endp


;****************************************************************
; Enables SMIs
;****************************************************************
Enable_SMIs proc

	; Enable internal and external SMIs
	mov	ecx, MSR_SMM_CTRL
	rdmsr
	or	eax, INTL_SMI_EN + EXTL_SMI_EN
	wrmsr

	ret

Enable_SMIs endp	




;***********************************************************************
; Enables the SMM instructions
;***********************************************************************
Enable_SMM_Instr proc
	
	mov	ecx, MSR_SMM_CTRL
	rdmsr
	or	eax, SMM_INST_EN
	wrmsr
	ret

Enable_SMM_Instr endp




;***********************************************************************
; Disables the A20 masking at the processor.
;***********************************************************************
Disable_A20 proc

	mov	ecx, [FooGlue]
	add	cx, FG_A20M
	rdmsr
	and	al, NOT A20M		; A20M = 0
	wrmsr
	ret
	
Disable_A20 endp





;***********************************************************************
; Gets the size of physical memory
; On Exit:
;  EAX = physical memory size in bytes
;***********************************************************************
Get_Memory_Size proc	

	mov	ecx, [BIOS_EDX]		; P2D_R descriptor of physical memory
	jecxz	Exit
	rdmsr
; EDX = 2000001F
; EAX = FDF00100
; 1MB thru <512MB-192K)
	
	shrd	eax, edx, 20		; Extract PMAX field
	shl	eax, 12			; Convert units from 4KB to bytes
Exit:	ret
	
Get_Memory_Size endp






; The linker won't allow code to be put in a PARA aligned segment.
; The following will align the next module to a paragraph boundary.
TEXT	SEGMENT PARA 'CODE'

	db	16 dup (0)

TEXT	ENDS

	END
	