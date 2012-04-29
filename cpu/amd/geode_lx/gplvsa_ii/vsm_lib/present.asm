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
;*     Implementation of SYS_VSM_PRESENT 


include sysmgr.inc
include vsa2.inc

.model tiny,c
.586p
.CODE


;***********************************************************************
; Returns TRUE if a VSM of the specified type is present
;***********************************************************************
sys_vsm_present PROC pascal \
	VSM_Type: byte

	mov	ecx, (VSM_Header PTR ds:[0]).SysStuff.SysMgr_Ptr
	sub	ecx, SPECIAL_LOC
	mov	ah, [VSM_Type]	
	xor	al, al			; Assume VSM is not present
VSM_Search:
	mov	ecx, fs:(VSM_Header PTR [ecx]).SysStuff.Flink
	jecxz	short Exit
	cmp	ah, fs:(VSM_Header PTR [ecx]).VSM_Type
	jne	VSM_Search

	inc	al			; VSM is present
Exit:	ret

sys_vsm_present endp


	END 

