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
;*     This file implements the SYS_GET_NEXT_MSG & SYS_REGISTER_EVENT  *
;*     macros as well as the code to perform a system call.  

include sysmgr.inc
include smimac.mac
include vsa2.inc

.model tiny,c
.586p
.CODE




public Async_VSM

Async_VSM	dd	0


;***********************************************************************
; USHORT sys_get_next_msg(ULONG *)
;
; This routine retrieves a message packet from a VSM's message queue.
;
; Input:
;   MsgPacket = ptr to message packet (relative to DS)
; Returns:
;   Message code
;***********************************************************************

sys_get_next_msg proc uses si di \
	MsgPacket:  PTR

CheckMsgQ:
	xor	di, di
	ASSUME	di: PTR VSM_Header

	mov	si, [di].SysStuff.Qtail	; Is the message queue empty ?
	cmp	si, [di].SysStuff.Qhead
	jne	MessageIsWaiting

	mov	ax, SYS_CODE_EXIT	; Return to the System Manager
	smint

	; Returns here when there are message(s) pending for this VSM.
	jmp	short CheckMsgQ


MessageIsWaiting:
	ASSUME	si: PTR Message
	lea	di, [si+sizeof(Message)]
	mov	dx, [si].Msg		; Get the message code

	mov	eax, [si].From_VSM	; Get VSM message is from
	mov	[Async_VSM], eax


	; Copy message packet into caller's buffer
	cld
	lea	si, [si].Param
	mov	cx, MAX_MSG_PARAM
	mov	bx, [MsgPacket]		; Get ptr to message buffer
CopyMsg:
	lodsd
	mov	[bx], eax
	add	bx, 4
	loop	CopyMsg

	;
	; Advance the message queue ptr
	;
	mov	si, OFFSET VSM_Header.SysStuff
	ASSUME	si: PTR System
	cmp	di, [si].EndMsgQ	; Is Qtail at end ?
	jb	UpdateQtail
	lea	di, [si].MsgQueue	; Yes, wrap ptr to start of queue
UpdateQtail:
	mov	[si].Qtail, di


	; Return value of function is the message code
	mov	ax, dx
Exit:	ret



	ASSUME	si:NOTHING

sys_get_next_msg endp


;***********************************************************************
; USHORT sys_query_msg_queue(ULONG *)
;
; This routine queries the VSM's message queue
;
; Input:
;   If a message is present:
;       MsgPacket = ptr to message packet (relative to DS)
; Returns:
;   If a message is present:
;       Message code
;   Else
;       0xFFFF
;***********************************************************************

sys_query_msg_queue proc uses si  \
	MsgPacket:  PTR

	xor	bx, bx
	ASSUME  bx: PTR VSM_Header

	mov	si, [bx].SysStuff.Qtail	; Is the message queue empty ?
	cmp	si, [bx].SysStuff.Qhead
	mov	ax, 0FFFFh		; Return value if queue is empty
	je 	short Exit


	ASSUME	si: PTR Message
	mov	dx, [si].Msg		; Get the message code


	; Copy message packet into caller's buffer
	cld
	lea	si, [si].Param
	mov	cx, MAX_MSG_PARAM
	mov	bx, [MsgPacket]		; Get ptr to message buffer
CopyMsg:
	lodsd
	mov	dword ptr [bx], eax
	add	bx, 4
	loop	CopyMsg

	; NOTE:  Qtail is not advanced !!

	; Return value of function is the message code
	mov	ax, dx
Exit:	ret

sys_query_msg_queue endp






;***********************************************************************
; Registers this VSM as a handler for an event
;***********************************************************************
sys_register_event proc pascal uses edi \
	Event: EVENT,	\
	Param1: DWORD,	\
	Param2: DWORD,	\
	Priority: WORD

	mov	bx, [Event]		; Get event to register
	shl	ebx, 16
	mov	bx,  [Priority]		; Get event priority
	mov	ecx, [Param1]		; Get parameters
	mov	edi, [Param2]

	mov	ax, SYS_CODE_EVENT
	call	sys_system_call
	ret

sys_register_event endp





 ;***********************************************************************
; Performs a system call
;***********************************************************************
sys_system_call proc

	smint
	ret

sys_system_call endp



	END 

