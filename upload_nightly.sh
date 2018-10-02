#!/bin/sh

DIST="201705-nightly"
REMOTE="generic@sysmocom-downloads"
MACHINE=$1

if [ "z$MACHINE" == "z" ]; then
	echo "You must specify the machine type as argument!"
	exit 1
fi

# convert legacy machine name != download directory
if [ $MACHINE == "sysmobts-v2" ]; then
	MACHINE="sysmobts"
fi

rsync --delete -avz tmp/deploy/ipk/    $REMOTE:$MACHINE/$DIST/ipk
rsync --delete -avz tmp/deploy/images/ $REMOTE:$MACHINE/$DIST/images
rsync --delete -avz tmp/deploy/tools/  $REMOTE:$MACHINE/$DIST/tools
rsync --delete -avz tmp/deploy/sdk/    $REMOTE:$MACHINE/$DIST/sdk
rsync --delete -avz tmp/cache/    $REMOTE:$MACHINE/$DIST/cache-state
rsync --delete -avz cache/    $REMOTE:$MACHINE/$DIST/cache
rsync --delete -avz conf/    $REMOTE:$MACHINE/$DIST/conf
rsync -avz tmp/deploy/sources/  $REMOTE:$MACHINE/$DIST/sources
