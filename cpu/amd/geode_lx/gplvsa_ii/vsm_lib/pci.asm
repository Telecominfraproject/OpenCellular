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
;*     This file contains PCI related library functions.   
;*******************************************************************************


include sysmgr.inc
include pci.inc
include vsa2.inc
include gx2.inc

.model tiny,c
.586p
.CODE




externdef sys_system_call:proc





;**********************************************************************
; Writes the PCI Config Address
; Input:
;   EAX = config address
; Output:
;    DX = points to 0xCFC-0xCFF
;**********************************************************************
PCI_Common  proc

	mov	dx, PCI_CONFIG_ADDRESS
	ror	eax, 16			; Set 16 MSBs to 8000h
	mov	ax, 8000h
	ror	eax, 16
	out	dx, eax
	add	dl, 4			; PCI_CONFIG_DATA + 2 LSBs
	and	al, 3
	add	dl, al
	ret

PCI_Common endp



;**********************************************************************
; Reads a byte PCI register
;**********************************************************************
read_PCI_byte proc pascal PCI_Config_Addr:DWORD

	mov	eax, [PCI_Config_Addr]
	call	PCI_Common
	in	al, dx
	ret

read_PCI_byte endp


;**********************************************************************
; Reads a word PCI register
;**********************************************************************
read_PCI_word proc pascal \
	PCI_Config_Addr:DWORD

	mov	eax, [PCI_Config_Addr]
	call	PCI_Common
	in	ax, dx
	ret

read_PCI_word endp


;**********************************************************************
; Reads a dword PCI register
;**********************************************************************
read_PCI_dword proc pascal \
	PCI_Config_Addr:DWORD

	mov	eax, [PCI_Config_Addr]
	call	PCI_Common
	in	eax, dx
	mov	edx, eax
	shr	edx, 16
	ret

read_PCI_dword endp


;**********************************************************************
; Writes a byte PCI register
;**********************************************************************
write_PCI_byte proc pascal \
	PCI_Config_Addr: DWORD, \
	Data: BYTE

	mov	eax, [PCI_Config_Addr]
	call	PCI_Common
	mov	al, [Data]
	out	dx, al
	ret

write_PCI_byte endp


;**********************************************************************
; Writes a word PCI register
;**********************************************************************
write_PCI_word proc pascal \
	PCI_Config_Addr:DWORD, \
	Data:WORD

	mov	eax, [PCI_Config_Addr]
	call	PCI_Common
	mov	ax, [Data]
	out	dx, ax
	ret

write_PCI_word endp


;**********************************************************************
; Writes a dword PCI register
;**********************************************************************
write_PCI_dword proc pascal \
	PCI_Config_Addr: DWORD, \
	Data:            DWORD

	mov	eax, [PCI_Config_Addr]
	call	PCI_Common
	mov	eax, [Data]
	out	dx, eax
	ret

write_PCI_dword endp




;**********************************************************************
; Disables PCI trapping (except virtualized PCI devices)
;**********************************************************************
Disable_PCI_Traps proc

	; Has MPCI NB MSR been determined yet ?
	mov	ecx, [PBus_Addr]
	or	ecx, ecx
	jnz	GotPbus

	; No, get routing address for MPCI NB
	push	bx
	push	ID_MPCI		; MBus ID
	push	1		; Instance
	call	sys_lookup_device
	pop	bx

	mov	cx, dx
	shl	ecx, 16
	mov	cx, MPCI_PBUS
	mov	[PBus_Addr], ecx

	; Compute mask for virtual Southbridge
	mov     ax, word ptr (VSM_Header PTR ds:[0]).SysStuff.Southbridge
	shr	ah, 3
	mov	cl, ah
	mov	eax, 1
	shl	eax, cl
	or	al, 2			; Assume Northbridge is at 0x80000800
	mov	[PBus_Mask], eax
GotPbus:
	rdmsr
	mov	[PBus_Hi], edx		; Save original value for Restore_PCI_Traps
	mov	[PBus_Lo], eax
	and	eax, [PBus_Mask]	; Disable all except virtual NB & SB
	wrmsr
	ret

Disable_PCI_Traps endp

;**********************************************************************
; Restores PCI trapping
;**********************************************************************
Restore_PCI_Traps proc uses eax edx
       
	mov	edx, [PBus_Hi]
	mov	eax, [PBus_Lo]
	wrmsr
	ret

Restore_PCI_Traps endp


PBus_Addr    dd	0
PBus_Hi      dd	0
PBus_Lo      dd	0
PBus_Mask    dd	0

;**********************************************************************
; Performs a PCI configuration write cycle sans any PCI trapping except
; for virtualized PCI devices.
;**********************************************************************
write_PCI_no_trap proc pascal uses si \
 	PCI_Config_Addr:DWORD, \
 	Data:           DWORD, \
 	DataSize:       WORD

	mov	ebx, [PCI_Config_Addr]
	mov	si, [DataSize]

	; Disable PCI trapping on all but virtualized PCI devices
	call	Disable_PCI_Traps
	
	mov	eax, ebx
	call	PCI_Common
	mov	eax, [Data]
	cmp	si, BYTE_IO
	jne	short NotByte
	out	dx, al
	jmp	short RestoreTraps

NotByte:
	cmp	si, WORD_IO
	je	short WordWrite
	db	66h			; Make the following instruction an OUT DX,EAX
WordWrite:
	out	dx, ax
RestoreTraps:
	call	Restore_PCI_Traps	; Restore PCI trapping
Exit:	ret

write_PCI_no_trap endp


;**********************************************************************
; Performs a PCI configuration read cycle sans any PCI trapping except
; for virtualized PCI devices.
;**********************************************************************
read_PCI_no_trap proc pascal uses si\
	PCI_Config_Addr: DWORD, \
	DataSize:        WORD

	mov	ebx, [PCI_Config_Addr]
	mov	si, [DataSize]


	; Disable PCI trapping on all but virtualized PCI devices
	call	Disable_PCI_Traps
	
	mov	eax, ebx		; Output PCI address
	call	PCI_Common

	cmp	si, BYTE_IO
	jne	short NotByte
	in	al, dx
	jmp	short RestoreTraps

NotByte:
	cmp	si, WORD_IO
	je	short WordRead
	db	66h			; Makes the following instruction an IN EAX,DX
WordRead:
	in	ax, dx

RestoreTraps:
	push	eax
	call	Restore_PCI_Traps	; Restore PCI trapping
	pop	eax

	mov	edx, eax
	shr	edx, 16
Exit:	ret

read_PCI_no_trap endp



	END 

