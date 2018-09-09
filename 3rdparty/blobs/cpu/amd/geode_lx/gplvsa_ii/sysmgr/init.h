/*
* Copyright (c) 2006-2008 Advanced Micro Devices,Inc. ("AMD").
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
* License, or (at your option) any later version.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General
* Public License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place, Suite 330,
* Boston, MA 02111-1307 USA 
*/

#define	CR	0x0D
#define	LF	0x0A

#define	ERR_NO_FIT	(1 << 0)
#define	ERR_NO_SYS_MGR	(1 << 1)
#define	ERR_BAD_TYPE	(1 << 2)
#define	ERR_SMI_STATUS	(1 << 3)
#define	ERR_BAD_CPU		(1 << 4)
#define	ERR_NO_CHIPSET	(1 << 5)
#define	ERR_NOT_CPL0	(1 << 6)
#define	ERR_VERIFY		(1 << 7)
#define	ERR_VERSION		(1 << 8)
#define	ERR_INTERNAL	(1 << 9)
#define	ERR_INVALID_VSM	(1 << 10)



#define	DOS_LOAD		(1 << 0)
#define	VSM_LOAD		(1 << 1)

