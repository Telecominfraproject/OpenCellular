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
;*******************************************************************************
;*     Miscellaneous utility routines
;*******************************************************************************


include SYSMGR.INC
include VSA2.INC
include PCI.INC
include GX2.INC

.model tiny,c
.586p
.CODE

externdef SchedulerStack:   word
externdef SysMgr_VSM:       dword
externdef Saved_PCI:	    dword
externdef Nested_PCI:       dword
externdef MsgPacket:        dword
externdef ClocksPerMs:      dword
externdef StartSaveArea:    dword
externdef Header_Addr:      dword
externdef Nested_Flag:      dword
externdef VSM_Ptrs:         dword
externdef Events:           EVENT_ENTRY

	


;************************************************************************
;
; 8 bit I/O routines
;
;************************************************************************

in_8 proc  pascal \
	io_port: word

	mov	dx, [io_port]
	in 	al, dx
	ret
   
in_8 endp


out_8	proc pascal \
	io_port: word, \
	io_data: byte

	mov	dx, [io_port]
	mov	al, [io_data]
	out	dx, al
	ret

out_8	endp

;************************************************************************
;
; 16 bit I/O routines
;
;************************************************************************

in_16 proc  pascal \
	io_port: word

	mov	dx, [io_port]
	in 	ax, dx
	ret
   
in_16 endp


out_16	proc pascal \
	io_port: word, \
	io_data: word

	mov	dx, [io_port]
	mov	ax, [io_data]
	out	dx, ax
	ret

out_16	endp


;************************************************************************
;
; 32 bit I/O routines
;
;************************************************************************

in_32 proc  pascal \
	io_port: word

	mov	dx, [io_port]
	in 	eax, dx
	mov	edx, eax
	shr	edx, 16
	ret
   
in_32 endp


out_32	proc pascal \
	io_port: word, \
	io_data: dword

	mov	dx, [io_port]
	mov	eax, [io_data]
	out	dx, eax
	ret

out_32	endp




;************************************************************************
;
; Input:
;   Ptr to a VSM header
;
; Output:
;   Ptr to the next VSM in the chain
;
;************************************************************************
GetFlink proc pascal, \
	VSM_Ptr: dword

	mov	ebx, [VSM_Ptr]
	mov	eax, (VSM_Header PTR fs:[ebx]).SysStuff.Flink
	mov	edx, eax
	shr	edx, 16
	ret

GetFlink endp




;************************************************************************
; Writes a BYTE, WORD, DWORD to a 32-bit address
;************************************************************************
write_flat_size proc pascal \
	Address: dword, \
	Data:    dword, \
	Len:     byte

	mov	ebx, [Address]
	mov	eax, [Data]
	mov	cl,  [Len]
	cld
	cmp	cl, BYTE_IO
	jne	short CheckWord
	mov	fs:[ebx], al
	jmp	short Exit
	
CheckWord:	
	cmp	cl, WORD_IO
	je	short WriteWord
WriteDword:
	db	66H
WriteWord:
	mov	fs:[ebx], ax
Exit:	ret

write_flat_size endp

;************************************************************************
; Writes a DWORD to a 32-bit address
;************************************************************************
write_flat proc pascal \
	Address: dword, \
	Data:    dword

	mov	ebx, [Address]
	mov	eax, [Data]
	mov	fs:[ebx], eax
	ret

write_flat endp

;************************************************************************
; Reads a DWORD from a 32-bit address
;************************************************************************
read_flat proc pascal \
	Address: dword

	mov	ebx, [Address]
	mov	eax, fs:[ebx]
	mov	edx, eax
	shr	edx, 16
	ret

read_flat endp



;************************************************************************
;
; Copies the parameters for a synchronous event from the appropriate SMI
; header to the MsgPacket array.
; NOTE: This routine should only be called from a Synchronous SMI handler.
;
; On exit:
;   MsgPacket[1]:  15:0  - Flags field from SMM header
;   MsgPacket[2]:  15:0  - I/O address (or PCI address) from SMM header
;		   31:16 - Data Size field from SMM header
;   MsgPacket[3]:  Data (if I/O write)
;
;   Returns ptr to appropriate SMM header.
;
;************************************************************************
Get_Header_Params proc pascal \
	SMI_Event: dword

	mov     bx, OFFSET VSM_Header.SysStuff.State
	
	ASSUME	BX: PTR SmiHeader

	mov	eax, [SMI_Event]	; Is it a nested event ?
	test	[Nested_Flag], eax
	je	Copy_Params	

	not	eax			; Yes, clear the event
	and	[Nested_Flag], eax

	push	si			; Copy VSM's header to local buffer
	mov	si, bx

	lea	bx, [Nested_Header]
	push	bx
	mov	cx, sizeof(SmiHeader)/4
	cld
CopyHdr:
	lodsd	gs:[si]
	mov	dword ptr [bx], eax
	add	bx, 4
	loop	CopyHdr	

	pop	bx
	pop	si
	
	
	
	; Copy parameters from SMM header to MsgPacket[]
Copy_Params:
	
	movzx	eax, [bx].SMI_Flags
	mov	[MsgPacket+4*1], eax	; MsgPacket[1]


	; Is it a PCI trap ?
	mov	eax, dword ptr [bx].IO_addr
	mov	dx, ax
	and	dl, NOT 3
	cmp	dx, PCI_CONFIG_DATA
	jne	short StoreAddr
	
	; Yes, get PCI address from appropriate context
	mov	cl, al			; Get 2 LSBs of PCI address
	and	cl, 3
	mov	ax, word ptr [Nested_PCI]
	cmp	bx, OFFSET [Nested_Header]
	je	short Get_PCI

	mov	ax, word ptr [Saved_PCI]
Get_PCI:
	or	al, cl
StoreAddr:
	mov	[MsgPacket+4*2], eax	; MsgPacket[2]


	; Get write data
	mov	ecx, [bx].write_data
	shr	eax, 16			; Put I/O size into AL
	cmp	al, DWORD_IO		; Dword I/O ?
	je	short StoreData
	movzx	ecx, cx
	cmp	al, WORD_IO		; Word I/O ?
	je	short StoreData
	xor	ch, ch			; Byte I/O
	
StoreData:	
	mov	[MsgPacket+4*3], ecx	; MsgPacket[3]

	mov	ax, bx			; Return ptr to header
	ret

Nested_Header	SmiHeader {}


Get_Header_Params endp
	
	ASSUME	BX: NOTHING
	

;************************************************************************
; Returns the # milliseconds elapsed on a timer.
;************************************************************************
ElapsedSoFar proc  StartTime: PTR
	
	rdtsc				; Get current timestamp

	mov	bx, [StartTime]		; Subtract timer's start time
	sub	eax, [bx+0]
	sbb	edx, [bx+4]

	idiv	[ClocksPerMs]		; Convert delta to milliseconds
	mov	edx, eax
	shr	edx, 16
	ret
	
ElapsedSoFar endp



;************************************************************************
; Returns the timestamp counter in the passed buffer
;************************************************************************
Store_Timestamp proc pascal \
	TimeStamp:  PTR

	rdtsc
	mov	bx, [TimeStamp]
	mov	[bx+0], eax
	mov	[bx+4], edx
	ret

Store_Timestamp endp



;************************************************************************
; Returns the type of the VSM pointed to by the Vsm parameter
;************************************************************************
Get_VSM_Type proc pascal \
  	Vsm:dword

	mov	ebx, [Vsm]
	mov	al, fs:(VSM_Header PTR [ebx]).VSM_Type
	ret

Get_VSM_Type endp





;************************************************************************
; Translates a logical address to a physical address via the page tables
; NOTES:
;  - This routine should only be called if paging is enabled.
;  - All page & segment protection has already occurred.
;************************************************************************

PAGE_ATTR	equ	0FFFh
CR4_PSE_BIT	equ	0010h
PDIR_PS_BIT	equ	0080h
OFFSET_4M	equ	0003FFFFFh
PAGEBASE_4M	equ	0FFC00000h

Convert_To_Physical_Addr proc pascal \
	Logical: dword

	; Start by checking for 4MB page possibility
	mov	eax, CR4
	test	ax, CR4_PSE_BIT
	jz	short page4K

	mov	ebx, CR3		; Get ptr to page directory base
	and	ebx, 0FFFFFC00h		; Mask attribute bits

	mov	edx, [Logical]		; Get address to translate
	shr	edx, 22			; Get page directory index

	; See if this directory entry points to a 4MB page
	mov	ebx, fs:[ebx+edx*4]

	test	bx, PDIR_PS_BIT
	jz	short page4K

	and	ebx, PAGEBASE_4M	; Get the page base offset

	mov	eax, [Logical]		; Get address to translate
	and	eax, OFFSET_4M		; Peel off the directory index

	or	eax, ebx		; Combine the base and offset

	jmp	short Exit


page4K:
	mov	ebx, CR3		; Get ptr to page directory base
	and	ebx, 0FFFFFC00h		; Mask attribute bits

	mov	eax, [Logical]		; Get address to translate
	
	movzx	ecx, ax			; Extract page offset
	and	cx, PAGE_ATTR		; (contains offset into 4K page)
	
	shr	eax, 12			; Remove page offset
	mov	edx, eax
	
	shr	edx, 10			; Get page directory offset

	mov	ebx, fs:[ebx+edx*4]
	and	bx, NOT PAGE_ATTR 	; Remove page directory attributes

	and	eax, 03FFh		; Extract page table offset 
	mov	eax, fs:[ebx+eax*4]
	and	ax, NOT PAGE_ATTR 	; Remove page attributes
	
	add	eax, ecx		; Add page offset

Exit:	mov	edx, eax		; Return translated addr in DX:AX
	shr	edx, 16	
	ret
	
Convert_To_Physical_Addr endp


;************************************************************************
; Marks a 'Blocked' VSM to 'Ready'
;************************************************************************
Unblock_VSM proc pascal Vsm: dword

	mov	ebx, [Vsm]
	mov	fs:(VSM_Header PTR [ebx]).SysStuff.RunFlag, RUN_FLAG_READY
	ret

Unblock_VSM endp

;MEJ
;************************************************************************
; This routine determines if an event should wake the system.
; If so, the PMCore VSM's RunFlag is changed from 'Sleeping' to 'Ready'.
; There currently are two cases:
; 1) A timer for either the APM or PMCore VSM
; 2) A press of the sleep button while in Legacy PM mode
;
; Returns TRUE if the event is a wake event.
;************************************************************************
;IsWakeEvent proc pascal \
;  	Vsm:	DWORD
;
;	mov	ebx, [Vsm]
;
;	; Is event for a PM-related VSM ?
;	cmp	fs:(VSM_Header PTR [ebx]).VSM_Type, VSM_PM
;	je	short PossibleWakeEvent
;	cmp	fs:(VSM_Header PTR [ebx]).VSM_Type, VSM_APM
;	jne	short NotWakeEvent
;
;	; Yes, mark PM VSM 'Ready' if it is 'Sleeping'
;PossibleWakeEvent:
;	mov	ebx, [VSM_Ptrs+4*VSM_PM]
;	mov	al, fs:(VSM_Header PTR [ebx]).SysStuff.RunFlag
;	cmp	al, RUN_FLAG_SLEEPING
;	jne	short NotWakeEvent
;	mov	fs:(VSM_Header PTR [ebx]).SysStuff.RunFlag, RUN_FLAG_READY
;	jmp	Exit
;	
;
;NotWakeEvent:	
;	xor	al, al
;	
;Exit:	ret
;
;IsWakeEvent endp



;************************************************************************
; Returns the flat address to the VSM performing a system call
; Used for reporting errors in a system call.
;************************************************************************
Get_SysCall_Address proc pascal \
	Vsm: dword, \
	Depth: byte

	mov     eax, [Vsm]

	cmp	eax, [SysMgr_VSM]
	jne	short Get_VSM_Addr

if CHECKED_BUILD

	mov	dx, sp			; Save BP & SP
	shl	edx, 16
	mov	dx, bp

	mov	cl, [Depth]
	add	cl, 2			; Include stack frames for this routine's & Error_Report()
PopStackFrame:
	mov	bx, sp
	leave				; Pop a stack frame
	cmp	sp, OFFSET StartSaveArea
	jae	short Bail
	cmp	sp, [SchedulerStack]
	jb	short Bail
	
	
	dec	cl
	jnz	PopStackFrame
	jmp	short GetFault


Bail:	mov	sp, bx
GetFault:
	mov	bx, sp			; Get return address to faulting CALL
	movzx	ebx, word ptr [bx]
	sub     bx, 3			; Account for CALL sys_xxxxx
	
	mov	bp, dx			; Restore BP & SP
	shr	edx, 16
	mov	sp, dx
else
	xor	ebx, ebx		; Don't attempt to determine address
endif	
	jmp	short Result	

Get_VSM_Addr:

	ASSUME	BX: PTR word

	; There are 3 cases:
	; 1) A system call.
	; 2) In-line assembly (e.g. direct I/O virtual register)
	; 3) A subroutine call to offending I/O
	mov	bx, word ptr gs:(VSM_Header).SysStuff.State.Next_EIP
	cmp	gs:[bx-2], 380Fh	; Was it from a SMINT (system call)
	je	short Sys_Call

	mov	dx, gs:[bx]		; Get next two bytes of VSM's code
	
	dec	bx			; In case it is in-line assembly
	; If the next two instructions are LEAVE & RET, then it is a 
	; subroutine. Report the caller's address.
	cmp	dl, 0C9h		; Is the next instruction a LEAVE ?
	jne	short Result		; No, must be in-line assembly
	and	dh, NOT 1		; Yes, is the next one a struction a RET ?
	cmp	dh, 0C2h
	jne	short Result		; No, must be in-line assembly
Sys_Call:
	mov	bx, word ptr gs:(VSM_Header).SysStuff.SavedESP
	mov	bx, gs:[bx+3*4]		; Get caller's BP
	movzx   ebx, gs:[bx+2]		; Get SP at time of CALL sys_xxxx
	sub     bx, 3			; Account for CALL sys_xxxxx
Result:
	add     eax, ebx

	mov	edx, eax
	shr	edx, 16

	ret

Get_SysCall_Address endp



;************************************************************************
; Returns the 8 MSBs of the DEVID field of the Device Capabilities MSR
;************************************************************************
GetPortID proc 	MBD_Addr: dword

	mov	ecx, [MBD_Addr]
	mov	cx, MBD_MSR_CAP
	rdmsr
	shr	eax, ID_SHIFT
	xor	ah, ah
	ret

GetPortID endp


;************************************************************************
; Clears pending h/w emulation events in an GeodeLink device
;************************************************************************
ClearMbiu proc pascal Mbiu: dword

	mov	ecx, [Mbiu]
	or	ecx, ecx
	jz	short Exit
	mov	cx, MBD_MSR_SMI
	rdmsr
	mov	dl, 1
	wrmsr
	mov	cx, MBD_MSR_ERROR
	rdmsr
	wrmsr

Exit:	ret
	
ClearMbiu endp



;************************************************************************
; Trims a P2D_R descriptor by Range
;************************************************************************
Trim_P2D_R proc pascal uses esi \
	MsrAddr: dword, \
	Range: dword, \
	MsrData: PTR

	mov	ecx, [MsrAddr]		; Get current MSR value
	rdmsr

	mov	ebx, [Range]		; Adjust by Range bytes
	mov	esi, ebx
	shr	esi, (12+12)
	shl	ebx, (20-12)
	sub	eax, ebx
	sbb	edx, esi
	wrmsr

	mov	bx, [MsrData]		; Return modified MSR
	mov	dword ptr [bx+0], eax
	mov	dword ptr [bx+4], edx
	ret
	
Trim_P2D_R endp

	end
