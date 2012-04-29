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
;*     Implementation of routines specific to the Vail core. 


.model tiny,c
.586p
.ALPHA
DGROUP	GROUP _CODE, _TEXT
_CODE	SEGMENT PUBLIC use16 'CODE'
	ASSUME DS:_CODE









;************************************************************************
;************************************************************************
Sample_SMI_Pin proc
	ret
Sample_SMI_Pin endp



;************************************************************************
;************************************************************************
Clear_SMI_Pin proc

	ret
Clear_SMI_Pin endp




_CODE	ENDS


	END
