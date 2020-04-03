#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#

.PHONY: all
all:
	make -C paws
	make -C sctp-agent
	make -C paws db
	make -C sctp-agent db

.PHONY: clean
clean:
	make -C paws clean
	make -C sctp-agent clean