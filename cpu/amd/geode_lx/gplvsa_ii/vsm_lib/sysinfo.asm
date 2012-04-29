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
;*     Implementation of the macros:                                   *
;*        SYS_GET_SYSTEM_INFO                                          *
;*        SYS_LOGICAL_TO_PHYSICAL   

include sysmgr.inc
include vsa2.inc

.model tiny,c
.586p
.CODE



;***********************************************************************
; Gets hardware information
;***********************************************************************
sys_get_system_info proc pascal uses si \
	buffer:	PTR

	; Get a ptr to the System Manager's hardware information structure
	xor	bx, bx
	mov	esi, (VSM_Header PTR [bx]).SysStuff.SysMgr_Ptr
	movzx	eax, fs:(InfoStuff PTR [esi]).HardwareInfo
	add	esi, eax
	sub	esi, SPECIAL_LOC


	mov	bx, [buffer]

	; Fetch the data
	mov	cx, sizeof(Hardware)/2
	cld
Copy:	lodsw	fs:[esi]
	mov	[bx], ax
	add	bx, 2
	loop	Copy

	ret

sys_get_system_info endp



;***********************************************************************
; Converts a logical offset to a physical address
;***********************************************************************
sys_logical_to_physical proc pascal \
	Address: PTR

	movzx	eax, [Address]
	xor	bx, bx
	add	eax, (VSM_Header PTR [bx]).SysStuff.State._CS.base
	mov	edx, eax
	shr	edx, 16
	ret

sys_logical_to_physical endp 





	END 

