# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LIBS=-lcrypto
FIRMWARELIBS=$(TOP)/crypto/libcrypto.a $(TOP)/common/libcommon.a

all:	dumpRSAPublicKey verify_data signature_digest

dumpRSAPublicKey:	dumpRSAPublicKey.c
		$(CC) $(CFLAGS) $(LIBS) $< -o $@

verify_data:	verify_data.c
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@ $(FIRMWARELIBS)

signature_digest:	signature_digest.c
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@ $(FIRMWARELIBS)

clean:
	rm -f dumpRSAPublicKey verify_data signature_digest
