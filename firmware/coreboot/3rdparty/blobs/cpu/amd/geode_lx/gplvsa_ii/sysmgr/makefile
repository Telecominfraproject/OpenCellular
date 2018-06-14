
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
.SUFFIXES: .asm .c .h .inc .map .obj .mac

INCLUDE		= $(OBJECT);$(INCLUDE)

VSMNAME	= sysmgr


######################################################################
#
# 				Build Macros
#
######################################################################

INIT_ASM_OBJS	= \
	$(OBJECT)\init.obj \
	$(OBJECT)\cpu_init.obj \
	$(OBJECT)\chip.obj \
	$(OBJECT)\image.obj \

SYSMGR_ASM_OBJS	= \
	$(OBJECT)\sysmgr.obj \
	$(OBJECT)\bugs.obj \
	$(OBJECT)\utils.obj \
	$(OBJECT)\syscalls.obj \
	$(OBJECT)\smis.obj \
	$(OBJECT)\port92.obj \
	$(OBJECT)\sw_int.obj \
	$(OBJECT)\debug.obj \
	$(OBJECT)\vr_misc.obj \
	$(OBJECT)\cpu.obj \
	$(OBJECT)\idt.obj \
	$(OBJECT)\message.obj \
	$(OBJECT)\msr.obj \

SYSMGR_C_OBJS	= \
	$(OBJECT)\events.obj \
	$(OBJECT)\handlers.obj \
	$(OBJECT)\chipset.obj \
	$(OBJECT)\gpio.obj \
	$(OBJECT)\errors.obj \
	$(OBJECT)\history.obj \
	$(OBJECT)\io.obj \
	$(OBJECT)\virt_pci.obj \
	$(OBJECT)\mbus.obj \
	$(OBJECT)\vsa_init.obj \
	$(OBJECT)\descr.obj \
	$(OBJECT)\vr.obj \
	$(OBJECT)\topology.obj \
	$(OBJECT)\pci_rd.obj \
	$(OBJECT)\pci_wr.obj \
	$(OBJECT)\cs5536.obj \
	$(OBJECT)\swapsif.obj \
	$(OBJECT)\unregstr.obj \
	$(OBJECT)\mdd.obj \
	$(OBJECT)\pci_pm.obj \
	$(OBJECT)\gpio5536.obj \
	$(OBJECT)\mfgpt.obj \
	$(OBJECT)\mapper.obj \
	$(OBJECT)\timeout.obj \
	$(OBJECT)\io_trap.obj \
	$(OBJECT)\mbiu.obj \
	$(OBJECT)\timer.obj \
	$(OBJECT)\ohci.obj \

SYSMGR_C_CODS	= $(SYSMGR_C_SRCS:.obj=.cod)

SYSMGR_OBJS		= $(SYSMGR_ASM_OBJS) $(SYSMGR_C_OBJS)

SYSMGR_VSM		= $(OBJECT)\sysmgr.vsm

#######################################################################
#
#				Targets
#
#######################################################################


all: $(OBJECT) setenv sysmgr.vsm vsainit.bin 
!ifdef BUILDOBJ
	$(COPY) $(OBJECT)\sysmgr.vsm $(BUILDOBJ)
	$(COPY) $(OBJECT)\vsainit.bin $(BUILDOBJ)
!endif

sysmgr.vsm: $(SYSMGR_OBJS)
	$(LN) /MAP $(LOPTS_VSM) @<<sysmgr.lnk
	$(SYSMGR_OBJS: =+^
	)
$(SYSMGR_VSM)

;
<<NOKEEP

vsainit.bin init.exe: $(INIT_ASM_OBJS)
	$(LN) $(LD_OPTS) /MAP $**,$(OBJECT)\init.exe,$(OBJECT)\init.map;
	$(X2ROM) $(OBJECT)\init.exe $(OBJECT)\vsainit.bin
#	$(COPY) $(OBJECT)\init.rom $(OBJECT)\vsainit.bin

#This and only this clean target must exist as it is called by cleanall
#cleanall and cleanlocal are defined in rules.mak

clean: cleanlocal

$(OBJECT):
	-@md $(OBJECT)

#######################################################################
#
#				Dependencies
#
#######################################################################

$(SYSMGR_ASM_OBJS): $(MAKEDIR)\makefile \
		$(OBJECT)\sysmgr.inc \
		$(OBJECT)\smimac.mac \
		$(OBJECT)\vsa2.inc \
		$(OBJECT)\isa.inc \
		$(OBJECT)\chipset.inc \
		$(OBJECT)\vr.inc \
		$(OBJECT)\pci.inc \
		$(OBJECT)\hce.inc \
		$(OBJECT)\gx2.inc  \
		$(OBJECT)\protos.inc \
		$(OBJECT)\descr.inc \
		$(OBJECT)\mdd.inc \
		$(OBJECT)\cs5536.inc \

$(SYSMGR_C_OBJS): $(MAKEDIR)\makefile \
		$(OBJECT)\sysmgr.h \
		$(OBJECT)\vsa2.h \
		$(OBJECT)\isa.h \
		$(OBJECT)\chipset.h \
		$(OBJECT)\vr.h \
		$(OBJECT)\pci.h \
		$(OBJECT)\vpci.h  \
		$(OBJECT)\protos.h \
		$(OBJECT)\mdd.h \
		$(OBJECT)\timer.h \
		$(OBJECT)\mapper.h \
		$(OBJECT)\gx2.h  \
		$(OBJECT)\hce.h \
		$(OBJECT)\acpi.h \
		$(OBJECT)\cs5536.h \

$(INIT_ASM_OBJS): $(MAKEDIR)\makefile \
		$(OBJECT)\sysmgr.inc \
		$(OBJECT)\smimac.mac \
		$(OBJECT)\vsa2.inc \
		$(OBJECT)\isa.inc \
		$(OBJECT)\chipset.inc \
		$(OBJECT)\vr.inc \
		$(OBJECT)\pci.inc \
		$(OBJECT)\init.inc \
		$(OBJECT)\mdd.inc \
		$(OBJECT)\gx2.inc  \
		$(OBJECT)\cs5536.inc \

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

$(OBJECT)\hce.inc: 
	$(H2) /Fa$(OBJECT)\hce.inc /Znh /C $(INC_DIR)\hce.h

$(OBJECT)\protos.inc: 
	$(H2) /Fa$(OBJECT)\protos.inc /Zn"q" /C $(SYSMGR_SRC)\protos.h

