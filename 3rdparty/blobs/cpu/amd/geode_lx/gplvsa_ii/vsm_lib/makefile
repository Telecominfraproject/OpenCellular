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
######################################################################
#
#				Init variables
#
######################################################################
!ifndef VSA2ROOT
VSA2ROOT	= ..
!endif

BUILD_DIR	= $(VSA2ROOT)\build
OBJECT		= obj
!include $(BUILD_DIR)\setvars.mak
.SUFFIXES: .asm .h .inc .obj .mac

INCLUDE		= $(OBJECT);$(INCLUDE)

VSMNAME	= vsm_lib


######################################################################
#
#				Build Macros
#
######################################################################

VSMUTILS_ASM_OBJS	= \
	$(OBJECT)\vsa2util.obj \
	$(OBJECT)\pci.obj \
	$(OBJECT)\descr.obj \
	$(OBJECT)\utils.obj \
	$(OBJECT)\misc.obj \
	$(OBJECT)\message.obj \
	$(OBJECT)\virtual.obj \
	$(OBJECT)\irq.obj \
	$(OBJECT)\regs.obj \
	$(OBJECT)\sysinfo.obj \
	$(OBJECT)\sw_int.obj \
	$(OBJECT)\resource.obj \
	$(OBJECT)\critical.obj \
	$(OBJECT)\yield.obj \
	$(OBJECT)\hex.obj \
	$(OBJECT)\present.obj \
	$(OBJECT)\msrs.obj \

#######################################################################
#
#				Targets
#
#######################################################################

all: $(OBJECT) setenv tools.lib
!if !exist ($(BUILD_DIR)\obj)	
	md $(BUILD_DIR)\obj
!endif
	$(COPY) $(OBJECT)\tools.lib $(BUILD_DIR)\obj

tools.lib: $(VSMUTILS_ASM_OBJS)

#This and only this clean target must exist as it is called by cleanall
#cleanall and cleanlocal are defined in rules.mak

clean: cleanlocal
	-@IF EXIST $(BUILD_DIR)\obj\tools.lib $(DEL) $(BUILD_DIR)\obj\tools.lib
	-@IF NOT EXIST $(BUILD_DIR)\obj\*.* rd $(BUILD_DIR)\obj

$(OBJECT):
	-@md $(OBJECT)

#######################################################################
#
#				Dependencies
#
#######################################################################

$(VSMUTILS_ASM_OBJS): $(MAKEDIR)\makefile\
		$(OBJECT)\sysmgr.inc \
		$(OBJECT)\smimac.mac \
		$(OBJECT)\vsa2.inc\
		$(OBJECT)\isa.inc \
		$(OBJECT)\gx2.inc \
		$(OBJECT)\pci.inc \

######################################################################
#
#				Common Targets
#
######################################################################
# include common targets and inference rules
!include $(BUILD_DIR)\rules.mak

######################################################################
#
#				Inference Rules
#
######################################################################
# Override common inference rules here

{$(INC_DIR)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /Zni /C $<

{$(INC_DIR)\$(CPU)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /Zni /C $<

{$(SYSMGR_SRC)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /Zns /C $<

{$(SYSMGR_SRC)\$(CPU)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /Zns /C $<

{$(MAKEDIR)}.asm{$(OBJECT)}.obj:
	$(AS) /nologo $(AS_OPTS) /Fo$@ $< 
	$(LB) $(OBJECT)\tools.lib -+$@;

{$(MAKEDIR)\$(CPU)}.asm{$(OBJECT)}.obj:
	$(AS) /nologo $(AS_OPTS) /Fo$@ $< 
	$(LB) $(OBJECT)\tools.lib -+$@;
