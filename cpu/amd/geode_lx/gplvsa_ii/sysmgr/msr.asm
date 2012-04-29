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
;*     Routines related to MSRs.   

include VSA2.INC
include DESCR.INC


.model tiny,c
.586p
.CODE

extern SMM_Header: SmiHeader

;***********************************************************************
; Returns the high DWORD of an MSR.
; Usage: HighMsrValue = Read_MSR_HI(Msr_Address);
;***********************************************************************
Read_MSR_HI proc pascal \
	Msr:	dword

	mov	ecx, [Msr]
	rdmsr
	mov	ax, dx
	shr	edx, 16
	ret

Read_MSR_HI endp

;***********************************************************************
; Writes the high DWORD of an MSR. The low DWORD is preserved.
; Usage: Write_MSR_HI(Msr_Address, Data);
;***********************************************************************
Write_MSR_HI proc pascal \
	Msr:	dword, \
        Data:   dword	
	
	mov	ecx, [Msr]
	rdmsr				; Get low 32 bits
	mov	edx, [Data]
	wrmsr
	ret

Write_MSR_HI endp

;***********************************************************************
; Returns the low DWORD of an MSR.
; Usage: LowMsrValue = Read_MSR_LO(Msr_Address);
;***********************************************************************
Read_MSR_LO proc pascal \
	Msr:	dword

	mov	ecx, [Msr]
	rdmsr
	mov	edx, eax
	shr	edx, 16
	ret

Read_MSR_LO endp

;***********************************************************************
; Writes the low DWORD of an MSR. The high DWORD is preserved.
; Usage: Write_MSR_LO(Msr_Address, Data);
;***********************************************************************
Write_MSR_LO proc pascal \
	Msr:	dword, \
        Data:   dword	
	
	mov	ecx, [Msr]
	rdmsr				; Get high 32 bits
	mov	eax, [Data]
	wrmsr
	ret

Write_MSR_LO endp




;***********************************************************************
; Returns an MSR value in a buffer.
; Usage: Read_MSR(ULONG Msr, ULONG * Buffer);
;***********************************************************************
Read_MSR proc pascal \
	Msr:	dword, \
	Buffer: PTR
	
	mov	ecx, [Msr]
	rdmsr
	
	mov	bx, [Buffer]
	mov	[bx+0], eax
	mov	[bx+4], edx
	ret

Read_MSR endp


;***********************************************************************
; Writes an MSR.
; Usage: Write_MSR(ULONG Msr, ULONG * Buffer);
;***********************************************************************
Write_MSR proc pascal \
	Msr:	dword, \
	Buffer: PTR
	
	mov	ecx, [Msr]
	mov	bx, [Buffer]
	mov	eax, [bx+0]
	mov	edx, [bx+4]
	wrmsr
	ret

Write_MSR endp




;***********************************************************************
; Parses MSR_MBIU_CAP of an MBIU and returns descriptor counts in
; a byte buffer.
;***********************************************************************
Parse_Capabilities proc pascal  \
	Msr:    PTR, \
	Buffer: PTR
	
	mov	bx, [Msr]
	mov	eax, [bx+0]		; Get 1st dword of CAPABILITIES
	mov	edx, [bx+4]		; Get 2nd dword of CAPABILITIES
	mov     bx, [Buffer]
	mov	cl, 6			; Number of P2D descriptors
NxtCount:
	mov	ch, al
	and	ch, 3Fh			; 6 bits per field
	mov	[bx], ch
	inc	bx
	
	shrd	eax, edx, 6		; Shift next field into LSBs
	shr	edx, 6
	dec	cl	
	jnz	NxtCount

	mov	ch, al			; Get NIOD_BM
	and	ch, 3Fh
	mov	[bx+1], ch
	
	shr	eax, 6			; Get NIOD_SC
	mov	ch, al
	and	ch, 3Fh
	mov	[bx+2], ch
	
	shr	eax, 9			; Skip NCOH field
	mov	ch, al
	and	ch, 07h			; Get NPORTS
	mov	[bx+3], ch

	shr	ax, 9			; Get NSTAT_CNT
	and	al, 07h
	mov	[bx+4], al	
	ret
	
Parse_Capabilities endp	


;***********************************************************************
; Extract the 3 main fields from a P2D descriptor
;***********************************************************************
Parse_Descriptor proc pascal uses edi \
	P2D_Type: byte, \
	Source:  ptr, \
	Dest:  ptr
	
	mov	bx, [Source]

	mov	ecx, [bx]		; Get low MSR
	mov	eax, ecx
	mov	edx, [bx+4]		; Get high MSR

	cmp	bl, IOD_BM
	je	Descr_IO_BM

	shl	ecx, 12			; Extract 1st field
	shrd	eax, edx, 20		; Extract 2nd field
	shl	eax, 12
	
	xor	dl, dl			; Extract 3rd field
	shl	edx, 12-8
	mov	edi, edx		; EDI = Offset
	
	mov	bl, [P2D_Type]		; Dispatch to correct descriptor parser
	cmp	bl, P2D_BM 
	je	Descr_BM
	cmp	bl, P2D_BMO
	je	Descr_BMO
	cmp	bl, P2D_BMK
	je	Descr_BMK
	cmp	bl, P2D_R
	je	Descr_R
	cmp	bl, P2D_RO
	je	Descr_RO
	jmp	Exit	


Descr_IO_BM:
	and	ecx, 000FFFFFh		; ECX = IMASK
	shrd	eax, edx, 20		; EAX = IBASE
	xor	edi, edi		; EDI = 00000000

Descr_BMO:				; Assume OFFSET == 0
Descr_BM:
Descr_BMK:
	; ECX = PMASK
	; EAX = PBASE
	and	eax, ecx		; Start = PBASE & PMASK
	mov	edx, eax		; End = Start + ~PMASK
	not	ecx
	add	edx, ecx
	jmp	StoreResult
	

Descr_RO:				; Assume OFFSET == 0
Descr_R:
	; ECX = PMIN
	; EAX = PMAX
	xchg	eax, ecx		; Start = PMIN
	mov	edx, ecx		; End = PMAX
	or	dx, 0FFFh

StoreResult:
	mov	bx, [Dest]
	mov	[bx], eax		; Start
	mov	[bx+4], edx		; End
	add	edi, eax		; Physical = Start + Offset
	mov	[bx+8], edi

Exit:	ret


Parse_Descriptor endp




;***********************************************************************
;***********************************************************************
MergeFields proc pascal \
	Dest: PTR, \
	Field1: DWORD, \
	Field2: DWORD, \
	Field3: DWORD

	mov	bx, [Dest]

;//  Descr->MsrData[0]  = (PBase << 20) | (PMask & 0xFFFFF);
;//  Descr->MsrData[1]  = (PBase >> 12) | (POffset << 8);
	mov	eax, [Field2]
	mov	edx, eax
	shl	eax, 20
	mov	ecx, [Field1]
	and	ecx, 0FFFFFh
	or	eax, ecx
	mov	[bx], eax
		
	shr	edx, 12
	mov	eax, [Field3]
	shl	eax, 8
	or	eax, edx
	mov	[bx+4], eax
	ret
		
MergeFields endp




;***********************************************************************
; Returns TRUE if Range is a power of 2
;***********************************************************************
IsPowerOfTwo proc pascal \
	Range: dword

	mov	ebx, [Range]
	bsf	eax, ebx		; Scan LSB to MSB
	bsr	edx, ebx		; Scan MSB to LSB
	cmp	al, dl			; Same bit found ?
	mov	al, 0			; No, return FALSE
	jne	Exit
	mov	al, 1			; else return TRUE
Exit:	ret
	
IsPowerOfTwo endp



;***********************************************************************
; Returns index of first bit set when scanning from MSB to LSB
;***********************************************************************
BitScanReverse proc pascal \
	Range: dword

	mov	ebx, [Range]
	bsr	eax, ebx		; Scan MSB to LSB
	ret
		
BitScanReverse endp


;***********************************************************************
; Returns index of first bit set when scanning from LSB to MSB
;***********************************************************************
BitScanForward proc pascal \
	Range: dword

	mov	ebx, [Range]
	bsf	eax, ebx		; Scan LSB to MSB
	ret
		
BitScanForward endp












	end
