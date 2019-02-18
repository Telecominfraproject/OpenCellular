#!/bin/sh
ARG1=$1
ARG2=$2
ARG3=$3
ARG4=$4
LOCAL=""

C_ALL=all
C_TEMP=temp
C_TEMP_ALERT=temp_alert_limit
C_TEMP_HYST=temp_hyst
C_SHUNT_VOLTAGE=shunt_voltage
C_BUS_VOLTAGE=bus_voltage
C_CURRENT=current
C_CURRENT_ALERT_LIMIT=current_alert_limit
C_BB_ALERT_GPIO=bb_alert_gpio
C_FE_ALERT_GPIO=fe_alert_gpio

TEMP="$LOCAL/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0048/temp1_input"
TEMP_ALERT_LIMIT="$LOCAL/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0048/temp1_max"
TEMP_HYST="$LOCAL/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0048/temp1_max_hyst"
SHUNT_VOLTAGE="$LOCAL/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0044/in0_input"
BUS_VOLTAGE="$LOCAL/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0044/in1_input"
CURRENT="$LOCAL/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0044/curr1_input"
CURRENT_ALERT_LIMIT="$LOCAL/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0044/alert_limit_register"
BB_ALERT_GPIO="$LOCAL/sys/class/gpio/gpio1/value"
FE_ALERT_GPIO="$LOCAL/sys/class/gpio/gpio13/value"

printall()
{
   echo ""
   a=$(cat $TEMP)
   echo $C_TEMP":"$a" mCelcius"

   a=$(cat $TEMP_ALERT_LIMIT)
   echo $C_TEMP_ALERT":"$a" mCelcius"

   a=$(cat $TEMP_HYST)
   echo $C_TEMP_HYST":"$a" mCelcius"

   a=$(cat $SHUNT_VOLTAGE)
   echo $C_SHUNT_VOLTAGE":"$a" mV"

   a=$(cat $BUS_VOLTAGE)
   echo $C_BUS_VOLTAGE":"$a" mV"

   a=$(cat $CURRENT)
   echo $C_CURRENT":"$a" mA"

   a=$(cat $CURRENT_ALERT_LIMIT)
   echo $C_CURRENT_ALERT_LIMIT":"$a" mA"

   read_gpio 1 $BB_ALERT_GPIO $C_BB_ALERT_GPIO
   read_gpio 13 $FE_ALERT_GPIO $C_FE_ALERT_GPIO

   echo ""
}

setone()
{
   if [ "$1" = $C_TEMP_ALERT ]; then
       echo $2 > $TEMP_ALERT_LIMIT
       exit
   fi

   if [ "$1" = $C_CURRENT_ALERT_LIMIT ]; then
       echo $2 > $CURRENT_ALERT_LIMIT
       exit
   fi
   help

}

read_gpio()
{
    echo $1 > /sys/class/gpio/export
    a=$(cat $2)
    echo $3" Alert GPIO:"$a
    echo $1 > /sys/class/gpio/unexport
}
printone()
{
   echo ""
   if [ "$1" = $C_TEMP ]; then 
       a=$(cat $TEMP)
       echo "Temperature:"$a" mCelcius"
       exit
   fi

   if [ "$1" = $C_TEMP_ALERT ]; then 
       a=$(cat $TEMP_ALERT_LIMIT)
       echo "Temperature Alert Limit:"$a" mCelcius"
       exit
   fi

   if [ "$1" = $C_TEMP_HYST ]; then 
       a=$(cat $TEMP_HYST)
       echo "Temperature hyst:"$a" mCelcius"
       exit
   fi

   if [ "$1" = $C_SHUNT_VOLTAGE ]; then 
       a=$(cat $SHUNT_VOLTAGE)
       echo "Shunt Voltage:"$a" mV"
       exit
   fi

   if [ "$1" = $C_BUS_VOLTAGE ]; then 
       a=$(cat $BUS_VOLTAGE)
       echo "Bus Voltage:"$a" mV"
       exit
   fi

   if [ "$1" = $C_CURRENT ]; then 
       a=$(cat $CURRENT)
       echo "Current:"$a" mA"
       exit
   fi

   if [ "$1" = $C_CURRENT_ALERT_LIMIT ]; then 
       a=$(cat $CURRENT_ALERT_LIMIT)
       echo "Current Alert Limit:"$a" mA"
       exit
   fi

   if [ "$1" = $C_BB_ALERT_GPIO ]; then 
       read_gpio 1 $BB_ALERT_GPIO $C_BB_ALERT_GPIO
       exit
   fi

   if [ "$1" = $C_FE_ALERT_GPIO ]; then 
       read_gpio 13 $FE_ALERT_GPIO $C_FE_ALERT_GPIO
       exit
   fi
   help

}

help()
{
    echo ""
    echo "./occmd.sh <module_name> <get/set> <param/all> <value>"
    echo " <module_name> : bb"
    echo " <get>: $C_ALL/$C_TEMP/$C_TEMP_ALERT/"
    echo "      $C_TEMP_HYST/$C_SHUNT_VOLTAGE/"
    echo "      $C_BUS_VOLTAGE/$C_CURRENT/$C_CURRENT_ALERT_LIMIT/"
    echo "      $C_BB_ALERT_GPIO/$C_FE_ALERT_GPIO"
    echo " <set>:$C_TEMP_ALERT <param> -55000 to 125000 deg C"
    echo "       $C_CURRENT_ALERT_LIMIT <param> 600 to 2500 mA"
    echo ""
}

if [ "$ARG1" = "help" ]; then
    help
    exit
fi

if [ "$ARG1" = "bb" ]; then
    if [ "$ARG2" = "get" ]; then
            if [ "$ARG3" = "all" ]; then
                printall 
                exit
            fi
            printone $1
    else 
      if [ "$ARG2" = "set" ]; then
        setone $ARG3 $ARG4
        exit
      fi
     help
    fi
fi
help
