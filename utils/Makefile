# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LIBS=-lcrypto

all:	dumpRSAPublicKey

dumpRSAPublicKey:	dumpRSAPublicKey.c
		$(CC) $(CFLAGS) $(LIBS) $< -o $@
clean:
	rm -f dumpRSAPublicKey
