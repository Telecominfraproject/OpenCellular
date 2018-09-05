#!/bin/sh

trap "kill 0" EXIT

while [ -e /etc/passwd ]; do
	$* &
	LAST_PID=$!
	wait $LAST_PID
done
