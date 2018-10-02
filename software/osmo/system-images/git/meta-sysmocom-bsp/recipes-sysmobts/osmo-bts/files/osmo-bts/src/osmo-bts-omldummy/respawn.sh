#!/bin/bash

while [ -f /etc/passwd ]; do
	./osmo-bts-omldummy $*
	sleep 1
done
