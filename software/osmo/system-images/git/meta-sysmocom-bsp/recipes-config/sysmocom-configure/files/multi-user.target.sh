#!/bin/sh

set -eu

TEST_VALUE="default"

. /slot/system.conf

cat >/etc/symocom/test.cfg <<EOF
[main]
bla=$TEST_VALUE
EOF
