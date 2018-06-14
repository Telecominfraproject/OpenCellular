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

.model tiny,c
.586p
.CODE

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
	rdmsr		; Get high 32 bits
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


	END



