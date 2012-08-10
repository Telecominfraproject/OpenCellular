# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCALES=ar bg bn ca cs da de el en en_GB es es_419 et fa fi fil fr gu hi hr \
	hu id it iw ja kn ko lt lv ml mr ms nl no pl pt_BR pt_PT ro ru sk sl \
	sr sv ta te th tr uk vi zh_CN zh_TW

DIR_TARGETS=$(foreach DIR,$(LOCALES),$(DIR)-dir)

CVT=../text_to_png

default: $(DIR_TARGETS)

%-dir:
	$(CVT) --lan=$(subst -dir,,$@) \
	--font="$$(sed -nre 's/^$(subst -dir,,$@)="(.*)"/\1/p' font.conf)" \
	$(subst -dir,,$@)/*.txt

clean:
	rm -f */*.png

$(DIR_TARGETS) : FORCE

.PHONY: FORCE
