#!/bin/sh

LOGFILE=./log


if [ `ps |grep -c ccli` = 1 ];then

ccli 2> /dev/null << EOF > $LOGFILE
rsc
schedstats 0 0
exit
EOF

else
   echo *** ccli in use 
   exit 1
fi
 

if [ "$1" == "state" ];then
#  grep  -A2 state $LOGFILE |tail -1 |awk '{print $9}'
   grep  -A2 state $LOGFILE | sed -n 3p | awk '{print $9}'
else
  grep -i $1 .log
fi


exit 0 
           

