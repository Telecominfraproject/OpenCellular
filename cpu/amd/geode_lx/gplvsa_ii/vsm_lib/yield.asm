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
;*     This file implements the SYS_YIELD_CONTROL macro. 

include vsa2.inc
include sysmgr.inc
.model tiny,c
.586p
.CODE



externdef sys_system_call:proc



sys_yield_control proc pascal \
	Milliseconds: DWORD

	mov	ecx, [Milliseconds]
	jecxz	Exit

	mov	ax, SYS_CODE_YIELD		; Yield control to the System Manager
	call	sys_system_call

Exit:	ret

sys_yield_control endp


	END 

