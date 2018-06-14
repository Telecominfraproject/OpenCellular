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

VSMNAME	= lxvg
VSMDIR	= $(VSA2ROOT)\$(VSMNAME)

all:
	cd $(VSMDIR)
	$(MAKE) /nologo all "VSA2ROOT=$(VSA2ROOT)" "USER=$(USER)" "BUILDOBJ=$(OBJECT)" "CPU=$(CPU)"
	cd $(MAKEDIR)


clean:
	cd $(VSMDIR)
	$(MAKE) /nologo clean
	cd $(MAKEDIR)

cleanlocal:
	cd $(VSMDIR)
	$(MAKE) /nologo cleanlocal
	cd $(MAKEDIR)

cleanall:
	cd $(VSMDIR)
	$(MAKE) /nologo cleanall
	cd $(MAKEDIR)
