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
#	makefile  for VSA2



!ifndef VSA2ROOT
VSA2ROOT	= $(MAKEDIR)\..
!endif

USER		= $(VSA2ROOT)

OBJECT		= $(MAKEDIR)\obj

.SUFFIXES:
.SUFFIXES : .exe .vsm .lib .bin .mak .cln

!include setvars.mak

VSMS = \
	$(OBJECT)\sysmgr.vsm \
	$(OBJECT)\vsainit.bin \
	$(OBJECT)\legacy.vsm \
	$(OBJECT)\lxvg.vsm

VSM_CLN0		= $(VSMS:.vsm=.cln)
VSM_CLN			= $(VSM_CLN0:.bin=.cln)


#######################################################################
#
#				Targets
#
#######################################################################
all: $(OBJECT) $(VSMS) basic

basic: setenv
	@$(ECHO) Make LX VSA Image
	$(MAKE) /nologo vsa_lx.bin

$(VSMS): setenv

vsa_lx.bin $(OBJECT)\amd_vsa_lx.bin: $(OBJECT)\vsainit.bin $(OBJECT)\vsa2.bin
	@$(ECHO) Concatenate for LX Image ...
	-$(BINCOPY) $(OBJECT)\vsainit.bin+$(OBJECT)\vsa2.bin+$(OBJECT)\lxvg.vsm $(OBJECT)\amd_vsa_lx.bin

vsa2.bin $(OBJECT)\vsa2.bin: $(OBJECT)\sysmgr.vsm $(OBJECT)\legacy.vsm
	@$(ECHO) Concatenate for VSA2.BIN Image ...
	-$(BINCOPY) $(OBJECT)\sysmgr.vsm+$(OBJECT)\legacy.vsm $(OBJECT)\vsa2.bin

#This and only this clean target must exist as it is called by cleanall
#cleanall and cleanlocal are defined in rules.mak

clean: cleanlocal cleanlib $(VSM_CLN) tools_clean

$(OBJECT):
	-@md $(OBJECT)


############################################
# Tools.lib
############################################
tools_clean:
	-@cd $(VSA2ROOT)\vsm_lib
	-@$(MAKE) /nologo cleanall
	-@cd $(MAKEDIR)


###########################################################################
#						   All VSMs
###########################################################################
{$(MAKEDIR)}.mak{$(OBJECT)}.vsm:
	$(MAKE) /nologo -f $(MAKEDIR)\%|fF.mak "VSA2ROOT=$(VSA2ROOT)" "USER=$(VSA2ROOT)" "OBJECT=$(OBJECT)" "CPU=$(CPU)"

{$(MAKEDIR)}.mak{$(OBJECT)}.bin:
	$(MAKE) /nologo -f $(MAKEDIR)\%|fF.mak "VSA2ROOT=$(VSA2ROOT)" "USER=$(VSA2ROOT)" "OBJECT=$(OBJECT)" "CPU=$(CPU)"

{$(MAKEDIR)}.mak{$(OBJECT)}.cln:
	-@$(MAKE) /nologo -f $(MAKEDIR)\%|fF.mak cleanall "VSA2ROOT=$(VSA2ROOT)" "USER=$(VSA2ROOT)"


!include rules.mak

