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
;*     This file library functions for accessing descriptors  

include SYSMGR.INC
include VSA2.INC
include SMIMAC.MAC

.model tiny,c
.586p
.CODE




externdef Function:  byte
externdef ValidateMacro:proc
externdef ParamError:proc



;***********************************************************************
; void sys_get_descriptor(USHORT, void *)
;***********************************************************************
sys_get_descriptor proc pascal uses si\
        seg_reg: WORD, \
	buffer:	 PTR


	mov	[Function], GET_DESCR	; In case of error
	movzx	eax, [seg_reg]		; Get which register
	call	ValidateMacro		; Get ptr to registers
	jc	Exit

	mov	si, [buffer]		; Destination ptr
	cmp	ax, R_CS
	je	Get_CS


	cmp	ax, R_GS		; Validate offset
	ja	Error
	
	; Get descriptor from SysMgr's state save area
	xor	ah, ah			; Extract offset
	lea	ebx, [eax-8]		; Offset points to .selector
	mov	ax,  fs:[eax]		; Copy 10 bytes of descriptor info
	mov	[si+8], ax
	mov	eax, fs:[ebx]
	mov	[si], eax
	mov	eax, fs:[ebx+4]
	mov	[si+4], eax
	jmp	Exit


	; Get CS from SMM header.
	; Convert from linear format to descriptor format
Get_CS:
	ASSUME	SI: PTR Descriptor
	; CS base
	mov	eax, fs:(SmiHeader PTR [ecx])._CS.base
	mov	[si].base_15_0, ax
	shr	eax, 16
	mov	[si].base_23_16, al
	mov	[si].base_31_24, ah

	; CS limit
	mov	eax, fs:(SmiHeader PTR [ecx])._CS.limit
	test	fs:(SmiHeader PTR [ecx])._CS.attr, 8000h
	jz	short SetLimit
	shr	eax, 12
	or	eax, 800000h		; Set G bit
SetLimit:
	mov	[si].limit_15_0, ax
	shr	eax, 16
	mov	[si].limit_19_16, al

	; CS selector
	mov	ax, word ptr fs:(SmiHeader PTR [ecx])._CS.selector
	mov	[si].selector, ax

	; CS attributes
	mov	ax, fs:(SmiHeader PTR [ecx])._CS.attr
	mov	[si].attr, al
	jmp	short Exit

Error:	call	ParamError
Exit:	ret


sys_get_descriptor endp




;***********************************************************************
; void sys_set_descriptor(USHORT, void *)
;***********************************************************************
sys_set_descriptor proc pascal \
        seg_reg: word, \
	buffer:	PTR

	movzx	eax, [seg_reg]		; Get which register
	mov	[Function], SET_DESCR	; In case of error
	call	ValidateMacro		; Get ptr to registers
	jc	Exit


	mov	si, [buffer]		; Destination ptr
	ASSUME	SI: PTR Descriptor

	cmp	al, R_CS AND 0FFh
	je	Set_CS

	xor	ah, ah			; Extract offset
	lea	ecx, [eax-8]		; Offset points to .selector


	; Set a non-SMM descriptor
	cld
	lodsd				; Copy 10 bytes of descriptor info
	mov	fs:[ecx+0], eax
	lodsd
	mov	fs:[ecx+4], eax
	lodsw
	mov	fs:[ecx+8], ax
	jmp	short Exit



	; Convert descriptor format to linear format
Set_CS:			       
	; Set limit
	mov	al, [si].limit_19_16
	mov	cl, al
	and	ax, 000Fh
	shl	eax, 16
	mov	ax, [si].limit_15_0
	mov	fs:(SmiHeader PTR [ecx])._CS.limit, eax

	; Set base
	mov	ah, [si].base_31_24
	mov	al, [si].base_23_16
	shl	eax, 16
	mov	ax, [si].base_15_0
	mov	fs:(SmiHeader PTR [ecx])._CS.base, eax

	; Set attribute
	mov	al, [si].attr
	mov	ah, cl			; Contains G bit
	mov	fs:(SmiHeader PTR [ecx])._CS.attr, ax
	jmp	short Exit


Error:	call	ParamError
Exit:	ret

sys_set_descriptor endp






	END 

