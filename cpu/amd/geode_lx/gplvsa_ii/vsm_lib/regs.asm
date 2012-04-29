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
;*     This file library functions for accessing registers & headers

include SYSMGR.INC
include VSA2.INC
include SMIMAC.MAC



.model tiny,c
.586p
.CODE



public Function

externdef Async_VSM: dword
externdef sys_system_call:proc

Function	db	0, 0, 0, 0

;***********************************************************************
; Reports a parameter error
; 
; EAX = parameter
;***********************************************************************
ParamError proc
	; SYS_REPORT_ERROR(ERR_BAD_PARAMETER, macro, param);
	push	word ptr ERR_BAD_PARAMETER
	mov	bh, [Function]
	mov	bl, 1			; Parameter #1
	push	ebx			; Info1
	push	eax			; Info2
	call	sys_report_error
	ret

ParamError endp


;***********************************************************************
; Validates if a macro is valid for the current message.
; Output:  if "invalid macro usage"
;            sys_report_error(ERR_ILLEGAL_MACRO);
;            set CF
;          else
;            clear CF
;            EBX = ptr to relevant VSM's stack
;            ECX = ptr to relevant VSM's state
; NOTE: Must preserve AX
;***********************************************************************
ValidateMacro proc
	mov	ebx, [Async_VSM]
	test	ebx, ebx
	jz	short NotSynchronous

	; Mark blocked VSM as 'Ready'
	mov	fs:(VSM_Header PTR [ebx]).SysStuff.RunFlag, RUN_FLAG_READY
	jmp	short Macro_OK
	


NotSynchronous:
	; Always allow GET_REGISTER, GET_DESCRIPTOR & GET_HEADER
	test	byte ptr [Function], 1
	jnz	Async_Error

	; Get ptr to System Manager's VSM header
	mov	ebx, (VSM_Header PTR [bx]).SysStuff.SysMgr_Ptr
	sub	ebx, SPECIAL_LOC

Macro_OK:
	mov	ecx, ebx
	add	ebx, fs:(VSM_Header PTR [ecx]).SysStuff.SavedESP
	add	ebx, EXTRA_SAVE

	lea	ecx, (VSM_Header PTR [ecx]).SysStuff.State
	clc
	ret

Async_Error:

	; SYS_REPORT_ERROR(ERR_ILLEGAL_MACRO, macro, argument);
	push	word ptr ERR_ILLEGAL_MACRO
	push	dword ptr [Function]
	movzx	eax, ax
	push	eax
	call	sys_report_error

	mov	ax, 0FFFFh		; Return FFFFs
	mov	dx, ax
	stc
	ret

ValidateMacro endp





;***********************************************************************
; Helper routine for SYS_SET_REGISTER and SYS_SET_HEADER_DATA macros.
; Input:
;    AL = offset
;   EBX = base
;   ECX = data
;    DL = max field offset
;************************************************************ ***********
SetHeader:
	mov	dx, R_DR7		; Max valid field
SetField proc

	cmp	al, dl			; Valid field name ?
	ja	short Error		; No

	mov	dx, ax			; Save field size
	xor	ah, ah			; Extract offset
	movzx	eax, ax
	mov	fs:[ebx+eax], cl	; Store data as BYTE
	test	dx, (DWORD_SIZE OR WORD_SIZE) 
	jz	Exit
	test	dx, DWORD_SIZE		; WORD or DWORD ?
	jz	Set_Word
Set_Dword:
	db	66h			; DWORD 
Set_Word:
	mov	fs:[ebx+eax], cx	; WORD
	clc
Exit:	ret

Error:	call	ParamError
	ret

SetField endp



;***********************************************************************
; Helper routine for SYS_GET_REGISTER and SYS_GET_HEADER_DATA macros.
; Input:
;   EBX = base of structure
;    AX = offset
;    DX = maximum valid offset
; Output:
;   DX:AX = data
;***********************************************************************
GetField proc

	cmp	al, dl			; Valid field name ?
	ja	short Error		; No, return FFFFs

	mov	cx, ax			; Save field size
	xor	ah, ah			; Extract offset
	movzx	eax, ax
	mov	eax, fs:[ebx+eax] 	; Get requested value
	test	cx, DWORD_SIZE		; DWORD field ?
	jz	NotDword
	mov	edx, eax		; Put 16 MSBs into DX
	shr	edx, 16
	ret

NotDword:
	xor	dx, dx			; Zero 16 MSBs
	test	cx, WORD_SIZE		; WORD field ?
	jnz	Exit
	xor	ah, ah			; BYTE field
Exit:	ret

Error:	call	ParamError
	mov	ax, 0FFFFh		; Return FFFFs
	mov	dx, ax
	ret

GetField endp





;***********************************************************************
; ULONG sys_get_register(USHORT register)
;***********************************************************************
sys_get_register proc pascal \
	Register:WORD

	mov	ax, [Register]		; Get which register
	mov	[Function], GET_REG	; In case of error
	call	ValidateMacro		; Get ptr to registers
	jc	Exit
	
	test	ax, FROM_HEADER		; Is register in SMM header ?
	jnz	Get_Header
	
	mov	dx, R_GS
	call	GetField		; Get register from saved state buffer
	
Exit:	ret

sys_get_register endp





;***********************************************************************
; void sys_set_register(USHORT register, ULONG Data)
;***********************************************************************
sys_set_register proc pascal \
	Register: WORD, \
	Data:     DWORD

	mov	ax, [Register]		; Get register
	mov	[Function], SET_REG	; In case of error
	call	ValidateMacro		; Get ptr to registers
	jc	Exit

	test	ax, FROM_HEADER		; Is register in SMM header ?
	jnz	Set_Header
		
	mov	ecx, [Data]

	mov	dx, R_ESP		; Max valid register field
	call	SetField

Exit:	ret

sys_set_register endp




;***********************************************************************
; Gets a field from the top-level SMM header
; ULONG sys_get_header_data(USHORT SMM_Field)
;***********************************************************************
sys_get_header_data proc pascal \
	SMM_Field: WORD

	mov	ax, [SMM_Field]		; Get SMM header field
	mov	[Function], GET_HDR	; In case of error
	call	ValidateMacro
	jc	short Exit

	mov	ax, [SMM_Field]		; Get SMM header field
	test	ax, FROM_HEADER		; Validate field
	jnz	short Get_Header
	call	ParamError
	jmp	short Exit

Get_Header::
	mov	ebx, ecx
	mov	dx, R_DR7		; Max valid field
	call	GetField
Exit:	ret

sys_get_header_data endp



;***********************************************************************
; void sys_set_header_data(USHORT SMM_Field, ULONG Data)
;***********************************************************************
sys_set_header_data proc pascal \
	SMM_Field: WORD, \
	Data:      DWORD

	mov	ax, [SMM_Field]		; Get SMM header field
	mov	[Function], SET_HDR	; In case of error
	call	ValidateMacro
	jc	short Exit

	test	ax, FROM_HEADER		; Validate field
	jnz	short Set_Header
	call	ParamError
	jmp	short Exit
	
Set_Header::	
	mov	ebx, ecx
	mov	ecx, [Data]
	call	SetHeader
Exit:	ret

sys_set_header_data endp



;***********************************************************************
; Reports an error
;***********************************************************************
sys_report_error proc pascal uses di \
	Error_Code: WORD,  \
	Info1:      DWORD, \
	Info2:      DWORD

	mov	di,  [Error_Code]
	mov	ebx, [Info1]
	mov	ecx, [Info2]

	mov	ax, SYS_CODE_ERROR
	call	sys_system_call
	ret

sys_report_error endp


;***********************************************************************
; void sys_return_result(ULONG Result);
;
; Implements the SYS_RETURN_RESULT macro
; Returns a byte/word/dword result to the appropriate environment
;
;***********************************************************************
sys_return_result proc pascal \
	Result:  DWORD


	mov	ebx, [Result]

	; Return to another VSM's environment ?	
	mov	ecx, [Async_VSM]
	mov	eax, (VSM_Header PTR ds:[0]).SysStuff.SysMgr_Ptr
	xor	ax, ax
	cmp	ecx, eax
	je	RetToTopLevel

	; Yes, get the data size
	mov	dl, byte ptr fs:(VSM_Header PTR [ecx]).SysStuff.State.data_size
	mov	ax, R_AL
	cmp	dl, BYTE_IO
	je	SetReg
	mov	ax, R_AX
	cmp	dl, WORD_IO
	je	SetReg
	mov	ax, R_EAX
SetReg:
	push	ax			; Register AL/AX/EAX
	push	ebx			; Data
	call	sys_set_register
	jmp	Exit

RetToTopLevel:
	mov	ax, SYS_CODE_RESULT	; Let SysMgr return the result
	call	sys_system_call
Exit:	ret
	
sys_return_result endp


	END 

