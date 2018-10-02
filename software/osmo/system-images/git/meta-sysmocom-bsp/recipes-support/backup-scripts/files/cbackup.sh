#!/bin/sh
# Starts a lower priority version of cbackup
nice -n 5 /usr/bin/.cbackup "$@"

