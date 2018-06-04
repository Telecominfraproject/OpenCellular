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
;*     This file implements the critical section macros. 


include gx2.inc

.model tiny,c
.586p
.CODE





;***********************************************************************
; Disables SMI nesting
;***********************************************************************
EnterCriticalSection proc

	mov	ecx, MSR_SMM_CTRL
	rdmsr
	and	eax, NOT NEST_SMI_EN
	wrmsr	
	ret

EnterCriticalSection endp



;***********************************************************************
; Enables SMI nesting
;***********************************************************************
ExitCriticalSection proc

	mov	ecx, MSR_SMM_CTRL
	rdmsr
	or	eax, NEST_SMI_EN
	wrmsr	
	ret

ExitCriticalSection endp






	END 

