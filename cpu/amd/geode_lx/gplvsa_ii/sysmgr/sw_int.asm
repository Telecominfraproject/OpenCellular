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
;********************************************************************************
;*     Implementation of BIOS callbacks
;********************************************************************************


include SYSMGR.INC
include VSA2.INC
include SMIMAC.MAC
include GX2.INC
include ISA.INC

.model tiny,c
.586p
.ALPHA
DGROUP	GROUP _CODE, _TEXT
_CODE	SEGMENT PUBLIC use16 'CODE'
	ASSUME DS:_CODE

public  VSM_Buffer

SMINT_SEGMENT		equ	0E000h		; Segment to store SMINT instruction & ISR stack
ALLOW_SMM_SEGMENTS	equ	0		; 1 = ES segment allowed to point to SMM memory


externdef pascal Report_VSM_Error: proc
externdef pascal Schedule_VSM:     proc
externdef Sys_Exit:                proc
externdef pascal Hex_8:            proc
externdef pascal Hex_16:           proc
externdef pascal Hex_32:           proc

externdef BracketFlag:             byte
externdef SchedulerStack:          word
externdef INT_Vectors:             dword
externdef Current_VSM:             dword
externdef StartSaveArea:           dword
externdef EndSaveArea:             dword
externdef SysMgr_VSM:              dword
externdef Header_Addr:             dword
externdef SMM_Header:              SmiHeader

;************************************************************************
; Implements the SYS_STATE macro
;
; Input:
;    CX = Flag:  0 = Save    1 = Restore
;   EDI = Offset to VSM's register buffer
;    DS = System Manager's data segment
;************************************************************************
Sys_State proc

	cld
	mov	eax, [Current_VSM]		; Create flat ptr to calling VSM
	add	edi, eax

	push	eax				; Re-schedule the calling VSM
	call	Schedule_VSM
	

	lea	esi, [StartSaveArea]
	add	esi, [SysMgr_VSM]

	jcxz	CopyState			; Save or Restore ?

	xchg	esi, edi			; Restore

	; Copy the non-SMM state
CopyState:
	mov	ecx, OFFSET EndSaveArea
	sub	cx,  OFFSET StartSaveArea
	shr	cx, 1
	
	rep	movsw [edi], fs:[esi]
	jmp	Sys_Exit

Sys_State	endp




;************************************************************************
; Implements the SYS_SW_INTERRUPT macro
;
; Input:
;   EBX = 4 * INT number
;   ECX = Offset to VSM's register buffer
;************************************************************************
Sys_SW_INT proc

	cld
	mov	esi, [Current_VSM]
	push	esi				; Re-schedule the calling VSM
	call	Schedule_VSM

	cmp	bx, 4*MAX_INT			; Validate INT vector
	ja	Unsupported_INT
	mov	edx, [INT_Vectors+bx]
	or	edx, edx
	jz	Illegal_INT

	mov	[Saved_INT], ebx		; Save current vector & 
	mov	edi, edx			; patch with the original vector
	xchg	fs:[ebx], edi
	mov	[Saved_Vector], edi
	
	xor	edi, edi			; Don't allow calling VSM to execute
	push	edi
	call	Schedule_VSM

	ASSUME	di:PTR VSM_Header		; Mark the requesting VSM blocked
	mov	gs:[di].SysStuff.RunFlag, RUN_FLAG_BLOCKED


	; Handle PIC masks
	in	al, PIC1_MASK			; Save PIC masks & set user-defined masks
	mov	ah, al
	in	al, PIC2_MASK
	mov	[Saved_PIC], ax

	ASSUME	di:PTR INT_REGS
	mov	di, cx
	mov	ax, word ptr gs:[di].PIC0_Mask
	not	ax				; PIC masks are 0=enable
	cmp	ah, 0FFh			; Any PIC1 IRQs enabled ?
	je	short SetPIC
	and	al, NOT 04h			; Yes, enable IRQ2
SetPIC:
	or	al, 01h				; Always disable IRQ0
	out	PIC1_MASK, al
	mov	al, ah
	out	PIC2_MASK, al

	cmp	ax, 0FFFFh			; Are any IRQs enabled ?
	je	short RealMode
	or	gs:[di].Flags, EFLAGS_IF	; Yes, enable interrupts	
	
	;
	; Initialize real-mode state
	;
RealMode:
	mov	ax, gs:[di].Reg_ES		; Set up ES descriptor
	push	OFFSET BIOS_ES
	call	Set_Descriptor

	mov	ax, gs:[di].Reg_DS		; Set up DS descriptor
	push	OFFSET BIOS_DS
	call	Set_Descriptor

	add	esi, ecx			; Create flat ptr to calling VSM's registers
	mov	[VSM_Buffer], esi


	ASSUME	di:PTR SmiHeader

	lea	edi, [BIOS_Header]		; Set up CS descriptor & Next_EIP
	mov	word ptr [di].SS_Flags, DATA_ATTR
	mov	[di].SMI_Flags, SMI_FLAGS_CS_WRITABLE + SMI_FLAGS_CS_READABLE
	mov	word ptr [di].Next_EIP, dx	; IP
	shr	edx, 16				; Convert segment to linear address
	mov	[di]._CS.selector, dx		; CS selector
	shl	edx, 4
	mov	[di]._CS.base,  edx		; CS descriptor
	mov	[di]._CS.limit, 0FFFFh
	mov	[di]._CS.attr, CODE_ATTR	; CS attribute
	mov	eax, CR0			; Preserve the CD & NW bits
	and	eax, 60000000h
	or	eax, VSM_CR0
	mov	[di].r_CR0, eax

if ALLOW_SMM_SEGMENTS
 	mov	ecx, MSR_RCONF_SMM		; Make SMM memory writeable by interrupt code
	rdmsr
	mov	[OldRCONF], al
	and	al, NOT REGION_WP
	wrmsr
endif

	ASSUME	DI: NOTHING



	mov	ecx, 1000002Ch			; Save P2D_SC value & make UMBs R/W
	rdmsr
	mov	[ShadowMSR], dx
	or	dx, 0FFFFh
	wrmsr

	mov	ebx, SMINT_SEGMENT		; Patch SS descriptor
	mov	word ptr  [BIOS_SS+8], bx
	shl	ebx, 4
	mov	dword ptr [BIOS_SS+2], ebx
	mov	byte ptr  [BIOS_SS+5], DATA_ATTR
	add	ebx, 0FFF0h
	mov	[ShadowAddr], ebx
	mov	word ptr [BIOS_ESP], bx

	; Build a stack frame:
	; SMINT_SEGMENT:FFF0   SMINT_SEGMENT:FFF6  - return addresss
	; SMINT_SEGMENT:FFF4   Flags
	; SMINT_SEGMENT:FFF6   SMINT instruction
	mov	eax, SMINT_SEGMENT SHL 16	; Segment
	lea	ax, [bx+6]			; Offset (SP+6)
	xchg	eax, dword ptr fs:[ebx]
	mov	[ShadowMemory], eax
	mov	eax, 380F0000h			; FLAGS & SMINT
	xchg	eax, fs:[ebx+4]
	mov	[ShadowMemory+4], eax

	mov	eax, [Header_Addr]		; Copy top-level header address
	mov	[Hdr_Address], eax


	call	VSM_Registers			; Get registers for ISR


	call	Swap_States			; S wap top-level state with BIOS state

Exit:	jmp	Sys_Exit



Unsupported_INT:
Illegal_INT:


	mov	ax, ERR_BAD_INTERRUPT
	push	ax				; Push error code
	shr	ebx, 2
	push	ebx				; Info1 = Interrupt #
	push	dword ptr 0			; Info2 = 0x00000000
	call	Report_VSM_Error
	jmp	Exit

Sys_SW_INT endp










;********************************************************************************
;       Returns here from INT callback
;********************************************************************************
INT_Return proc

	mov	ebx, [Saved_INT]		; Restore the INT vector
	mov	edx, [Saved_Vector]
	mov	fs:[ebx], edx


if ALLOW_SMM_SEGMENTS
 	mov	ecx, MSR_RCONF_SMM		; Restore SMM region properties
	rdmsr
	mov	al, [OldRCONF]
	wrmsr
endif

	mov	esi, [Current_VSM]		; Mark the requesting VSM ready to run
	mov	fs:(VSM_Header PTR [esi]).SysStuff.RunFlag, RUN_FLAG_READY

	call	Swap_States			; Restore the original non-SMM state

	call	VSM_Registers			; Return register values & flags


	xor	eax, eax
	mov	[VSM_Buffer], eax
	
	sub     [SchedulerStack], 4		; Pop the scheduler sentinel

	mov	ebx, [ShadowAddr]		; Restore shadow memory
	mov	eax, [ShadowMemory]
	mov	fs:[ebx], eax
	mov	eax, [ShadowMemory+4]
	mov	fs:[ebx+4], eax

	mov	ecx, 1000002Ch			; Restore R/W attributes of UMBs
	rdmsr
	mov	dx, [ShadowMSR]
	wrmsr
	
	mov	ax, [Saved_PIC]			; Restore PIC masks
	out	PIC2_MASK, al
	mov	al, ah
	out	PIC1_MASK, al	
	ret

INT_Return endp



;************************************************************************
; Swaps the top-level state with the BIOS callback state.
; Info to be saved:
;  - SMM header & pointer
;  - Descriptors
;  - GP registers
;  - IDT ptr
;  - PCI config address
;************************************************************************
Swap_States proc

	lea	si, [StartSaveArea]		; Swap thread states
	lea	di, [Saved_State]
	mov	cx, OFFSET EndSaveArea		; Compute # dwords of state
	sub	cx, si
	shr	cx, 2
StateLoop:
	lodsd					; Exchange a dword
	xchg	[di], eax
	mov	dword ptr [si-4], eax
	add	di, 4
	loop	StateLoop

	lea	si, [SMM_Header]		; Now do the top-level SMM header
	lea	di, [BIOS_Header]
	mov	cl, sizeof(SmiHeader)/4
Hdr_Loop:
	lodsd
	xchg	[di], eax
	mov	[si-4], eax
	add	di, 4
	loop	Hdr_Loop

	ret

Swap_States endp


;************************************************************************
; Swaps the VSM's registers with the local state
;************************************************************************
VSM_Registers proc

	mov	esi, [VSM_Buffer]		; Caller's register buffer
	mov	ax, word ptr [BIOS_Header].EFLAGS
	xchg	fs:(INT_REGS PTR [esi]).Flags, ax
	mov	word ptr [BIOS_Header].EFLAGS, ax
	
	lea	bx, [Reg_Table]
	mov	cx, (End_Reg_Table-Reg_Table)/2
Reg_Loop:
	mov	di, [bx]			; Get offset of register
	mov	eax, [di]			; Swap with register structure
	xchg	eax, fs:[esi]
	mov	[di], eax
	add	bx, 2				; Increment ptrs
	add	esi, 4
	loop	Reg_Loop

	ret

; This table translates between a INT_REGS structure and the PUSHA/POPA ordering
Reg_Table:
	dw	OFFSET BIOS_EAX
	dw	OFFSET BIOS_EBX
	dw	OFFSET BIOS_ECX
	dw	OFFSET BIOS_EDX
	dw	OFFSET BIOS_EBP
	dw	OFFSET BIOS_ESI
	dw	OFFSET BIOS_EDI
End_Reg_Table:

VSM_Registers endp


;************************************************************************
; Sets a descriptor to a real-mode descriptor
; Input:
;   AX = selector
;        If selector = 0x0000, a 4 GB descriptor (big-real) is used
;        If selector = 0xFFFF, the calling VSM's base/limit is used
;   DI = ptr to descriptor
;************************************************************************
Set_Descriptor proc pascal uses di \
	DescrPtr: PTR Descriptor

	ASSUME	di: PTR Descriptor
	mov	di, [DescrPtr]

	mov	ebx, 0FFFFh			; 64K limit

	or	ax, ax				; Selector = 0000h?
	jnz	short SetSelector
	mov	ebx, 008FFFFFh			; Yes, then use 4 GB limit (big-real mode)
SetSelector:

	movzx	eax, ax	
	mov	[di].selector, ax
	shl	eax, 4


if ALLOW_SMM_SEGMENTS
	; If ES == 0xFFFF, use VSM's base
	cmp	ax, bx
	jne	short Set_Limit
	mov	eax, [Current_VSM]		; No, use VSM's base & limit
	mov	bx, ax
	shr	bx, 4
	mov	[di].selector, bx
	mov	ebx, (VSM_Header PTR fs:[eax]).DS_Limit
endif

Set_Limit:
	; EAX = base
	; EBX = limit
	mov	[di].limit_15_0, bx		; Store 24-bit limit
	shr	ebx, 16
	mov	[di].limit_19_16, bl

	mov	[di].base_15_0, ax		; Store	32-bit base
	shr	eax, 16
	mov	[di].base_23_16, al
	mov	[di].base_31_24, ah
	ret

	ASSUME	di: NOTHING

Set_Descriptor endp









   		align	4
Saved_State:
;******************************************************************************
; The following must match the structure at StartSaveArea in SYSMGR.ASM
BIOS_PCI	dd	80000000h
BIOS_EDI	dd	0
BIOS_ESI	dd	0
BIOS_EBP	dd	0
		dd	0			; ESP (not used)
BIOS_EBX	dd	0
BIOS_EDX	dd	0
BIOS_ECX	dd	0
BIOS_EAX	dd	0

BIOS_ESP	dd	0

BIOS_SS		Descriptor {0FFFFh, 0, 0, DATA_ATTR, 0, 0, 0}
BIOS_DS		Descriptor {0FFFFh, 0, 0, DATA_ATTR, 0, 0, 0}
BIOS_ES		Descriptor {0FFFFh, 0, 0, DATA_ATTR, 0, 0, 0}
BIOS_FS		Descriptor {0FFFFh, 0, 0, DATA_ATTR, 0, 0, 0}
BIOS_GS		Descriptor {0FFFFh, 0, 0, DATA_ATTR, 0, 0, 0}

Hdr_Address	dd	0

IDT_Selector	dd	00920000h
IDT_Base	dd	00000000h
IDT_Limit	dd	0000FFFFh

	  
		dw	0			; Pad
		
;******************************************************************************
SMM_CONTROL	equ EXTL_SMI_EN + INTL_SMI_EN + SMM_INST_EN + NEST_SMI_EN

BIOS_Header	SmiHeader {,SMM_CONTROL,,,,,,,,,VSM_CR0, VSM_EFLAGS, VSM_DR7}

ShadowMemory	dd	0, 0
ShadowAddr	dd	0
ShadowMSR	dw	0

VSM_Buffer	dd	0


Saved_PIC	dw	0
Saved_INT	dd	0
Saved_Vector	dd	0
OldRCONF	db	0

_CODE	ENDS


	END
