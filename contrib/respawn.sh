#!/bin/sh

PID=$$
echo "-1000" > /proc/$PID/oom_score_adj

trap "kill 0" EXIT

while [ -e /etc/passwd ]; do
	cat /lib/firmware/sysmobts-v?.bit > /dev/fpgadl_par0
	sleep 2s
	cat /lib/firmware/sysmobts-v?.out > /dev/dspdl_dm644x_0
	sleep 1s
	echo "0" > /sys/class/leds/activity_led/brightness
	(echo "0" > /proc/self/oom_score_adj && exec nice -n -20 $*) &
	LAST_PID=$!
	wait $LAST_PID
	sleep 10s
done
