# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INCLUDES += -I./include \
		-I$(FWDIR)/lib/include \
		-I$(FWDIR)/lib/cryptolib/include \
		-I../common/include \
		-I../misclibs/include

BUILD_ROOT := ${BUILD}/vfirmware

ALL_SRCS = firmware_image.c

include ../common.mk

all: $(ALL_OBJS)

