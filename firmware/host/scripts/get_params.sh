#!/bin/sh
if [ -z "$1" ]; then
        echo "Invalid Option"
    echo "Usage : ./get_params <subsystem> <class>"
        exit
elif [ $1 = "bms" ]; then
if [ $2 = "config" ]; then
../bin/occmd bms.ec.config.temp_sensor1.lowlimit.get
../bin/occmd bms.ec.config.temp_sensor1.highlimit.get
../bin/occmd bms.ec.config.temp_sensor1.critlimit.get
../bin/occmd bms.ec.config.temp_sensor2.lowlimit.get
../bin/occmd bms.ec.config.temp_sensor2.highlimit.get
../bin/occmd bms.ec.config.temp_sensor2.critlimit.get
../bin/occmd bms.ec.config.current_sensor1.currlimit.get
../bin/occmd bms.ec.config.current_sensor2.currlimit.get
elif [ $2 = "status" ]; then
../bin/occmd bms.ec.status.temp_sensor1.temperature.get
../bin/occmd bms.ec.status.temp_sensor2.temperature.get
../bin/occmd bms.ec.status.current_sensor1.busvoltage.get
../bin/occmd bms.ec.status.current_sensor1.shuntvoltage.get
../bin/occmd bms.ec.status.current_sensor1.current.get
../bin/occmd bms.ec.status.current_sensor1.power.get
../bin/occmd bms.ec.status.current_sensor2.busvoltage.get
../bin/occmd bms.ec.status.current_sensor2.shuntvoltage.get
../bin/occmd bms.ec.status.current_sensor2.current.get
../bin/occmd bms.ec.status.current_sensor2.power.get
../bin/occmd bms.ps.status.si1141.partId.get
../bin/occmd bms.ps.status.si1141.revId.get
../bin/occmd bms.ps.status.si1141.seqId.get
else
        echo "Invalid Option"
fi
elif [ $1 = "gpp" ]; then
if [ $2 = "config" ]; then
../bin/occmd gpp.ap.config.temp_sensor1.lowlimit.get
../bin/occmd gpp.ap.config.temp_sensor1.highlimit.get
../bin/occmd gpp.ap.config.temp_sensor1.critlimit.get
../bin/occmd gpp.ap.config.temp_sensor2.lowlimit.get
../bin/occmd gpp.ap.config.temp_sensor2.highlimit.get
../bin/occmd gpp.ap.config.temp_sensor2.critlimit.get
../bin/occmd gpp.ap.config.current_sensor1.currlimit.get
../bin/occmd gpp.ap.config.mp2951.inputVoltageOnLimit.get
../bin/occmd gpp.ap.config.mp2951.inputVoltageOffLimit.get
../bin/occmd gpp.msata.config.current_sensor1.currlimit.get
elif [ $2 = "status" ]; then
../bin/occmd gpp.ap.status.temp_sensor1.temperature.get
../bin/occmd gpp.ap.status.temp_sensor2.temperature.get
../bin/occmd gpp.ap.status.current_sensor1.busvoltage.get
../bin/occmd gpp.ap.status.current_sensor1.shuntvoltage.get
../bin/occmd gpp.ap.status.current_sensor1.current.get
../bin/occmd gpp.ap.status.current_sensor1.power.get
../bin/occmd gpp.ap.status.mp2951.vendorId.get
../bin/occmd gpp.ap.status.mp2951.productId.get
../bin/occmd gpp.msata.status.current_sensor1.busvoltage.get
../bin/occmd gpp.msata.status.current_sensor1.shuntvoltage.get
../bin/occmd gpp.msata.status.current_sensor1.current.get
../bin/occmd gpp.msata.status.current_sensor1.power.get
else
        echo "Invalid Option"
fi
elif [ $1 = "ethernet" ]; then
if [ $2 = "config" ]; then
../bin/occmd ethernet.port1.config.speed.get
../bin/occmd ethernet.port1.config.duplex.get
../bin/occmd ethernet.port1.config.powerDown.get
../bin/occmd ethernet.port1.config.enable_interrupt.get
../bin/occmd ethernet.port1.config.switch_reset.get
../bin/occmd ethernet.port1.config.restart_autoneg.get
../bin/occmd ethernet.port2.config.speed.get
../bin/occmd ethernet.port2.config.duplex.get
../bin/occmd ethernet.port2.config.powerDown.get
../bin/occmd ethernet.port2.config.enable_interrupt.get
../bin/occmd ethernet.port2.config.switch_reset.get
../bin/occmd ethernet.port2.config.restart_autoneg.get
../bin/occmd ethernet.port3.config.speed.get
../bin/occmd ethernet.port3.config.duplex.get
../bin/occmd ethernet.port3.config.powerDown.get
../bin/occmd ethernet.port3.config.enable_interrupt.get
../bin/occmd ethernet.port3.config.switch_reset.get
../bin/occmd ethernet.port3.config.restart_autoneg.get
../bin/occmd ethernet.port4.config.speed.get
../bin/occmd ethernet.port4.config.duplex.get
../bin/occmd ethernet.port4.config.powerDown.get
../bin/occmd ethernet.port4.config.enable_interrupt.get
../bin/occmd ethernet.port4.config.switch_reset.get
../bin/occmd ethernet.port4.config.restart_autoneg.get
../bin/occmd ethernet.port5.config.speed.get
../bin/occmd ethernet.port5.config.duplex.get
../bin/occmd ethernet.port5.config.powerDown.get
../bin/occmd ethernet.port5.config.enable_interrupt.get
../bin/occmd ethernet.port5.config.switch_reset.get
../bin/occmd ethernet.port5.config.restart_autoneg.get
../bin/occmd ethernet.port6.config.speed.get
../bin/occmd ethernet.port6.config.duplex.get
../bin/occmd ethernet.port6.config.powerDown.get
../bin/occmd ethernet.port6.config.enable_interrupt.get
../bin/occmd ethernet.port6.config.switch_reset.get
../bin/occmd ethernet.port6.config.restart_autoneg.get
../bin/occmd ethernet.port7.config.speed.get
../bin/occmd ethernet.port7.config.duplex.get
../bin/occmd ethernet.port7.config.powerDown.get
../bin/occmd ethernet.port7.config.enable_interrupt.get
../bin/occmd ethernet.port7.config.switch_reset.get
../bin/occmd ethernet.port7.config.restart_autoneg.get
../bin/occmd ethernet.temp_sensor.config.lowlimit.get
../bin/occmd ethernet.temp_sensor.config.highlimit.get
../bin/occmd ethernet.temp_sensor.config.critlimit.get
elif [ $2 = "status" ]; then
../bin/occmd ethernet.port1.status.speed.get
../bin/occmd ethernet.port1.status.duplex.get
../bin/occmd ethernet.port1.status.autoneg_on.get
../bin/occmd ethernet.port1.status.powerDown.get
../bin/occmd ethernet.port1.status.autoneg_complete.get
../bin/occmd ethernet.port1.status.link_up.get
../bin/occmd ethernet.port2.status.speed.get
../bin/occmd ethernet.port2.status.duplex.get
../bin/occmd ethernet.port2.status.autoneg_on.get
../bin/occmd ethernet.port2.status.powerDown.get
../bin/occmd ethernet.port2.status.autoneg_complete.get
../bin/occmd ethernet.port2.status.link_up.get
../bin/occmd ethernet.port3.status.speed.get
../bin/occmd ethernet.port3.status.duplex.get
../bin/occmd ethernet.port3.status.autoneg_on.get
../bin/occmd ethernet.port3.status.powerDown.get
../bin/occmd ethernet.port3.status.autoneg_complete.get
../bin/occmd ethernet.port3.status.link_up.get
../bin/occmd ethernet.port4.status.speed.get
../bin/occmd ethernet.port4.status.duplex.get
../bin/occmd ethernet.port4.status.autoneg_on.get
../bin/occmd ethernet.port4.status.powerDown.get
../bin/occmd ethernet.port4.status.autoneg_complete.get
../bin/occmd ethernet.port4.status.link_up.get
../bin/occmd ethernet.port5.status.speed.get
../bin/occmd ethernet.port5.status.duplex.get
../bin/occmd ethernet.port5.status.autoneg_on.get
../bin/occmd ethernet.port5.status.powerDown.get
../bin/occmd ethernet.port5.status.autoneg_complete.get
../bin/occmd ethernet.port5.status.link_up.get
../bin/occmd ethernet.port6.status.speed.get
../bin/occmd ethernet.port6.status.duplex.get
../bin/occmd ethernet.port6.status.autoneg_on.get
../bin/occmd ethernet.port6.status.powerDown.get
../bin/occmd ethernet.port6.status.autoneg_complete.get
../bin/occmd ethernet.port6.status.link_up.get
../bin/occmd ethernet.port7.status.speed.get
../bin/occmd ethernet.port7.status.duplex.get
../bin/occmd ethernet.port7.status.autoneg_on.get
../bin/occmd ethernet.port7.status.powerDown.get
../bin/occmd ethernet.port7.status.autoneg_complete.get
../bin/occmd ethernet.port7.status.link_up.get
../bin/occmd ethernet.temp_sensor.status.temperature.get
else
        echo "Invalid Option"
fi
elif [ $1 = "hci" ]; then
if [ $2 = "config" ]; then
../bin/occmd hci.led.config.temp_sensor1.lowlimit.get
../bin/occmd hci.led.config.temp_sensor1.highlimit.get
../bin/occmd hci.led.config.temp_sensor1.critlimit.get
elif [ $2 = "status" ]; then
../bin/occmd hci.led.status.temp_sensor1.temperature.get
else
        echo "Invalid Option"
fi
elif [ $1 = "system" ]; then
if [ $2 = "status" ]; then
../bin/occmd system.comp_all.status.eeprom_sid.gbcboardinfo.get
../bin/occmd system.comp_all.status.eeprom_sid.ocserialinfo.get
../bin/occmd system.comp_all.config.eeprom_mac.address.get
../bin/occmd system.comp_all.echo
#../bin/occmd ethernet.port1.reset
#../bin/occmd ethernet.port2.reset
#../bin/occmd ethernet.port3.reset
#../bin/occmd ethernet.port4.reset
#../bin/occmd ethernet.port5.reset
#../bin/occmd ethernet.port6.reset
#../bin/occmd ethernet.port7.reset
../bin/occmd hci.led.fw.set 1
else
        echo "Invalid Option"
fi
elif [ $1 = "post" ]; then
../bin/occmd system.comp_all.post.enable.set
../bin/occmd system.comp_all.post.results.get
elif [ $1 = "debug" ]; then
if [ $2 = "I2C" ]; then
../bin/occmd debug.I2C.bus0.get 32 1 239
../bin/occmd debug.I2C.bus1.get 24 2 5
../bin/occmd debug.I2C.bus2.get 32 2 6
#../bin/occmd debug.I2C.bus3.get 68 2 255
#../bin/occmd debug.I2C.bus4.get 65 2 254
../bin/occmd debug.I2C.bus6.get 68 2 4
#../bin/occmd debug.I2C.bus7.get 69 2 5
../bin/occmd debug.I2C.bus8.get 90 2 0
elif [ $2 = "GPIO" ]; then
../bin/occmd debug.ec.PA.get 1
../bin/occmd debug.ec.PB.get 1
../bin/occmd debug.ec.PC.get 1
../bin/occmd debug.ec.PD.get 5
../bin/occmd debug.ec.PE.get 1
../bin/occmd debug.ec.PF.get 1
../bin/occmd debug.ec.PG.get 1
../bin/occmd debug.ec.PJ.get 1
../bin/occmd debug.ec.PK.get 1
../bin/occmd debug.ec.PL.get 5
../bin/occmd debug.ec.PM.get 1
../bin/occmd debug.ec.PN.get 1
../bin/occmd debug.ec.PP.get 3
../bin/occmd debug.ec.PQ.get 1
../bin/occmd debug.gbc.ioexpanderx70.get 1
../bin/occmd debug.gbc.ioexpanderx71.get 0
else
        echo "Invalid Option"
fi
else
        echo "Invalid Option"
fi

echo "Done..."
