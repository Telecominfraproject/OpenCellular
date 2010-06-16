# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INCLUDES += -I./include \
		-I$(TOP)/common/include \
		-I$(TOP)/vboot_firmware/lib/cryptolib/include \
		-I$(TOP)/vfirmware/include \
		-I$(TOP)/vkernel/include

BUILD_ROOT := ${BUILD}/misclibs

ALL_SRCS = file_keys.c signature_digest.c

include ../common.mk

