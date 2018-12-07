#!/bin/sh

echo "##############################"
echo "     GET CONFIG PARAMS - 1    "
echo "##############################"
./get_params.sh bms config
./get_params.sh gpp config
./get_params.sh ethernet config
./get_params.sh hci config

echo "##############################"
echo "      SET CONFIG PARAMS       "
echo "##############################"
./set_config_params.sh bms
./set_config_params.sh gpp
./set_config_params.sh ethernet
./set_config_params.sh hci

echo "##############################"
echo "     GET CONFIG PARAMS - 2    "
echo "##############################"
./get_params.sh bms config
./get_params.sh gpp config
./get_params.sh ethernet config
./get_params.sh hci config

echo "##############################"
echo "      GET STATUS PARAMS       "
echo "##############################"
./get_params.sh bms status
./get_params.sh gpp status
./get_params.sh ethernet status
./get_params.sh hci status
./get_params.sh system status

echo "##############################"
echo "   GET POST                   "
echo "##############################"
./get_params.sh post

echo "##############################"
echo "   GET DEBUG COMMANDS - 1     "
echo "##############################"
./get_params.sh debug I2C
#./get_params.sh debug GPIO

echo "##############################"
echo "      SET DEBUG COMMANDS      "
echo "##############################"
./set_config_params.sh debug I2C
#./set_config_params.sh debug GPIO

echo "##############################"
echo "   GET DEBUG COMMANDS - 2     "
echo "##############################"
./get_params.sh debug I2C
#./get_params.sh debug GPIO

echo "    >>>  TESTING DONE  <<<       "
