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
;*   Function:
;*     This file implements routines to access MSRs. 



.model tiny,c
.586p
.CODE





;************************************************************************
; Returns the low dword of the specified MSR
;************************************************************************
Read_MSR_LO proc pascal  \
	MSR_Addr: dword
	
	mov	ecx, [MSR_Addr]
	rdmsr
	mov	edx, eax
	shr	edx, 16
	ret

Read_MSR_LO endp

;************************************************************************
; Writes the low dword of the specified MSR
;************************************************************************
Write_MSR_LO proc pascal  \
	MSR_Addr: dword, \
	MSR_Data: dword
	
	mov	ecx, [MSR_Addr]
	rdmsr
	mov	eax, [MSR_Data]
	wrmsr
	ret

Write_MSR_LO endp



;***********************************************************************
; Returns an MSR value in a buffer.
; Usage: Read_MSR(ULONG Msr, ULONG * Buffer);
;***********************************************************************
Read_MSR proc pascal \
	MSR_Addr:DWORD, \
	pMSR_Data:PTR DWORD
	
	mov	ecx, [MSR_Addr]
	rdmsr
	mov	bx, [pMSR_Data]
	mov	dword ptr [bx+0], eax
	mov	dword ptr [bx+4], edx
	ret

Read_MSR endp


;***********************************************************************
; Writes an MSR.
; Usage: Write_MSR(ULONG Msr, ULONG * Buffer);
;***********************************************************************
Write_MSR proc pascal \
	MSR_Addr:DWORD, \
	pMSR_Data:PTR DWORD
	
	mov	ecx, [MSR_Addr]
	mov	bx, [pMSR_Data]
	mov	eax, dword ptr [bx+0]
	mov	edx, dword ptr [bx+4]
	wrmsr
	ret

Write_MSR endp

	END 
