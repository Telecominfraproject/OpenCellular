# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INCLUDES += -I./include \
	-I$(FWDIR)/lib/include \
	-I$(FWDIR)/lib/cgptlib/include \
	-I$(FWDIR)/lib/cryptolib/include \
	-I$(FWDIR)/lib/tpm_lite/include \
	-I$(HOSTDIR)/include
BUILD_ROOT = ${BUILD}/tests/tpm_lite

TEST_NAMES = tpmtest_clear \
             tpmtest_earlyextend \
             tpmtest_earlynvram \
             tpmtest_earlynvram2 \
	     tpmtest_enable \
	     tpmtest_fastenable \
	     tpmtest_globallock \
             tpmtest_lock \
             tpmtest_readonly \
             tpmtest_redefine \
             tpmtest_spaceperm \
	     tpmtest_startup \
             tpmtest_writelimit \

TEST_BINS = $(addprefix ${BUILD_ROOT}/,$(TEST_NAMES))

ALL_DEPS = $(addsuffix .d,${TEST_BINS})
CFLAGS +=  -MMD -MF $@.d

LIBS := ${TEST_LIB} $(HOSTLIB) $(FWLIB)

all: $(TEST_BINS)

${BUILD_ROOT}/%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MF $@.d -c -o $@ $<

${BUILD_ROOT}/tpmtest_%: %.c ${LIBS}
	$(CC) $(CFLAGS) $(INCLUDES) $< ${LIBS} -o $@ -lcrypto -lrt $(LDFLAGS)

-include ${ALL_DEPS}
