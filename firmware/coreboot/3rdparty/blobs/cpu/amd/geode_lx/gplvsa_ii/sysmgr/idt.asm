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
; Routines to:
; -set up an SMM-based IDT
; -save/restore of FPU state only as needed
;********************************************************************************



include sysmgr.inc
include vsa2.inc
include gx2.inc

.model tiny,c
.586p
.CODE


CR	equ	0Dh
LF	equ	0Ah


public    Saved_INTs
public    Trap_Common
externdef Trap_Code: 	dword
externdef SysMgr_VSM:	dword
externdef Current_VSM:	dword
externdef IDT_Base:	dword
externdef IDT_Limit:	dword
externdef IDT_Selector:dword
externdef pascal Parse_Capabilities: proc
externdef pascal Read_MSR_HI:        proc

FPU_Owner	dd	0
FPU_State	db	108 dup (0)
CR0_PE		equ	1
CR0_EM		equ	4


;***********************************************************************
; Saves the non-SMM IDT and installs VSA's exception vectors
;***********************************************************************
Install_IDT	proc

	; Save IDT of interrupted thread
	mov	ecx, 1329h
	rdmsr
	mov	[IDT_Selector], eax
	mov	eax, IDT_SIZE-1
	wrmsr
	
	mov	cl, 39h
	rdmsr
	mov	[IDT_Base], eax
	mov	[IDT_Limit], edx
	mov	eax, [SysMgr_VSM]
	add	eax, OFFSET Saved_INTs
	mov	dx, IDT_SIZE
	wrmsr
	
	
	nop
	mov	byte ptr [$-1], 0C3h	; Patch a RET at the NOP above

	mov	eax, [SysMgr_VSM]	; Initialize FPU owner
	mov	[FPU_Owner], eax

; The following code is only necessary if SysMgr starts on a non-MB boundary.
;    HandlerBase = {SMM_base[31:20], CS_selector[15:0], 4'b0}
;	lea	di, [Saved_INTs+2]	; Patch SysMgr's CS into vector table
;	mov	cx, IDT_SIZE/4
;	shr	eax, 4			; Compute SysMgr's CS
;InsertCS:
;	mov	[di], ax		; Store SysMgr's CS into vector table
;	add	di, 4
;	loop	InsertCS

	ret
	

Install_IDT endp



;***********************************************************************
; Restores the IDT ptr and FPU state (if modified)
;***********************************************************************
Restore_IDT	proc

	; Restore the IDT of interrupted thread
	mov	ecx, 1329h
	mov	eax, [IDT_Selector]
	wrmsr
	mov	cl, 39h
	mov	eax, [IDT_Base]
	mov	edx, [IDT_Limit]
	wrmsr
	

	; The RET will be patched with a NOP if FPU usage occurs
RestoreRET::	
	ret

	; Restore the RET at RestoreRET
	mov	byte ptr [RestoreRET], 0C3h

	; Get last VSM to use FPU
	mov	esi, [SysMgr_VSM]
	xchg	[FPU_Owner], esi

	; Set VSM's CR0.EM
	or	fs:(VSM_Header PTR [esi]).SysStuff.State.r_CR0, CR0_EM

	; Save VSM's FPU state
	; NOTE: This can be done in real-mode
	fnsave	fs:(VSM_Header PTR [esi]).SysStuff.FPU_State
		
	; Restore non-SMM FPU state
	; NOTE: This must be done in 32-bit protected mode
	mov	eax, CR0		; Enter protected mode for FPU save
	or	al, CR0_PE
	mov	CR0, eax
	jmp	$+2

	db	66h
	frstor	byte ptr cs:[FPU_State]

	and	al, NOT CR0_PE		; Return to real mode
	mov	CR0, eax

	ret

Restore_IDT endp











;***********************************************************************
; Handler for Trap 7 (Device Not Available)
;***********************************************************************
Trap7	proc

	push	eax

	; Clear interrupted VSM's CR0.EM (so it will own the FPU) 
	mov	eax, CR0
	and	al, NOT CR0_EM
	mov	CR0, eax


	; Get current owner of FPU
	mov	eax, cs:[FPU_Owner]

	; Is it non-SMM thread ?
	cmp	eax, cs:[SysMgr_VSM]
	jne	Set_EM
	

	; Yes, enable FPU restore code in Restore_IDT
	mov	byte ptr cs:[RestoreRET], 90h

	; Save non-SMM FPU state
	; NOTE: This must be done in 32-bit protected mode
	mov	eax, CR0		; Enter protected mode for FPU save
	or	al, CR0_PE
	mov	CR0, eax
	jmp	$+2
	
	db	66h
	fnsave	byte ptr cs:[FPU_State]

	and	al, NOT CR0_PE		; Return to real mode
	mov	CR0, eax

	jmp	short Record_FPU_Owner
	
	
	 

Set_EM:
	; Set previous owner's CR0.EM
	or	fs:(VSM_Header PTR [eax]).SysStuff.State.r_CR0, CR0_EM

	; Save the FPU state of the previous owner
	; NOTE: This can be done in 16-bit real-mode
	fnsave	byte ptr fs:(VSM_Header PTR [eax]).SysStuff.FPU_State
	
	; Set FPU flag
	mov	fs:(VSM_Header PTR [eax]).SysStuff.FPU_Flag, 1


Record_FPU_Owner:
	; Record the new owner of the FPU
	mov	eax, cs:[Current_VSM]
	mov	cs:[FPU_Owner], eax

	; Has this VSM used the FPU previously ?
	cmp	fs:(VSM_Header PTR [eax]).SysStuff.FPU_Flag, 0
	je	short Exit
	
	; Yes, restore its FPU state
	; NOTE: This can be done in 16-bit real-mode
	lea	eax, (VSM_Header PTR [eax]).SysStuff.FPU_State
	frstor	byte ptr fs:[eax]

Exit:  	pop	eax
	iret			; Return to the interrupted VSM

Trap7	endp	
	




;***********************************************************************
; Exception vectors (segments will be patched)
;***********************************************************************
Saved_INTs:
	dd	OFFSET Trap_Code + 8*0
	dd	OFFSET Trap_Code + 8*1
	dd	OFFSET Trap_Code + 8*2
	dd	OFFSET Trap_Code + 8*3
	dd	OFFSET Trap_Code + 8*4
	dd	OFFSET Trap_Code + 8*5
	dd	OFFSET Trap_Code + 8*6
	dd	OFFSET Trap_Code + 8*7
	dd	OFFSET Trap_Code + 8*8
	dd	OFFSET Trap_Code + 8*9
	dd	OFFSET Trap_Code + 8*0Ah
	dd	OFFSET Trap_Code + 8*0Bh
	dd	OFFSET Trap_Code + 8*0Ch
	dd	OFFSET Trap_Code + 8*0Dh
	dd	OFFSET Trap_Code + 8*0Eh
	dd	OFFSET Trap_Code + 8*0Fh
IDT_SIZE	equ	($-Saved_INTs)








;***********************************************************************
; Macros for performing stackless CALLs & RETs
;***********************************************************************
ROM_CALL macro Subr
	local RetAddr
	mov	dx, RetAddr		; Put return addr in BP
	jmp	Subr
RetAddr:
	endm
       


ROM_RET	macro
	jmp	dx			; Return to caller
	endm



;***********************************************************************
; BX = Exception #
;***********************************************************************
Trap_Common:
	mov	cs:[TrapNum], bx
	mov	cs:[Reg_ECX], ecx

	
	; Disable SMIs
	mov	ecx, MSR_SMM_CTRL
	xor	eax, eax
	wrmsr	

	ROM_CALL NewLine

	cmp	bl, LastTrap
	jbe	@f
	mov	bl, LastTrap
@@:
	add	bx, bx
	mov	bx, cs:[bx+TrapMsgTbl]
	ROM_CALL String

	lea	bx, [Msg_in]
	ROM_CALL String

	;
	; Display the VSM which generated the exception
	;
	pop	ecx			; Get SEG:OFFSET of faulting code
	mov	cs:[SegOff], ecx
	mov	esi, cs:[Current_VSM]
	mov	ah, fs:(VSM_Header PTR [esi]).VSM_Type
	lea	si, [VSM_Table]
	ASSUME	SI: PTR TableItem
	test	ecx, 0FFFF0000h		; Is it System Manager ?
	jnz	ScanVSM
	mov	eax, cs:[SysMgr_VSM]
	mov	cs:[Current_VSM], eax
	jmp	ShowVSM
ScanVSM:
	mov	al, cs:[si].Vsm		; Get VSM Type from table
	cmp	ah, al
	je	ShowVSM
	cmp	al, VSM_ANY		; End of table ?
	je	ShowVSM
	add	si, sizeof(TableItem)	; Advance to next table entry
	jmp	ScanVSM
	
ShowVSM:
	mov	bx, cs:[si].MsgStr
	ROM_CALL String

	lea	bx, Msg_VSM		; VSM
	ROM_CALL String

	ROM_CALL NewLine
	lea	bx, Msg_IP		; IP = xxxx
	ROM_CALL String

	cmp	cs:[TrapNum], 000Dh	; Is it Trap 0Dh ?
	jne	ShowIP
	mov	ebx, cs:[Current_VSM]	; Yes, check for bad MSR address
	test	ecx, 0FFFF0000h		; Did SysMgr cause it ?
	jnz	ComputeAddr
	mov	ebx, cs:[SysMgr_VSM]	; Yes
ComputeAddr:
	movzx	esi, cx
	add	esi, ebx
	mov	eax, dword ptr fs:[esi]
	cmp	al, 0Fh
	jne	short ShowIP
	cmp	ah, 30h			; WRMSR	?
	je	short BadMSR
	cmp	ah, 32h			; RDMSR ?
	jne	ShowIP
BadMSR:
	mov	cs:[MSR_Access], ah
	mov	si, sp			; Show caller
	mov	cx, ss:[si+4]
	sub	cx, 3

	; If one of the MSR routines, show caller
	cmp	si, OFFSET Read_MSR_HI
	jb	short ShowIP
	cmp	si, OFFSET Parse_Capabilities
	jae	short ShowIP
	mov	cx, ss:[si+2]
ShowIP:


	mov	ax, cx
	ROM_CALL Hex16

	mov	al, '/'
	out	dx, al
	movzx	eax, cx
	add	eax, cs:[Current_VSM]
	ROM_CALL Hex32
		

	lea	bx, Msg_SP		; SP = xxxx
	ROM_CALL String
	mov	ax, sp
	ROM_CALL Hex16

	lea	bx, Msg_BP		; BP = xxxx
	ROM_CALL String
	mov	ax, bp
	ROM_CALL Hex16

	cmp	cs:[MSR_Access], 0	; Was it an MSR access ?
	je	Beep
	ROM_CALL NewLine		; Yes
	lea	bx, [Msg_MSR]
	ROM_CALL String
	lea	bx, [Msg_MSR_Wr]
	cmp	cs:[MSR_Access], 30h
	je	short ShowMSR
	lea	bx, [Msg_MSR_Rd]
ShowMSR:
	ROM_CALL String
	mov	eax, cs:[Reg_ECX]	; Display invalid MSR address
	ROM_CALL Hex32


HI_TONE	equ	1193180/3000	; 3 KHz
LO_TONE	equ	1193180/750	; 750 Hz
INTERVAL	equ	100000/15	; .10 second
BEEP_LOOPS	equ	10

Beep:
	mov	bx, BEEP_LOOPS
 	in	al, 61h
	or	al, 3		; connect speaker to timer 2
	out	61h, al
BeepLoop:
	mov	dx, HI_TONE	; Start frequency
TwoTone:
	mov	al, 0B6h	; timer 2 mode set
	out	43h, al		; set mode
	mov	ax, dx
	out	42h, al		; set LSB of counter
	mov	al, ah
	out	42h, al		; set MSB of counter

	mov	cx, INTERVAL	; # 15 usec intervals
Interval:
	in	al, 61h		; Wait for 15 usec
	xor	al, ah
	test	al, 10h
	jz	Interval
	xor	ah, 10h
	loop	Interval

	shl	dx, 1		; Divide frequency by 2
	cmp	dx, LO_TONE	; End frequency
	jb	TwoTone
	dec	bx		; Decrement loop counter
	jnz	BeepLoop	; Start tones over

 	in	al, 61h		; Turn off speaker
	and	al, NOT 3
	out	61h, al

Halt:	hlt
	jmp	Halt
	
	
	
;***********************************************************************
; VSM strings for display of VSM causing the exception
;***********************************************************************
TableItem struc
  Vsm	   db	?
  MsgStr   dw	?
TableItem ends


VSM_Table:
	TableItem {VSM_SYS_MGR,	Sys_Mgr_VSM}
	TableItem {VSM_AUDIO,	XpressAudio}
	TableItem {VSM_VGA,	SoftVGA}
	TableItem {VSM_LEGACY,	Legacy_VSM}
	TableItem {VSM_PM, 	PM_VSM}
	TableItem {VSM_OHCI,	OHCI_VSM}
	TableItem {VSM_i8042,	i8042_VSM}
	TableItem {VSM_ACPI,	ACPI_VSM}
	TableItem {VSM_APM,	APM_VSM}
	TableItem {VSM_SMB,	SMB_VSM}
	TableItem {VSM_BATTERY,	Battery_VSM}
	TableItem {VSM_RTC,	RTC_VSM}
	TableItem {VSM_S2D,	S2D_VSM}
	TableItem {VSM_SPY,	Spy_VSM}
	TableItem {VSM_NETWORK,	Network_VSM}
	TableItem {VSM_GPIO,	GPIO_VSM}
	TableItem {VSM_USB,	USB_VSM}
	TableItem {VSM_FLASH,	Flash_VSM}
	TableItem {VSM_INFRARED,Infrared_VSM}
	TableItem {VSM_THERMAL,	Thermal_VSM}
	TableItem {VSM_NULL, 	Null_VSM}
	TableItem {VSM_VIP,	VIP_VSM}
	TableItem {VSM_LPC,	LPC_VSM}
	TableItem {VSM_VUART,	VUART_VSM}
	TableItem {VSM_MICRO,	Micro_VSM}
	TableItem {VSM_USER1,	User1_VSM}
	TableItem {VSM_USER2,	User2_VSM}
	TableItem {VSM_USER3,	User3_VSM}
	TableItem {VSM_SUPERIO,	SuperIO_VSM}
	TableItem {VSM_ANY,	Unknown_VSM}		; Catch-all


Msg	macro	TrapString
	db	TrapString
	db	0
	endm

XpressAudio:	Msg "Audio"
SoftVGA:	Msg "SoftVG"
Legacy_VSM:	Msg "Legacy"
USB_VSM:	Msg "USB"
GPIO_VSM:	Msg "GPIO"
ACPI_VSM:	Msg "ACPI"
Sample_VSM:	Msg "Sample"
APM_VSM:	Msg "APM"
Battery_VSM:	Msg "Battery"
PM_VSM:	Msg "PM"
S2D_VSM:	Msg "SaveToRAM"
Sys_Mgr_VSM:	Msg "SysMgr"
i8042_VSM:	Msg "i8042"
OHCI_VSM:	Msg "OHCI"
SuperIO_VSM:	Msg "SuperIO"
Null_VSM:	Msg "Null"
Spy_VSM:	Msg "Spy"
RTC_VSM:	Msg "RTC"
SPY_VSM:	Msg "Spy"
Network_VSM:	Msg "Network"
Infrared_VSM:	Msg "Infrared"
Thermal_VSM:	Msg "Thermal"
VIP_VSM:	Msg "VIP"
LPC_VSM:	Msg "LPC"
User1_VSM:	Msg "User1"
User2_VSM:	Msg "User2"
User3_VSM:	Msg "User3"
SMB_VSM:	Msg "SMB"
Flash_VSM:	Msg "Flash"
VUART_VSM:	Msg "VUART"
Micro_VSM:	Msg "Micro"
Unknown_VSM:	Msg "VSM???"


;***********************************************************************
; Strings describing the type of exception
;***********************************************************************
TrapMsgTbl:
	dw	OFFSET Trap00
	dw	OFFSET Trap01
	dw	OFFSET Trap02
	dw	OFFSET Trap03
	dw	OFFSET Trap04
	dw	OFFSET Trap05
	dw	OFFSET Trap06
	dw	OFFSET Trap07
	dw	OFFSET Trap08
	dw	OFFSET Trap09
	dw	OFFSET Trap0A
	dw	OFFSET Trap0B
	dw	OFFSET Trap0C
	dw	OFFSET Trap0D
	dw	OFFSET Trap0E
	dw	OFFSET Trap0F
	dw	OFFSET Trap10
	dw	OFFSET Trap11
	dw	OFFSET Trapnn
LastTrap equ ($-TrapMsgTbl)/2



Trap00:	Msg	"Divide by Zero"
Trap01:	Msg	"Debug"
Trap02:	Msg	"NMI/INT 02h"
Trap03:	Msg	"Breakpoint"
Trap04:	Msg	"Overflow"
Trap05:	Msg	"Bounds Check"
Trap06:	Msg	"Invalid Opcode"
Trap07:	Msg	"Device Not Available"
Trap08:	Msg	"Double Fault"
Trap09:	Msg	"Invalid TSS"
Trap0A:	Msg	"INT 0Ah"
Trap0B:	Msg	"Segment Not Present"
Trap0C:	Msg	"Stack Fault"
Trap0D:	Msg	"General Protection"
Trap0E:	Msg	"Page Fault"
Trap0F:	Msg	"INT 0Fh"
Trap10:	Msg	"INT 10h"
Trap11:	Msg	"Aligment Check"
Trapnn:	Msg	"Trap ??"



;***********************************************************************
; Miscellaneous strings
;***********************************************************************
Msg_in:        Msg " in "
Msg_VSM:       Msg " VSM"
Msg_SP:        Msg "  SP="
Msg_BP:        Msg "  BP="
Msg_IP:        Msg "IP="
Msg_MSR:       Msg "Invalid MSR "
Msg_MSR_Rd:    Msg "read: "
Msg_MSR_Wr:    Msg "write: "

;***********************************************************************
; Stackless display routines
;***********************************************************************
NewLine:
	mov	al, CR
	out	DBG_PORT, al
	ROM_RET

String:
	mov	al, cs:[bx]
	or	al, al
	jz	StringExit
	out	DBG_PORT, al
	in	al, 80h
	inc	bx			; Advance ptr
	jmp	String
StringExit:
	ROM_RET


Seg_Addr:
	mov	si, bp
	rol	eax, 16
	xchg	ah, al
	ROM_CALL Hex8
	xchg	ah, al
	ROM_CALL Hex8
	mov	al, ':'
	out	DBG_PORT, al

	rol	eax, 16
	xchg	ah, al
	ROM_CALL Hex8
	xchg	ah, al
	ROM_CALL Hex8

	mov	bp, si			; Restore ret addr
	ROM_RET


Hex32:	rol	edx, 16			; Save return address in upper EDX

	rol	eax, 8
	ROM_CALL Hex8
	rol	eax, 8
	ROM_CALL Hex8
	rol	eax, 8
	ROM_CALL Hex8
	rol	eax, 8
	ROM_CALL Hex8
	mov	al, ' '
	out	DBG_PORT, al
	rol	edx, 16			; Restore return address
	ROM_RET



Hex16:	rol	edx, 16			; Save return address in upper EDX
	xchg	ah, al
	ROM_CALL Hex8
	xchg	ah, al
	ROM_CALL Hex8
	rol	edx, 16			; Restore return address
	mov	al, ' '
	out	DBG_PORT, al
	ROM_RET


Hex8:
	; Display 4 MSBs
	mov	di, ax
	rol	al, 4
	and	al, 0Fh
	add	al, '0'			; Convert to ASCII
	cmp	al, '9'
	jbe	@f
	add	al, 7			; 'A'-'F'
@@:	out	DBG_PORT, al
	in	al, 80h
	mov	ax, di

	
	; Display 4 LSBs
	and	al, 0Fh
	add	al, '0'			; Convert to ASCII
	cmp	al, '9'
	jbe	Char
	add	al, 7			; 'A'-'F'
Char:	out	DBG_PORT, al
	in	al, 80h
	ROM_RET

SegOff   dd	0
Reg_ECX  dd	0
TrapNum  dw	0
MSR_Access	db	0


	end

