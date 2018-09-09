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
;******************************************************************************
;*     Implements the virtual register VR_MISCELLANEOUS.
;******************************************************************************


.model tiny,c
.586
.CODE

include VSA2.INC
include SYSMGR.INC
include VR.INC
include CHIPSET.INC
include GX2.INC

externdef	MsgPacket:	          byte
externdef	HistoryStart:             word
externdef	HistoryEnd:               word
externdef	HistoryWrap:              word
externdef	SysMgr_VSM:	          dword
externdef	VSM_Ptrs:	          dword
externdef	Saved_EAX:	          dword
externdef	Saved_EBX:	          dword
externdef	Saved_ECX:	          dword
externdef	Saved_EDX:	          dword
externdef	Saved_ESI:	          dword
externdef	Saved_EDI:	          dword
externdef	Saved_EBP:	          dword
externdef	Saved_DS:	          dword
externdef	Saved_ES:	          dword
externdef	Saved_FS:	          dword
externdef	Saved_GS:	          dword
externdef	Saved_SS:	          dword
externdef	IDT_Selector:		  dword
externdef	IDT_Base:		  dword
externdef	IDT_Limit:		  dword
externdef	VSM_ListHead:             dword

externdef	pascal Get_MSR_Linkage:   proc
externdef	pascal Broadcast_Message: proc 
externdef	pascal Register_Event:    proc
externdef	Get_Errors:               proc
externdef	VirtualRegisterEvent:     proc
externdef	HardwareInfo:	          Hardware
externdef	History:	          EVENT_HISTORY
externdef	Events:		          EVENT_ENTRY
externdef	SMM_Header:	          SmiHeader

; Required for OHCI h/w bugs
externdef	OHCI1_Hdr:                dword
externdef	OHCI2_Hdr:                dword
externdef	OHCI1_Smi:                dword
externdef	OHCI2_Smi:                dword

;******************************************************************************
	align	2

public VSM_Filter
VSM_Filter	db	VSM_ANY
		db	0		; Pad for alignment
Next_VSM   	dd	0



ReadMiscTable:
	dw	OFFSET Get_VSA_Version   ; 0
	dw	OFFSET Read_High_Memory  ; 1
	dw	OFFSET Get_VSM_Info      ; 2
	dw	OFFSET Signature         ; 3
	dw	OFFSET Get_HW_Info       ; 4
	dw	OFFSET Get_VSM_Version   ; 5
	dw	OFFSET JustReturn        ; 6
	dw	OFFSET MSR_Read          ; 7
	dw	OFFSET JustReturn	 ; 8
MAX_READ  equ ($-ReadMiscTable)/2

WriteMiscTable:
	dw	OFFSET JustReturn        ; 0
	dw	OFFSET Write_High_Memory ; 1	
	dw	OFFSET JustReturn        ; 2
	dw	OFFSET JustReturn        ; 3
	dw	OFFSET Select_HW_Info    ; 4
	dw	OFFSET JustReturn        ; 5
	dw	OFFSET Warm_Boot         ; 6
	dw	OFFSET MSR_Write         ; 7
	dw	OFFSET Do_WBINVD	 ; 8
MAX_WRITE  equ ($-WriteMiscTable)/2


VSM_Info_Table:
	dw	OFFSET Get_VSM_Basics
	dw	OFFSET Get_VSM_Events
	dw	OFFSET Get_VSM_Statistics
	dw	OFFSET Get_VSM_History
	dw	OFFSET Get_VSM_Hardware
	dw	OFFSET Get_Errors
	dw	OFFSET Set_VSM_Type
	dw	OFFSET Get_Linkage
MAX_INFO equ  ($-VSM_Info_Table)/2



;***************************************************************************
; Handler for reads from a VRC_MISCELLANEOUS register
; Returns ULONG with value read
;***************************************************************************
VR_Miscellaneous_Read proc pascal uses si di \
	Index: BYTE

	mov	al, [Index]
	cmp	al, MAX_READ
	mov	di, 0			; Read
	jae	short VRC_Common

	lea	bx, [ReadMiscTable]
	call	Dispatch
	ret

VR_Miscellaneous_Read endp


;***************************************************************************
; Handler for writes to a VRC_MISCELLANEOUS register
;***************************************************************************
VR_Miscellaneous_Write proc pascal uses si di \
	Index: BYTE

	mov	al, [Index]
	cmp	al, MAX_WRITE
	jb	short UseTable
	mov	di, 1			; Write
VRC_Common::	
	mov	bh, VRC_MISCELLANEOUS
	mov	bl, al
	mov	edx, [SysMgr_VSM]	; From_VSM
	mov	cx, word ptr [Saved_EAX]
	call	VirtualRegisterEvent
	jmp	short Exit
		
UseTable:
	lea	bx, [WriteMiscTable]
	call	Dispatch
Exit:	ret

VR_Miscellaneous_Write endp

;***************************************************************************
; Dispatches to routine defined by table at BX[AL]
;***************************************************************************
Dispatch proc

	xor	ah, ah
	add	ax, ax
	add	bx, ax
	call	word ptr [bx]
JustReturn::
	ret
Dispatch endp




;***************************************************************************
; Dispatches to sub-function of GET_VSM_INFO (determined by caller's BL)
;***************************************************************************
Get_VSM_Info proc

	mov	al, byte ptr [Saved_EBX]
	cmp	al, MAX_INFO
	jae	short Exit
	lea	bx, [VSM_Info_Table]
	call	Dispatch
Exit:	ret

Get_VSM_Info endp


;***************************************************************************
; SI = PCI address
;***************************************************************************
Get_Linkage proc


	push	word ptr [Saved_ESI]
	call	Get_MSR_Linkage

	mov	word ptr [Saved_ECX], ax	; Return MSR address in caller's ECX
	mov	word ptr [Saved_ECX+2], dx
	
	ret	

Get_Linkage endp


;***************************************************************************
; Returns in EAX the location by EBX.
; CF is toggled
;***************************************************************************
Read_High_Memory proc

	mov	ebx, [Saved_EBX]
	mov	eax, fs:[ebx]
	mov	[Saved_EAX], eax
	mov	edx, eax
	shr	edx, 16

	; Toggle the caller's carry flag
	xor	byte ptr ([SMM_Header]).EFLAGS, EFLAGS_CF
	ret

Read_High_Memory endp

;***************************************************************************
; Modifies a memory location according to:
;   *EBX = *EBX & ESI | EDI;
;   CF is toggled
;***************************************************************************
Write_High_Memory proc

	mov	ebx, [Saved_EBX]	; Address
	mov	edx, [Saved_ESI]	; AND mask
	mov	eax, [Saved_EDI]	; OR mask
	or	edx, edx
	jz	short WriteBack

	mov	ecx, fs:[ebx]
	and	ecx, edx
	or	eax, ecx
WriteBack:
	mov	fs:[ebx], eax
	; Toggle the caller's carry flag
	xor	byte ptr ([SMM_Header]).EFLAGS, EFLAGS_CF
	ret

Write_High_Memory endp




;***************************************************************************
; Modifies an MSR
; On entry:
;       ECX = MSR address
;   ESI:EDI = AND mask
;   EBX:EAX = OR mask
; On exit:
;   CF is toggled
;***************************************************************************
MSR_Write proc

	mov	ecx, [Saved_ECX]	; Get MSR address
	call	Read_MSR

	and	edx, [Saved_ESI]	; Apply AND mask
	and	eax, [Saved_EDI]
	or	edx, [Saved_EBX]	; Apply OR mask
	or	eax, [Saved_EAX]

	test	ecx, 0E0000000h
	jnz	Not_CPU_MSR
	stc				; Indicate this is a WRMSR
	call	MSR_Handler		; Update the MSR
	jmp	short Exit

Not_CPU_MSR:
	wrmsr
	; Toggle the caller's carry flag
Exit:	xor	byte ptr ([SMM_Header]).EFLAGS, EFLAGS_CF
	ret

MSR_Write endp






;***************************************************************************
; Reads an MSR
; On entry:
;     ECX = MSR address
; On exit:
;     EDX:EAX = MSR data
;     CF is toggled
;***************************************************************************
MSR_Read  proc

	mov	ecx, [Saved_ECX]	; Get MSR address

	call	Read_MSR		; Handle special MSRs

	mov	[Saved_EAX], eax	; Return results
	mov	[Saved_EDX], edx

	; Toggle the caller's carry flag
	xor	byte ptr ([SMM_Header]).EFLAGS, EFLAGS_CF
	ret

MSR_Read endp


;***************************************************************************
; Input:
;      ECX = MSR address
; Output:
;      EBX = OHCI BAR for multiplexer workaround (0 if not applicable)
;  EDX:EAX = MSR contents
;***************************************************************************
Read_MSR proc

	test	ecx, 0E0000000h		; Is the MSR in the CPU ?
	jnz	Not_CPU_MSR
	xor	edx, edx		; Yes, then possibly requires special handling
	clc				; Indicate this is a RDMSR
	call	MSR_Handler		; Read the MSR
	ret

Not_CPU_MSR:
	rdmsr
	ret

Read_MSR endp







;***************************************************************************
; Reads/writes MSRs that require special handling
;  a) those modified by SysMgr
;  b) those located in the SMM header
;  c) those that are SMM-related are not allowed to be written
; On entry:
;  CF = 0 for MSR read
;       1 for MSR write	
;***************************************************************************

MSR_Handler proc
	ASSUME	BX: PTR SpecialMSR
	lea	bx, [Special_MSRs]
	mov	di, [SpecialMSR.RdHandler]
	jnc	MSR_Loop
	mov	di, [SpecialMSR.WrHandler]
MSR_Loop:
	cmp	cx, [bx].MSR_Addr	; Does MSR match ?
	je	Match
	add	bx, sizeof(SpecialMSR)	; Advance table ptr
	cmp	[bx].MSR_Addr, 0000h	; End of table ?
	jne	MSR_Loop
NotSpecial:
	cmp	di, [SpecialMSR.WrHandler] ; Read or Write ?
	je	short WriteMSR
	rdmsr
	jmp	short Exit

WriteMSR:
	wrmsr
	jmp	short Exit
	

Match:	mov	si, [bx].DataOffset
	call	word ptr [bx+di]	; Jump to MSR handler
Exit:	ret
	
	ASSUME	BX: NOTHING

SpecialMSR struc
  MSR_Addr	dw	?
  RdHandler	dw	?
  WrHandler	dw	?
  DataOffset	dw	?
SpecialMSR ends 
	

Special_MSRs:
	; 		MSR_Addr	Rd_Handler    Wr_Handler	DataOffset
	SpecialMSR	{MSR_EFLAGS,	Get_Header,   Set_Header,	(SMM_Header[0]).EFLAGS}
	SpecialMSR	{MSR_CR0,	Get_Header,   Set_CR0,		(SMM_Header[0]).r_CR0}
	SpecialMSR	{MSR_DR7,	Get_DR7,      Set_DR7,		(SMM_Header[0]).r_DR7}
	SpecialMSR	{1321h,		Get_CS_Sel,   Set_CS_Sel,	(SMM_Header[0])}
	SpecialMSR	{1331h,		Get_CS_Base,  Set_CS_Base,	(SMM_Header[0])}

	SpecialMSR	{1320h,		Get_Selector, Set_Selector,	Saved_ES}
	SpecialMSR	{1322h,		Get_Selector, Set_Selector,	Saved_SS}
	SpecialMSR	{1323h,		Get_Selector, Set_Selector,	Saved_DS}
	SpecialMSR	{1324h,		Get_Selector, Set_Selector,	Saved_FS}
	SpecialMSR	{1325h,		Get_Selector, Set_Selector,	Saved_GS}

	SpecialMSR	{1330h,		Get_Base,     Set_Base,		Saved_ES}
	SpecialMSR	{1332h,		Get_Base,     Set_Base,		Saved_SS}
	SpecialMSR	{1333h,		Get_Base,     Set_Base,		Saved_DS}
	SpecialMSR	{1334h,		Get_Base,     Set_Base,		Saved_FS}
	SpecialMSR	{1335h,		Get_Base,     Set_Base,		Saved_GS}

	SpecialMSR	{1329h,		Get_IDT_Sel,  Set_IDT_Sel,	IDT_Selector}
	SpecialMSR	{1339h,		Get_IDT_Base, Set_IDT_Base,	IDT_Base}
	
 	; SMM related MSRs: don't allow these to be written
	SpecialMSR	{MSR_SMM_CTRL,	NotSpecial,   Exit}
	SpecialMSR	{MSR_SMM_HDR,	NotSpecial,   Exit}
	SpecialMSR	{MSR_SMM_LOC,	NotSpecial,   Exit}
	SpecialMSR	{MSR_RCONF_SMM,	NotSpecial,   Exit}
		
	dw	0  ; End of table

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	; Get MSR value from SMM Header
Get_Header proc
	mov	eax, [si]
	ret
Get_Header endp


	; Store MSR value into SMM Header
Set_Header proc
	mov	[si], eax
	ret
Set_Header endp

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	; Segment selector & attributes	
	ASSUME	SI: PTR Descriptor
Get_Selector proc
	mov	ax, word ptr [si].attr
	and	ah, 0F0h			; Mask Limit 19:16
	shl	eax, 16
	mov	ax, [si].selector
	ret
Get_Selector endp
		
Set_Selector proc
	mov	[si].selector, ax
	shr	eax, 16
	and	ah, 0F0h			; Mask Limit 19:16
	and	word ptr [si].attr, 0F00h	; Preserve Limit 19:16
	or	word ptr [si].attr, ax
	ret
Set_Selector endp		



;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	; Segment base & limit
Get_Base proc
	mov	ah, [si].base_31_24		; Get base
	mov	al, [si].base_23_16
	shl	eax, 16
	mov	ax, [si].base_15_0
	
	movzx	dx, [si].limit_19_16		; Get limit
	mov	bh, dl
	and	dl, 0Fh				; Preserve only limit
	shl	edx, 16
	mov	dx, [si].limit_15_0	
	test	bh, G_BIT			; Handle page granular bit
	jz	short Exit
	shl	edx, 12
	or	dx, 0FFFh	
Exit:	ret
Get_Base endp


Set_Base proc
	mov	[si].base_15_0, ax		; Set base in [Saved_?S]
	ror	eax, 16
	mov	[si].base_31_24, ah
	mov	[si].base_23_16, al
	rol	eax, 16				; Restore base
	and	[si].limit_19_16, NOT G_BIT	; Clear Granularity bit
	
	test	edx, 0FFF00000h			; 32-bit limit ?
	jz	short Limit16

	or	[si].limit_19_16, G_BIT		; Set Granularity bit
	shr	edx, 12
Limit16:
	mov	[si].limit_15_0, dx
	shr	edx, 16
	and	[si].limit_19_16, 0F0h
	and	dl, 0Fh
	or	[si].limit_19_16, dl
	ret
Set_Base endp

	ASSUME	SI: PTR SmiHeader
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	; CS selector & attributes
Get_CS_Sel proc
	mov	eax, dword ptr [si]._CS.selector
	ret
Get_CS_Sel endp

Set_CS_Sel proc
	mov	dword ptr [si]._CS.selector, eax
	ret
Set_CS_Sel endp

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	; CS base & limit
Get_CS_Base proc
	mov	eax, [si]._CS.base
	mov	edx, [si]._CS.limit
	ret
Get_CS_Base endp

Set_CS_Base proc
	mov	[si]._CS.base, eax
	mov	[si]._CS.limit, edx
	ret
Set_CS_Base endp



	ASSUME	SI: NOTHING

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; CR0
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Set_CR0 proc

CD	equ	40000000h
NW	equ	20000000h
	mov	ebx, (CD+NW)
	mov	edi, ebx
	not	edi
	and	ebx, eax		; Extract CD and NW
	cmp	ebx, NW			; Don't allow CD=0 & NW=1
	je	Exit
	mov	[si], eax		; Write new CR0 to the SMM header
		
	; 1) Update VSA's CR0
	; 2) Update SMM RCONF cache bits
	; 3) Update all VSM's CR0[CD] and CR0[NW]
	; 4) Flush the cache
	wbinvd
	mov	esi, CR0
	and	esi, edi 
	or	esi, ebx
	mov	CR0, esi

	push	bx
	mov	ecx, MSR_RCONF_DEFAULT	; Re-synch MSR_RCONF_SMM with MSR_RCONF_DEFAULT
	rdmsr
	mov	bl, al
	mov	ecx, MSR_RCONF_SMM	; Update MSR_RCONF_SMM	
	rdmsr
	mov	al, REGION_WP		; Write-protect SMM region in non-SMM
	mov	dl, bl
	wrmsr
	wbinvd
	pop	bx

	; Update all VSM's CR0
	mov	ecx, [VSM_ListHead]
VSM_Loop:
	jecxz	Exit
	and	fs:(VSM_Header PTR [ecx]).SysStuff.State.r_CR0, edi
	or	fs:(VSM_Header PTR [ecx]).SysStuff.State.r_CR0, ebx
	mov     ecx, fs:(VSM_Header PTR [ecx]).SysStuff.Flink
	jmp	VSM_Loop

Exit:	ret
Set_CR0	endp

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	; DR7
Get_DR7 proc
	mov	edx, [si]		; Get DR7 in SMM header
	mov	eax, DR6
Exit:	ret
Get_DR7 endp

Set_DR7 proc
	xchg	[si], edx		; Set DR7 field in SMM header
	mov	eax, DR6		; EAX = current DR7
	wrmsr				; Update DR6
	ret
Set_DR7 endp
	
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	; IDT selector & attributes
Get_IDT_Sel proc
	mov	eax, [si]
	ret
Get_IDT_Sel endp

Set_IDT_Sel proc
	mov	[si], eax
	ret
Set_IDT_Sel endp

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	; IDT base & limit
Set_IDT_Base proc
	mov	[si], eax
	mov	[IDT_Limit], edx
	ret
Set_IDT_Base endp

Get_IDT_Base proc
	mov	eax, [si]
	mov	edx, [IDT_Limit]
	ret
Get_IDT_Base endp

MSR_Handler endp





;***************************************************************************
; Sets the VSM type for filtering events kept in the History buffer.
;***************************************************************************
Set_VSM_Type proc

	mov	al, byte ptr [Saved_EBX+1]
	mov	[VSM_Filter], al
	ret

Set_VSM_Type endp

;***************************************************************************
;   SYS_BROADCAST_MSG(MSG_WARM_BOOT, &Param, VSM_ANY);
;***************************************************************************
Warm_Boot proc

	xor	eax, eax
	lea	bx, [MsgPacket]
	mov	[bx+0], eax
	mov	[bx+4], eax
	mov	[bx+8], eax
	
	push	word ptr MSG_WARM_BOOT	; Message code
	push	word ptr VSM_ANY	; To VSMs
	push	eax			; From_VSM
	call    Broadcast_Message
	ret

Warm_Boot endp




;***************************************************************************
; Returns VSA II signature in caller's EAX
;***************************************************************************
Signature proc

	mov	ax, VSA2_SIGNATURE AND 0FFFFh
	mov	dx, VSA2_SIGNATURE SHR 16
	ret

Signature endp


;***************************************************************************
; Returns version of VSA II in caller's AX
;***************************************************************************
Get_VSA_Version proc

	mov	ax, VSA_VERSION
	ret

Get_VSA_Version endp




;***************************************************************************
; Returns the version # of the specified VSM type
;***************************************************************************
Get_VSM_Version proc

	; Get caller's BL
	mov	dl, byte ptr [Saved_EBX]
Find_VSM::
	mov	ecx, [SysMgr_VSM]
	mov	ax, 0FFFFh		; Not found
VSM_Loop:
	jecxz	Exit
	cmp	dl, fs:(VSM_Header PTR [ecx]).VSM_Type
	je	Found
	mov	ecx, fs:(VSM_Header PTR [ecx]).SysStuff.Flink
	jmp	VSM_Loop

Found:	mov	ax, fs:(VSM_Header PTR [ecx]).VSM_Version
Exit:	mov	word ptr [Saved_EAX], ax
	ret


Get_VSM_Version endp







;***************************************************************************
; Helper routine
; On Entry:
;   Caller's BH - VSM type to process
; On Exit:
;   ECX - ptr to the next VSM to be processed
;***************************************************************************
Common_Entry proc

	mov	ecx, [Next_VSM]
	mov	dl, byte ptr [Saved_EBX+1]	; Caller's BH
	cmp	dl, 0FFh			; Next VSM ?
	je	Exit

	call	Find_VSM
	cmp	ax, 0FFFFh
	stc
	jz	Return

Exit:	clc
Return:ret

Common_Entry	endp

;***************************************************************************
; Updates Next_VSM with the Flink field of the current VSM
;***************************************************************************
Common_Exit proc

	; Get ptr to next VSM	
	mov	ecx, fs:(VSM_Header PTR [ecx]).SysStuff.Flink
	mov	[Next_VSM], ecx
	ret

Common_Exit endp





;***************************************************************************
; Return's to caller's registers:
;   EAX - VSM_Version::VSM_Type  (0xFFFF if no more)
;   EBX - Base of VSM
;   ECX - IP::SP
;    DX - VSM_Length
;***************************************************************************
Get_VSM_Basics proc

	call	Common_Entry
	jc	short Exit

	; Put info into caller's registers
	mov	[Saved_EBX], ecx

	mov	ax, fs:(VSM_Header PTR [ecx]).VSM_Version
	shl	eax, 16
	movzx	ax, fs:(VSM_Header PTR [ecx]).VSM_Type
	mov	[Saved_EAX], eax

	mov	ax, word ptr fs:(VSM_Header PTR [ecx]).SysStuff.State.Next_EIP
	shl	eax, 16
	mov	ax, word ptr fs:(VSM_Header PTR [ecx]).SysStuff.SavedESP
	mov	[Saved_ECX], eax

	mov	ax, word ptr fs:(VSM_Header PTR [ecx]).DS_Limit
	mov	word ptr [Saved_EDX], ax

	call	Common_Exit

Exit:	ret

Get_VSM_Basics endp


;***************************************************************************
; Entry:
;        CL - History entry # (0-based)
;        CH - 1 to clear history buffer
; Exit:
;        AL - VSM type
;        AH - Event
;       EBX - Count
;       ECX - Param1
;       EDX - Param2
;   EDI:ESI - TimeStamp
;***************************************************************************
Get_VSM_History proc

if HISTORY

	; Get starting index
	mov	ax, [HistoryStart]
	mov	cx, [HistoryEnd]
	cmp	ax, cx
	je	NoHistory

	movzx	dx, byte ptr [Saved_ECX]
 	add	ax, dx
	mov	dl, HISTORY		; Index = (HistoryStart+CX) % HISTORY
	div	dl
	movzx	ax, ah

	cmp	ax, cx			; if (Index < HistoryEnd)
	jb	short GetItem		;    goto GetItem
	mov	dx, ax
	cmp	dx, cx
	jne	short GetItem

	cmp	byte ptr [Saved_ECX+1], 1
	jne	NoHistory
	xor	ax, ax
	mov	[HistoryStart], ax
	mov	[HistoryEnd], ax
	mov	[HistoryWrap], ax
	jmp	NoHistory

GetItem:


	; Get ptr to History[] entry
	lea	bx, History
	ASSUME	BX: PTR EVENT_HISTORY
	mov	dl, sizeof(EVENT_HISTORY)
	mul	dl
	add	bx, ax

	; Get info about the event
	mov	ah, byte ptr [bx].Event
	mov	ecx, [bx].Vsm
	mov	al, fs:(VSM_Header PTR [ecx]).VSM_Type
	mov	word ptr [Saved_EAX], ax

	mov	eax, [bx].Count
	mov	[Saved_EBX], eax

	mov	eax, [bx].Param1
	mov	[Saved_ECX], eax

	mov	eax, [bx].Param2
	mov	[Saved_EDX], eax

	mov	eax, [bx].TimeStamp
	mov	[Saved_ESI], eax
	mov	eax, [bx].TimeStamp+4
	mov	[Saved_EDI], eax
	ret

endif


NoHistory:
	xor	eax, eax
	mov	[Saved_EAX], eax
	ret


Get_VSM_History endp


;***************************************************************************
; Returns:
;  AX = Chipset ID
;  BX = CPU ID
;  CX = CPU MHz
; EDX = PCI address of Southbridge 
;***************************************************************************
Get_VSM_Hardware proc

	; Get ptr to Hardware structure
	lea	bx, [HardwareInfo]
	ASSUME	BX: PTR Hardware

	mov	ax, [bx].Chipset_ID
	mov	word ptr [Saved_EAX], ax

	mov	ax, [bx].CPU_ID
	mov	word ptr [Saved_EBX], ax

	mov	ax, [bx].CPU_MHz
	mov	word ptr [Saved_ECX], ax

	mov	eax, [bx].Chipset_Base	
	mov	[Saved_EDX], eax

	ret

	ASSUME	BX: NOTHING

Get_VSM_Hardware endp


;***************************************************************************
; Returns:
;  AX = Chipset ID
;***************************************************************************
Get_HW_Info proc

	mov	ax, 0FFFFh		; Value for illegal index
	movzx	bx, [HW_Index]
	cmp	bl, MAX_HW_INFO
	ja	short Exit

	add	bx, bx
	mov	bx, [HW_Offsets+bx]
	mov	ax, [bx]
Exit:
	mov	word ptr [Saved_EAX], ax
	ret

Get_HW_Info endp

HW_Index	db	0

HW_Offsets:
	dw	OFFSET HardwareInfo.Chipset_ID
	dw	OFFSET HardwareInfo.Chipset_Rev
	dw	OFFSET HardwareInfo.Chipset_Base
	dw	OFFSET HardwareInfo.CPU_ID
	dw	OFFSET HardwareInfo.CPU_Revision
	dw	OFFSET HardwareInfo.CPU_MHz
	dw	OFFSET HardwareInfo.PCI_MHz
MAX_HW_INFO equ ($-HW_Offsets)/2-1



;***************************************************************************
; Entry:
;   AL - Hardware item to be returned by Select_HW_Info
;        0 = Southbridge ID
;        1 = Southbridge Revision
;        2 = Southbridge PCI address
;        3 = CPU ID
;        4 = CPU Revision
;        5 = CPU MHz
;        6 = PCI MHz
;***************************************************************************
Select_HW_Info proc

	mov	al, byte ptr [Saved_EAX]
	mov	[HW_Index], al
	ret

Select_HW_Info endp

;***************************************************************************
; Entry:
;   ESI - VSM of interest
; Exit:
;   EAX - Priority::Event
;   ECX - Param1
;   EDX - Param2
;***************************************************************************
Get_VSM_Events proc

	mov	esi, [Saved_ESI]	; Get the VSM of interest
	mov	cx, word ptr [CurrentIndex]
EventIndex:
	mov	al, sizeof(EVENT_ENTRY)
	mul	cl
	lea	bx, [Events]
	add	bx, ax
	ASSUME	BX: PTR EVENT_ENTRY
	cmp	esi, [bx].Vsm		; Is it the VSM of interest ?
	je	FoundEvent

	mov	cl, [bx].Link		; Get link to next event
	or	cl, cl
	jnz	EventIndex

	; Increment Event
	inc	ch			; Increment event index
	mov	cl, ch
	cmp	ch, MAX_EVENT		; Last event ?
	jbe	EventIndex
	xor	cx, cx			; Yes, reset variables

	; Set the caller's carry flag
	or	byte ptr ([SMM_Header]).EFLAGS, EFLAGS_CF
	jmp	short Exit


FoundEvent:
	mov	eax, [bx].Param1	; Return Param1 in ECX
	mov	[Saved_ECX], eax
	mov	eax, [bx].Param2	; Return Param2 in EDX
	mov	[Saved_EDX], eax

	mov	ax, [bx].Priority	; Return Priority::Event in EAX
	shl	eax, 16
	movzx	ax, ch
	mov	[Saved_EAX], eax
	
	mov	cl, [bx].Link		; Get link to next event

Exit:	mov	word ptr [CurrentIndex], cx
	ret

	ASSUME	BX:NOTHING


CurrentIndex	db	0
CurrentEvent	db	0

Get_VSM_Events endp



;***************************************************************************
; Entry:
;   Caller's BH - VSM to report (0FFh for next)
;   Caller's CL - clear statistics flag (1=clear)
; Exit:
;    EDX:EAX - # SMIs
;    EBX:ECX - # clocks
;         BP - VSM Type
;    If SysMgr:
;     EDI:ESI - start time
;  16 MSBs BP - clock adjustment 
;   Carry Flag set if no more VSMs
; 
;***************************************************************************
Get_VSM_Statistics proc

	call	Common_Entry
	jc	Exit
	or	ecx, ecx
	jz	NoMoreVSMs


	mov	dl, byte ptr [Saved_ECX]	; Get /S flag
	cld


	; If SysMgr, freeze statistics & return extra information
	cmp	fs:(VSM_Header PTR [ecx]).VSM_Type, VSM_SYS_MGR
	jne	ReturnInfo

	mov	bx, OFFSET VSM_Header.SysStuff
	ASSUME	BX: PTR System

	;     EDI:ESI - start time
	; 16 MSBs EBP - adjustment (clocks/SMI)
	mov	ax, word ptr [bx].Adjustment
	mov	word ptr [Saved_EBP+2], ax

	mov	eax, [bx+0].StartClocks
	mov	[Saved_ESI], eax
	mov	eax, [bx+4].StartClocks
	mov	[Saved_EDI], eax

	; Clear statistics ?
	cmp	dl, 1
	jne	short FreezeStats

	; Yes, record a new start time
	push	dx
	rdtsc
	mov	[bx+0].StartClocks, eax
	mov	[bx+4].StartClocks, edx
	pop	dx

	ASSUME	BX:NOTHING



	; Freeze statistics of all VSMs
FreezeStats:
	push	ecx
	mov	ebx, ecx
	xor	eax, eax
VSM_Loop:
	lea	esi, (VSM_Header PTR [ebx]).SysStuff.Clocks
	lea	edi, (VSM_Header PTR [ebx]).SysStuff.FrozenClocks
	mov	ecx, 4
	rep	movsd [edi], es:[esi]

	; Clear statistics ?
	cmp	dl, 1
	jne	short NextVSM
	lea	edi, (VSM_Header PTR [ebx]).SysStuff.Clocks
	mov	cl, 4
	rep	stosd [edi]

NextVSM:
	mov	ebx, es:(VSM_Header PTR [ebx]).SysStuff.Flink
	or	ebx, ebx
	jnz	VSM_Loop

	pop	ecx


	; Put info into caller's registers
ReturnInfo:
	movzx	ax, es:(VSM_Header PTR [ecx]).VSM_Type
	mov	word ptr [Saved_EBP], ax

	lea	esi, (VSM_Header PTR [ecx]).SysStuff.FrozenClocks
	lodsd	es:[esi]
	mov	[Saved_EAX], eax
	lodsd	es:[esi]
	mov	[Saved_EDX], eax
	lodsd	es:[esi]
	mov	[Saved_ECX], eax
	lodsd	es:[esi]
	mov	[Saved_EBX], eax

	call	Common_Exit
	ret

NoMoreVSMs:
	; Set caller's CF
	or	byte ptr ([SMM_Header]).EFLAGS, EFLAGS_CF
Exit:	ret

Get_VSM_Statistics endp


;***************************************************************************
; Invalidates the cache(s)
;***************************************************************************
Do_WBINVD proc

	wbinvd
	ret

Do_WBINVD endp


;***************************************************************************
; Updates the CR0 field of each VSM with the current non-VSA CR0
;***************************************************************************
Update_VSMs_CR0 proc

	mov	si, OFFSET [SMM_Header[0].r_CR0]
	mov	eax, [si]		; Get current CR0
	call	Set_CR0
	ret

Update_VSMs_CR0 endp

	END
