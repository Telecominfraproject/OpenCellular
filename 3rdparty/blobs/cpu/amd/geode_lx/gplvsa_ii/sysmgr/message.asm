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
;*     Implements message handling routines 

include SYSMGR.INC
include VSA2.INC



.model tiny,c
.586p
.CODE

externdef SysMgr_VSM:            dword
externdef MsgPacket:             dword
externdef VSM_ListHead:          dword
externdef Current_VSM:           dword

externdef Events:                EVENT_ENTRY
externdef Sys_Exit:              proc
externdef pascal Schedule_VSM:   proc



;***********************************************************************
; This routine is called from the System Manager to enter a message
; packet into a VSM's message queue.
;
; Input:
;   To_VSM   = Flat ptr to VSM to which message is to be sent
;   MsgCode  = Priority::Message
;   From_VSM = value to be put in Async_VSM field
;   MsgPacket contains the message parameters
;***********************************************************************
Send_Message proc pascal uses di si \
	From_VSM:   dword, \
	To_VSM:     dword, \
	MsgCode:    dword

	mov     edi, [To_VSM]
	cmp     edi, [SysMgr_VSM]	; Don't send messages to SysMgr
	je      short Exit

	mov     eax, [MsgCode]
	lea     si, [MsgPacket]
	mov     ebx, [From_VSM]
	call    Insert_Msg
Exit:	ret

Send_Message endp








;***********************************************************************
; This routine enters a message packet into a VSM's message queue.
;
; Input:
;   EAX = Priority::Message
;   EBX = From_VSM
;   EDI = To_VSM
;    SI = Ptr to message packet
;***********************************************************************
Insert_Msg proc 

	cld

	push    ebx			; Save From_VSM

	; Get ptr to the head of the message queue
	movzx   ebx, fs:(VSM_Header PTR [edi]).SysStuff.Qhead

	; Compute ptr to the next available message queue entry
	lea     dx, [bx+sizeof(Message)]
	cmp     dx, fs:(VSM_Header PTR [edi]).SysStuff.EndMsgQ
	jb      short Check_Q_Overflow
	mov     dx, OFFSET VSM_Header.SysStuff.MsgQueue
Check_Q_Overflow:

	; Is Qhead == Qtail ?
	cmp     dx, fs:(VSM_Header PTR [edi]).SysStuff.Qtail
	jne     short UpdateQueueHead

	; Yes, then message queue has overflowed	
	mov     [si+8], eax		; Store previous message into Param2
	mov     ax, MSG_QUEUE_OVERFLOW	; Replace previous message
	mov     dx, bx			; Qhead = old Qhead
UpdateQueueHead:
	mov     fs:(VSM_Header PTR [edi]).SysStuff.Qhead, dx

	pop     edx			; From_VSM
	push    edi			; Parameter to Schedule_VSM() below

	; Store the message into the VSM's message queue.
	; NOTE: This code is dependent on "#typedef Message"
	add     edi, ebx
	stosd   [edi]			; Priority::Message

	mov     eax, edx		; From_VSM
	stosd   [edi]

REPEAT MAX_MSG_PARAM
	lodsd				; Copy parameters to message queue
	stosd   [edi]
ENDM
	
	
	
	; Sort messages by Priority
if SUPPORT_PRIORITY

	pop     ebx			; To_VSM
	push    ebx
	
	push    fs:(VSM_Header PTR [ebx]).SysStuff.Qhead
BubbleSort:	

	; Get ptr to latest queue entry
	movzx   esi, fs:(VSM_Header PTR [ebx]).SysStuff.Qhead
	sub     esi, sizeof(Message)
	cmp     si, OFFSET VSM_Header.SysStuff.MsgQueue
	jae     short @f
	mov     si, fs:(VSM_Header PTR [ebx]).SysStuff.EndMsgQ
	sub     si, sizeof(Message)
@@:
	cmp	si, fs:(VSM_Header PTR [ebx]).SysStuff.Qtail
	je	Bail

	; Get ptr to previous queue entry
	lea     edi, [esi-sizeof(Message)]
	cmp     di, OFFSET VSM_Header.SysStuff.MsgQueue
	jae     short @f
	mov     di, fs:(VSM_Header PTR [ebx]).SysStuff.EndMsgQ
	sub     di, sizeof(Message)
@@:
	
	mov     fs:(VSM_Header PTR [ebx]).SysStuff.Qhead, di

	; Add VSM base to ptrs
	add     edi, ebx
	add     esi, ebx
	
	; Compare their priorities	
	mov     ax, fs:(Message PTR [esi]).Priority
	cmp     ax, fs:(Message PTR [edi]).Priority
	jbe     short Bail
	
	
	
	; Swap the entries in the message queue
	mov     cx, sizeof(Message)/4	; Assumes entry is multiple of dword
Interchange:
	mov     eax, fs:[edi]
	xchg    fs:[esi], eax
	stosd	[edi]
	add     esi, 4
	loop    Interchange

	jmp	BubbleSort


Bail:
	pop     fs:(VSM_Header PTR [ebx]).SysStuff.Qhead

endif
	
       
	; Schedule the VSM to execute
	pop	eax
	cmp	eax, [SysMgr_VSM]
	je	short Exit
	push	eax
	call    Schedule_VSM
Exit:
	ret
	

Insert_Msg endp










;************************************************************************
;
; Sends a message to each VSM of type VSM_Type (or all if VSM_ANY).
; Called from HANDLERS.C
;
;************************************************************************
Broadcast_Message proc  pascal uses si di \
	MsgCode:     word,	\
	VSM_Type:    word,      \
	From_VSM:    dword

	mov     edi, [VSM_ListHead]	; Get ptr to list of VSMs
VSM_Loop:
	or      edi, edi		; End of VSM list ?
	jz      Exit_Broadcast

	mov     dx,  [VSM_Type]
	mov     ah, fs:(VSM_Header PTR [edi]).VSM_Type

	cmp     dl, VSM_ANY		; Schedule all VSMs ? 
	je      Schedule_It

	test    dh, (VSM_ALL_EXCEPT SHR 8)
	jz      CheckType

	cmp     dl, ah			; Skip the one that matches
	je      Skip_VSM
	jmp     Schedule_It

CheckType:
	cmp     dl, ah			; Is it the correct VSM ?
	jne     Skip_VSM

Schedule_It:

	cmp     edi, [From_VSM]		; Is it the requesting VSM ?
	je      Skip_VSM		; Yes, don't send to sender

	push    edi

	movzx   eax, [MsgCode]		; Send the message
	xor     ebx, ebx
	lea     si, [MsgPacket]

;   EAX = Priority::Message
;   EBX = 00000000h (a broadcast is not a synchronous event)
;    SI = Ptr to message packet
;   EDI = Ptr to VSM header where message is to be sent
	call    Insert_Msg
	pop     edi


Skip_VSM:
	mov     edi, fs:(VSM_Header PTR [edi]).SysStuff.Flink
	jmp     VSM_Loop

Exit_Broadcast:
	ret

Broadcast_Message endp



;************************************************************************
; Broadcasts a message to one or more VSMs.
;
; Input:
;   EBX = message code (16 MSBs)
;    BH = Flags
;    BL = VSM_Type
;   ECX = Param1
;   ESI = Param2
;   EDI = Param3
;************************************************************************
Sys_Broadcast proc

	mov	dx, bx			; Put VSM_Type::Flags into DX
	shr	ebx, 16			; Put message into BX

	mov     eax, esi
	lea     si, [MsgPacket]
	mov	[si+0], ecx
	mov	[si+4], eax
	mov	[si+8], edi

	; Re-schedule the broadcasting VSM
	; RunFlag will be changed from RUN_FLAG_ACTIVE to RUN_FLAG_READY
	mov     edi, [Current_VSM]
	push    edi
	call    Schedule_VSM
	; If RunFlag == RUN_FLAG_READY, Insert_Msg() won't schedule it again
	mov	fs:(VSM_Header PTR [edi]).SysStuff.RunFlag, RUN_FLAG_BLOCKED

	; Push parameters for call to Broadcast_Message()
	push    bx			; Message code
	push    dx			; VSM type
	push    edi			; From_VSM
	
	; Normally, the message is sent to the broadcasting VSM 
	; after all VSMs have handled the broadcasted message.
	; There are two exceptions to this rule:
	; 1) VSM_NOT_SELF   | <VSM type>
	; 2) VSM_ALL_EXCEPT | <self>
	test    dh, (VSM_NOT_SELF SHR 8)
	jnz     short Broadcast

	test    dh, (VSM_ALL_EXCEPT SHR 8)
	jz      short SendToSelf
	cmp     dl, fs:(VSM_Header PTR [edi]).VSM_Type
	je      short Broadcast

SendToSelf:
	; Put callback address onto the scheduler stack
	mov	eax, OFFSET BroadcastCallback
	push	eax
	call    Schedule_VSM

	mov	ax, BROADCAST_PRIORITY	; EAX = Priority::Message 
	shl	eax, 16
	mov	ax, bx

	mov     edi, [SysMgr_VSM]	; EDI = To_VSM
	mov     ebx, [Current_VSM]	; EBX = From_VSM
	call    Insert_Msg

Broadcast:
	call    Broadcast_Message

	jmp     Sys_Exit		; Exit w/o scheduling current VSM

Sys_Broadcast endp


;************************************************************************
; Control comes here when all VSMs have processed the broadcasted message.
; Copies the broadcasted message from SysMgr's message queue to the
; broadcasting VSM's message queue.
;************************************************************************
BroadcastCallback proc
	ASSUME  DI: PTR VSM_Header
	ASSUME  BX: PTR Message

	; Point BX to parameters in SysMgr's message queue head
	xor     di, di
	mov     bx, [di].SysStuff.Qhead
	sub     bx, sizeof(Message)
	cmp     bx, OFFSET VSM_Header.SysStuff.MsgQueue
	jb      short BadMsgStack
	mov     [di].SysStuff.Qhead, bx
	
	mov     eax, dword ptr [bx].Msg	; Get Priority::Message
	mov	edi, [bx].From_VSM	; To_VSM (the VSM it was originally from)

	lea	si, [bx].Param
	xor     ebx, ebx		; From_VSM (asynchronous)
	call    Insert_Msg
BadMsgStack:	
	ret
	
	ASSUME  BX: NOTHING
	ASSUME  DI: NOTHING

BroadcastCallback endp



;***********************************************************************
; Passes an event to the next VSM registered for this event
; NOTE: This system call is now obsolete.
;***********************************************************************
Sys_PassEvent proc 

	jmp     Sys_Exit		; Exit to VSM dispatcher
	
Sys_PassEvent endp





	END



