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
;*     Port 92h routines  
;*******************************************************************************


include SYSMGR.INC
include VSA2.INC
include GX2.INC
include CHIPSET.INC
include CS5536.INC
include ISA.INC
include PCI.INC
include MDD.INC
include HCE.INC
include SMIMAC.MAC

.model tiny,c
.586p
.CODE
  

externdef VSA_Exit:        proc
externdef MDD_Base:        dword
externdef FooGlue:         dword
externdef OHCI1_Smi:       dword
externdef OHCI2_Smi:       dword
externdef Nested_Flag:     dword
externdef Saved_EAX:       dword
externdef Saved_EBX:       dword
externdef Saved_ECX:       dword
externdef Saved_EDX:       dword
externdef Saved_ESI:       dword
externdef Saved_EDI:       dword
externdef Saved_EBP:       dword
externdef Saved_ESP:       dword
externdef IDT_Base:        dword
externdef IDT_Limit:       dword
externdef IDT_Selector:    dword
externdef SMI_Sources:     dword
externdef SMM_Header:      SmiHeader
externdef Saved_SS:        Descriptor
externdef Data_Descriptor: Descriptor
externdef HardwareInfo:    Hardware





;***********************************************************************
;
; A20_Sync - synchronize port 92h and virtual A20 pin
;
;***********************************************************************
A20_Sync proc uses si edi

	; Determine if it is a nested access to port 92h
	btr	word ptr [Nested_Flag], SMI_SRC_A20_INDEX
	jc	short Get_A20_Data       
	rsdc	gs, cs:[Data_Descriptor]
Get_A20_Data:
	ASSUME	SI: ptr SmiHeader
	mov     si, OFFSET VSM_Header.SysStuff.State
	mov	bl, byte ptr gs:[si].write_data ; Get I/O data
	mov	si, gs:[si].IO_addr	; Get I/O address
	ASSUME  SI:NOTHING
	
	; Set A20M in FooGlue appropriately
	mov	ecx, [FooGlue]
	add	cx, FG_A20M
	rdmsr
	and	al, NOT A20M		; A20M = 0
	
	and     bl, 02h                 ; Check A20M setting
	jnz     short Set_A20
	or      al, A20M	        ; Force A20 low
Set_A20:
 	wrmsr

	cmp	si, 0092h		; If port 92h, no update necessary
	je	short SyncHceControl

	cmp	si, KYBD_DATA		; If not 8042 data port, ignore
	jne	Return

	; Sync Port 92h to keyboard controller
	mov	ecx, [MDD_Base]		; Disable Port 92h trapping
	mov	cx, MBD_MSR_SMI
	rdmsr
	push	ax
	and	al, NOT A20_P_EN
	wrmsr
	
	mov	al, bl			; Update Port 92h A20 state
	out	92h, al 

	pop	ax			; Restore Port 92h trapping
	wrmsr

	; Synchronize HceControl[A20State] with port 92h
SyncHceControl:       
	mov     ecx, [OHCI1_Smi]	; Read OHCI1 h/w BAR0
	test	ecx, 0FFF00000h
	jnz	short Get_HC_Base
	mov     ecx, [OHCI2_Smi]	; Use OHCI2 if OHCI1 not used
	test	ecx, 0FFF00000h
	jz	Return
Get_HC_Base:
	mov     cx, 1000h + BAR0/4	; 5536 Embedded PCI MSR
	cmp	[HardwareInfo].Chipset_ID, DEVICE_ID_5536
	je	short ReadBAR
	mov	cx, USBMSROHCB		; 5536 OHCI MSR
ReadBAR:
	rdmsr

	mov	bh, 0			; Clear MSR restore flag
	or      eax, eax		; Is BAR initialized ?
	jnz	short SyncA20State
	mov	bh, 1			; Set MSR restore flag

	push	eax			; Save current OHCI MSR
	push	edx
	push	ecx

	; OHCI BAR is not initialized or OHCI header is hidden.
	; Temporarily map the OHCI register set just above VSA
	; This range has been declared as OS-reserved in INT 15h E820h
	mov	ecx, MSR_RCONF_SMM
	rdmsr
	lea	edi, [edx+1000h]
	pop	ecx
	push	ecx

	mov	edx, 00000000Eh
	mov	eax, edi
	wrmsr

	mov	ecx, [MDD_Base]
	mov	cx, MSR_LBAR_KEL1
	rdmsr
	push	eax			; Save current KEL LBAR
	push	edx
	push	ecx
	mov	eax, edi
	mov	edx, 0FFFFF001h
	wrmsr

	; Synchronize HceControl.A20State
SyncA20State:
	or      ax, OFFSET (HCOR Ptr ds:[0]).HceControl
	mov     dx, fs:[eax]
	cmp     dx, 0FFFFh		; Is Memory Space enabled ?
	je	short Exit
CLEAR_A20	equ	(A20_STATE OR IRQ1_ACTIVE OR IRQ12_ACTIVE)
	; SWAPSiF for SiBZ 3509/3571: KEL SMIs are level instead of edge-triggered
	; Manifests itself as PBZ 3878: PS/2 keyboard hangs after running PCIDIAG
	test	dx, (IRQ1_ACTIVE OR IRQ12_ACTIVE)
	jz	short NoIRQ
	or	[SMI_Sources], SMI_SRC_KEL	; Fake another KEL SMI
NoIRQ:
	and     dx, NOT CLEAR_A20	; Clear A20State
	test    bl, 02h
	jz      Update_HceControl
	or      dx, A20_STATE		; Set A20State
Update_HceControl:
	mov     fs:[eax], dx

Exit:
	or	bh, bh			; Do OHCI MSRs need to be restored?
	je	short Return

	pop	ecx			; Restore KEL LBAR
	pop	edx
	pop	eax
	wrmsr

	pop	ecx			; Restore OHCI MSR
	pop	edx
	pop	eax
	wrmsr
Return:
	ret

A20_Sync endp







;***********************************************************************
; Sets the top-level RSM state to the reset state:
;  1) Real-mode IDT
;  2) Real-mode descriptors; segment regisers = 0000
;  3) CS:EIP = F000:FFF0
;  4) CS base = FFFF0000
;  5) CS limit = 64K
;  6) DX = CPU ID
;***********************************************************************
set_reset_state proc   uses si

	; Check if user code is doing a 286-style shutdown
	in      al, CMOS_INDEX			; Preserve CMOS index
	mov     ah, al

	mov     al, 0Fh
	out     CMOS_INDEX, al
	in      al, CMOS_DATA

	xchg    al, ah
	out     CMOS_INDEX, al			; Restore CMOS index

	cmp     ah, 09H
	je      UserShutdown

	cmp     ah, 05H
	je      UserShutdown

	cmp     ah, 0AH
	je      UserShutdown

	cmp     ah, 0FFH			; JMP if no CMOS present
	je      UserShutdown





CACHE_LINE_SIZE	equ	32

	; If cache is disabled, there is no way to guarantee that
	; the entire code sequence from DisableCKE to Pre_Fetch
	; will be fetched before CKE is disabled.  It was decided
	; we'd rather see the original POST 0xBF hang rather than
	; a hang in SMM.
	mov	ebp, [MDD_Base]			; Prepare reset MSR
	mov	bp, MSR_SOFT_RESET
	mov	eax, CR0			; Is cache disabled ?
	test	eax, 40000000h
	jnz	Reset

	lea	si, [DisableCKE]		; Move code to start of a cache line
	mov	di, si
	and	di, NOT (CACHE_LINE_SIZE-1)
	mov	bx, di				; Remember where we moved it to
	cld
@@:	lodsd
	mov	[di], eax
	add	di, 4
	cmp	si, OFFSET Pre_Fetch
	jb	@b
	
	mov	ecx, 2000001Dh			; Memory controller
	rdmsr
	or	ah, 3				; Set CKE Mask = 11b

	jmp	Pre_Fetch



	; Bug #118.226 - Some DIMMs hang at POST 0xBF on a reboot
	;
	; Need to de-assert CKE off before command signals are tri-stated.
	; There can be no memory accesses after the following WRMSR.
	; If cache is disabled, there is no way to satisfy this condition,
	; so some DIMMs with cache disabled is not supported with
	; respect to resets.
	db	CACHE_LINE_SIZE dup (90h)
DisableCKE:
	wrmsr

	; Reset the CPU via MDD MSR_SOFT_RESET
Reset:	mov	ecx, ebp
	mov	al, 1
	wrmsr
	jmp	$
Pre_Fetch:
	; Prefetch 4 cache lines of NOPs so there won't be any
	; prefetching to memory after the MC MSR is written.
	db	4*CACHE_LINE_SIZE dup (90h)	; 4 cache lines
	jmp	bx

UserShutdown:

	mov	ecx, [FooGlue]			; Disable A20 mask
	add	cx, FG_A20M
	rdmsr
	and	al, NOT A20M			; A20M = 0
	wrmsr

	; Disable the cache & flush it
	mov     eax, CR0
	or      eax, 60000000h
	mov     CR0, eax
	wbinvd

	; Set real-mode IDT
	mov     [IDT_Limit], 0FFFFh		; Limit
	mov     [IDT_Base], 00000000h		; Base
	mov     [IDT_Selector], 00920000h	; Selector & Attributes


	; INIT is broken on LX 2.x and 3.0
	cmp     [HardwareInfo].CPU_ID, DEVICE_ID_LX
	jne     short Set_INIT
	mov     ax, [HardwareInfo].CPU_Revision
	cmp     al, 30h				; Is it 3.0?
	jae     short FakeInit
	and     al, 0F0h			; Is it 2.x?
	cmp     al, 20h
	je      short FakeInit

	call	VSA_Exit

	; Reset via INIT MSR
Set_INIT:
	mov     ecx, [FooGlue]			; Reset the CPU
	add	cx, FG_INIT
	xor	eax, eax
	wrmsr
	mov	al, INIT
	wrmsr

	; Fake an INIT by setting up initial CPU state
FakeInit:
	lea     si, [Saved_SS]			; Set descriptors to 64K real-mode
	mov     cx, 5                   	; # descriptors to fill
	mov     eax, 0000FFFFh			; limit_15_0 & base_15_0
DescrLoop:
	mov     [si+0], eax			; limit_15_0 & base_15_0
	mov     dword ptr [si+4], 00009200h	; base_23_16, attr, limit_19_16 & base_31_24
	mov     word ptr [si+8], 0000h		; selector
	add     si, sizeof (Descriptor)		; Advance ptr to next descriptor
	loop    DescrLoop


	; Set top-level SMM header to reset state
	mov     bx, OFFSET SMM_Header
	ASSUME	BX: PTR SmiHeader
	mov     [bx]._CS.limit,  eax
	mov     [bx]._CS.base,   0FFFF0000h
	mov     [bx]._CS.selector, 0F000h
	mov     [bx]._CS.attr,   92h

	mov     [bx].Next_EIP,   0000FFF0h
	mov     [bx].r_CR0,      60000010h	; Real-mode; cache disabled
	mov     [bx].r_DR7,      00000400h
	mov     [bx].EFLAGS,     00000002h
	mov     [bx].SMI_Flags,  8001h		; CS is R/W

	mov     [bx].SS_Flags,   DATA_ATTR

	; Initialize general purpose register
	mov     [Saved_EDX], 00000540h		; CPU ID
	xor     eax, eax
	mov     [Saved_EAX], eax
	mov     [Saved_EBX], eax
	mov     [Saved_ECX], eax
	mov     [Saved_EDI], eax
	mov     [Saved_ESI], eax
	mov     [Saved_EBP], eax
	mov     [Saved_ESP], eax

	ret

set_reset_state endp

	end
