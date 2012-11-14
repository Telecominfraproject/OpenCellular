# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This will regenerate the BIOS bitmap images for all platforms. You
# shouldn't need to do this, though.

# These are all the known locales, sorted more-or-less geograpically
ALL_LOCALES=en es_419 pt_BR en_GB fr es pt_PT ca it de \
  el nl da no sv fi et lv lt ru pl cs sk hu sl sr hr bg ro \
  uk tr iw ar fa hi th vi id fil zh_CN zh_TW ko ja

# Here are the launch locales for Stumpy/Lumpy (issue 6595), same ordering.
LOCALES=en es_419 pt_BR fr es it de nl da no sv ko ja

# Added more locales for future (crosbug.com/p/12846).
LOCALES+= id th

# More locales (crosbug.com/p/11969)
LOCALES+= ar ms zh_CN zh_TW

default: outside_chroot strings images

outside_chroot:
	@if [ -e /etc/debian_chroot ]; then \
		echo "PIL color quantization is broken inside the chroot."; \
		echo "You must be outside the chroot to do this"; \
		echo "(and you probably shouldn't be doing it anyway)."; \
		exit 1; \
	fi

strings:
	$(MAKE) -C strings

images:
	$(MAKE) -C images LOCALES="$(LOCALES)" all

clean:
	$(MAKE) -C strings clean
	$(MAKE) -C images clean

.PHONY: outside_chroot strings images clean
