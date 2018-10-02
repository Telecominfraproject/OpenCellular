#!/bin/sh

# Dispatch based on SSH_ORIGINAL_COMMAND. rsync, diff and merge.
# TODO: Make this interactive and show the diff before and then
# do the merge?


set -e

# Extract first part...
item=1
for i in $SSH_ORIGINAL_COMMAND;
do
	if [ $item = "1" ]; then
		CMD=$i
	elif [ $item = "11" ]; then
		MACHINE=$i
	elif [ $item = "111" ]; then
		RELEASE=$i
	else
		break
	fi
	item="1$item"
done

case "$CMD" in
	"rsync")
		exec /usr/local/bin/rrsync $1
		;;
	"diff-testing")
		cd $1
		cd ../
		exec `dirname $0`/make-stable.sh $MACHINE $RELEASE dry-run
		;;
	"merge-testing")
		cd $1
		cd ../
		exec `dirname $0`/make-stable.sh $MACHINE $RELEASE
		;;
esac
