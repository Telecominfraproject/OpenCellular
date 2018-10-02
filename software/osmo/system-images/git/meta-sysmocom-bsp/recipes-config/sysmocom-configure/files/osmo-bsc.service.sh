#!/bin/sh

set -eu

OSMOBSC_VALUE="bar"

. /slot/system.conf

cat >/etc/osmocom/osmo-bsc.cfg <<EOF
#dummy: OSMOBSC_VALUE=$OSMOBSC_VALUE
EOF

