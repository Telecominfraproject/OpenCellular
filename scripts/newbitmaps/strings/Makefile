# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

.SUFFIXES: .txt .TXT .png
.PHONY: clean all font localized_text

LOCALEDIR=localized_text

.TXT.png:
	./text_to_png $<

.txt.png:
	./text_to_png $<

all: font Url.png hwid_placeholder.png localized_text

localized_text:
	make -C $(LOCALEDIR)

font:
	./build_font $@

clean:
	rm -f *.png
	rm -rf font
	make -C $(LOCALEDIR) $@
