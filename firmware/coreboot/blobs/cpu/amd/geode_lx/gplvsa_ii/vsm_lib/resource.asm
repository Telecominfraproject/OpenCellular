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
;*     This file implements the resource-related macros:
;*          SYS_MBUS_DESCRIPTOR
;*          SYS_IO_DESCRIPTOR
;*          SYS_ALLOCATE_RESOURCE
;*          SYS_SET_DECODE    
;*******************************************************************************


include sysmgr.inc
.model tiny,c
.586p
.CODE



externdef sys_system_call:proc



;***********************************************************************
; ULONG sys_mbus_descriptor(USHORT Address, ULONG * Buffer, UCHAR IO_Flag);
;
; Returns the MSR address & contents of the descriptor assigned to a
; virtualized PCI address or an I/O address.
;
;***********************************************************************
sys_mbus_descriptor proc pascal   \
	Address: WORD, \
 	Buffer: PTR,   \
	IO_Flag: WORD
	
	mov	bx, [Address]
	mov	cx, [IO_Flag]
	mov	ax, SYS_CODE_DESCRIPTOR
	call	sys_system_call	
	; Returns:
	;   ECX = MSR address
	;   EDX:EAX = original MSR value

	mov	bx, [Buffer]		; Store MSR value into caller's buffer
	mov	[bx+0], eax
	mov	[bx+4], edx

	push	ecx			; Return MSR address
	pop	ax
	pop	dx
	ret

sys_mbus_descriptor endp




;***********************************************************************
; ULONG sys_resource(USHORT Resource, ULONG Param1, ULONG Param2);
;
; Records a VSM's use of a non-shareable resource (e.g. GPIO, IRQ, etc.)
; If the resources is a memory or I/O PCI BAR, it returns the assigned
; PCI address.
;
;***********************************************************************
sys_resource proc pascal  uses edi si \
	Resource:   BYTE,  \
	BAR:        WORD,  \
	BaseRange:  DWORD, \
	Pci_ID:     WORD,  \
	MBus_ID:    WORD

	mov	bl, [Resource]
	mov	si, [BAR]
	mov	ecx, [BaseRange]
	mov	di, [MBus_ID]
	shl	edi, 16
	mov	di, [Pci_ID]
	mov	ax, SYS_CODE_RESOURCE
	call	sys_system_call

	; If xxxx != 0x0000, then return 0x8000xxxx, where xxxx is return value
	xor	dx, dx	
	or	ax, ax
	jz	Exit
	or	dh, 80h
Exit:	ret

sys_resource endp




;***********************************************************************
; ULONG __pascal sys_lookup_device(USHORT DeviceID, USHORT Instance);
; Returns the routing bits for the specified Device ID.
;***********************************************************************
sys_lookup_device proc pascal \
	Device_ID: word, \
	Instance:  word

	mov	bx, [Device_ID]
	mov	cx, [Instance]
	mov	ax, SYS_CODE_LOOKUP
	call	sys_system_call
	ret

sys_lookup_device endp




;***********************************************************************
;void Set_Address_Decode(USHORT Address, USHORT Decode) 
;***********************************************************************
sys_address_decode proc pascal \
	Address:   WORD, \
	Decode: WORD

	mov	bx, [Address]
	mov	cx, [Decode]
	mov	ax, SYS_CODE_DECODE
	call	sys_system_call
	ret

sys_address_decode endp



	END 

