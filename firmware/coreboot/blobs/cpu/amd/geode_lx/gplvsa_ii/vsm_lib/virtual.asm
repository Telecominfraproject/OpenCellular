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
;*     This file contains the virtual register library functions.

include sysmgr.inc
include vsa2.inc

.model tiny,c
.586p
.CODE




externdef sys_system_call:proc




;***********************************************************************
; IMPLEMENTATION NOTE:
;
; Using a system call for virtual register access by a VSM has
; advantages over performing I/O:
;
; 1) There is no dependency on VIRTUAL_BASE. It can be changed
;    without requiring recompilation of the VSM.
;
; 2) Better performance.  There is only one SMI for a system call
;    versus two for I/O (index & data).
;
; 3) It solves the problem of atomic accesses to index/data pairs.      
;
;***********************************************************************


;***********************************************************************
; void sys_set_virtual_register(USHORT Index, USHORT Data)
;***********************************************************************

sys_set_virtual_register proc pascal \
	VR_Index:WORD, \
	VR_Data: WORD

	mov	bx, VR_Index
	mov	cx, VR_Data
	mov	ax, SYS_CODE_SET_VIRTUAL
	call	sys_system_call

	ret

sys_set_virtual_register endp




;***********************************************************************
; USHORT sys_get_virtual_register(USHORT Index)
;***********************************************************************
sys_get_virtual_register proc pascal  \
	VR_Index:word

	mov	bx, VR_Index
	mov	ax, SYS_CODE_GET_VIRTUAL
	call	sys_system_call

	ret

sys_get_virtual_register endp






	END 

