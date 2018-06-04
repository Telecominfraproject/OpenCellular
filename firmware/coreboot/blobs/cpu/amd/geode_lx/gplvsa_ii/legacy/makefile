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
#
######################################################################
#
#                               Init variables
#
######################################################################
.SUFFIXES: .asm .c .h .inc .map .obj .mac

!ifndef VSA2ROOT
VSA2ROOT        = ..
!endif

BUILD_DIR       = $(VSA2ROOT)\build
OBJECT		= obj
!include $(BUILD_DIR)\setvars.mak

INCLUDE		= $(OBJECT);$(INCLUDE)

VSMNAME	= legacy


######################################################################
#
#                               Build Macros
#
######################################################################

LEGACY_ASM_OBJS = \
	$(OBJECT)\header.obj \
	$(OBJECT)\msr.obj \

LEGACY_C_OBJS	= \
	$(OBJECT)\legacy.obj \
	$(OBJECT)\init.obj \
	$(OBJECT)\events.obj \
	$(OBJECT)\flash.obj \
	$(OBJECT)\cs5536.obj \
	$(OBJECT)\ide.obj \
	$(OBJECT)\blockio.obj \
	$(OBJECT)\uarts.obj \
	$(OBJECT)\virtregs.obj \
	$(OBJECT)\swapsif.obj \
	$(OBJECT)\sysinfo.obj \

LEGACY_LNK_OBJS	= \
	header.obj  \
	legacy.obj \
	msr.obj \
	init.obj \
	events.obj \
	flash.obj \
	cs5536.obj \
	ide.obj \
	blockio.obj \
	uarts.obj \
	virtregs.obj \
	swapsif.obj \
	sysinfo.obj \


LEGACY_OBJS = $(LEGACY_ASM_OBJS) $(LEGACY_C_OBJS) 
LEGACY_VSM	= legacy.vsm

#######################################################################
#
#                               Targets
#
#######################################################################

all: $(OBJECT) setenv legacy.vsm
	$(COPY) $(LEGACY_VSM) $(BUILDOBJ)

legacy.vsm: $(LEGACY_OBJS)
   cd $(OBJECT)
	$(LN) $(LOPTS_VSM) $(LEGACY_LNK_OBJS), ..\$(LEGACY_VSM),, ..\..\build\obj\$(TOOL_LIB);
	cd ..

#This and only this clean target must exist as it is called by cleanall
#cleanall and cleanlocal are defined in rules.mak

clean: cleanlocal cleanlib

$(OBJECT):
	-@md $(OBJECT)

#######################################################################
#
#                               Dependencies
#
#######################################################################

$(LEGACY_ASM_OBJS): $(MAKEDIR)\makefile \
		$(OBJECT)\sysmgr.inc \
		$(OBJECT)\smimac.mac \
		$(OBJECT)\vsa2.inc \
		$(OBJECT)\isa.inc \
		$(OBJECT)\chipset.inc \
		$(OBJECT)\vr.inc \
		$(OBJECT)\pci.inc \
		$(OBJECT)\acpi.inc \
		$(OBJECT)\gx2.inc \
		$(OBJECT)\cs5536.inc \
		$(OBJECT)\pci.inc \
		$(OBJECT)\mdd.inc \
		
$(LEGACY_C_OBJS): $(MAKEDIR)\makefile \
		$(BUILD_DIR)\obj\$(TOOL_LIB) \
		$(OBJECT)\sysmgr.h \
		$(OBJECT)\vsa2.h \
		$(OBJECT)\isa.h \
		$(OBJECT)\chipset.h  \
		$(OBJECT)\vr.h \
		$(OBJECT)\pci.h \
		$(OBJECT)\legacy.h \
		$(OBJECT)\mapper.h \
		$(OBJECT)\protos.h \
		$(OBJECT)\mdd.h \
		$(OBJECT)\acpi.h \
		$(OBJECT)\gx2.h \
		$(OBJECT)\cs5536.h \
		
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
