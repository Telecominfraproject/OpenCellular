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
;********************************************************************************
;*     This file contains the Southbridge specific code.  
;********************************************************************************




include smimac.mac
include chipset.inc
include pci.inc
include sysmgr.inc
include init.inc
include cs5536.inc


.model small,c
.586
.CODE


externdef Errors:	word
externdef Device_ID:	word
externdef Chipset_Base:	dword

;***********************************************************************
; Clear all SMI source registers
;***********************************************************************
Clear_SMIs proc

	mov	ax, [Device_ID]		; Get Southbridge Device ID
	cmp	ax, DEVICE_ID_5536	; If CS5536, exit
	clc
	je	Exit
	
	or	[Errors], ERR_NO_CHIPSET
	stc
	jmp	Exit
	
Exit:	ret

Clear_SMIs endp




;****************************************************************
; Returns information about the Southbridge
;
; On Exit, if CF clear:
;   EBX = PCI address of Southbridge
;    CL = Chipset Revision
;    DX = Device ID
;****************************************************************
Get_Sbridge_Info proc

	mov	ebx, 80000000h+(15 SHL 11) ; Start at Bus 0;  DevNum 15
SouthbridgeLoop:
	call	Read_PCI32
	cmp	ax, VENDOR_ID_CYRIX	; Is it a Cyrix chipset ?
	je	short CorrectVendorID
	cmp	ax, VENDOR_ID_NATIONAL	; Is it a National Semiconductor chipset ?
	je	short CorrectVendorID
	cmp	ax, VENDOR_ID_AMD	; Is it an AMD chipset ?
	jne	short NextDevice
CorrectVendorID:
	shr	eax, 16
	push	ax			; Save device ID

	mov	bl, 8			; Read Class Code & Rev ID
	call	Read_PCI32
	mov	cl, al			; Save revision in CL
	shr	eax, 16	
	cmp	ax, 0601h		; Is it an ISA bridge ?
	pop	dx
	mov	bl, 0			; Vendor & Device ID
	je	Exit			; CF is cleared 


	; Check the DeviceID
	cmp	dx, DEVICE_ID_5536-1	; Hardware CS5536 Device ID
	jne	NextDevice
FoundSB:
	; On CS5536, the virtualized Device ID is h/w Device ID + 1
	inc	dx
	push	cx
	push	dx
	mov	ecx, 5100002Fh		; Write-post I/O to port 84h for debug
	mov	edx, 00084001h
	mov	eax, edx
	or	al, 8
	wrmsr
	pop	dx
	pop	cx
	jmp	Exit

NextDevice:
	add	bh, 8			; Next device
	cmp	bh, (19 SHL 3)		; Last possible DevNum ?
	jbe	SouthbridgeLoop

	; Can't find supported Southbridge chipset
	or	[Errors], ERR_NO_CHIPSET
	stc
	
Exit:	ret

Get_Sbridge_Info endp


;****************************************************************
; Generates a s/w SMI
;****************************************************************
Software_SMI proc

	smint
	ret
	
Software_SMI endp	


;*********************************************************************************
; Reads 32-bit PCI config register specified by EBX
;*********************************************************************************
Read_PCI32 proc

	mov	dx, PCI_CONFIG_ADDRESS
	mov	eax, ebx
	out	dx, eax
	mov 	dx, PCI_CONFIG_DATA
	in	eax, dx
	ret

Read_PCI32 endp




;*********************************************************************************
; Writes a 32 bit Southbridge register
; Input:
;   DL - register offset
;  EAX - value to write
;*********************************************************************************
SetReg_32 proc uses bx

	push	eax
	mov	bl, dl
	call	Address8
	pop	eax
	out	dx, eax
	ret

SetReg_32 endp


;*********************************************************************************
; Writes an 8 bit register
; Input:
;   BL = register #
;   BH = Data
;*********************************************************************************
SetReg_8 proc

	call	Address8
	mov	al, bh
	out	dx, al
	ret

SetReg_8 endp



;*********************************************************************************
; Reads an 8 bit register
; Input:
;   BL = register #
; Output:
;   AL = Data
;*********************************************************************************
GetReg_8 proc

	call	Address8
	in	al, dx
	ret

GetReg_8 endp

;*********************************************************************************
; Helper routine for SetReg_32, SetReg_8 & GetReg_8
;*********************************************************************************
Address8 proc

	mov	dx, PCI_CONFIG_ADDRESS
	mov	eax, [Chipset_Base]
	mov	al, bl
	and	al, NOT 3
	out	dx, eax
	mov	dx, PCI_CONFIG_DATA
	and	bl, 3
	add	dl, bl
	ret

Address8 endp





;*********************************************************************************
; Determines the base address of the top-level SMI status register
; On Exit:
;   EAX = address
;*********************************************************************************
Get_SMI_Base proc
	
	mov	ebx, [Chipset_Base]		; Patch SMI_Base
	add	bx, PM_FUNCTION + BAR0
	call	Read_PCI32
	mov	al, SMI_STATUS
	ret

Get_SMI_Base endp	
	


;*********************************************************************************
; Determines the base address of the internal IRQ register
; On Exit:
;   EAX = address
;*********************************************************************************
Get_IRQ_Base proc
	
	mov	ebx, [Chipset_Base]		; Patch SMI_Base
	add	bx, AUDIO_FUNCTION + BAR0	; Patch IRQ_Base
	call	Read_PCI32
	mov	al, 1Ah
	ret

Get_IRQ_Base endp



	END
	