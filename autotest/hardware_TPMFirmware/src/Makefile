# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TPM = tpm
TPM_SRC_DIR = $(GCLIENT_ROOT)/src/third_party/$(TPM)
TPM_WORK_DIR = $(TPM)

TLCL = tpm_lite
TLCL_SRC_DIR = $(GCLIENT_ROOT)/src/platform/$(TLCL)
TLCL_WORK_DIR = $(TLCL)

BINDIR = .
PROGRAMS = \
           clear \
           enable \
           globallock \
           lock \
           readonly \
           writelimit

all:
	rm -rf $(TPM_WORK_DIR)
	cp -a $(TPM_SRC_DIR) $(TPM_WORK_DIR)
	$(MAKE) -C $(TPM_WORK_DIR)/nvtool clean
	$(MAKE) -C $(TPM_WORK_DIR)/nvtool
	rm -rf $(TLCL_WORK_DIR)
	cp -a $(TLCL_SRC_DIR) $(TLCL_WORK_DIR)
	$(MAKE) -C $(TLCL_WORK_DIR)/src clean
	$(MAKE) cross USE_TPM_EMULATOR=0 -C $(TLCL_WORK_DIR)/src
	# gets rid of host binary which confuses ARM build
	rm $(TLCL_WORK_DIR)/src/tlcl/generator
	cp $(TPM_WORK_DIR)/nvtool/tpm-nvtool $(BINDIR)
	set -e; for i in $(PROGRAMS); \
	  do cp $(TLCL_WORK_DIR)/src/testsuite/tpmtest_$$i $(BINDIR); done
