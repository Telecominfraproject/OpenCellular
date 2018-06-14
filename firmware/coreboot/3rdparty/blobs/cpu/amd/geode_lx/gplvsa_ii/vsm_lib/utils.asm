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
;*     This file contains generic VSM library functions.

.model tiny,c
.586p
.CODE






;***********************************************************************
; UCHAR in_8(USHORT io_port)
;***********************************************************************
in_8 proc  pascal io_port:word

	mov	dx, io_port
	in	al, dx
	ret
   
in_8 endp


;***********************************************************************
; void out_8(USHORT io_port, UCHAR io_data)
;***********************************************************************
out_8	proc pascal \
	io_port:WORD, \
	io_data:BYTE

	mov	dx, io_port
	mov	al, io_data
	out	dx, al
	ret

out_8	endp

;***********************************************************************
; USHORT in_16(USHORT io_port)
;***********************************************************************
in_16 proc  pascal, io_port:WORD

	mov	dx, io_port
	in	ax, dx
	ret
   
in_16 endp


;***********************************************************************
; void out_16(USHORT io_port, USHORT io_data)
;***********************************************************************
out_16	proc pascal \
	io_port:WORD, \
	io_data:WORD

	mov	dx, io_port
	mov	ax, io_data
	out	dx, ax
	ret

out_16	endp



;***********************************************************************
; ULONG in_32(USHORT io_port)
;***********************************************************************
in_32	proc   pascal	io_port: WORD

	mov	dx, io_port
	in	eax, dx
	mov	edx, eax
	shr	edx, 16
	ret

in_32	endp


;***********************************************************************
;
; void out_32(USHORT io_port, ULONG io_data)
;***********************************************************************
out_32	proc   pascal \
	io_port:WORD, \
	io_data:DWORD

	mov	dx, io_port
	mov	eax, io_data
	out	dx, eax
	ret

out_32	endp







write_flat proc pascal \
	Address: DWORD,\
	Data:    DWORD

	mov	ebx, [Address]
	mov	eax, [Data]
	mov	fs:[ebx], eax
	ret

write_flat endp


read_flat proc pascal \
	Address: DWORD

	mov	ebx, [Address]
	mov	eax, fs:[ebx]
	mov	edx, eax
	shr	edx, 16
	ret

read_flat endp


write_flat_word proc pascal \
	Address: DWORD,\
	Data:    WORD

	mov	ebx, [Address]
	mov	ax, [Data]
	mov	fs:[ebx], ax
	ret

write_flat_word endp


read_flat_word proc pascal \
	Address: DWORD

	mov	ebx, [Address]
	mov	ax, fs:[ebx]
	ret

read_flat_word endp



write_flat_byte proc pascal \
	Address: DWORD,\
	Data:    BYTE

	mov	ebx, [Address]
	mov	al, [Data]
	mov	fs:[ebx], al
	ret

write_flat_byte endp


read_flat_byte proc pascal \
	Address: DWORD

	mov	ebx, [Address]
	mov	al, fs:[ebx]
	ret

read_flat_byte endp




read_timestamp proc

	RDTSC
	mov	edx, eax
	shr	edx, 16
	ret

read_timestamp endp



	END 

