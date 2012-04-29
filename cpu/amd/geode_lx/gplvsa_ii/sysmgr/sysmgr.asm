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
;*     This file contains the entry point to the SMM code.             *
;*     This code performs the following:                               *
;*       1) saves the processor state                                  *
;*       2) reads the top-level SMI source register(s)                 *
;*       3) issues message(s) to the appropriate VSM(s)                *
;*       4) dispatches to VSM(s) until all messages are handled        *
;*       5) restores the processor state 

include SYSMGR.INC
include VSA2.INC
include PCI.INC
include SMIMAC.MAC
include CHIPSET.INC
include CS5536.INC
include GX2.INC

.model tiny,c
.586p

.ALPHA
DGROUP	GROUP _CODE, _TEXT
_CODE	SEGMENT PUBLIC use16 'CODE'
	ASSUME DS:_CODE


BRACKETS  equ	0

public    SysMgr_Entry
public    Nested_SMI
public    Saved_EAX, Saved_AX
public    Saved_EBX, Saved_ECX, Saved_EDX, Saved_PCI
public    Saved_ESI, Saved_EDI, Saved_EBP, Saved_ESP
public    Saved_SS,  Saved_ES,  Saved_DS,  Saved_FS, Saved_GS
public    IDT_Selector, IDT_Base, IDT_Limit
public    VSMs_EAX
public    Nested_PCI, Nested_EDI, Nested_EAX, Nested_ES
public    StartSaveArea, EndSaveArea
public    SchedulerStack
public    Sys_Exit
public    VSM_ListHead
public    SMM_Header
public    SysMgr_VSM
public    Data_Descriptor
public    Flat_Descriptor
public    Current_VSM
public    Nested_Flag
public    HardwareInfo
public    SMI_Base
public    IRQ_Base, IRQ_Mask
public    Header_Addr
public    BracketFlag
public    ExitSysCall
public    EmptyMsgQueue
public    Dispatcher
public    Trap_Code

externdef Trap_Common:        proc
externdef Trap7:              proc
externdef Get_SMI_Sources:    proc
externdef VSA_Entry:          proc
externdef VSA_Exit:           proc
externdef Show_SMI_Source:    proc
externdef Generate_IRQ:       proc
externdef pascal Hex_8:       proc
externdef pascal Hex_16:      proc
externdef pascal Hex_32:      proc
externdef pascal SMINT_Handler:proc

externdef INT_Vectors:        dword
externdef SMI_Sources:        dword
externdef SynchEvents:        dword
externdef HiPrioritySMIs:     dword
externdef MSRs:               dword

externdef SysCall_Table:      word

externdef NumDescriptors:     byte
externdef Events:             byte
externdef MSRs:               byte
externdef _end:	       byte
externdef edata:              byte

externdef Handler_Table:      SMI_ENTRY








; NOTE: "#define EXTRA_SAVE" in SYSMGR.H must match the number of bytes
;        of state (over and above the registers) that is saved by the
;        following two macros:
SAVE_STATE macro
	pushad				; Save general purpose registers
	mov	dx, PCI_CONFIG_ADDRESS 	; Save PCI Configuration Address
	in	eax, dx
	push	eax

	endm

 
RESTORE_STATE macro
	pop	eax			; PCI Configuration Address
	mov	dx, PCI_CONFIG_ADDRESS
	out	dx, eax
	popad				; Restore general purpose registers

	endm




Start:
; NOTE: The VSA II installer patches a "JMP SysMgr_Entry" over the signature field
	dd	VSM_SIGNATURE		; VSM signature
	db	VSM_SYS_MGR		; VSM type
	db	0FFh			; Any CPU
	dw	DEVICE_ID_5536		; VSA for CS5536
	dw	VSA_VERSION		; System Manager version 
	dd	OFFSET edata		; Size of System Manager
	dw	OFFSET SysMgr_Entry	; EntryPoint
	dd	OFFSET _end		; DS Limit
	dw	0007h			; Requirements: 4096-byte boundary
 	dw	VSA_VERSION		; VSA version

Stack_Descriptor:
 	Descriptor {_end,   0000h, 00h, DATA_ATTR, 00h, 00h, 0000h}
Data_Descriptor:
 	Descriptor {_end,   0000h, 00h, DATA_ATTR, 00h, 00h, 0000h}

Flat_Descriptor:
	Descriptor {0FFFFh, 0000h, 00h, DATA_ATTR, 8Fh, 00h, 0000h}


		dw	0		; .AlignSystem
SMM_Header	SmiHeader {}		; .State
VSM_ListHead	dd	0		; .Flink (ptr to 1st VSM; zero if no VSMs)

Trap	macro  Trap_Num
	ORG	Trap_Code + (Trap_Num * 8)
	mov	bx, Trap_Num
	jmp	Trap_Common
	endm

	ORG	sizeof(VSM_Header)
	align	16

;***********************************************************************
; Exception vectors
;***********************************************************************
Trap_Code:
	Trap	0
	Trap	1
	Trap	2
	Trap	3
	Trap	4
	Trap	5
	Trap	6

	ORG	Trap_Code + (7 * 8)
	jmp	Trap7

	Trap	8
	Trap	9
	Trap	0Ah
	Trap	0Bh
	Trap	0Ch
	Trap	0Dh
	Trap	0Eh
	Trap	0Fh



;***********************************************************************
;       Non-nested VSA entry point                                     *
;***********************************************************************
SysMgr_Entry:

	;
	; Save state of interrupted task & initialize VSA environment
	;
	svdc	cs:[Saved_DS], ds	; Save DS descriptor & set to SysMgr segment
	rsdc	ds, cs:[Data_Descriptor]
	ASSUME  DS:_CODE

	svdc	[Saved_ES], es		; Save ES descriptor
	svdc	[Saved_FS], fs		; Save FS descriptor	
	svdc	[Saved_GS], gs		; Save GS descriptor

	svdc	[Saved_SS], ss		; Save SS descriptor & set to SysMgr segment
	rsdc    ss, [Stack_Descriptor]
	mov     [Saved_ESP], esp	; Save ESP & set up SysMgr stack
	mov     esp, OFFSET SysMgrStack

	SAVE_STATE			; Save the general purpose registers on SysMgr's stack


	rdtsc				; Get start time of this SMI
	mov     (VSM_Header PTR ds:[0]).SysStuff.StartTime, eax
	mov     (VSM_Header PTR ds:[4]).SysStuff.StartTime, edx

	rsdc	fs, [Flat_Descriptor]	; Set FS descriptor to a 4 GB flat segment
	rsdc	es, [Flat_Descriptor]	; Set ES descriptor to a 4 GB flat segment

if BRACKETS
	cmp	[BracketFlag], 0
	je	short NoBracket
	mov	dx, DBG_PORT
	mov	al, '['
	out	dx, al
	
NoBracket:
endif


	call	VSA_Entry		; Perform VSA entry setup

	xor	eax, eax
	mov	[Nested_Flag], eax

	;
	; Check for SMINT
	;
	test	[SMM_Header].SMI_Flags, 1000b
	jz	short Main_SMI_Loop 

	push	word ptr [Saved_EAX]
	call	SMINT_Handler
	jmp	Dispatcher
	
;***********************************************************************
;***********************************************************************
;***********************************************************************
;                                                                      *
;	Main SMI Loop                                                  *
;                                                                      *
;   1) Read the top-level SMI sources.                                 *
;   2) If no SMIs are pending, exit SMM.                               *
;   3) Call an SMI handler for each pending SMI source.                *
;   4) Dispatch to VSMs that have non-empty message queues.            *
;   5) Rinse and repeat.                                               *
;                                                                      *
;***********************************************************************
;***********************************************************************
;***********************************************************************
Main_SMI_Loop:

	call	Get_SMI_Sources		; Get source(s) of external SMIs
	test	ebx, ebx		; If no SMIs pending, exit SMM
	jz	SMI_Resume


	;
	; Invoke the handler for each pending SMI source
	;
	mov	[SMI_Sources], ebx
RunHandlers:
	lea	di, Handler_Table - sizeof (SMI_ENTRY)
NextHandler:
	add	di, sizeof (SMI_ENTRY)	; Advance ptr to next handler entry

	mov	eax, (SMI_ENTRY PTR [di]).SMI_Mask
	and	eax, [SMI_Sources]	; Sources = TopLevelSources & Handler_Table.SMI_Mask
	jz	NextHandler

	push	eax			; call Handler_Table.Handler(Sources)
	call	(SMI_ENTRY PTR [di]).Handler
	pop	eax

	not	eax			; TopLevelSources &= ~Sources;
	and	[SMI_Sources], eax
	jnz	NextHandler		; if (!TopLevelSources) break;




	;
	; Dispatch to the VSM on top of the scheduler's stack
	;
Dispatcher:
	mov	si, [SchedulerStack]	; Get scheduler's ptr
	mov	ebx, [si]		; Pop next VSM
	sub	si, 4

	test	ebx, 0FFFF0000h		; Is it a VSM ?
	jnz	RunTask
	test	bx, bx			; Callback routine ?
	je	Main_SMI_Loop
	mov	[SchedulerStack], si	; Yes, update scheduler ptr
	call	bx			; Go to callback routine
	jmp	Dispatcher
       
       
RunTask:
	mov	[SchedulerStack], si	; Save scheduler's ptr
	mov	[Current_VSM], ebx	; Save ptr to the current VSM
ExecuteTask:	
	;
	; Point SMHR to the VSM's SMM header.
	;
	lea	eax, (VSM_Header PTR [ebx+sizeof(SmiHeader)]).SysStuff.State
	mov	ecx, MSR_SMM_HDR
	wrmsr
	;
	; Restore the VSM's state
	;
	rsdc	ds, fs:(VSM_Header PTR [ebx])._DS
	
	xor	edi, edi
	ASSUME	di:PTR VSM_Header
	rsdc	gs, [di]._DS
	rsdc	ss, [di]._SS		; Restore VSM's SS:SP
	lea	bx, [di].SysStuff
	ASSUME	bx:PTR System
	mov	sp, word ptr [bx].SavedESP


	;
	; Update statistics
	;
	rdtsc				; Record start time of the VSM
	mov	[bx+0].StartTime, eax
	add	[bx+0].NumSMIs, 1	; Increment SMI count
	adc	[bx+4].NumSMIs, edi

	;
	; Mark VSM active unless it is sleeping
	;
	mov	al, RUN_FLAG_ACTIVE
SetRunFlag:
	xchg	[bx].RunFlag, al
	cmp	al, RUN_FLAG_SLEEPING
	je	SetRunFlag

	RESTORE_STATE			; No, restore registers
	
	rsm				; Resume to the VSM


	ASSUME	DI: NOTHING
	ASSUME	BX: NOTHING






;***********************************************************************
;      Restore state & resume to non-SMM code                          *
;***********************************************************************
	align	16
SMI_Resume:


	; Generate internal IRQ(s)
	xor	ecx, ecx
	xchg	ecx, [IRQ_Mask]
	jecxz	NoIRQ
	call	Generate_IRQ
NoIRQ:


	call	VSA_Exit		; Perform VSA exit
	jc	Main_SMI_Loop
			 





	; Increment count of SMIs
	mov     si, OFFSET VSM_Header.SysStuff
	ASSUME	SI: PTR System
	xor	ecx, ecx
	add	[si+0].NumSMIs, 1
	adc	[si+4].NumSMIs, ecx

	; Compute total clocks for this SMI
	rdtsc
	sub	eax, [si+0].StartTime
	; Accumulate total clocks spent executing VSA code
	add	[si+0].Clocks, eax
	adc	[si+4].Clocks, ecx

	ASSUME  SI:NOTHING




if BRACKETS
	cmp	[BracketFlag], 0
	je	@f
	mov	dx, DBG_PORT
	mov	al, ']'
	out	dx, al
@@:
endif



;***********************************************************************
;         Restore the state of the interrupted task                    *
;***********************************************************************
	RESTORE_STATE			; Restore GP registers & PCI addr

	mov	esp, [Saved_ESP]	; Restore ESP
	rsdc	ss,  [Saved_SS]		; Restore descriptors
	rsdc	es,  [Saved_ES]
	rsdc	fs,  [Saved_FS]
	rsdc	gs,  [Saved_GS]
	rsdc	ds,  [Saved_DS]		; Must be restored last
	rsm				; Resume to non-SMM thread






;***********************************************************************
; An SMI has occurred that is not a system call:                       *
;                                                                      *
; If synchronous SMI:                                                  *
;    - Save the state of the interrupted VSM                           *
;    - Reschedule the interrupted VSM                                  *
;    - Execute SMI handlers                                            *
; If asynchronous SMI:                                                 *
;    - If event is not high priority or VSM is marked no-preempt,      *
;      return to interrupted VSM immediately.                          *
;    - Otherwise, execute SMI handlers                                 *
;***********************************************************************
	align	16
NotSysCall:

	;
	; Save VSM's state and set up SysMgr stack
	;
	SAVE_STATE			; Save the VSM's state
	mov     word ptr gs:(VSM_Header).SysStuff.SavedESP, sp

	rsdc    ss, [Stack_Descriptor]	; Setup System Manager's stack
	lea	sp, [StartSaveArea]


	call	Get_SMI_Sources		; Get source(s) of nested SMIs

	mov	[SMI_Sources], ebx	; Record the pending SMI sources
	test	ebx, [SynchEvents]	; Is nested SMI a syncronous event ?
	jz	short AsyncSMI

	;
	; The SMI is synchronous (I/O or virtualized PCI trap)
	;
	and	ebx, [SynchEvents]	; Record the nested SMI source
	or	[Nested_Flag], ebx

	; It's a trapped or virtualized PCI.
	; Record some info about the event.
	svdc	[Nested_ES], gs		; Save GS descriptor
	mov	esi, gs:(VSM_Header).SysStuff.SavedESP
	mov	eax, gs:[si+0]		; Get VSM's PCI address
	mov	edi, gs:[si+4]		; Get VSM's EDI
	mov	[Nested_PCI], eax	; Required by PCI_Handler
	mov	[Nested_EDI], edi	; Required if INS
	add	esi, VSM_STACK_FRAME - 4
	add	esi, [Current_VSM]	; Ptr to EAX on VSM's stack
	mov	[Nested_EAX], esi

ServiceNow:

	mov	gs:(VSM_Header).SysStuff.RunFlag, RUN_FLAG_READY
Reschedule:	
	add	[SchedulerStack], 4	; Re-schedule the interrupted VSM

	mov	ebp, [SMI_Sources]	; Needed by RunHandlers

	; Update the clock count used by the VSM
	rdtsc
	sub	eax, gs:(VSM_Header).SysStuff.StartTime
	xor	edx, edx
	add	gs:(VSM_Header).SysStuff.Clocks+0, eax
	adc	gs:(VSM_Header).SysStuff.Clocks+4, edx

	jmp	RunHandlers


	;
	; The SMI is asynchronous
	;
AsyncSMI:
	or	ebx, ebx		; If null SMI, just return to VSM
	jz	short NoPreempt

	cmp	gs:(VSM_Header).SysStuff.RunFlag, RUN_FLAG_SLEEPING
	je	Reschedule

	jmp	ServiceNow	

	; Is this a high priority SMI ?
;	test	ebx, [HiPrioritySMIs]
;	jnz	short ServiceNow	; Yes, execute SMI handlers


	;
	; Resumes to a VSM from an low-priority asynchronous SMI
	;
NoPreempt:
	;
	; Resumes to a VSM from a system call
	;
ExitSysCall:				; Return to caller
	rsdc	ss, gs:(VSM_Header)._SS
	mov     sp, word ptr gs:(VSM_Header).SysStuff.SavedESP
	rsdc    ds, gs:(VSM_Header)._DS
	RESTORE_STATE
	rsm







;***********************************************************************
; The current VSM has emptied its message queue.  It will not be       *
; executed again until a new message is entered into its queue.        *
; NOTE: SysMgr will be using the last VSM's stack.                     *
;***********************************************************************
EmptyMsgQueue:


	mov	gs:(VSM_Header).SysStuff.RunFlag, RUN_FLAG_INACTIVE
	jmp	short UpdateClocks

Sys_Exit:
	; System calls should not count as an SMI
	sub	gs:(VSM_Header).SysStuff.NumSMIs+0, 1	; Decrement number of SMIs
	sbb	gs:(VSM_Header).SysStuff.NumSMIs+4, 0
UpdateClocks:


	; Update the clock count used by the VSM
	rdtsc
	sub	eax, gs:(VSM_Header).SysStuff.StartTime
	xor	edx, edx
	add	gs:(VSM_Header).SysStuff.Clocks+0, eax
	adc	gs:(VSM_Header).SysStuff.Clocks+4, edx

	jmp	Dispatcher



;***********************************************************************
; Schedules a VSM to execute
;***********************************************************************
Schedule_VSM proc pascal uses bx \
	Vsm: DWORD

	mov     eax, [Vsm]
	test	eax, 0FFFF0000h		; Is it a callback ?
	jz	Schedule

	; Mark VSM ready, if not already on scheduler's stack	
	mov	bl, RUN_FLAG_READY	
	xchg	bl, fs:(VSM_Header PTR [eax]).SysStuff.RunFlag
;	cmp	bl, RUN_FLAG_READY
;	je      short Exit

Schedule:
	mov     bx, [SchedulerStack]	; Get scheduler's ptr
	add     bx, 4		    	; Push the VSM
	mov     [bx], eax
	mov     [SchedulerStack], bx	; Update scheduler's ptr 
Exit:	ret

Schedule_VSM endp






;***********************************************************************
;***********************************************************************
;***********************************************************************
;*                                                                     *
;*                    Nested SMI Entry Point                           *
;*                                                                     *
;*  A nested SMI can occur for one of four reasons:                    *
;*  1) A system call                                                   *
;*  2) An asynchronous event (timer, GPIO, etc.)                       *
;*  3) A trapped I/O from a VSM (virtual register, PCI access)         *
;*  4) Return from a BIOS callback                                     *
;*                                                                     *
;***********************************************************************
;***********************************************************************
;***********************************************************************
	align	16
Nested_SMI:

	; Set DS descriptor to System Manager's
	rsdc	ds, cs:[Data_Descriptor]


	; Is it a system call (SMINT) ?
	test	byte ptr gs:(VSM_Header).SysStuff.State.SMI_Flags, 1000b
	jz	NotSysCall

;*************************************************************************
;*************************************************************************
;*************************************************************************
;                                                                        *
;       SMI is a system call                                             *
;                                                                        *
; The VSM's registers are saved on the VSM's stack.                      *
; DS: and SS:SP are initialized to SysMgr's environment.                 *
;                                                                        *
; On Entry:                                                              *
;   AX = System call code                                                *
;   Other registers, depending on the system call                        *
;                                                                        *
;*************************************************************************
;*************************************************************************
;*************************************************************************
SystemCall:
	cmp	ax, SYS_CODE_EXIT	; Check for valid system code
	ja 	short IllegalSysCall	; If invalid, record an error


	mov	[VSMs_EAX], eax
	SAVE_STATE			; Save the VSM's state
	mov     word ptr gs:(VSM_Header).SysStuff.SavedESP, sp

	; Setup System Manager's stack
	rsdc    ss, [Stack_Descriptor]
	lea	sp, [StartSaveArea]

	; Dispatch to system call routine
	movzx	eax, word ptr [VSMs_EAX]
	jmp	[SysCall_Table + eax*2]


	

;***********************************************************************
; An illegal system call was detected
;***********************************************************************
IllegalSysCall:
	; Is it ResumeFromRAM ?
	cmp	ax, SYS_RESUME_FROM_RAM
	je	ResumeFromRAM

	mov	di, ERR_UNDEF_SYS_CALL
	mov	ebx, eax		; Info1 = system call code
	mov	ecx, [Current_VSM]	; Info2 = offending VSM
	mov	ax, SYS_CODE_ERROR	; Fake a SYS_CODE_ERROR
	jmp	SystemCall



;***********************************************************************
	; BIOS is resuming after a Save-to-RAM:
	; 1) Re-init VSA state
	; 2) Re-schedule the suspended VSM
	; 3) Go to VSM dispatcher
;***********************************************************************
ResumeFromRAM:
	rsdc    fs, [Flat_Descriptor]	; Set FS to a 4 GB flat segment
	rsdc    es, [Flat_Descriptor]	; Set ES to a 4 GB flat segment

	mov     ebx, [Current_VSM]
	mov	ax, fs:(VSM_Header PTR [ebx]).SysStuff.ResumeVector
	mov	fs:(VSM_Header PTR [ebx]).SysStuff.State.Next_IP, ax
	mov     fs:(VSM_Header PTR [ebx]).SysStuff.RunFlag, RUN_FLAG_READY	
	jmp     ExecuteTask









		align	4
Current_VSM	dd	0
HardwareInfo	Hardware { }
BracketFlag	db	0





;*************************************************************************************
;
; IMPLEMENTATION NOTES:
;
;  - The scheduler's stack and System Manager's stack grow toward each other.
;
;  - The variables at StartSaveArea are grouped together because they, along with
;    the SMM header, represent the entire state of the interrupted task.  This is
;    important since this state must be saved & restored across a BIOS callback.
;
;*************************************************************************************
		align	2
SchedulerStack dw	OFFSET Scheduled_VSMs

		align	4
Scheduled_VSMs	dd	0			; Marks bottom of scheduler's stack



		ORG	SYSMGRS_STACK
StartSaveArea:
Saved_PCI	dd	?

; NOTE: the following 8 variables must be in the correct order for PUSHAD/POPAD
Saved_EDI	dd	?
Saved_ESI	dd	?
Saved_EBP	dd	?
		dd	?			; ESP (not used)
Saved_EBX	dd	?
Saved_EDX	dd	?
Saved_ECX	dd	?
Saved_AX:
Saved_EAX	dd	?
SysMgrStack:					; <==== System Manager's stack begins here

Saved_ESP	dd	OFFSET SysMgrStack	; DO NOT MOVE !!!


;
; NOTE: The SET_REGISTER, GET_REGISTER, GET_DESCRIPTOR, & SET_DESCRIPTOR
;       macros assume these are in this exact location and order:
Saved_SS	Descriptor { }
Saved_DS	Descriptor { }
Saved_ES	Descriptor { }
Saved_FS	Descriptor { }
Saved_GS	Descriptor { }

Header_Addr	dd	0			; Ptr to end of SMM header

IDT_Selector	dd	0			; Saved IDT state
IDT_Base	dd	0
IDT_Limit	dd	0		



		dw	0			; Pad
EndSaveArea	label	byte
;
; This table contains System Manager structures accesses by INIT.EXE and INFO.EXE.
; Must match the InfoStuff structure in VSA2.H
;
		ORG	SPECIAL_LOC

		dw	OFFSET Events
		dw	OFFSET MSRs
		dw	OFFSET INT_Vectors
		dw	OFFSET HardwareInfo
IRQ_Base	dd	0			; Memory-mapped location of Internal IRQs
IRQ_Mask	dd	0			; Mask of IRQs to be generated
; NOTE: The following fields are not referenced by VSMs.
;       They are in this structure so INIT.ASM can initialize them.
SysMgr_VSM	dd	0			; Logical address of System Manager
SMI_Base	dd	0			; Memory-mapped location of SMI sources
		dw	OFFSET Header_Addr	; Offset of SysMgr.SysStuff.State
		dw	OFFSET StartSaveArea	; Initial value of SysMgr.SysStuff.SavedESP
		dw	OFFSET MSRs
		dw	OFFSET NumDescriptors

Nested_EAX	dd	0			; Flat ptr to interrupted VSM's EAX
Nested_EDI	dd	0			; Value of interrupted VSM's EDI
Nested_PCI	dd	0			; PCI config address of interrupted VSM
Nested_Flag	dd	0

VSMs_EAX	dd	0
Nested_ES	Descriptor {}


_CODE	ENDS
 		
	END	Start

