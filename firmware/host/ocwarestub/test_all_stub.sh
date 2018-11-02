#!/bin/sh

echo "##############################"
echo "     GET CONFIG PARAMS - 1    "
echo "##############################"
./get_params_stub.sh bms config
./get_params_stub.sh gpp config
./get_params_stub.sh sdr config
./get_params_stub.sh rf config
./get_params_stub.sh power config
./get_params_stub.sh ethernet config
./get_params_stub.sh sync config

echo "##############################"
echo "      SET CONFIG PARAMS       "
echo "##############################"
../scripts/set_config_params.sh bms
../scripts/set_config_params.sh gpp
../scripts/set_config_params.sh sdr
../scripts/set_config_params.sh rf
../scripts/set_config_params.sh power
../scripts/set_config_params.sh ethernet
../scripts/set_config_params.sh sync

echo "##############################"
echo "     GET CONFIG PARAMS - 2    "
echo "##############################"
./get_params_stub.sh bms config
./get_params_stub.sh gpp config
./get_params_stub.sh sdr config
./get_params_stub.sh rf config
./get_params_stub.sh power config
./get_params_stub.sh ethernet config
./get_params_stub.sh sync config

echo "##############################"
echo "      GET STATUS PARAMS       "
echo "##############################"
./get_params_stub.sh bms status
./get_params_stub.sh gpp status
./get_params_stub.sh sdr status
./get_params_stub.sh rf status
./get_params_stub.sh power status
./get_params_stub.sh ethernet status
./get_params_stub.sh sync status
./get_params_stub.sh testmodule status
./get_params_stub.sh obc status
./get_params_stub.sh hci status
./get_params_stub.sh system status

echo "##############################"
echo "   GET DEBUG COMMANDS - 1     "
echo "##############################"
./get_params_stub.sh debug I2C
./get_params_stub.sh debug GPIO
./get_params_stub.sh debug MDIO


echo "##############################"
echo "      SET DEBUG COMMANDS      "
echo "##############################"
../scripts/set_config_params.sh debug I2C
../scripts/set_config_params.sh debug GPIO
../scripts/set_config_params.sh debug MDIO


echo "##############################"
echo "   GET DEBUG COMMANDS - 2     "
echo "##############################"
./get_params_stub.sh debug I2C
./get_params_stub.sh debug GPIO
./get_params_stub.sh debug MDIO
echo "    >>>  TESTING DONE  <<<       "
