#
# Copyright (c) 2006 Advanced Micro Devices,Inc. ("AMD").
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# This code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General
# Public License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA 02111-1307 USA 
#

##############################################################################
#
#	Directories
#
##############################################################################
BUILD_DIR		= $(VSA2ROOT)\build
SYSMGR_SRC		= $(VSA2ROOT)\sysmgr
VSMUTILS_SRC	= $(VSA2ROOT)\vsm_lib
LEGACY_SRC		= $(VSA2ROOT)\legacy
INC_DIR		= $(VSA2ROOT)\inc
H_DIR			= $(VSA2ROOT)\sysmgr

SHELL			=

##############################################################################
#
#	Tools / Options for tools
#
##############################################################################
ECHO		= echo
COPY		= copy
BINCOPY	= copy /b
MOVE		= move
DEL		= del
REN		= ren
SETENV	= set
CD		= cd
AS		= ml
CC		= cl
H2		= h2inc
LN		= link
LB		= lib
X2ROM = exe2bin

AS_OPTS	= /c /Cx /Sa /W3 $(ALIST) /I$(OBJECT)
CC_OPTS	= /c /AT /Gs /FPi87 /G3fsy /W3 /Fc$(OBJECT)\ /I$(OBJECT) $(COPTS_OPT) $(CLIST)

COPTS_OPT	= /Ow /W3 
LOPTS_OPT	= /NONULLS /nologo /MAP
LOPTS_SYS	= /MAP /TINY /nologo
LOPTS_VSM	= /MAP /TINY /nologo

TOOL_LIB	= tools.lib
HEAD_LIB	= header.lib


