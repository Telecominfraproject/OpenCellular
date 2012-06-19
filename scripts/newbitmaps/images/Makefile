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

BACKGROUND_IMAGE=Background_white.bmp
GRAYSCALE_IMAGES=arrow_left.bmp arrow_right.bmp divider_top.bmp divider_btm.bmp
OTHER_IMAGES=Devmode_graphic.bmp Insert_graphic.bmp Remove_graphic.bmp \
  Yuck_graphic.bmp Url.bmp hwid_unknown.bmp chrome_logo.bmp

FONTS=hwid_fonts.bin

default: outside_chroot
	@echo "Specify a target to build for:"
	@echo "  ${TARGETS}"

outside_chroot:
	@if [ -e /etc/debian_chroot ]; then \
		echo "ImageMagick is too complex to build inside the chroot."; \
		echo "You must be outside the chroot to do this"; \
		echo "(and you probably shouldn't be doing it anyway)."; \
		exit 1; \
	fi

${TARGETS}:: outside_chroot


# The image size with UEFI BIOS is always 800x600, which is stretched to fill
# the entire screen. With previous devices the physical screen size was either
# 1280x800 (16:10) or 1366x768 (16:9). There's not a lot of difference between
# those, so let's just assume 16:9 for future platforms to make things simpler.
_x86_max=800x600!
_x86_scale=59%x78%
_x86_opts=-colors 256 -compress none -alpha off
_arm_max=800x600!
_arm_scale=59%x78%

x86::
	# create output directories
	mkdir -p "out_$@"
	# copy stuff we need
	cp "${FONTS}" "out_$@"
	# Scale and preserve the 256 color index of background image
	echo "  out_$@/${BACKGROUND_IMAGE}" && \
	convert ${BACKGROUND_IMAGE} -scale '${_x86_max}' ${_x86_opts} \
		-remap gradient.png "BMP3:out_$@/${BACKGROUND_IMAGE}" || \
		exit 1;
	# Preserve the 256 color index of grayscale images
	@for i in ${GRAYSCALE_IMAGES}; do \
		echo "  out_$@/$$i" && \
		convert $$i -scale '${_x86_scale}' ${_x86_opts} \
		-remap gradient.png "BMP3:out_$@/$$i" || \
		exit 1; \
	done
	@for i in ${OTHER_IMAGES}; do \
		echo "  out_$@/$$i" && \
		convert $$i -scale '${_x86_scale}' ${_x86_opts} \
			"BMP3:out_$@/$$i" || \
		exit 1; \
	done
	# produce the new yaml
	cd "out_$@" && ../make_default_yaml ${DEFAULT_LOCALES}
	cd "out_$@" && bmpblk_utility -c DEFAULT.yaml bmpblock.bin
	ls -l "out_$@"/bmpblock.bin


arm::
	# create output directories
	mkdir -p "out_$@"
	for i in localized_images/*; do \
		mkdir -p "out_$@/$$i"; \
	done
	# copy stuff we need
	cp "${FONTS}" "out_$@"
	convert ${BACKGROUND_IMAGE} -append \
		-colors 256 -unique-colors "out_$@/base_cmap.png"
	convert localized_images/*/*.bmp -append \
		-colors 256 -unique-colors "out_$@/loc_cmap.png"
	# scale the background exactly...
	echo "  out_$@/${BACKGROUND_IMAGE}"; \
		convert ${BACKGROUND_IMAGE} -scale "${_arm_max}" \
		-remap "out_$@/base_cmap.png" "out_$@/${BACKGROUND_IMAGE}"; \
	done
	# scale the localized string images using percentages...
	@for i in ${GRAYSCALE_IMAGES} ${OTHER_IMAGES} localized_images/*/*.bmp; do \
		echo "  out_$@/$$i"; \
		convert $$i -scale "${_arm_scale}" \
			-remap "out_$@/loc_cmap.png" "out_$@/$$i"; \
	done
	# produce the new yaml
	cd "out_$@" && ../make_default_yaml ${DEFAULT_LOCALES}
	perl -i -p -e 's/^compression:.*/compression: 2/;' "out_$@/DEFAULT.yaml"
	# Note: manually use bmpblk_utility to create the binary from out_$@




clean:
	rm -rf out_*
