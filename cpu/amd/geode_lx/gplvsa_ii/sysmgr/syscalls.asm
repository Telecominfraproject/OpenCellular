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
;*******************************************************************************
;*     Implements several system calls  
;*******************************************************************************


include SYSMGR.INC
include VSA2.INC
include PCI.INC
include SMIMAC.MAC
include CHIPSET.INC
include VR.INC




.model tiny,c
.586p
.CODE

public SysCall_Table
public ReturnULONG


externdef Sys_SW_INT:                   proc
externdef Sys_Broadcast:                proc
externdef Sys_PassEvent:                proc
externdef Sys_State:                    proc
externdef EmptyMsgQueue:                proc
externdef ExitSysCall:                  proc
externdef Dispatcher:                   proc
externdef Sys_Exit:                     proc
externdef Record_VSM_Locations:         proc
externdef pascal Lookup_PCI:            proc
externdef pascal Lookup_IO:             proc
externdef pascal Find_MBus_ID:          proc
externdef pascal Send_Event:            proc
externdef pascal Schedule_VSM:          proc
externdef pascal Allocate_BAR:          proc
externdef pascal Register_Event:        proc
externdef pascal Report_VSM_Error:      proc
externdef pascal Allocate_Resource:     proc
externdef pascal Set_Address_Decode:    proc
externdef pascal Enable_PCI_Trapping:   proc
externdef pascal Unregister_VSM_Events: proc
externdef pascal IRQY_Mapper:           proc
externdef pascal Return_Virtual_Value:  proc

externdef SysMgr_VSM:            dword
externdef VSM_ListHead:          dword
externdef Current_VSM:           dword
externdef MsgPacket:             dword
externdef SMI_Base:              dword
externdef IRQ_Base:              dword
externdef VSM_Ptrs:              dword
externdef Events:                EVENT_ENTRY
externdef HardwareInfo:          Hardware





	align	2
SysCall_Table:
	dw	OFFSET Sys_Register	; 00 - SYS_CODE_EVENT
	dw	OFFSET Sys_Yield	; 01 - SYS_CODE_YIELD
	dw	OFFSET Sys_SW_INT	; 02 - SYS_CODE_SW_INT
	dw	OFFSET Sys_PassEvent	; 03 - SYS_CODE_PASS_EVENT
	dw	OFFSET Sys_Unload       ; 04 - SYS_CODE_UNLOAD
	dw	OFFSET Sys_Registers	; 05 - SYS_CODE_REGISTER
	dw	OFFSET Sys_RdWrPCI	; 06 - SYS_CODE_PCI_ACCESS
	dw	OFFSET Sys_SetVirtual	; 07 - SYS_CODE_SET_VIRTUAL
	dw	OFFSET Sys_GetVirtual	; 08 - SYS_CODE_GET_VIRTUAL
	dw	OFFSET Sys_Broadcast	; 09 - SYS_CODE_BROADCAST
	dw	OFFSET Sys_State	; 0A - SYS_CODE_STATE
	dw	OFFSET Sys_Report_Error ; 0B - SYS_CODE_ERROR
	dw	OFFSET Sys_Resource     ; 0C - SYS_CODE_RESOURCE
	dw	OFFSET Sys_Decode	; 0D - SYS_CODE_DECODE
	dw	OFFSET Sys_GetDescr	; 0E - SYS_CODE_DESCRIPTOR
	dw	OFFSET Sys_Lookup	; 0F - SYS_CODE_LOOKUP
	dw	OFFSET Sys_IRQ_Mapper	; 10 - SYS_CODE_IRQ_MAPPER
	dw	OFFSET Sys_Result     	; 11 - SYS_CODE_RESULT
	dw	OFFSET Sys_Duplicate    ; 12 - SYS_CODE_DUPLICATE
	dw	OFFSET EmptyMsgQueue    ; 13 - SYS_CODE_EXIT


;************************************************************************
; Implements the WRITE_PCI_NO_TRAP macros
;
; On Entry:
;   SI = data size (bit 7 = 1 if I/O write);
;  EBX = PCI configuration address
;  ECX = data (if write)
;************************************************************************
Sys_RdWrPCI proc

	push    ebx			; Save PCI address
	push    ecx			; Save data
	
	; Disable trapping of PCI address
	push    bx			; PCI_Address
	push	word ptr 0000
	call    Enable_PCI_Trapping

	movzx   bp, al			; Save previous trap setting

	pop     ecx			; Restore data
	pop     ebx			; Restore PCI address

	; Write the PCI address
	mov     dx, PCI_CONFIG_ADDRESS
	mov     eax, ebx
	out     dx, eax
	add     dl, 4
	and     al, 3
	add     dl, al

	btr     si, 7			; Read or write ?
	jc      PCI_Write		; Jmp if write
        
	cmp     si, BYTE_IO		; Byte, Word, or Dword ?
	je      Read_Byte
	cmp     si, WORD_IO
	je      Read_Word


Read_Dword:
	db      66h			; Force next instruction to be IN EAX,DX
Read_Word:
	in      ax, dx
	jmp     short ReturnValue


Read_Byte:
	in      al, dx
ReturnValue:

	push    eax			; Save return value

	call    Restore_Trapping	; Re-enable PCI trapping

	pop     ax			; Restore return value
	pop     dx		

ReturnULONG::
	mov     bx, word ptr gs:(VSM_Header).SysStuff.SavedESP
	mov	word ptr gs:[bx+8*4], ax
	mov	word ptr gs:[bx+6*4], dx
	jmp	ExitSysCall
	


;************************************************************************
PCI_Write:

	mov     eax, ecx		; Get data to be written
   	cmp     si, WORD_IO		; Byte, Word, or Dword ?
	je      short Write_Word
	cmp     si, DWORD_IO
	je      short Write_Dword
	cmp     si, BYTE_IO
	jne     short Ignore		; Ignore if error in parameter

Write_Byte:
	out     dx, al
	jmp     short Restore_PCI_Trap


Write_Dword:
	db      66h			; Force next instruction to be OUT DX,EAX
Write_Word:
	out     dx, ax

Restore_PCI_Trap:
	call    Restore_Trapping	; Re-enable PCI trapping

Ignore:
	jmp     ExitSysCall


Sys_RdWrPCI endp


;***********************************************************************
; Restores PCI trapping
; On entry:
;   BP = 1 means re-enable PCI trapping
;***********************************************************************
Restore_Trapping proc

	; Is trapping to be re-enabled ?
	cmp     bp, 1
	jne     short Exit

	; Yes, restore PCI trapping on this address
	; Enable_PCI_Trapping(PCI_Address, EnableFlag);
	push    bx			; PCI address
	push    bp			; EnableFlag
	call    Enable_PCI_Trapping
Exit:	ret

Restore_Trapping endp




;***********************************************************************
;
; Register an event to the calling VSM
;
; On entry:
;   EBX = Event::Priority
;   ECX = Parameter 1
;   EDI = Parameter 2
;***********************************************************************
Sys_Register proc

	; Register_Event(Event, Priority, VSM, Param1, Param2);
	push    ebx			; Event::Priority
	push    [Current_VSM]		; VSM ptr
	push    ecx			; Param1
	push    edi 			; Param2
	call    Register_Event
	jmp     ExitSysCall

Sys_Register endp


;***********************************************************************
; Input:
;   BX = Virtual register index
; Output:
;   AX = returned data
;***********************************************************************
Sys_GetVirtual proc

	xor     cx, cx			; Data = 0000
	xor     di, di			; 0 = read
	jmp     VirtualCommon

Sys_GetVirtual endp



;***********************************************************************
; Input:
;   BX = Virtual register index
;   CX = Data
;***********************************************************************
Sys_SetVirtual proc

	call    Handle_PM
	
	mov     di, 1			; 1 = write
VirtualCommon::
	cmp     bh, VRC_KEYBOARD	; Allow VRC_KEYBOARD to pass 32-bits
	je      Reschedule
	movzx   ecx, cx
Reschedule:		
	mov     edx, [Current_VSM]	; Re-schedule calling VSM
	push    edx
	call    Schedule_VSM

	call    VirtualRegisterEvent

	mov     ax, 0FFFFh		; Value of an undefined virtual register
	mov     bx, word ptr gs:(VSM_Header).SysStuff.SavedESP
	mov	word ptr gs:[bx+8*4], ax
	jmp	Dispatcher
	
	


Sys_SetVirtual endp


Handle_PM proc

if 0;SUPPORT_PM	; requires 78 bytes
	; If OHCI VSM is sending a USB event to KBD VSM, send an EVENT_IO_TRAP
	; to the PM VSM so system will exit DOZE mode.
	cmp     bh, VRC_KEYBOARD
	jne     short Exit
	mov     eax, [VSM_Ptrs+4*VSM_PM]; Is PM installed ?
	or      eax, eax
	jz      short Exit
	
	
	; Is PM VSM registered for the I/O trap ?
	mov	al, EVENT_IO_TRAP
FindPM:	
	mov	ah, sizeof(EVENT_ENTRY)
	mul	ah
	mov	si, ax
	mov	eax, (EVENT_ENTRY PTR [Events+si]).Vsm
	or	eax, eax
	jz	short Exit
	cmp	eax, edi		; Is it the PM VSM ?
	jne	short Next_VSM
	cmp	(EVENT_ENTRY PTR [Events+si]).Param1, 0060h
	je	short PM_Is_Dozing
Next_VSM:	
	mov	al, (EVENT_ENTRY PTR [Events+si]).Link
	or	al, al
	jnz	FindPM
	jmp	short Exit
	
	
PM_Is_Dozing:
	push    bx			; Yes, save virtual register info
	push    cx

	mov     eax, MSG_EVENT		; Send the PM VSM an EVENT_IO_TRAP[0x60] message
	mov     ebx, [SysMgr_VSM]
	lea     si, [PM_Packet]
	call    Insert_Msg

	pop     cx			; Restore virtual register info
	pop     bx

endif

Exit:	ret

PM_Packet dd	 EVENT_IO_TRAP, 0, 0060h

Handle_PM endp



;***********************************************************************
; Sends an EVENT_VIRTUAL_REGISTER to VSM(s)
; On entry:
;  BX = Class::Index
;  CX = write data
;  DI = 0 (read) or 1 (write)
; EDX = From_VSM
;***********************************************************************
VirtualRegisterEvent proc

	lea     si, [MsgPacket]		; Fill the message packet
	movzx   eax, bx
	mov     [si+1*4], eax		; MsgPacket[1] : virtual register index
	mov     ax, di
	mov     [si+2*4], eax		; MsgPacket[2] : 0=read   1=write
	mov     [si+3*4], ecx 	 	; MsgPacket[3] : write data

	mov     ax, EVENT_VIRTUAL_REGISTER ; No, so it's a VR access
	push    ax			; Event
	push    edx			; From_VSM
	call    Send_Event
	ret
	
VirtualRegisterEvent endp






;***********************************************************************
; Implements the SYS_UNLOAD_VSM macro
;***********************************************************************
Sys_Unload proc

	; Find prior VSM to current VSM
	mov     eax, [Current_VSM]
	or	eax, eax
	jz	short Exit	
	
	; Remove current VSM from linked list
	mov     ebx, (VSM_Header PTR fs:[eax]).SysStuff.Blink
	mov     ecx, (VSM_Header PTR fs:[eax]).SysStuff.Flink
	or	ebx, ebx		; Is there a previous VSM ?
	jnz	short IsBackLink
	mov	[VSM_ListHead], ecx	; No, update VSM_ListHead with VSM at Flink 
	jmp	short FixupBlink
IsBackLink:
	mov     (VSM_Header PTR fs:[ebx]).SysStuff.Flink, ecx
FixupBlink:
	or	ecx, ecx
	jz	short Unregister
	mov     (VSM_Header PTR fs:[ecx]).SysStuff.Blink, ebx

	; Unregister events registered to this VSM
Unregister:
	push    eax
	call	Unregister_VSM_Events

	call    Record_VSM_Locations
Exit:	jmp     Sys_Exit		; Exit to SysMgr

Sys_Unload endp





;***********************************************************************
; Implements the SYS_ALLOCATE_RESOURCE macro
; Input:
;   BL  - Resource type
;   Other registers - various parameters
;***********************************************************************
Sys_Resource  proc

	; Only BAR types are implemented at this time
	cmp	bl, RESOURCE_MEMORY
	je	short BAR_Resource
	cmp	bl, RESOURCE_MMIO
	je	short BAR_Resource
	cmp	bl, RESOURCE_IO
	je	short BAR_Resource
	cmp	bl, RESOURCE_SCIO
	je	short BAR_Resource

	; Not a BAR resource	
	push    bx			; Resource
	push    ecx			; Param
  	call    Allocate_Resource
	jmp	ExitSysCall
	
	
BAR_Resource:
	push	bx			; BAR type
	push	si			; BAR offset
	push	ecx			; Range

	ror	edi, 16
	push	di			; MBus_ID
	ror	edi, 16
	push	di			; PCI Device_ID
	call    Allocate_BAR

	jmp	ReturnULONG

Sys_Resource  endp



;***********************************************************************
; Implements the SYS_REPORT_ERROR macro
;  DI = Error code
; EBX = Info1
; ECX = Info2
;***********************************************************************
Sys_Report_Error proc

	push    di
	push    ebx
	push    ecx
	call    Report_VSM_Error

	jmp     ExitSysCall


Sys_Report_Error endp


YIELD_HANDLE   equ  (ONE_SHOT OR SYS_YIELD OR 0000BEEFh)

;***********************************************************************
; Implements the SYS_YIELD macro
;
; Input:
;   ECX = milliseconds to suspend the VSM
;***********************************************************************
Sys_Yield proc

	mov     eax, [Current_VSM]      ; VSM that is yielding control
	mov     fs:(VSM_Header PTR [eax]).SysStuff.RunFlag, RUN_FLAG_WAITING
	
	push    word ptr EVENT_TIMER    ; Event
	push    word ptr 0000           ; Priority
	push    eax			; Vsm
	push    ecx                     ; Param1 (ms)
	push    dword ptr YIELD_HANDLE  ; Param2 (handle)
	call    Register_Event
	
	jmp     Sys_Exit		; Exit to SysMgr. Don't return to VSM

Sys_Yield endp


;***********************************************************************
; This system call is obsolete
; Input:
;   BH = 0 for Set, 1 for Get
;   BL = register definition
;  ECX = data if Set
;***********************************************************************
Sys_Registers proc

	jmp     ExitSysCall

Sys_Registers endp


;***********************************************************************
; Implements the SYS_MAP_IRQ macro
; Maps IRQ source to specified IRQ
; Input:
;   BL = Unrestricted Y source (0x00-0x0F)
;   CL = IRQ setting
;***********************************************************************
Sys_IRQ_Mapper proc

	push    bx
	push    cx
	call    IRQY_Mapper
	jmp     ExitSysCall

Sys_IRQ_Mapper endp



;***********************************************************************
; Input:
;   BX = Address
;   CX = Decode flag
;***********************************************************************
Sys_Decode proc

	push    bx			; Address
	push    cx			; Decode (POSITIVE_DECODE or SUBTRACTIVE_DECODE)
	call    Set_Address_Decode

	jmp     ExitSysCall

Sys_Decode endp





;***********************************************************************
; Implements the SYS_MBUS_DESCRIPTOR & SYS_IO_DESCRIPTOR macros
;
; On entry:
;   BX = Address
;   CX = 0 if PCI else I/O
; Returns:
;  MSR address of corresponding resource
;***********************************************************************
Sys_GetDescr proc

	jcxz	PCI

        ; MSR_Address = Lookup_IO(Address);
	push	bx
	call	Lookup_IO

	mov	bl, 2			; Restore IO_Flag
	jmp	Common
	
        ; MSR_Address = Lookup_PCI(Address);
PCI:	push	bx
	call	Lookup_PCI

	xor	bx, bx
Common:	
	mov	cx, dx			; Was resource found ?
	shl	ecx, 16
	mov	cx, ax
	xor	eax, eax		; EDX:EAX = 00000000:00000000
	xor	edx, edx
	jecxz	ReturnMSRInfo

	rdmsr				; Yes, read MSR

	test	bl, 2			; Set MSR to default ?
	jz	ReturnMSRInfo
	push	edx			; Yes, save current value
	push	eax

	test	eax, 000F0000h		; IOD_BM or IOD_SC ?
	mov	eax, 00000000h		; Default for IOD_SC
	mov	edx, eax
	jz	SetDefault
	mov	eax, 0FFF00000h		; Default for IOD_BM
	mov	edx, 0000000FFh
SetDefault:
	wrmsr				; Set MSR to default value

	pop	eax			; Restore original MSR value
	pop	edx

	; Return MSR address in ECX and MSR value in EDX:EAX
ReturnMSRInfo:
	mov     bx, word ptr gs:(VSM_Header).SysStuff.SavedESP
	mov	gs:[bx+7*4], ecx	; Return MSR address in ECX
	mov	gs:[bx+8*4], eax	; Return MSR value in EDX:EAX
	mov	gs:[bx+6*4], edx
	jmp	ExitSysCall

Sys_GetDescr endp




;***********************************************************************
; Implements the SYS_LOOKUP_DEVICE macro
;
; On entry:
;   BX = MBus Device_ID
;   CX = Instance
; Returns:
;  DX:AX = MSR routing address to MBus device
;***********************************************************************
Sys_Lookup proc

	; Find_MBus_ID(USHORT MBus_ID, UCHAR Instance);
	push	bx
	push	cx
	call	Find_MBus_ID
	xor     ax, ax

	jmp	ReturnULONG

Sys_Lookup endp






;***********************************************************************
; Implements the SYS_RETURN_RESULT macro
; Returns a byte/word/dword result to the correct context.
; Input:
;   EBX = Result to be returned
;***********************************************************************
Sys_Result proc

	push	OFFSET VSM_Header.SysStuff.State
	push    ebx			; Value
	call    Return_Virtual_Value

	jmp     ExitSysCall

Sys_Result endp


;***********************************************************************
; Implements the SYS_DUPLICATE_VSM macro
; On entry: BX = memory model
;                 Legend:    (C=copy of parent VSM) (N=new copy)
;                    CS    DS    SS
;                0:  N     N     N
;                1:  C     N     N
;***********************************************************************
Sys_Duplicate proc

;	mov	[MemoryModel], bx   ; Ignore parameter for new
	
	; Find end of VSMs in memory
	mov	esi, [Current_VSM]
	mov	ebx, esi
FindLastVSM:
	mov	edi, ebx
	mov	ebx, (VSM_Header PTR fs:[ebx]).SysStuff.Flink
	or	ebx, ebx
	jnz	FindLastVSM

	; EDI points to last VSM in memory.  Find end of last VSM.
	mov	edx, edi
	add	edi, (VSM_Header PTR fs:[edi]).DS_Limit


	; Copy this VSM's image
	mov	ecx, (VSM_Header PTR fs:[esi]).DS_Limit
	cmp	[MemoryModel], 0	; Use same Code segment ?
	je	short CopyImage
	mov	ecx, sizeof(VSM_Header) + VSM_STACK_SIZE
CopyImage:		
	push	edi
	push	esi
	push	ecx
	shr	ecx, 2			; Convert BYTE count to DWORDs
	rep	movsd [edi], es:[esi]
	pop	ecx
	pop	esi
	pop	edi

	; Patch descriptors
	mov	(VSM_Header PTR fs:[edi])._DS.base_15_0, di
	mov	(VSM_Header PTR fs:[edi])._SS.base_15_0, di
	mov	eax, edi
	shr	eax, 16
	mov	(VSM_Header PTR fs:[edi])._DS.base_31_24, ah
	mov	(VSM_Header PTR fs:[edi])._DS.base_23_16, al
	mov	(VSM_Header PTR fs:[edi])._SS.base_31_24, ah
	mov	(VSM_Header PTR fs:[edi])._SS.base_23_16, al

	mov	(VSM_Header PTR fs:[edi])._DS.limit_15_0, cx
	mov	(VSM_Header PTR fs:[edi])._SS.limit_15_0, cx
;	Required for segments > 64K	
;	shr	ecx, 16
;	mov	(VSM_Header PTR fs:[edi])._DS.limit_19_16, cl
;	mov	(VSM_Header PTR fs:[edi])._SS.limit_19_16, cl


	cmp	[MemoryModel], 0	; Use same Code segment ?
	jne	short LinkNewVSM
	mov	(VSM_Header PTR fs:[edi]).SysStuff.State._CS.base, edi

LinkNewVSM:	
	; Link the new VSM into the VSM list
	mov	(VSM_Header PTR fs:[edx]).SysStuff.Flink, edi
	mov	(VSM_Header PTR fs:[edi]).SysStuff.Blink, edx
	mov	(VSM_Header PTR fs:[edi]).SysStuff.Flink, 0

	; Init EIP
	movzx	eax, (VSM_Header PTR fs:[edi]).EntryPoint
	mov	(VSM_Header PTR fs:[edi]).SysStuff.State.Next_EIP, eax

	; Init ESP & create the initial stack frame
	mov	eax, (VSM_Header PTR fs:[edi]).DS_Limit
	sub	ax, VSM_STACK_FRAME
	mov	(VSM_Header PTR fs:[edi]).SysStuff.SavedESP, eax

	; Schedule the new VSM
	push    edi
	call    Schedule_VSM

	jmp     ExitSysCall
	ret			; Gets rid of assembler warning

; Warning: don't make MemoryModel a variable on the stack
; The JMP ExitSysCall bypasses the LEAVE instruction and BP is trashed
MemoryModel	dw	0

Sys_Duplicate endp


;***********************************************************************
; Returns TRUE if the VSM is currently yielded
;***********************************************************************
VSM_Is_Yielded proc Vsm: dword

	mov     ebx, [Vsm]
	mov     al, 0			; return FALSE
	cmp     fs:(VSM_Header PTR [ebx]).SysStuff.RunFlag, RUN_FLAG_WAITING
	jne     short Exit
	mov     al, 1			; return TRUE
Exit:
	ret

VSM_Is_Yielded endp


	END



