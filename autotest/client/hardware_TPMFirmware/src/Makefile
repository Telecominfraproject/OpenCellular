# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BINDIR = .
PROGRAMS = \
	earlyextend      \
	earlynvram       \
	earlynvram2      \
	enable           \
	fastenable       \
	globallock       \
	redefine_unowned \
	spaceperm        \
	testsetup        \
	timing           \
	writelimit       \

all:
	$(MAKE) -C $(VBOOT_DIR) clean
	$(MAKE) -C $(VBOOT_DIR)
	set -e; \
	for i in $(PROGRAMS); do \
	  cp $(VBOOT_DIR)/build/tests/tpm_lite/tpmtest_$$i $(BINDIR); \
	done
