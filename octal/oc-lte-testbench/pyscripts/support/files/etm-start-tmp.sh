#!/bin/sh

source /tmp/system.conf

usage()
{
    echo "Usage: $0 [ETM]"
    echo ""
    echo "    Start ETM"
    echo ""
    echo "    <ETM>    ETM Test (1.1, 1.2, 2, 3.1, 3.2, 3.3)"
    exit 1
}

if [ "$#" -eq "0" ]; then
    ETM="1.1"
elif [ "$#" -eq "1" ]; then
    ETM=$1
else
    >&2 usage
fi

if [ "${ETM}" == "1.1" ]; then
    etm="1"
elif [ "${ETM}" == "1.2" ]; then
    etm="2"
elif [ "${ETM}" == "2" ]; then
    etm="3"
elif [ "${ETM}" == "3.1" ]; then
    etm="4"
elif [ "${ETM}" == "3.2" ]; then
    etm="5"
elif [ "${ETM}" == "3.3" ]; then
    etm="6"
else
    >&2 usage
fi

killall pltD 2> /dev/null
cd /tmpfs; tar -zxf /tmp/wfm/ETM_${BW}MHz_TV.tgz
cd /tmpfs; ./pltD -drv dl -bw ${BW} -tc 0001 -etm ${etm} -ant 2 -c 1 > /dev/null &
