#!/bin/sh
if [ -z "$1" ]; then
        echo "Invalid Option"
    echo "Usage : ./set_config_params <subsystem>"
        exit
elif [ $1 = "bms" ]; then
../bin/occmd bms.ec.config.temp_sensor1.lowlimit.set -19
../bin/occmd bms.ec.config.temp_sensor1.highlimit.set 76
../bin/occmd bms.ec.config.temp_sensor1.critlimit.set 81
../bin/occmd bms.ec.config.temp_sensor2.lowlimit.set -19
../bin/occmd bms.ec.config.temp_sensor2.highlimit.set 76
../bin/occmd bms.ec.config.temp_sensor2.critlimit.set 81
../bin/occmd bms.ec.config.current_sensor1.currlimit.set 900
../bin/occmd bms.ec.config.current_sensor2.currlimit.set 900
elif [ $1 = "gpp" ]; then
../bin/occmd gpp.ap.config.temp_sensor1.lowlimit.set -19
../bin/occmd gpp.ap.config.temp_sensor1.highlimit.set 81
../bin/occmd gpp.ap.config.temp_sensor1.critlimit.set 85
../bin/occmd gpp.ap.config.temp_sensor2.lowlimit.set -19
../bin/occmd gpp.ap.config.temp_sensor2.highlimit.set 81
../bin/occmd gpp.ap.config.temp_sensor2.critlimit.set 85
../bin/occmd gpp.ap.config.current_sensor1.currlimit.set 1400
#../bin/occmd gpp.ap.config.mp2951.inputVoltageOnLimit.set 35
#../bin/occmd gpp.ap.config.mp2951.inputVoltageOffLimit.set 35
../bin/occmd gpp.msata.config.current_sensor1.currlimit.set 1400
elif [ $1 = "hci" ]; then
../bin/occmd hci.led.config.temp_sensor1.lowlimit.set -19
../bin/occmd hci.led.config.temp_sensor1.highlimit.set 76
../bin/occmd hci.led.config.temp_sensor1.critlimit.set 81
elif [ $1 = "ethernet" ]; then
../bin/occmd ethernet.port1.config.speed.set 1
../bin/occmd ethernet.port1.config.duplex.set 0
../bin/occmd ethernet.port1.config.powerDown.set 1
../bin/occmd ethernet.port1.config.enable_interrupt.set 1
../bin/occmd ethernet.port1.config.switch_reset.set 0
../bin/occmd ethernet.port1.config.restart_autoneg.set 0
../bin/occmd ethernet.port2.config.speed.set 1
../bin/occmd ethernet.port2.config.duplex.set 1
../bin/occmd ethernet.port2.config.powerDown.set 1
../bin/occmd ethernet.port2.config.enable_interrupt.set 0
../bin/occmd ethernet.port2.config.switch_reset.set 1
../bin/occmd ethernet.port2.config.restart_autoneg.set 1
../bin/occmd ethernet.port3.config.speed.set 1
../bin/occmd ethernet.port3.config.duplex.set 1
../bin/occmd ethernet.port3.config.powerDown.set 0
../bin/occmd ethernet.port3.config.enable_interrupt.set 1
../bin/occmd ethernet.port3.config.switch_reset.set 1
#../bin/occmd ethernet.port3.config.restart_autoneg.set 1
../bin/occmd ethernet.port4.config.speed.set 1
../bin/occmd ethernet.port4.config.duplex.set 1
../bin/occmd ethernet.port4.config.powerDown.set 1
../bin/occmd ethernet.port4.config.enable_interrupt.set 1
../bin/occmd ethernet.port4.config.switch_reset.set 1
../bin/occmd ethernet.port4.config.restart_autoneg.set 1
#../bin/occmd ethernet.port5.config.speed.set 2
#../bin/occmd ethernet.port5.config.duplex.set 1
#../bin/occmd ethernet.port5.config.powerDown.set 0
#../bin/occmd ethernet.port5.config.enable_interrupt.set 1
#../bin/occmd ethernet.port5.config.switch_reset.set 1
#../bin/occmd ethernet.port5.config.restart_autoneg.set 1
#../bin/occmd ethernet.port6.config.speed.set 1
#../bin/occmd ethernet.port6.config.duplex.set 0
#../bin/occmd ethernet.port6.config.powerDown.set 0
#../bin/occmd ethernet.port6.config.enable_interrupt.set 1
#../bin/occmd ethernet.port6.config.switch_reset.set 1
#../bin/occmd ethernet.port6.config.restart_autoneg.set 1
#../bin/occmd ethernet.port7.config.speed.set 1
#../bin/occmd ethernet.port7.config.duplex.set 1
#../bin/occmd ethernet.port7.config.powerDown.set 1
#../bin/occmd ethernet.port7.config.enable_interrupt.set 1
#../bin/occmd ethernet.port7.config.switch_reset.set 1
#../bin/occmd ethernet.port7.config.restart_autoneg.set 1
elif [ $1 = "debug" ]; then
if [ $2 = "I2C" ]; then
../bin/occmd debug.I2C.bus0.set 32 1 239 100
../bin/occmd debug.I2C.bus1.set 24 2 5 500
../bin/occmd debug.I2C.bus2.set 32 2 6 53400
#../bin/occmd debug.I2C.bus3.set 68 2 255 8000
#../bin/occmd debug.I2C.bus4.set 65 2 254 21500
../bin/occmd debug.I2C.bus6.set 68 2 4 11138
#../bin/occmd debug.I2C.bus7.set 69 2 5 25500
../bin/occmd debug.I2C.bus8.set 90 2 0 16600
elif [ $2 = "GPIO" ]; then
../bin/occmd debug.ec.PA.set 1 1
../bin/occmd debug.ec.PB.set 1 1
../bin/occmd debug.ec.PC.set 1 1
../bin/occmd debug.ec.PD.set 7 0
../bin/occmd debug.ec.PE.set 1 0
../bin/occmd debug.ec.PF.set 1 1
../bin/occmd debug.ec.PG.set 1 1
../bin/occmd debug.ec.PJ.set 1 1
../bin/occmd debug.ec.PK.set 1 1
../bin/occmd debug.ec.PL.set 5 1
../bin/occmd debug.ec.PM.set 1 0
../bin/occmd debug.ec.PN.set 1 0
../bin/occmd debug.ec.PP.set 3 0
../bin/occmd debug.ec.PQ.set 1 0

../bin/occmd debug.gbc.ioexpanderx70.set 1 0
../bin/occmd debug.gbc.ioexpanderx71.set 0 0
else
echo "Invalid Option"
fi

else
        echo "Invalid Option"
fi
echo "Done..."
