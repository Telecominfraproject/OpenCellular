# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This makefile lets you MANUALLY create a set of images and the corresponding
# DEFAUL.yaml file for a particular platform. It should NOT be run
# automatically, but only when the master images have changed following review
# by the localization team. Additionally, it needs to be run outside of the
# chroot, since it uses ImageMagick to transform the master images, and
# ImageMagick is much to complex to bother installing into the chroot just for
# this occasional need.

TARGETS=std n1366usb n1366usb2 s800 s1024 s1024usb s1024usb2
TARGETS+=s1280a8x5 s1280x850a8x5

# To generate all locales, make from top level.
LOCALES=en

default: std

all: $(TARGETS)

$(TARGETS):
	LOCALES="$(LOCALES)" ./build_images "$@"

clean:
	rm -rf out_*

.PHONY: $(TARGETS) clean all default
