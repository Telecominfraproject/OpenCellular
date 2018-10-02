#!/bin/sh

# Upload build results, config and cache to the downloads server. Use
# make install-ssh-config SSH_PORT=XYZ SSH_HOST=abc SSH_USER=foo to
# install the right entry for the .ssh/config.

if [ $# -ne 2  ]; then
	echo "Need to pass MACHINE RELEASE as argument for upload"
	exit 1
fi

set -ex

rsync --delete -avz tmp/deploy/ipk/	sysmocom-downloads:$1/$2/ipk
rsync --delete -avz tmp/deploy/images/	sysmocom-downloads:$1/$2/images
rsync --delete -avz tmp/deploy/tools/	sysmocom-downloads:$1/$2/tools
rsync --delete -avz tmp/deploy/sdk/	sysmocom-downloads:$1/$2/sdk
rsync --delete -avz tmp/cache/		sysmocom-downloads:$1/$2/cache-state
rsync --delete -avz cache/		sysmocom-downloads:$1/$2/cache
rsync --delete -avz conf/		sysmocom-downloads:$1/$2/conf
rsync -avz tmp/deploy/sources/		sysmocom-downloads:$1/$2/sources

