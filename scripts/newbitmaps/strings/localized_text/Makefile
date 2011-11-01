# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCALES=ar bg ca cs da de el en en_GB es es_419 et fa fi fil fr hi hr hu \
	id it iw ja ko lt lv nl no pl pt_BR pt_PT ro ru sk sl sr sv th tr \
	uk vi zh_CN zh_TW

# Use "make PT=15" for 800x600 (x86)
PT=22


DIR_TARGETS=$(foreach DIR,$(LOCALES),$(DIR)-dir)

CVT=../text_to_bmp --point=$(PT)


default: $(DIR_TARGETS)

%-dir:
	$(CVT) $(subst -dir,,$@)/*.txt

ar-dir:
	$(CVT) --rtl --font="Droid Sans Arabic" $(subst -dir,,$@)/*.txt

fa-dir:
	$(CVT) --rtl $(subst -dir,,$@)/*.txt

hi-dir:
	$(CVT) --font="Lohit-Hindi" $(subst -dir,,$@)/*.txt

iw-dir:
	$(CVT) --rtl --font="Droid Sans Hebrew" $(subst -dir,,$@)/*.txt

ja-dir:
	$(CVT) --font="IPAPGothic" $(subst -dir,,$@)/*.txt

ko-dir:
	$(CVT) --font="NanumGothic" $(subst -dir,,$@)/*.txt

th-dir:
	$(CVT) --font="Droid Sans Thai" $(subst -dir,,$@)/*.txt

zh_CN-dir:
	$(CVT) --font="Droid Sans Fallback" $(subst -dir,,$@)/*.txt

zh_TW-dir:
	$(CVT) --font="Droid Sans Fallback" $(subst -dir,,$@)/*.txt



$(DIR_TARGETS) : FORCE

.PHONY: FORCE
