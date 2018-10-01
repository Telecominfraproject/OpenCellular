#!/bin/sh
# Starts a lower priority version of mbackup
nice -n 5 /usr/bin/.mbackup "$@"

