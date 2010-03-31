# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TOP ?= ../
CC ?= gcc
INCLUDES += -I./include \
		-I$(TOP)/common/include \
		-I$(TOP)/cryptolib/include \
		-I$(TOP)/vfirmware/include \
		-I$(TOP)/vkernel/include

CFLAGS ?= -Wall -DNDEBUG -O3 -Werror $(INCLUDES)
TOP ?= ../

MISCLIB_OUT = file_keys.o signature_digest.o

all: $(MISCLIB_OUT)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean: 
	rm -f $(MISCLIB_OUT)
