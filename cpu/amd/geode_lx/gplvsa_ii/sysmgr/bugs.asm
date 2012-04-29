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
;*     This file contains hardware bug fixes. 

include VSA2.INC
include ISA.INC
include GX2.INC
include CHIPSET.INC

.model tiny,c
.586p
.CODE

externdef Nested_SMI:       proc
externdef SysMgr_Entry:     proc
externdef Restore_IDT:      proc
externdef Install_IDT:      proc
externdef Sample_SMI_Pin:   proc
externdef Clear_SMI_Pin:    proc
externdef Header_Addr:      dword
externdef HC_Status:        dword
externdef SMI_Sources:      dword
externdef HardwareInfo:     Hardware


;*******************************************************************************
; Remove the RTC fix if an RTC VSM is installed.
;*******************************************************************************
Remove_RTC_Fix proc

	mov	word ptr [Fix_RTC], 0C3F8h	; CLC  RET
	ret

Remove_RTC_Fix endp


; *******************************************************************************
; This routine is called upon entry to a non-nested SMI.
; *******************************************************************************
VSA_Entry proc

	; Patch a "jmp Nested_SMI" at Start
	mov	word ptr ds:[1], OFFSET Nested_SMI-3

	; Install exception handlers
	call	Install_IDT
	ret

VSA_Entry endp





; *******************************************************************************
; This routine is called upon exit from a non-nested SMI.
; It performs:
;   1) RTC fix
;   2) USB fix
;   3) IDT restore
;   4) Re-enable Suspend Modulation
;
; If CF=1 is returned, then loop back to SMI handlers.
; *******************************************************************************
VSA_Exit proc

	mov	eax, [Header_Addr]	; Restore the original SMM header
	mov	ecx, MSR_SMM_HDR
	wrmsr

	call	Fix_RTC
	jc	short Exit		; Don't exit from VSA

	; Restore "JMP SysMgr_Entry" at Start
	mov	word ptr ds:[1], OFFSET SysMgr_Entry-3


	call	Restore_IDT		; Restore IDT
	
	clc

Exit:	ret

VSA_Exit endp










; *******************************************************************************
; Handle the case where the RTC UIP bit was clear, application code was about to
; read the RTC, but an SMI occurs.  When VSA returns, the RTC is updating, so the
; application reads a bad value.
;
; The Solution:
;
;   The Time Stamp Counter is read upon entry to VSA.  On exit, if UIP is low or
; it is determined that the SMI has taken < 250 us, then just exit.  This leaves
; 44 us for the application to finish reading the RTC safely.   If the SMI has
; taken > 250 us, then spin for 500 us minus the time spent servicing the SMI.  
; This guarantees the RTC has finished updating.  Then set the SET bit, wait for
; UIP to go low, then clear the SET bit.
;
; *******************************************************************************
RTC_TIME	equ	250

Fix_RTC	proc

	in	al, CMOS_INDEX		; Get RTC index
	cmp	al, 9			; Only RTC indices 0-9 are UIP sensitive
	ja	Exit

	mov	bl, al			; Save RTC index
	push	si
	mov	al, CMOS_STATUS_A	; Read RTC Status A
	out	CMOS_INDEX, al
	in	al, CMOS_DATA
	test	al, UIP
	jz	Restore_RTC_Index


	mov	al, CMOS_STATUS_B	; Exit if StatusB[SET] = 1
	out	CMOS_INDEX, al
	in	al, CMOS_DATA
	test	al, SET
	jz	Restore_RTC_Index



	ASSUME	SI: PTR System		; Compute time servicing SMI in us
	mov	si, OFFSET VSM_Header.SysStuff
	rdtsc		      
	sub	eax, [si+0].StartTime
	sbb	edx, [si+4].StartTime
	movzx	ecx, [HardwareInfo].CPU_MHz
	cmp	edx, ecx		; If sitting in Dowser or Standby too long,
	jae	Restore_RTC_Index	; the divide could overflow.
	div	ecx

	cmp 	eax, RTC_TIME		; If < RTC_TIME us, exit
	jb	Restore_RTC_Index


	call	Clear_SMI_Pin
WaitForUpdate:
	call	Sample_SMI_Pin
	stc
	jnz	short SMI_Abort		; Yes, go process it


	rdtsc				; Wait for additional 300 us
	sub	eax, [si+0].StartTime
	sbb	edx, [si+4].StartTime
	div	ecx
	cmp	eax, RTC_TIME + 300
	jb	WaitForUpdate

	ASSUME	SI: NOTHING


	; StatusB[SET] = 1;
	mov	al, CMOS_STATUS_B	; Read RTC StatusB
	out	CMOS_INDEX, al
	in	al, CMOS_DATA
	test	al, SET			; If (SET == 1)
	jnz	short Restore_RTC_Index	;   bail
	or	al, SET			; else
	out	CMOS_DATA, al		;   SET = 1

	; Wait for StatusA[UIP] to go inactive
	mov	cx, 0FFFFh		; RTC timeout
SpinUIP:
	mov	al, CMOS_STATUS_A	; Yes, spin until UIP is clear
	out	CMOS_INDEX, al
	in	al, CMOS_DATA
	test	al, UIP
	loopnz	SpinUIP

	mov	al, CMOS_STATUS_B	; SET = 0
	out	CMOS_INDEX, al
	in	al, CMOS_DATA
	and	al, NOT SET
	out	CMOS_DATA, al

Restore_RTC_Index:
	clc
SMI_Abort:
	pop	si
	mov	al, bl			; Restore CMOS index
	out	CMOS_INDEX, al
Exit:	ret


Fix_RTC	endp



	END

