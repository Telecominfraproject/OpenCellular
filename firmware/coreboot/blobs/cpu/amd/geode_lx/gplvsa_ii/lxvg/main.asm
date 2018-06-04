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

;*     This file contains the VSM header for the LXVG VSM  


.model tiny,c
.486
.CODE

include VSA2.INC

externdef edata:proc
externdef _end:proc
externdef vsa2_message_loop:proc

public  VSM_Hdr


VSM_Hdr:

	dd	VSM_SIGNATURE		; VSM signature
	db	VSM_VGA			; VSM type
	db	0FFh			; Any CPU
	dw	0FFFFh			; Any Chipset
	dw	0101h			; VSM version 01.01
	dd	OFFSET edata		; Size of VSM module
	dw	OFFSET vsa2_message_loop; EntryPoint
	dd	OFFSET _end		; DS Limit
	dw	0000h			; Requirements
	dw	VSA_VERSION		; VSA version
	db	sizeof(VSM_Header) - ($-VSM_Hdr) dup (0)


	END	VSM_Hdr




