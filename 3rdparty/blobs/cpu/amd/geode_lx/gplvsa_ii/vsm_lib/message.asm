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
;*     This file implements the SYS_BROADCAST_MSG system call

include sysmgr.inc
include vsa2.inc

.model tiny,c
.586p
.CODE




externdef sys_system_call:proc
externdef Async_VSM:  dword
externdef EventIndex: byte

;***********************************************************************
; void sys_broadcast_msg(MSG Code, ULONG * Params, UCHAR VSM_Type)
;***********************************************************************
sys_broadcast_msg proc pascal uses esi edi \
	MsgCode:     MSG,	\
	Params:      PTR,	\
	VSM_Type:    WORD


	mov	bx,  [MsgCode]		; Get MSG to broadcast
	shl	ebx, 16			; Put into 16 MSBs
	mov	di,  [Params]		; Get parameters
	mov	ecx, [di+0]		; Param1
	mov	esi, [di+4]		; Param2
	mov	edi, [di+8]		; Param3
	mov	bx,  [VSM_Type]		; Get VSM(s) to send message to
	
	mov	ax, SYS_CODE_BROADCAST
	call	sys_system_call
 	ret

sys_broadcast_msg endp


	END 

