# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
INCLUDES += -I./include \
		-I$(FWDIR)/lib/include \
		-I$(FWDIR)/lib/cryptolib/include \
		-I../common/include \
		-I../misclibs/include
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror
KERNEL_OUT = kernel_image.o

all: $(KERNEL_OUT)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(KERNEL_OUT)
