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
;*     This file contains hex debug routines   


.model tiny,c
.586p
.CODE





Hex_32	proc	pascal Num:dword

	pushad
	mov	ebx, [Num]
	mov	cx, 8
@@:	rol	ebx, 4
	call	Hex_4
	loop	@b
	call	Space
	popad
	ret

Hex_32	endp



Hex_16	proc	pascal Num:word

	pusha
	mov	cx, 4
	mov	bx, [Num]
@@:	rol	bx, 4
	call	Hex_4
	loop	@b
	call	Space
	popa
	ret

Hex_16	endp


Hex_8	proc	pascal Num:byte

	pusha
	mov	cx, 2
	mov	bl, [Num]
@@:	rol	bl, 4
	call	Hex_4
	loop	@b
	call	Space
	popa
	ret

Hex_8	endp


Hex_4:	mov	al, bl
	and	al, 0Fh
	add	al, '0'			; Convert to ASCII
	cmp	al, '9'
	jbe	Char
	add	al, 7			; 'A'-'F'
Char:	mov	dx, 84h
	out	dx, al
	in	al, 80h
	ret

Space:	mov	al, ' '
	jmp	Char




	END 

