#!/bin/sh

set -e

usage="Usage: $0 [seconds]\n"

err_msgs=
command -v chvt >/dev/null || err_msgs="${err_msgs}"'Need `chvt`\n'
command -v openvt >/dev/null || err_msgs="${err_msgs}"'Need `openvt`\n'
command -v fgconsole >/dev/null || err_msgs="${err_msgs}"'Need `fgconsole`\n'
[ -n "$err_msgs" ] && err_msgs="${err_msgs}"'e.g. install the `kbd` package\n'

[ -x build/gfx_test ] || \
	err_msgs="${err_msgs}"'Please run from *libgfxinit* source dir and build `gfx_test` first.\n'

[ "$#" -gt 1 ] && err_msgs="${err_msgs}${usage}"

if [ -n "$err_msgs" ]; then
	printf "$err_msgs"
	exit 1
fi

if [ "$#" -lt 1 ]; then
	# default duration of 5s
	set 5
fi

reload_i915=0
prepare_vt() {
	# switch VT, we might be in X
	orig_vt=`fgconsole`
	openvt -s -- true

	# poll until the VT switch is done
	while [ `fgconsole` -eq $orig_vt ]; do :; done

	# take i915 out of charge
	echo 0 >/sys/devices/virtual/vtconsole/vtcon1/bind

	# try unloading it
	if lsmod | grep -q i915 && modprobe -r i915 >/dev/null 2>&1; then
		reload_i915=1
	fi
}

restore_vt() {
	# reload i915
	if [ $reload_i915 -eq 1 ]; then
		modprobe i915
	fi

	# put i915 back in charge
	echo 1 >/sys/devices/virtual/vtconsole/vtcon1/bind

	# return to original VT
	chvt $orig_vt
}

prepare_vt

build/gfx_test "$@" || true

restore_vt
