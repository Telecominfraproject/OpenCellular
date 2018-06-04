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
;*     This file contains the code for reading the source of SMIs. 
;********************************************************************************


include GX2.INC
include VSA2.INC
include SYSMGR.INC
include CS5536.INC
include CHIPSET.INC
include MDD.INC
include PCI.INC

.model tiny,c
.586p
.CODE 
ASSUME DS:_TEXT

SYNCHRONOUS equ (SMI_SRC_PCI_TRAP OR SMI_SRC_DESCR_HIT OR SMI_SRC_PM OR SMI_SRC_RESET OR SMI_SRC_BLOCKIO OR SMI_SRC_A20)
SMI_Sources	dd	0
Video_Sources	dd	0
Audio_Sources	dw	0
Stats_Sources	dd	0
SynchEvents     dd	SYNCHRONOUS
HiPrioritySMIs  dd	SMI_SRC_AUDIO OR SMI_SRC_USB1 OR SMI_SRC_USB2

public    SynchEvents, HiPrioritySMIs
public    SMI_Sources, Audio_Sources, Stats_Sources, Video_Sources

externdef pascal Hex_8:proc
externdef pascal Hex_16:proc
externdef pascal Hex_32:proc
externdef pascal Generate_IRQ_5536: proc

externdef SMI_Base:     dword
externdef Mbiu0:        dword
externdef Mbiu1:        dword
externdef Mbiu2:        dword
externdef MPCI_NB:      dword 
externdef MPCI_SB:      dword 
externdef MDD_Base:     dword
externdef MCP_NB:       dword
externdef ATA_Error:    dword
externdef OHCI1_Smi:    dword
externdef OHCI2_Smi:    dword
externdef HardwareInfo: Hardware



;*************************************************************************
; Installs the SMI routine(s) appropriate to the chipset.
; The default routine(s) are for CS5536.  
;*************************************************************************
Install_SMI_Routines proc

	;5536 already set, do nothing. mov	[OHCI_SMI], OHCSSTAT
Exit:	ret

Install_SMI_Routines endp




;************************************************************************
; Asserts/deasserts internal IRQs
; On Entry:
;  ECX = IRQ mask
;        16 LSBs - enable
;        16 MSBs - disable
;************************************************************************
Generate_IRQ proc

	push	cx
	call	word ptr [IRQ_Routine]
	ret

IRQ_Routine::
	dw	OFFSET Generate_IRQ_5536

Generate_IRQ endp




;*************************************************************************
; Reads & clears all SMI source registers (MSRs & Southbridge)
; On Exit:
;   EBX = SMI sources
;   All other registers may be destroyed
;*************************************************************************
Get_SMI_Sources proc

	mov	ebx, [SMI_Sources]	; Get SMIs that have not yet been handled
	call	Get_Northbridge_SMIs
	or	ebx, ebx		; If any NB SMIs, handle them quickly
	jz	short Get_SB_SMIs
	ret
Get_SB_SMIs:

	jmp	word ptr [SMI_Routine]	; Get Southbridge SMI(s)

SMI_Routine::
	dw	OFFSET Get_5536_SMIs

Get_SMI_Sources endp











;***********************************************************************
; Gets SMI sources from Northbridge
; On Entry:
;   EBX = pending SMI sources
; NOTES:
; - May set SMI source bits in EBX.
;***********************************************************************
MPCI_ERROR	equ	(MARE + TARE + PARE + BME + SYSE)
VGA_BLANK	equ	08h
VG_BLANK	equ	01h
VERT_BLANK	equ	(VGA_BLANK OR VG_BLANK)

Get_Northbridge_SMIs proc

	mov	ecx, [MPCI_NB]		; MBIU1.MPCI.0.0.0.0
	mov	cx, MBD_MSR_SMI
	rdmsr
	wrmsr				; Clear the MPCI SMI event(s)

	mov	dl, al			; Only record enabled events
	not	dl
	shr	eax, 16
	and	al, dl			; Only consider unmasked events
	jz	short Graphics_SMIs	; Jmp if no MPCI SMI is pending	

	test	al, VPHE SHR 16		; Is it a virtualized PCI header ?
	jz	short CheckMPCI
	or	ebx, SMI_SRC_PCI_TRAP	; Yes, record the event
	
	cmp	al, VPHE SHR 16		; Is VPHE the only SMI pending ?
	je	short Exit		; Yes, do a quick exit
  	

CheckMPCI:	
	test	ax, MPCI_ERROR SHR 16	; Is it an MPCI error ?
	jz	short Graphics_SMIs
	or	ebx, SMI_SRC_MPCI	; Yes

	mov	cx, MBD_MSR_ERROR	; Clear the MPCI error(s) 
	rdmsr
	wrmsr
	




	;*******************************************
	; Check for graphics SMIs
	;*******************************************

; Value in Video_Sources:
;
;   Bit		            Description
;  -----	-----------------------------------
;    0		DF: SMI #1 (TBD)
;    1 	DF: SMI #2 (TBD)
;    2		GP: SMI (address or type violation)
;    3		reserved
;    4		VG: Miscellaneous Output SMI
;    5		VG: Input Status register SMI
;    6		reserved
;    7		VG: CRTC invalid I/O SMI
Graphics_SMIs:
	; Video Generator
	mov	ecx, VG_SMI_MSR
	rdmsr
	wrmsr				; Clear SMI(s)

	; Because the VG records all events that occur in the upper half of the MSR
	; regardless of whether or not they generate SMIs, we need to clear out the
	; events that aren't generating SMIs and only look for valid ones. 
	not eax						; NOT the SMI mask
	and	edx,eax					; Clear out untrapped events

	test	dl, VERT_BLANK		; Is a vertical blank pending ?
	jz	short SaveVideoSources
	and	dl, NOT (VERT_BLANK)
	or	ebx, SMI_SRC_RETRACE	; Yes, record EVENT_VBLANK
SaveVideoSources:
	or	[Video_Sources], edx
	jz	short Statistic_SMIs
	or	ebx, SMI_SRC_VG		; Yes, then record EVENT_GRAPHICS

	
	;*******************************************
	; Check for Statistic Counter SMI(s)	
	;*******************************************
Statistic_SMIs:
	mov	ecx, [Mbiu0]		; Check MBIU0 Statistics Counters
	call	Get_MBD_SMIs
	or	byte ptr [Stats_Sources+0], dl

	mov	ecx, [Mbiu1]		; Check MBIU1 Statistics Counters
	call	Get_MBD_SMIs
	or	byte ptr [Stats_Sources+1], dl
Exit:
	ret

Get_Northbridge_SMIs endp


	
	
;***********************************************************************
; Checks for events in MBD_MSR_SMI:
;   - descriptor traps
;   - statistic counters
; On entry: ECX = base address of GLIU
; Returns: DL = statistic event mask
;***********************************************************************
Get_MBD_SMIs proc	

	mov	cx, MBD_MSR_ERROR	; Clear errors
	rdmsr
	wrmsr

	mov	cx, MBD_MSR_SMI
	rdmsr
	
	and	dl, 1Fh			; Any SMIs pending ?
	jz	short Exit		; No, just exit
	wrmsr

	btr	dx, 0			; Is it a MBIU descriptor hit ?
	jnc	short StatCntrs
	or	ebx, SMI_SRC_DESCR_HIT	; Yes

StatCntrs:
	or	dl, dl			; Any hits on statistic counters ?			
	jz	short Exit
	or	ebx, SMI_SRC_STAT	; Yes, record the event
	or	al, dl			; Disable further events

	wrmsr
	
	shr	dl, 1
Exit:
	ret

Get_MBD_SMIs endp







;*************************************************************************
; Reads and clears CS5536 SMI source registers
;*************************************************************************
Get_5536_SMIs proc

	mov	ecx, [MDD_Base]		; Check for MDD SMIs
	mov	cx, MBD_MSR_SMI
	rdmsr
	wrmsr				; Clear any pending SMI(s)
	or	dx, dx			; Any MDD SMIs pending ?
	jz	short Get_MPCI_SB	; No

	xor	eax, eax	
MDD_Loop:
	bsf	ax, dx 			; Determine next pending event
	jz	Exit
	btr	dx, ax			; Clear the status bit
	or	ebx, dword ptr [eax*4+MDD_Sources]
	jmp	short MDD_Loop


	; MDD SMI sources
MDD_Sources:
	dd	0			; HLT_ASMI_STAT
	dd	0			; SHUTDOWN_ASMI_STAT
	dd	SMI_SRC_KEL		; KEL_ASMI_STAT
	dd	SMI_SRC_PIC		; PIC_ASMI_STAT
	dd	SMI_SRC_PME		; PM_ASMI_STAT
	dd	SMI_SRC_RESET		; INIT_K_STAT
	dd	SMI_SRC_A20		; A20_P_STAT
	dd	SMI_SRC_RESET		; INIT_P_STAT
	dd	0			; UART1_SSMI_STAT
	dd	0			; UART2_SSMI_STAT
	dd	0			; RESERVED_STAT
	dd	0			; LPC_SSMI_STAT
	dd	0			; DMA_SSMI_STAT
	dd	SMI_SRC_A20		; A20_K_STAT
	dd	SMI_SRC_ACPI		; PM2_CNT_SSMI_STAT
	dd	SMI_SRC_ACPI		; PM1_CNT_SSMI_STAT



Get_MPCI_SB:
	mov	ecx, [MPCI_SB]		; Get Southbridge MPCI events
	mov	cx, MBD_MSR_SMI
	rdmsr
	wrmsr				; Clear any pending SMI(s)

	mov	ecx, [Mbiu2]		; Check for MBIU2 SMIs
	call	Get_MBD_SMIs
	or	byte ptr [Stats_Sources+2], dl

	mov	ecx, [OHCI1_Smi]	; Check for USB #1 SMIs
	jcxz	short Exit
	rdmsr
	test	dl, [OHCI_SMI]		; OHCI SMI pending ?
	jz	short Exit		; JMP if not
	wrmsr				; Yes, clear the SMI
	or	ebx, SMI_SRC_USB1	; Record the SMI event

Exit:	ret

Get_5536_SMIs endp


OHCSSTAT		equ 10h		; 5536
OHCI_SMI		db  OHCSSTAT	; Assume 5536; could patched by Install_SMI_Routines() if other
	



	end

