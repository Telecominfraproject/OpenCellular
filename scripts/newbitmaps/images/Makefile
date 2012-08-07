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

# These are all the known locales, sorted more-or-less geograpically. We
# generally don't have room in the BIOS for all of them at once.
DEFAULT_LOCALES=en es_419 pt_BR en_GB fr es pt_PT ca it de \
  el nl da no sv fi et lv lt ru pl cs sk hu sl sr hr bg ro \
  uk tr iw ar fa hi th vi id fil zh_CN zh_TW ko ja

default: outside_chroot
	@echo "Specify a target to build for:"
	@echo "  ${TARGETS}"

outside_chroot:
	# TODO(hungte) It's now not easy to have bmpblk_utility outside
	# chroot... We need some better way to do this.
	@if [ -e /etc/debian_chroot ]; then \
		echo "ImageMagick is too complex to build inside the chroot."; \
		echo "You must be outside the chroot to do this"; \
		echo "(and you probably shouldn't be doing it anyway)."; \
		exit 1; \
	fi

${TARGETS}:: outside_chroot

x86::
	./build_images "$@"
	cd "out_$@" && ../make_default_yaml ${DEFAULT_LOCALES}
	cd "out_$@" && bmpblk_utility -c DEFAULT.yaml bmpblock.bin
	ls -l "out_$@"/bmpblock.bin


arm::
	./build_images "$@"
	cd "out_$@" && ../make_default_yaml ${DEFAULT_LOCALES}
	cd "out_$@" && bmpblk_utility -c DEFAULT.yaml bmpblock.bin
	ls -l "out_$@"/bmpblock.bin

clean:
	rm -rf out_*
