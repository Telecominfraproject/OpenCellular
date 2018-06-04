;/*
;* Copyright (c) 2007-2008 Advanced Micro Devices,Inc. ("AMD").
;*
;* This library is free software; you can redistribute it and/or modify
;* it under the terms of the GNU Lesser General Public License as
;* published by the Free Software Foundation; either version 2.1 of the
;* License, or (at your option) any later version.
;*
;* This code is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
;* Lesser General Public License for more details.
;
;* You should have received a copy of the GNU Lesser General
;* Public License along with this library; if not, write to the
;* Free Software Foundation, Inc., 59 Temple Place, Suite 330,
;* Boston, MA 02111-1307 USA
;*/


.MODEL TINY,c
.CODE
.586p

;----------------------------------------------------------------------------
; BASE ADDRESSES
; These global variables are used to access the memory mapped regions.
; VSA II maintains a flat 4 Gig selector in FS.
;----------------------------------------------------------------------------

extrn framebuffer_base:dword
extrn GPregister_base:dword
extrn VGregister_base:dword
extrn DFregister_base:dword
extrn VIPregister_base:dword



;----------------------------------------------------------------------------
; READ_FB_32: Returns a 32-bit value from the frame buffer.
;
; Parameter specifies 32-bit offset.
;----------------------------------------------------------------------------

read_fb_32	proc c	address: dword

	mov		ebx, [address]
	add		ebx, [framebuffer_base]
	mov		eax, fs:[ebx]
	mov		edx, eax
	shr		edx, 16
	ret

read_fb_32	endp

;----------------------------------------------------------------------------
; WRITE_FB_32: Writes a 32-bit value to the frame buffer.
;
; Parameters specify 32-bit offset and 32-bit data value.
;----------------------------------------------------------------------------

write_fb_32 proc c	address: dword, data: dword

	mov		ebx, [address]
	add		ebx, [framebuffer_base]
	mov		eax, [data]
	mov		fs:[ebx], eax
	ret

write_fb_32 endp


;----------------------------------------------------------------------------
; READ_GP_32: Returns 32-bit value from the graphics processor register space.
;
; Parameter specifies 32-bit offset.
;----------------------------------------------------------------------------

read_gp_32	proc c	address: dword

	mov		ebx, [address]
	add		ebx, [GPregister_base]
	mov		eax, fs:[ebx]
	mov		edx, eax
	shr		edx, 16
	ret

read_gp_32	endp

;----------------------------------------------------------------------------
; WRITE_GP_32: Writes 32-bit value to graphics processor register space.
;
; Parameters specify 32-bit offset and 32-bit data value.
;----------------------------------------------------------------------------

write_gp_32 proc c	address: dword, data: dword

	mov		ebx, [address]
	add		ebx, [GPregister_base]
	mov		eax, [data]
	mov		fs:[ebx], eax
	ret

write_gp_32 endp


;----------------------------------------------------------------------------
; READ_VG_8: Returns 8-bit value from the video generator register space.
;
; Parameter specifies 32-bit offset.
;----------------------------------------------------------------------------

read_vg_8	proc c	address: dword

	mov		ebx, [address]
	add		ebx, [VGregister_base]
	mov		al, fs:[ebx]
	ret

read_vg_8	endp

;----------------------------------------------------------------------------
; WRITE_VG_8: Writes 8-bit value to video generator register space.
;
; Parameters specify 32-bit offset and 8-bit data value.
;----------------------------------------------------------------------------

write_vg_8 proc c  address: dword, data: byte

	mov		ebx, [address]
	add		ebx, [VGregister_base]
	mov		al, [data]
	mov		fs:[ebx], al
	ret

write_vg_8 endp


;----------------------------------------------------------------------------
; READ_VG_32: Returns 32-bit value from the video generator register space.
;
; Parameter specifies 32-bit offset.
;----------------------------------------------------------------------------

read_vg_32	proc c	address: dword

	mov		ebx, [address]
	add		ebx, [VGregister_base]
	mov		eax, fs:[ebx]
	mov		edx, eax
	shr		edx, 16
	ret

read_vg_32	endp

;----------------------------------------------------------------------------
; WRITE_VG_32: Writes 32-bit value to video generator register space.
;
; Parameters specify 32-bit offset and 32-bit data value.
;----------------------------------------------------------------------------

write_vg_32 proc c	address: dword, data: dword

	mov		ebx, [address]
	add		ebx, [VGregister_base]
	mov		eax, [data]
	mov		fs:[ebx], eax
	ret

write_vg_32 endp


;----------------------------------------------------------------------------
; READ_DF_32: Returns 32-bit value from the display filter register space.
;
; Parameter specifies 32-bit offset.
;----------------------------------------------------------------------------

read_df_32	proc c	address: dword

	mov		ebx, [address]
	add		ebx, [DFregister_base]
	mov		eax, fs:[ebx]
	mov		edx, eax
	shr		edx, 16
	ret

read_df_32	endp

;----------------------------------------------------------------------------
; WRITE_DF_32: Writes 32-bit value to display filter register space.
;
; Parameters specify 32-bit offset and 32-bit data value.
;----------------------------------------------------------------------------

write_df_32 proc c	address: dword, data: dword

	mov		ebx, [address]
	add		ebx, [DFregister_base]
	mov		eax, [data]
	mov		fs:[ebx], eax
	ret

write_df_32 endp


;----------------------------------------------------------------------------
; READ_VIP_32: Returns 32-bit value from the video input port register space.
;
; Parameter specifies 32-bit offset.
;----------------------------------------------------------------------------

read_vip_32	proc c	address: dword

	mov		ebx, [address]
	add		ebx, [VIPregister_base]
	mov		eax, fs:[ebx]
	mov		edx, eax
	shr		edx, 16
	ret

read_vip_32	endp

;----------------------------------------------------------------------------
; WRITE_VIP_32: Writes 32-bit value to video input port register space.
;
; Parameters specify 32-bit offset and 32-bit data value.
;----------------------------------------------------------------------------

write_vip_32 proc c	 address: dword, data: dword

	mov		ebx, [address]
	add		ebx, [VIPregister_base]
	mov		eax, [data]
	mov		fs:[ebx], eax
	ret

write_vip_32 endp


asmRead		proc public uses bx eax ecx edx, msrReg:word, msrAddr:dword, ptrHigh:word, ptrLow:word
	mov		ecx, msrAddr
	mov		cx, msrReg
	RDMSR

	mov		bx, ptrHigh
	mov		[bx], edx
	mov		bx, ptrLow
	mov		[bx], eax

	ret

asmRead		endp


asmWrite	proc public uses bx eax ecx edx, msrReg:word, msrAddr:dword, ptrHigh:word, ptrLow:word

	mov		ecx, msrAddr
	mov		cx, msrReg

	mov		bx, ptrHigh
	mov		edx, [bx]
	mov		bx, ptrLow
	mov		eax, [bx]

	WRMSR
	ret

asmWrite	endp

END
