#!/bin/sh

case $2 in
up)
        echo "UP"
        /sbin/ifup $1
        ;;
down)
        echo "DOWN"
        /sbin/ifdown $1
        ;;
esac
