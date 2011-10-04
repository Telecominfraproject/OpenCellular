# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NORMAL=bg ca cs da de el en en_GB es es_419 et fi fil fr hi hr hu \
	id it ja ko lt lv nl no pl pt_BR pt_PT ro ru sk sl sr sv th tr \
	uk vi zh_CN zh_TW

SPECIAL=ar fa iw

RTL=--rtl
TNR=--font='Times New Roman'

default: $(foreach DIR,$(NORMAL),$(DIR)-dodir) $(SPECIAL)

%-dodir:
	@../text_to_bmp $(subst -dodir,,$@)/*

ar:
	@../text_to_bmp $(RTL) $(TNR) $@/*

fa:
	@../text_to_bmp $(RTL) $(TNR) $@/*

iw:
	@../text_to_bmp $(RTL) $@/*


.PHONY: $(SPECIAL)
