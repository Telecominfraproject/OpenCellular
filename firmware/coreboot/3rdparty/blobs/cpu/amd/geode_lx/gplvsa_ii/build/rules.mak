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
#	Common targets
#
##############################################################################



setenv:
!if "$(VARS_SET)" != "VSA_II"
	@$(SETENV) PATH=.\;$(VSA2ROOT)\uti;$(PATH);
	@$(SETENV) Lib=.\;$(VSA2ROOT)\lib;$(VSA2ROOT)\build;$(LIB);
	@$(SETENV) VARS_SET=VSA_II
!endif
	@echo INCLUDE=$(INCLUDE)

$(BUILD_DIR)\obj\$(TOOL_LIB): 
	cd $(VSA2ROOT)\vsm_lib
	$(MAKE) /nologo all "BUILDOBJ=$(OBJECT)" "CPU=$(CPU)"
	cd $(MAKEDIR)

cleanlib:
	cd $(VSA2ROOT)\vsm_lib
	$(MAKE) /nologo cleanall
	cd $(MAKEDIR)	

cleanlocal: 
	-@IF EXIST $(OBJECT)\*.def $(DEL) $(OBJECT)\*.def
	-@IF EXIST $(OBJECT)\*.lnk $(DEL) $(OBJECT)\*.lnk
	-@IF EXIST $(OBJECT)\*.map $(DEL) $(OBJECT)\*.map
	-@IF EXIST $(OBJECT)\*.obj $(DEL) $(OBJECT)\*.obj
	-@IF EXIST $(OBJECT)\*.exe $(DEL) $(OBJECT)\*.exe
	-@IF EXIST $(OBJECT)\*.rom $(DEL) $(OBJECT)\*.rom
	-@IF EXIST $(OBJECT)\*.cpu $(DEL) $(OBJECT)\*.cpu
	-@IF EXIST $(OBJECT)\*.scc $(DEL) $(OBJECT)\*.scc
	-@IF EXIST $(OBJECT)\*.inc $(DEL) $(OBJECT)\*.inc
	-@IF EXIST $(OBJECT)\*.h $(DEL) $(OBJECT)\*.h
	-@IF EXIST $(OBJECT)\*.lst $(DEL) $(OBJECT)\*.lst
	-@IF EXIST $(OBJECT)\*.bak $(DEL) $(OBJECT)\*.bak
	-@IF EXIST $(OBJECT)\*.mac $(DEL) $(OBJECT)\*.mac
	-@IF EXIST $(OBJECT)\*.asm $(DEL) $(OBJECT)\*.asm
	-@IF EXIST $(OBJECT)\*.cod $(DEL) $(OBJECT)\*.cod
	-@IF EXIST $(MAKEDIR)\*.map $(DEL) $(MAKEDIR)\*.map
	-@IF EXIST $(MAKEDIR)\arccode.h $(DEL) $(MAKEDIR)\arccode.h

cleanall: clean 
	-@IF EXIST $(MAKEDIR)\*.vsm $(DEL) $(MAKEDIR)\*.vsm
	-@IF EXIST $(OBJECT)\*.vsm $(DEL) $(OBJECT)\*.vsm
	-@IF EXIST $(OBJECT)\*.bin $(DEL) $(OBJECT)\*.bin
	-@IF EXIST $(OBJECT)\*.lib $(DEL) $(OBJECT)\*.lib
	-@IF EXIST $(OBJECT) rd $(OBJECT)


##############################################################################
#
#	Common inference rules
#
##############################################################################

{$(INC_DIR)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /C $<

{$(INC_DIR)\$(CPU)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /C $<

{$(SYSMGR_SRC)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /C $<

{$(SYSMGR_SRC)\$(CPU)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /C $<

{$(MAKEDIR)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /C $<

{$(MAKEDIR)\$(CPU)}.h{$(OBJECT)}.inc:
	$(H2) /Fa$(OBJECT)\$(@F) /nologo /C $<

{$(INC_DIR)}.h{$(OBJECT)}.h:
	$(COPY) $< $@

{$(INC_DIR)\$(CPU)}.h{$(OBJECT)}.h:
	$(COPY) $< $@

{$(SYSMGR_SRC)}.h{$(OBJECT)}.h:
	$(COPY) $< $@

{$(SYSMGR_SRC)\$(CPU)}.h{$(OBJECT)}.h:
	$(COPY) $< $@

{$(MAKEDIR)}.h{$(OBJECT)}.h:
	$(COPY) $< $@

{$(MAKEDIR)\$(CPU)}.h{$(OBJECT)}.h:
	$(COPY) $< $@

{$(MAKEDIR)}.inc{$(OBJECT)}.inc:
	$(COPY) $< $@

{$(MAKEDIR)\$(CPU)}.inc{$(OBJECT)}.inc:
	$(COPY) $< $@

{$(MAKEDIR)}.c{$(OBJECT)}.obj:
	$(CC) /nologo $(CC_OPTS) /Fo$@ $< 

{$(MAKEDIR)\$(CPU)}.c{$(OBJECT)}.obj:
	$(CC) /nologo $(CC_OPTS) /Fo$@ $< 

{$(MAKEDIR)}.asm{$(OBJECT)}.obj:
	$(AS) /nologo $(AS_OPTS) /Fo$@ $< 

{$(MAKEDIR)\$(CPU)}.asm{$(OBJECT)}.obj:
	$(AS) /nologo $(AS_OPTS) /Fo$@ $< 

{$(SYSMGR_SRC)}.mac{$(OBJECT)}.mac:
	$(COPY) $< $@

{$(SYSMGR_SRC)\$(CPU)}.mac{$(OBJECT)}.mac:
	$(COPY) $< $@

