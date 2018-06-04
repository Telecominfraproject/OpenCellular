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
;*     This file contains the installation code for VSA II.
;*
;*   BIOS Usage:
;*     CALL far ptr [VSA_Entry]
;*         where VSA_Entry:   dw 0020h, <segment>
;*
;*******************************************************************************


include vsa2.inc
include sysmgr.inc
include init.inc
include smimac.mac
include chipset.inc
;include vpci.inc
DESCRIPTOR struc
DescrType  BYTE  ?      ; Type of MSR
Flag       BYTE  ?      ; See definitions below
Link       BYTE  ?      ; Link to next MSR
Split      BYTE  ?      ; Index of descriptor that was split

Owner      WORD  ?      ; PCI Address this descriptor belongs to
Mbiu       BYTE  ?      ; MBUI on which this descriptor is located
Port       BYTE  ?      ; Port this descriptor routes to

MsrAddr    DWORD ?      ; Routing address of MSR (descriptor/LBAR/RCONF)
MsrData0   DWORD ?      ; MSR data low
MsrData1   DWORD ?      ; MSR data high
Physical   DWORD ?      ; Physical memory assigned  (00000000 if none)
Range      DWORD ?      ; Actual I/O range for IOD_SC
Address    WORD  ?      ; Address of I/O Trap or Timeout
DESCRIPTOR ENDS


.model small,c
.586
.CODE

VERIFY_VSM_COPY equ	1		; 0=skip verify


SW_SMI		equ	0D0h


; VSA loader debug codes:
VSA_ENTERED  	equ     10h		; VSA installation has been entered
VSA_INIT1    	equ     11h		; Returned from Setup
VSA_SYSMGR	equ     12h		; Image of SysMgr was found
VSA_VSM_FOUND	equ     13h		; A VSM has been found (followed by VSM type)
VSA_COPY_START	equ     14h		; Start of module copy
VSA_COPY_END	equ     15h		; End of module copy
VSA_VRFY_START equ     1Ch		; Start of verifying module copy
VSA_VRFY_END   equ     1Dh		; End of verifying module copy
VSA_FILL_START equ     1Eh		; Start of filling BSS
VSA_FILL_END   equ     1Fh		; End of filling BSS
VSA_INIT2    	equ     16h		; Returned from copying VSA image
VSA_INIT3    	equ     17h		; Performing VSA initialization SMI
VSA_INIT4    	equ     18h		; Returned from s/w SMI
VSA_INIT5    	equ     19h		; Returned from initializing statistics
VSA_INIT6    	equ     1Ah		; Returning to BIOS
VSA_ERROR	equ     0EEh		; Installation error. EBX contains error mask



; NOTES:
;  1) A VSM's CS segment must be writeable since the message queue is
;     stored there and the VSM must be able to update the pointers.
;  2) The nested flag must be set so when the System Manager RSMs
;     to a VSM, the processor remains in SMM.
VSM_FLAGS	equ 	SMI_FLAGS_CS_WRITABLE + SMI_FLAGS_NESTED + SMI_FLAGS_CS_READABLE





POST	macro	Code

    	mov	al, Code
    	out	80h, al

    	endm

public	Device_ID
public	Chipset_Base
public	Errors
public	BIOS_ECX
public	BIOS_EDX


externdef Software_SMI:	proc
externdef Get_Sbridge_Info:	proc
externdef Get_IRQ_Base:	proc
externdef Get_SMI_Base:	proc
externdef Get_CPU_Info:	proc
externdef Clear_SMIs:		proc
externdef Get_SMM_Region:	proc
externdef Init_SMM_Region:	proc
externdef Enable_SMIs:		proc
externdef Enable_SMM_Instr:	proc
externdef Disable_A20:		proc
externdef Get_Memory_Size:	proc
externdef Set_CPU_Fields:	proc
externdef VSA_Image: 		byte




	ORG	0000h
;***********************************************************************
; Entry Point for DOS installer
;***********************************************************************
VSA_Installation:

	mov	dx, offset Msg_Not_DOS_BUILD
	push	cs			; DS <= CS
	pop	ds
	mov	ah, 9			; Call DOS PrintString
	int	21h


ReturnToDOS:
	sti
	mov	ah, 4Ch			; Return to DOS
	int	21h


	org	001Eh			; Used by INFO to skip over init code
	dw	offset VSA_Image


;***********************************************************************
; Entry point for BIOS installer
;***********************************************************************
	org	0020h

VSA_Install proc far

	POST	VSA_ENTERED

	pushf
	push	cs			; DS <- CS
	pop	ds
	ASSUME	DS:_TEXT
	mov	[BIOS_ECX], ecx		; MSR address of SMM memory descriptor (P2D_BMO)
	mov	[BIOS_EDX], edx		; MSR address of system memory descriptor (P2D_R)


;
; Make sure VSM's SmiHeader is DWORD aligned
;
	mov	si, VSM_Header.SysStuff.State
	test	si, 3
	jz	AlignmentOK
	mov	bx, [Errors]
	or	bx, ERR_INTERNAL
	jmp	DisplayErrors

AlignmentOK:


	; Set up for SMM
	call	Setup
	jc	DisplayErrors	

	; Load System Manager and VSMs into memory
	POST	VSA_INIT1
	call	ProcessModules
	POST	VSA_INIT2

	push	cs
	pop	ds
	
	mov	bx, [Errors]		; Any errors detected ?
	test	bx, bx
	jz	Init_VSA
DisplayErrors:
	POST	VSA_ERROR

	jmp	Exit


Init_VSA:
	cli

	; Set SMM MSRs
	mov	ebx, [SysMgrEntry]
	call	Init_SMM_Region


	; Generate s/w SMI to initialize VSA
	POST	VSA_INIT3		; POST 17h


	call	Enable_SMIs		; Enable SMIs
	movzx	eax, [SysCall_Code]
	mov	ebx, [InitParam1]
	mov	ecx, [InitParam2]
	call	Software_SMI

	POST	VSA_INIT4		; POST 18h


	call	InitStatistics		; Initialize VSA statistics

	POST	VSA_INIT5		; POST 19h


Exit:
	POST	VSA_INIT6		; POST 1Ah
	popf
	ret

VSA_Install endp

	ASSUME	DS:NOTHING
	

;***********************************************************************
;***********************************************************************
;***********************************************************************
;***********************************************************************





;***********************************************************************
;
; Some of the time in SMM cannot be recorded by VSA.
; This unaccounted time consists of:
;  1) the cycles used to write the SMM header.
;  2) the cycles for the SMM entry code before RDTSC is executed.
;  3) the cycles between the exit RDTSC through the RSM instruction.
;
; The following code calculates how many clocks this time consists
; of and stores it in a VSA structure.  It is used as an adjustment
; to the statistics calculations.
;
;***********************************************************************
InitStatistics proc

	in	al, 92h			; Save A20 mask
	push	ax

	mov	si, 0			; Don't toggle A20 (no SMI)
	call	DeltaSMI
	mov	edi, eax		; EDI = overhead of DeltaSMI()

	mov	ebx, [SysMgrEntry]	; Get ptr to SysMgr's header
	add	ebx, VSM_Header.SysStuff
	mov	es:(System PTR [ebx]).Clocks, 0
	mov	si, 2			; Generate an SMI by toggling A20
	call	DeltaSMI

	mov	ecx, es:(System PTR [ebx]).Clocks
	sub	eax, ecx		; Subtract clocks VSA determined
	sub	eax, edi		; Subtract DeltaSMI() overhead
	mov	es:(System PTR [ebx]).Adjustment, eax

	pop	ax			; Restore A20 mask
	out	92h, al 

	; Initialize statistics structure
	RDTSC				; Record VSA start time
	mov	es:(System PTR [ebx+0]).StartClocks, eax
	mov	es:(System PTR [ebx+4]).StartClocks, edx
	xor	eax, eax
	mov	es:(System PTR [ebx+0]).Clocks, eax
	mov	es:(System PTR [ebx+4]).Clocks, eax
	mov	es:(System PTR [ebx+0]).NumSMIs, eax
	mov	es:(System PTR [ebx+4]).NumSMIs, eax
	ret

InitStatistics endp

;***********************************************************************
; Helper routine for InitStatistics()
; Input: SI = XOR mask for port 92h
;***********************************************************************
DeltaSMI proc

	RDTSC				; Record VSA start time
	mov	ecx, eax

	in	al, 92h			; Generate an SMI by toggling A20 mask
	xor	ax, si
	out	92h, al

	RDTSC				; Compute actual delta clocks
	sub	eax, ecx
	jnc	Exit
	neg	eax			; TSC rolled over
Exit:	ret

DeltaSMI endp




;***********************************************************************
; Setup for VSA installation:
; - Gets information about the system: CPU, Southbridge, memory, PCI
; - Sets ES to be 4 GB descriptor
; - Clears pending SMIs
;***********************************************************************
Setup proc near

	ASSUME	DS:_TEXT

	cli

	call	Get_CPU_Info		; Get information about the CPU
	mov	[Cpu_Revision], ax
	mov	[Cpu_ID], si
	mov	[PCI_MHz], bx
	mov	[CPU_MHz], cx
	mov	[DRAM_MHz], dx

	call	Get_Sbridge_Info	; Get information about the Southbridge
	jc	short SetupExit
	mov	[Chipset_Base], ebx
	mov	[Device_ID], dx
	mov	[Chipset_Rev], cl

	call	Get_SMM_Region		; Get SMM entry point
	mov	[ModuleBase], eax
	mov	[SysMgr_Location], eax
	mov	[VSA_Size], bx
	movzx	ebx, bx			; Get end of VSA memory
	shl	ebx, 10
	add	eax, ebx
	mov	[VSA_End], eax

	call	Enable_SMM_Instr	; Enable SMM instructions


	rsdc	es, [Flat_Descriptor]	; Setup ES as a 4 GB flat descriptor


	call	Disable_A20		; Turn off A20 masking

	
	call	Get_Memory_Size		; Get physical memory size
	mov	[TotalMemory], eax


	call	Clear_SMIs		; Clear all pending SMIs

SetupExit:
	mov	[SetupComplete], 0FFh
	clc
	ret


	ASSUME	DS: NOTHING

Setup endp











;*****************************************************************************
;
; Loads the VSA II image into memory
; NOTE: The System Manager must be the first module of the VSA image. 
;
;*****************************************************************************
ProcessModules proc

	; Point DS:ESI to loaded VSA image
	lea	bx, [VSA_Image]			; Point DS to start of file image
	movzx	esi, bx
	and	si, 000Fh
	shr	bx, 4
	mov	ax, cs
	add	ax, bx
	mov	ds, ax

	; Point EDI to where System Manager will be loaded
	mov	edi, cs:[ModuleBase]		; Get last ptr value
	call	AlignVSM
	mov	cs:[SysMgrEntry], edi		; Record entry point of System Manager


	; Ensure the System Manager is the first module unless loading from DOS
	mov	ax, ERR_NO_SYS_MGR
 	cmp	(VSM_Header PTR [si]).Signature, VSM_SIGNATURE	
	jne	ErrExit
 	cmp	(VSM_Header PTR [si]).VSM_Type, VSM_SYS_MGR
	je	LoadSysMgr


LoadSysMgr:
	POST	VSA_SYSMGR
	call	PatchSysMgr			; Apply patches to the System Manager

	;   EDI = flat ptr of System Manager base
	; DS:SI = near ptr to System Manager image
	call	CopyModule			; Install System Manager
	jc	ErrExit



	; Sequence through each VSM and install it.
VSM_Loop:
	mov	eax, VSM_SIGNATURE		; Check for a VSM signature
 	cmp	eax, (VSM_Header PTR [si]).Signature
	jne	short Return
	call	Load_VSM			; Load the VSM
	jnc	VSM_Loop


ErrExit:or	cs:[Errors], ax
Return:	ret

ProcessModules endp






;*****************************************************************************
; Copies INT vector table to VSA. If installing VSA from DOS, copies the
; INT_Vector[] from current System Manager to the VSA image being installed.
;*****************************************************************************
SnagInterruptVectors proc

Exit:	ret

SnagInterruptVectors endp






;*******************************************************************
;
; Aligns a VSM ptr according to requirements flag
; Input:
;   EDI = ptr to be aligned
; Output:
;   EDI = aligned ptr
;
;*******************************************************************
AlignVSM proc uses eax

	mov	cx, cs:[Requirments]		; Compute alignment
	and	cl, MASK Alignment@@tag_i0	; Compute mask from 2^(n+4)
	add	cl, 4
	xor	eax, eax
	mov	al, 1
	shl	eax, cl
	dec	eax

	add	edi, eax			; Align the next VSM load address
	not	eax				; to requested boundary
	and	edi, eax
	ret

AlignVSM endp




;*******************************************************************
; Patches various fields within System Manager
; INPUT:
;   SI = ptr to System Manager
;  EDI = ptr to where System Manager will reside
;*******************************************************************
PatchSysMgr proc

	pushad

	;
	; Patch "jmp <EntryPoint>" over System Manager's VSM signature
	;
	mov	ax, (VSM_Header PTR [si]).EntryPoint
	sub	ax, 3
	shl	eax, 8
	mov	al, 0E9h			; JMP opcode
	mov	(VSM_Header PTR [si]).Signature, eax

	;
	; Patch SysMgr's Hardware structure
	;
	lea     bx, [si+SPECIAL_LOC]
	mov     bx, (InfoStuff PTR [bx]).HardwareInfo
	ASSUME  BX: PTR Hardware

	mov	ax, cs:[Device_ID]
	mov	[bx].Chipset_ID, ax
	
	mov	ax, cs:[PCI_MHz]
	mov	[bx].PCI_MHz, ax
	
	movzx	ax, cs:[Chipset_Rev]
	mov	[bx].Chipset_Rev, ax

	mov	eax, cs:[Chipset_Base]
	mov	[bx].Chipset_Base, eax

	mov	ax, cs:[Cpu_ID]
 	mov	[bx].CPU_ID, ax

	mov	ax, cs:[Cpu_Revision]
	mov	[bx].CPU_Revision, ax

	mov	ax, cs:[CPU_MHz]
	mov	[bx].CPU_MHz, ax

	mov	ax, cs:[DRAM_MHz]
	mov	[bx].DRAM_MHz, ax

	mov	eax, cs:[TotalMemory]
	mov	[bx].SystemMemory, eax

	mov	eax, cs:[SysMgr_Location]
	mov	[bx].VSA_Location, eax
	mov	[si].SysStuff.SysMgr_Ptr, eax

	mov	ax, cs:[VSA_Size]
	mov	[bx].VSA_Size, ax
	ASSUME	BX:NOTHING

	;
	; Patch SysMgr's descriptors
	;
	mov	eax, (VSM_Header PTR [si]).DS_Limit 
	mov	[SysMgrSize], ax
	lea	bx, (VSM_Header PTR [si])._DS
	call	Init_Descr

	mov	eax, SYSMGRS_STACK + VSM_STACK_FRAME	
	lea	bx, (VSM_Header PTR [si])._SS
	call	Init_Descr
	
	;
	; Initialize the SysMgr's message queue
	;
	call	Init_Msg_Q

	;
	; Patch variables in SysMgr
	;
	mov	eax, cs:[Chipset_Base]
	mov	[si].SysStuff.Southbridge, eax

	mov	bx, si
	add	si, SPECIAL_LOC
	ASSUME	SI: PTR InfoStuff
	
	movzx	eax, [si].SysMgr_Stack
	mov	(VSM_Header PTR [bx]).SysStuff.SavedESP, eax

	mov	[si].SysMgr_VSM, edi

	lea	eax, (VSM_Header PTR [edi]).SysStuff.State + sizeof(SmiHeader)
	add	bx, [si].Header_Addr
	mov	[bx], eax

	call	Get_SMI_Base
	mov	[si].SMI_Base, eax
	
	call	Get_IRQ_Base
	mov	[si].IRQ_Base, eax

	
Exit:

	call	SnagInterruptVectors		; Snapshot INT vectors from current VSA

	popad
	ret


PatchSysMgr endp




;*******************************************************************
;
; Loads a VSM into memory
; Input: 
;   DS:SI = ptr to VSM to load
;
;*******************************************************************
Load_VSM proc

	POST	VSA_VSM_FOUND


	; Initialize the VSM's Header
	ASSUME	si:PTR VSM_Header

	mov	ax, [si].Flag
	mov	cs:[Requirments], ax
	test	ax, MASK SkipMe@@tag_i0
	jz	LoadIt

Skip_VSM:	
	add	esi, [si].VSM_Length		; Point to end of this VSM
	call	Flat_ESI
	jmp	Next_VSM
LoadIt:

	; Initialize CPU dependent fields in the VSM's SMM header
	call	Set_CPU_Fields


	; Initialize EIP to VSM's entry point 
	movzx	ecx, [si].EntryPoint
	mov	[si].SysStuff.State.Next_EIP, ecx

	or	[si].SysStuff.State.SMI_Flags, VSM_FLAGS


	; Store ptrs to certain System Manager structures for fast access
	mov	eax, cs:[SysMgrEntry]
	add	eax, SPECIAL_LOC
	mov	[si].SysStuff.SysMgr_Ptr, eax

	mov	eax, cs:[Chipset_Base]
	mov	al, SW_SMI
	mov	[si].SysStuff.Southbridge, eax



	; Initialize the VSM's resume header
	; Get size of DS segment
	mov	eax, [si].DS_Limit		; _DS.limit_15_0

	
	inc	eax				; Round to WORD boundary
	and	al, NOT 1
	mov	ecx, eax

	; Initialize the CS descriptor fields
	; NOTE: CS descriptor fields in SMM header are in "linear" format.
	mov	[si].SysStuff.State._CS.limit, ecx
	mov	[si].SysStuff.State._CS.attr, CODE_ATTR

	mov	[si].SysStuff.State.EFLAGS, VSM_EFLAGS
	mov	ecx, CR0			; Preserve the CD & NW bit
	and	ecx, 60000000h
	or	ecx, VSM_CR0
	mov	[si].SysStuff.State.r_CR0,  ecx
	mov	[si].SysStuff.State.r_DR7,  VSM_DR7   


	; Determine size of VSM's stack
	movzx	ecx, [si]._SS.limit_15_0	; Get SS limit
	or	cx, cx				; Did VSM specify a stack size ?
	jnz	CheckMemSize
	mov	cx, VSM_STACK_SIZE	 	; No, use default stack size

;*******************************************************************
;  SI = ptr to VSM image to be loaded
; EAX = DS limit
; ECX = stack size
;*******************************************************************
CheckMemSize:
	add	eax, ecx			; Compute descriptor limits
	add	ax, 15				; Round up to next paragraph
	and	al, NOT 15
	mov	[si].DS_Limit,eax		; Update for use by BIOS scan

	lea	ebx, [edi+eax]			; If not enough memory, skip this VSM
	cmp	ebx, [VSA_End]
	jae	Skip_VSM
	call	AlignVSM
	mov	cs:[ModuleBase], edi		; Save the VSM module pointer in EDI


;*******************************************************************
;
; Initialize VSM's descriptors
;
;  SI = ptr to VSM image to be loaded
; EAX = DS limit
; EDI = Runtime address for this VSM
;
;*******************************************************************


	; Set VSM's CS selector to <load address> & 0xFFFFF
	mov	ecx, edi
	and	ecx, 000FFFFFh
	shr	ecx, 4
	mov	[si].SysStuff.State._CS.selector, cx

	mov	[si].SysStuff.State._CS.base, edi


	; Initialize the VSM's stack ptr
	mov	ebx, eax
	and	bl, 0FCh			; Round DOWN to nearest DWORD
	sub	bx, VSM_STACK_FRAME
	mov	[si].SysStuff.SavedESP, ebx

	lea	bx, [si]._DS			; Init DS
	call	Init_Descr

	lea	bx, [si]._ES			; Init ES
	call	Init_Descr

	lea	bx, [si]._SS			; Init SS
	call	Init_Descr


	; Initialize the VSM's message queue
	call	Init_Msg_Q
	
 
	; Update the doubly linked list of VSMs
	mov	ebx, edi
	xchg	ebx, cs:[Blink]
	or	ebx, ebx			; 1st VSM other than System Manager ?
	jnz	SetFlink

       	; Store ptr to 1st VSM in the System Manager
	mov	eax, cs:[SysMgrEntry]
	mov	es:(VSM_Header PTR [eax]).SysStuff.Flink, edi

	jmp	Setlinks

	; EBX points to previous VSM
SetFlink:
	mov	(VSM_Header PTR es:[ebx]).SysStuff.Flink, edi
Setlinks:
	mov	[si].SysStuff.Blink, ebx

	mov	eax, cs:[Flink]
	mov	[si].SysStuff.Flink, eax

	call	CopyModule
	ret


Load_VSM endp


END_MSG_QUEUE   equ  (MAX_MSG_CNT)*sizeof(Message)


Init_Msg_Q proc
	
	mov	bx, offset VSM_Header.SysStuff.MsgQueue
	mov	[si].SysStuff.Qhead, bx 	; Store queue head ptr
	mov	[si].SysStuff.Qtail, bx 	; Store queue tail ptr
	add	bx, END_MSG_QUEUE		; End of queue
	mov	[si].SysStuff.EndMsgQ, bx
	ret
	
Init_Msg_Q endp	







;*******************************************************************
;
; Initializes a descriptor
; On entry:
;   BX = ptr to descriptor
;  EAX = limit
;  EDI = base
;
;*******************************************************************
Init_Descr proc
	ASSUME	bx: PTR Descriptor

	push	eax
	push	edi
	mov	[bx].limit_15_0, ax		; limit
	shr	eax, 16	
	mov	[bx].limit_19_16, al
 	mov	[bx].base_15_0, di		; base[15:00]
	shr	edi, 16
	mov	ax, di
	mov	[bx].base_23_16, al		; base[31:16]
	mov	[bx].base_31_24, ah
	mov	[bx].attr, DATA_ATTR		; attribute
	pop	edi
	pop	eax
	ret

	ASSUME	bx: NOTHING

Init_Descr endp


;*******************************************************************
;
; Copies a module to its runtime location.
; On Entry:
;    DS:SI - ptr to source
;   ES:EDI - ptr to destination
; On Exit:
;       CF - set if error. AX = error code
;*******************************************************************
CopyModule proc



	; Get length of segment & compute size of BSS
	mov	ecx, [si].VSM_Length
	movzx	edx, [si]._DS.limit_15_0
	sub	edx, ecx
 

	call	Flat_ESI


	;
	; Copy the VSM image to its runtime location
	;

if VERIFY_VSM_COPY
	push	ecx			; Save byte count & ptrs
	push	esi
	push	edi
endif
	
	POST	VSA_COPY_START		; 14h

	mov	ah, cl
	shr	ecx, 2			; Convert BYTE count to DWORD count
	cld
	rep	movsd [edi], es:[esi]

	and	ah, 3 			; Copy remaining bytes
	mov	cl, ah
	rep	movsb [edi], es:[esi]

	POST	VSA_COPY_END		; 15h

if VERIFY_VSM_COPY

	pop	edi			; Restore original ptrs and byte count 
	pop	esi
	pop	ecx

	;
	; Verify the VSM image copy
	;
	mov	ebx, edi		; Save ptr to start of VSM
	shr	ecx, 2			; Convert byte count to dword count

	POST	VSA_VRFY_START           ; 1Ch

	repe    cmpsd es:[esi], [edi]
	jne	VerifyError

	mov	cl, ah			; Compare leftover byte(s)
	repe    cmpsb es:[esi], [edi]
	je	Verify_Passed

VerifyError:

	mov	ax, ERR_VERIFY
	stc
	jmp	Exit
	
Verify_Passed:
	POST	VSA_VRFY_END		; 1Dh

endif  ; VERIFY_VSM_COPY




	POST	VSA_FILL_START

	;
	; Clear the uninitialized data space
	;
	mov	ecx, edx		; Fill BSS with zeros
	shr	ecx, 2
	xor	eax, eax
	rep	stosd [edi]
	mov	cl, dl
	and	cl, 3
	je	BSS_Cleared
	rep	stosb [edi]
BSS_Cleared:


	POST	VSA_FILL_END
	



	; Search the image for the next VSM.
	; Some VSMs lie about their size.  Search a byte at a time
	; for 'VSM ', first backward, then forward.
Next_VSM::
	mov	edx, -1			; Start search backwards
MAX_SEARCH  equ 16
Search:
	mov	cx, MAX_SEARCH		; Range in bytes to scan for VSM

SearchNext:
	cmp	dword ptr es:[esi], VSM_SIGNATURE
	je	NextVSMfound
	add	esi, edx 
	loop	SearchNext

	add	esi, MAX_SEARCH
	neg	edx			; Look the other direction
 	cmp	dl, 1			; Have we already looked in that direction ?
	je	Search
	jmp	OK_Exit			; No more VSMs found


NextVSMfound:

	; Convert ESI into DS:SI format
	mov	edx, esi
	shr	edx, 4
	mov	ds, dx
	and	esi, 0000000Fh
OK_Exit:
	clc
Exit:	ret



CopyModule endp


;*******************************************************************
; Converts DS:SI to a flat ptr in ESI.
;*******************************************************************
Flat_ESI proc

	push	eax
	mov	ax, ds			; Convert DS:SI to a flat ptr
	movzx	eax, ax
	shl	eax, 4
	movzx	esi, si
	add	esi, eax
	pop	eax
	ret

Flat_ESI endp


BIOS_ECX	dd	0
BIOS_EDX	dd	0
TotalMemory	dd	0
SysMgr_Location dd	0
VSA_End	dd	0
Chipset_Base	dd	0
SysMgrEntry	dd	00000000h
ModuleBase	dd	00000000h
Blink		dd	00000000h
Flink		dd	00000000h
InitParam1	dd	0
InitParam2	dd	0

SysMgrSize	dw	0
VSA_Size	dw	0
Errors	 	dw	0
Device_ID	dw	0
Cpu_ID		dw	0
Cpu_Revision	dw	0
SysCall_Code	dw	SYS_BIOS_INIT
Requirments	dw	0
CPU_MHz	dw	0
PCI_MHz	dw	0
DRAM_MHz	dw	0
Chipset_Rev	db	0
SetupComplete	db	0
LoadFlag	db	0

Flat_Descriptor	Descriptor {0FFFFh, 0000h, 00h, DATA_ATTR, 8Fh, 00h, 0}

Msg_CompareError	db	"...compare error at 0x$"
Msg_Not_DOS_BUILD	db	CR,LF, "This version of INIT doesn't support DOS install.$"


	END VSA_Installation



