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

TARGETS=x86 arm
# To generate all locales, make from top level.
LOCALES=en

default: outside_chroot
	@echo "Specify a target to build for:"
	@echo "  ${TARGETS}"

# TODO(hungte) It's now not easy to have bmpblk_utility outside
# chroot... We need some better way to do this.
outside_chroot:
	@if [ -e /etc/debian_chroot ]; then \
		echo "ImageMagick is too complex to build inside the chroot."; \
		echo "You must be outside the chroot to do this"; \
		echo "(and you probably shouldn't be doing it anyway)."; \
		exit 1; \
	fi

${TARGETS}:: outside_chroot

x86::
	LOCALES="$(LOCALES)" ./build_images "$@"
	cd "out_$@" && ../make_default_yaml $(LOCALES)
	cd "out_$@" && bmpblk_utility -c DEFAULT.yaml bmpblock.bin
	ls -l "out_$@"/bmpblock.bin


arm::
	LOCALES="$(LOCALES)" ./build_images "$@"
	cd "out_$@" && ../make_default_yaml $(LOCALES)
	cd "out_$@" && bmpblk_utility -c DEFAULT.yaml bmpblock.bin
	ls -l "out_$@"/bmpblock.bin

clean:
	rm -rf out_*
