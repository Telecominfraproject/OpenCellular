#!/bin/bash
SLEEP_TIME=200
CARBON_HOST="127.0.0.1"
CARBON_PORT="2003"
if [ $# -ne 2 ]
  then
	echo "First start gRPC client with: ./ocp_grpc_config_client.host 2>&1 | tee <log.file>"
    echo "Usage: ./dsCollector.sh <log.file> <Index for starting>"
    exit 1
fi

if [ -e $1 ]
then
    echo "$1 log file found."
else
    echo "$1 log file doesn't exist."
    exit 1
fi

LOGFILE=$1
METRIC_PREFIX="ocps.telemetry"
DATA_ATTR=("TimeStamp " "P1V " "P1C " "ADP1V " "ADP1C" "L1V " "l1c " "L2V " "l2c " "L3V " "l3c " "L4V " "l4c " "L5V " "l5c" "PowerBattery " "PowerSolar " "PowerADP " "PowerL1 " "PowerL2 " "PowerL3 "  "PowerL4 " "PowerL5 " "BatteryVoltage " "BatteryCurrent " )  
START_INDEX=$2
SERIAL_ID=`cat $LOGFILE | grep "Serial id read is" -m 1 | awk -F' ' '{ print $NF}'` 
echo "Reading file $LOGFILE for serial id $SERIAL_ID"
while :
do
  DATA_POINTS=`cat $LOGFILE | grep ${DATA_ATTR[0]} | wc -l`
  echo "Reading $DATA_POINTS entries from $LOGFILE."
  if [ "$DATA_POINTS" -lt "$2" ];then
      echo "We only have $DATA_POINTS entries in the $1 log file.";
      exit 1
  fi
  for i in `seq $START_INDEX $((DATA_POINTS-1))` 
  do
    echo "Scanning $i entry."
    INDEX=$i
    j=0
    for attr in "${DATA_ATTR[@]}"
    do
      METRIC_NAME=$METRIC_PREFIX.$attr
      echo "Searching data Attribute $METRIC_NAME for index $INDEX."
      DATA_POINT[$INDEX,$j]=`cat $LOGFILE | grep ^$attr -m $((INDEX+1)) | tail -n 1 | awk -F': ' '{ print $NF}'`
      echo "DATA_POINT[$INDEX,$j] ($attr) is ${DATA_POINT[$INDEX,$j]}."  
      echo "Addded data point ${METRIC_NAME}${DATA_POINT[$INDEX,$j]} ${DATA_POINT[$INDEX,0]} to the db." 
      echo ${METRIC_NAME}${DATA_POINT[$INDEX,$j]} ${DATA_POINT[$INDEX,0]} | nc ${CARBON_HOST} ${CARBON_PORT}
      j=$((j+1))
    done
    START_INDEX=$((i+1))
    sleep 3;
  done
  echo "Added total $START_INDEX data points."
  echo "Sleeping for $SLEEP_TIME seconds."
  sleep $SLEEP_TIME;
done
