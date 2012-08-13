# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

OUTPUT=bmpblock.bin

all:
	for X in *.png; do \
		python -c "import Image; \
		Image.open('$$X').convert('P').save('$${X/png/bmp}')"; \
	done
	bmpblk_utility -c config.yaml $(OUTPUT)

clean:
	rm -rf $(OUTPUT) *.bmp

.PHONY: all clean
