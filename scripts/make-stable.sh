#!/bin/sh

# Merge -testing into -stable by using hard links so we don't
# double the space requirement.

if [ $# -lt 2  ]; then
	echo "Need to pass MACHINE RELEASE as argument for upload"
	exit 1
fi

MACHINE=$1
RELEASE=$2
DRYRUN=$3

if [ "x$DRYRUN" != "x" ]; then
	BASE_ARGS="--recursive --delete --links --verbose --dry-run "
else
	BASE_ARGS="--delete -avH"
fi

DIRS="images ipk sdk tools cache-state sources cache conf"

for i in $DIRS;
do
	if [ ! -e $PWD/web-files/$MACHINE/$RELEASE-testing/$i ]; then
		echo "Skipping $i, directory doesn't exist"
		echo ""
		continue
	fi

	if [ "x$DRYRUN" != "x" ]; then
		ARGS="$BASE_ARGS"
	else
		ARGS="$BASE_ARGS --link-dest=$PWD/web-files/$MACHINE/$RELEASE-testing/$i"
	fi

	echo "Checking $i"
	rsync $ARGS \
		web-files/$MACHINE/$RELEASE-testing/$i/ \
		web-files/$MACHINE/$RELEASE/$i/ | egrep -v "sending incre|sent |total"
done
