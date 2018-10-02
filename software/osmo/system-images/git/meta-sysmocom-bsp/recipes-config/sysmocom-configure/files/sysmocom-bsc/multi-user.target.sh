#!/bin/sh

TEST_VALUE="default"

. /slot/system.conf

cat >/etc/sysmocom/test.cfg <<EOF
[main]
bla=$TEST_VALUE
model=sysmocom-bsc
EOF
