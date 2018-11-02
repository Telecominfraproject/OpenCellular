#!/bin/sh

echo "##############################"
echo "     GET CONFIG PARAMS - 1    "
echo "##############################"
./get_params.sh bms config default
./get_params.sh gpp config default
./get_params.sh sdr config default
./get_params.sh rf config default
./get_params.sh power config default
./get_params.sh ethernet config default
./get_params.sh sync config default
./get_params.sh hci config default

echo "##############################"
echo "      SET CONFIG PARAMS       "
echo "##############################"
./set_config_params.sh bms
./set_config_params.sh gpp
./set_config_params.sh sdr
./set_config_params.sh rf
./set_config_params.sh power
./set_config_params.sh ethernet
./set_config_params.sh sync
./set_config_params.sh hci

echo "##############################"
echo "     GET CONFIG PARAMS - 2    "
echo "##############################"
./get_params.sh bms config verify
./get_params.sh gpp config verify
./get_params.sh sdr config verify
./get_params.sh rf config verify
./get_params.sh power config verify
./get_params.sh ethernet config verify
./get_params.sh sync config verify
./get_params.sh hci config verify

echo "##############################"
echo "        POST COMMANDS         "
echo "##############################"
./get_params.sh system post

echo "##############################"
echo "      GET STATUS PARAMS       "
echo "##############################"
./get_params.sh bms status
./get_params.sh gpp status
./get_params.sh sdr status
./get_params.sh rf status
./get_params.sh power status
./get_params.sh ethernet status
./get_params.sh sync status
./get_params.sh testmodule status
./get_params.sh obc status
./get_params.sh hci status
./get_params.sh system status

echo "##############################"
echo "   GET COMMANDS - 1           "
echo "##############################"
./get_params.sh command

echo "##############################"
echo "   GET DEBUG COMMANDS - 1     "
echo "##############################"
./get_params.sh debug I2C default
./get_params.sh debug GPIO default
./get_params.sh debug MDIO default

echo "##############################"
echo "      SET DEBUG COMMANDS      "
echo "##############################"
./set_config_params.sh debug I2C FW
./set_config_params.sh debug GPIO FW
./set_config_params.sh debug MDIO

echo "##############################"
echo "   GET DEBUG COMMANDS - 2     "
echo "##############################"
./get_params.sh debug I2C verify
./get_params.sh debug GPIO verify
./get_params.sh debug MDIO verify

echo "    >>>  TESTING DONE  <<<       "
