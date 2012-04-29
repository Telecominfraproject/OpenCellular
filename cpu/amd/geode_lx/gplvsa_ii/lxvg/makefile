# Copyright (c) 2007-2008 Advanced Micro Devices,Inc. ("AMD").
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

# You should have received a copy of the GNU Lesser General
# Public License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA 02111-1307 USA

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

.SUFFIXES: .asm .c .h .inc .map .obj .mac

INCLUDE		= $(OBJECT)


######################################################################
#
#				Build Macros
#
######################################################################

VGA_C_OBJS	= \
		$(OBJECT)\vsa2.obj \
		$(OBJECT)\init.obj \
		$(OBJECT)\msr.obj \
		$(OBJECT)\decode.obj \
		$(OBJECT)\vgdata.obj \
		$(OBJECT)\lxhwctl.obj \

VGA_ASM_OBJS	= \
		$(OBJECT)\main.obj \
		$(OBJECT)\utils.obj \


VGA_OBJS	= $(VGA_ASM_OBJS)  $(VGA_C_OBJS)

VGA_VSM	= $(OBJECT)\lxvg.vsm

#######################################################################
#
#				Targets
#
#######################################################################


all: setenv
	$(MAKE) lxvga.vsm "CPU=lx"

lxvga.vsm: $(OBJECT) $(VGA_OBJS)
	$(LN) $(LOPTS_VSM) @<<
$(VGA_OBJS:	 =+^
)
$(VGA_VSM)
lxvg.map
$(BUILD_DIR)\obj\$(TOOL_LIB) ;
<<NOKEEP
	$(COPY) $(VGA_VSM) $(BUILDOBJ)

#This and only this clean target must exist as it is called by cleanall
#cleanall and cleanlocal are defined in rules.mak

clean: cleanlocal cleanlib

$(OBJECT):
	-@md $(OBJECT)

#######################################################################
#
#				Dependencies
#
#######################################################################

$(VGA_ASM_OBJS): $(MAKEDIR)\makefile \
		$(OBJECT)\vsa2.inc \

$(VGA_C_OBJS): $(MAKEDIR)\makefile \
		$(BUILD_DIR)\obj\$(TOOL_LIB) \
		$(OBJECT)\vsa2.h \
		$(OBJECT)\isa.h \
		$(OBJECT)\chipset.h \
		$(OBJECT)\vr.h \
		$(OBJECT)\pci.h \
		$(OBJECT)\lxvg.h \
		$(OBJECT)\vgdata.h \

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

{$(MAKEDIR)}.c{$(OBJECT)}.obj:
	$(CC) /AT /W3 /G3s /Gx /Oi /c /I$(OBJECT) $(CDEFS) /Fo$@ $<

{$(MAKEDIR)\$(CPU)}.c{$(OBJECT)}.obj:
	$(CC) /AT /W3 /G3s /Gx /Oi /c /I$(OBJECT) $(CDEFS) /Fo$@ $<

{$(MAKEDIR)}.asm{$(OBJECT)}.obj:
	$(AS) /nologo $(AS_OPTS) $(CDEFS) /Fo$@ $<

{$(MAKEDIR)\$(CPU)}.asm{$(OBJECT)}.obj:
	$(AS) /nologo $(AS_OPTS) $(CDEFS) /Fo$@ $<

{$(MAKEDIR)}.h{$(OBJECT)}.h:
	copy $< $@

{$(USER)\lxvg}.h{$(OBJECT)}.h:
	copy $< $@

{$(USER)\lxvg}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /Zns /C $<

{$(USER)\lxvg}.asm{$(OBJECT)}.obj:
	$(AS) /nologo $(AS_OPTS) $(CDEFS) /Fo$@ $<

{$(USER)\lxvg}.c{$(OBJECT)}.obj:
	$(CC) /AT /W3 /G3s /Gx /Oi /c /I$(OBJECT) $(CDEFS) /Fo$@ $<
