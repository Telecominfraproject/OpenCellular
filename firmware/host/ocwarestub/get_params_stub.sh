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
../bin/occmd bms.ec.config.current_sensor1.currlimit.get
../bin/occmd bms.ec.config.current_sensor2.currlimit.get
elif [ $2 = "status" ]; then
../bin/occmd bms.ec.status.temp_sensor1.temperature.get
../bin/occmd bms.ec.status.current_sensor1.busvoltage.get
../bin/occmd bms.ec.status.current_sensor1.shuntvoltage.get
../bin/occmd bms.ec.status.current_sensor1.current.get
../bin/occmd bms.ec.status.current_sensor1.power.get
../bin/occmd bms.ec.status.current_sensor2.busvoltage.get
../bin/occmd bms.ec.status.current_sensor2.shuntvoltage.get
../bin/occmd bms.ec.status.current_sensor2.current.get
../bin/occmd bms.ec.status.current_sensor2.power.get
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
../bin/occmd gpp.ap.config.temp_sensor3.lowlimit.get
../bin/occmd gpp.ap.config.temp_sensor3.highlimit.get
../bin/occmd gpp.ap.config.temp_sensor3.critlimit.get
../bin/occmd gpp.ap.config.current_sensor1.currlimit.get
../bin/occmd gpp.msata.config.current_sensor1.currlimit.get
elif [ $2 = "status" ]; then
../bin/occmd gpp.ap.status.temp_sensor1.temperature.get
../bin/occmd gpp.ap.status.temp_sensor2.temperature.get
../bin/occmd gpp.ap.status.temp_sensor3.temperature.get
../bin/occmd gpp.ap.status.current_sensor1.busvoltage.get
../bin/occmd gpp.ap.status.current_sensor1.shuntvoltage.get
../bin/occmd gpp.ap.status.current_sensor1.current.get
../bin/occmd gpp.ap.status.current_sensor1.power.get
../bin/occmd gpp.msata.status.current_sensor1.busvoltage.get
../bin/occmd gpp.msata.status.current_sensor1.shuntvoltage.get
../bin/occmd gpp.msata.status.current_sensor1.current.get
../bin/occmd gpp.msata.status.current_sensor1.power.get
else
        echo "Invalid Option"
fi
elif [ $1 = "sdr" ]; then
if [ $2 = "config" ]; then
../bin/occmd sdr.comp_all.config.current_sensor1.currlimit.get
../bin/occmd sdr.fpga.config.temp_sensor1.lowlimit.get
../bin/occmd sdr.fpga.config.temp_sensor1.highlimit.get
../bin/occmd sdr.fpga.config.temp_sensor1.critlimit.get
../bin/occmd sdr.fpga.config.current_sensor1.currlimit.get
elif [ $2 = "status" ]; then
../bin/occmd sdr.comp_all.status.current_sensor1.busvoltage.get
../bin/occmd sdr.comp_all.status.current_sensor1.shuntvoltage.get
../bin/occmd sdr.comp_all.status.current_sensor1.current.get
../bin/occmd sdr.comp_all.status.current_sensor1.power.get
../bin/occmd sdr.fpga.status.temp_sensor1.temperature.get
../bin/occmd sdr.fpga.status.current_sensor1.busvoltage.get
../bin/occmd sdr.fpga.status.current_sensor1.shuntvoltage.get
../bin/occmd sdr.fpga.status.current_sensor1.current.get
../bin/occmd sdr.fpga.status.current_sensor1.power.get
../bin/occmd sdr.comp_all.status.eeprom.dev_id.get
else
        echo "Invalid Option"
fi
elif [ $1 = "rf" ]; then
if [ $2 = "config" ]; then
../bin/occmd rffe.ch1_sensor.config.temp_sensor1.lowlimit.get
../bin/occmd rffe.ch1_sensor.config.temp_sensor1.highlimit.get
../bin/occmd rffe.ch1_sensor.config.temp_sensor1.critlimit.get
../bin/occmd rffe.ch1_sensor.config.current_sensor1.currlimit.get
../bin/occmd rffe.ch2_sensor.config.temp_sensor1.lowlimit.get
../bin/occmd rffe.ch2_sensor.config.temp_sensor1.highlimit.get
../bin/occmd rffe.ch2_sensor.config.temp_sensor1.critlimit.get
../bin/occmd rffe.ch2_sensor.config.current_sensor1.currlimit.get
#../bin/occmd rffe.ch1_fe.config.rf.band get
#../bin/occmd rffe.ch1_fe.config.rf.arfcn get
../bin/occmd rffe.ch1_fe.config.tx.atten.get
../bin/occmd rffe.ch1_fe.config.rx.atten.get
#../bin/occmd rffe.ch2_fe.config.rf.band get
#../bin/occmd rffe.ch2_fe.config.rf.arfcn get
../bin/occmd rffe.ch2_fe.config.tx.atten.get
../bin/occmd rffe.ch2_fe.config.rx.atten.get
elif [ $2 = "status" ]; then
../bin/occmd rffe.ch1_sensor.status.temp_sensor1.temperature.get
../bin/occmd rffe.ch1_sensor.status.current_sensor1.busvoltage.get
../bin/occmd rffe.ch1_sensor.status.current_sensor1.shuntvoltage.get
../bin/occmd rffe.ch1_sensor.status.current_sensor1.current.get
../bin/occmd rffe.ch1_sensor.status.current_sensor1.power.get
../bin/occmd rffe.ch2_sensor.status.temp_sensor1.temperature.get
../bin/occmd rffe.ch2_sensor.status.current_sensor1.busvoltage.get
../bin/occmd rffe.ch2_sensor.status.current_sensor1.shuntvoltage.get
../bin/occmd rffe.ch2_sensor.status.current_sensor1.current.get
../bin/occmd rffe.ch2_sensor.status.current_sensor1.power.get
../bin/occmd rffe.ch1_fe.status.power.forward.get
../bin/occmd rffe.ch1_fe.status.power.reverse.get
../bin/occmd rffe.ch2_fe.status.power.forward.get
../bin/occmd rffe.ch2_fe.status.power.reverse.get
../bin/occmd rffe.comp_all.status.eeprom.dev_id.get
else
        echo "Invalid Option"
fi
elif [ $1 = "power" ]; then
if [ $2 = "config" ]; then
../bin/occmd power.leadacid_sensor.config.temp_sensor1.lowlimit.get
../bin/occmd power.leadacid_sensor.config.temp_sensor1.highlimit.get
../bin/occmd power.leadacid_sensor.config.temp_sensor1.critlimit.get
../bin/occmd power.leadacid.config.battery.batteryVoltageLow.get
../bin/occmd power.leadacid.config.battery.batteryVoltageHigh.get
../bin/occmd power.leadacid.config.battery.batteryCurrentLow.get
../bin/occmd power.leadacid.config.battery.inputVoltageLow.get
../bin/occmd power.leadacid.config.battery.inputCurrentHigh.get
../bin/occmd power.leadacid.config.battery.inputCurrentLimit.get
../bin/occmd power.leadacid.config.battery.icharge.get
../bin/occmd power.leadacid.config.battery.vcharge.get
../bin/occmd power.lion.config.battery.batteryVoltageLow.get
../bin/occmd power.lion.config.battery.batteryVoltageHigh.get
../bin/occmd power.lion.config.battery.batteryCurrentLow.get
../bin/occmd power.lion.config.battery.inputVoltageLow.get
../bin/occmd power.lion.config.battery.inputCurrentHigh.get
../bin/occmd power.lion.config.battery.inputCurrentLimit.get
../bin/occmd power.lion.config.battery.icharge.get
../bin/occmd power.lion.config.battery.vcharge.get
elif [ $2 = "status" ]; then
../bin/occmd power.leadacid_sensor.status.temp_sensor1.temperature.get
../bin/occmd power.leadacid.status.battery.batteryVoltage.get
../bin/occmd power.leadacid.status.battery.batteryCurrent.get
../bin/occmd power.leadacid.status.battery.systemVoltage.get
../bin/occmd power.leadacid.status.battery.inputVoltage.get
../bin/occmd power.leadacid.status.battery.inputCurrent.get
../bin/occmd power.leadacid.status.battery.dieTemperature.get
../bin/occmd power.lion.status.battery.batteryVoltage.get
../bin/occmd power.lion.status.battery.batteryCurrent.get
../bin/occmd power.lion.status.battery.systemVoltage.get
../bin/occmd power.lion.status.battery.inputVoltage.get
../bin/occmd power.lion.status.battery.inputCurrent.get
../bin/occmd power.lion.status.battery.dieTemperature.get
../bin/occmd power.comp_all.status.powerSource.extBattAccessebility.get
../bin/occmd power.comp_all.status.powerSource.extBattAvailability.get
../bin/occmd power.comp_all.status.powerSource.intBattAccessebility.get
../bin/occmd power.comp_all.status.powerSource.intBattAvailability.get
../bin/occmd power.comp_all.status.powerSource.poeAccessebility.get
../bin/occmd power.comp_all.status.powerSource.poeAvailability.get
../bin/occmd power.comp_all.status.powerSource.solarAccessebility.get
../bin/occmd power.comp_all.status.powerSource.solarAvailability.get
else
        echo "Invalid Option"
fi
elif [ $1 = "ethernet" ]; then
if [ $2 = "config" ]; then
../bin/occmd ethernet.port0.config.speed.get
../bin/occmd ethernet.port0.config.duplex.get
../bin/occmd ethernet.port0.config.powerDown.get
../bin/occmd ethernet.port0.config.enable_sleepMode.get
../bin/occmd ethernet.port0.config.enable_interrupt.get
#../bin/occmd ethernet.port0.config.switch_reset get
#../bin/occmd ethernet.port0.config.restart_autoneg get
../bin/occmd ethernet.port1.config.speed.get
../bin/occmd ethernet.port1.config.duplex.get
../bin/occmd ethernet.port1.config.powerDown.get
../bin/occmd ethernet.port1.config.enable_sleepMode.get
../bin/occmd ethernet.port1.config.enable_interrupt.get
#../bin/occmd ethernet.port1.config.switch_reset get
#../bin/occmd ethernet.port1.config.restart_autoneg get
../bin/occmd ethernet.port2.config.speed.get
../bin/occmd ethernet.port2.config.duplex.get
../bin/occmd ethernet.port2.config.powerDown.get
../bin/occmd ethernet.port2.config.enable_sleepMode.get
../bin/occmd ethernet.port3.config.enable_interrupt.get
#../bin/occmd ethernet.port2.config.switch_reset get
#../bin/occmd ethernet.port2.config.restart_autoneg get
../bin/occmd ethernet.port3.config.speed.get
../bin/occmd ethernet.port3.config.duplex.get
../bin/occmd ethernet.port3.config.powerDown.get
../bin/occmd ethernet.port3.config.enable_sleepMode.get
../bin/occmd ethernet.port3.config.enable_interrupt.get
#../bin/occmd ethernet.port3.config.switch_reset get
#../bin/occmd ethernet.port3.config.restart_autoneg get
../bin/occmd ethernet.port4.config.speed.get
../bin/occmd ethernet.port4.config.duplex.get
../bin/occmd ethernet.port4.config.powerDown.get
../bin/occmd ethernet.port4.config.enable_sleepMode.get
../bin/occmd ethernet.port4.config.enable_interrupt.get
#../bin/occmd ethernet.port4.config.switch_reset get
#../bin/occmd ethernet.port4.config.restart_autoneg get
elif [ $2 = "status" ]; then
../bin/occmd ethernet.port0.status.speed.get
../bin/occmd ethernet.port0.status.duplex.get
../bin/occmd ethernet.port0.status.sleep_mode_en.get
../bin/occmd ethernet.port0.status.autoneg_on.get
../bin/occmd ethernet.port0.status.autoneg_complete.get
../bin/occmd ethernet.port0.status.link_up.get
../bin/occmd ethernet.port1.status.speed.get
../bin/occmd ethernet.port1.status.duplex.get
../bin/occmd ethernet.port1.status.sleep_mode_en.get
../bin/occmd ethernet.port1.status.autoneg_on.get
../bin/occmd ethernet.port1.status.autoneg_complete.get
../bin/occmd ethernet.port1.status.link_up.get
../bin/occmd ethernet.port2.status.speed.get
../bin/occmd ethernet.port2.status.duplex.get
../bin/occmd ethernet.port2.status.sleep_mode_en.get
../bin/occmd ethernet.port2.status.autoneg_on.get
../bin/occmd ethernet.port2.status.autoneg_complete.get
../bin/occmd ethernet.port2.status.link_up.get
../bin/occmd ethernet.port3.status.speed.get
../bin/occmd ethernet.port3.status.duplex.get
../bin/occmd ethernet.port3.status.sleep_mode_en.get
../bin/occmd ethernet.port3.status.autoneg_on.get
../bin/occmd ethernet.port3.status.autoneg_complete.get
../bin/occmd ethernet.port3.status.link_up.get
../bin/occmd ethernet.port4.status.speed.get
../bin/occmd ethernet.port4.status.duplex.get
../bin/occmd ethernet.port4.status.sleep_mode_en.get
../bin/occmd ethernet.port4.status.autoneg_on.get
../bin/occmd ethernet.port4.status.autoneg_complete.get
../bin/occmd ethernet.port4.status.link_up.get
else
        echo "Invalid Option"
fi
elif [ $1 = "sync" ]; then
if [ $2 = "config" ]; then
../bin/occmd sync.sensor.config.temp_sensor1.lowlimit.get
../bin/occmd sync.sensor.config.temp_sensor1.highlimit.get
../bin/occmd sync.sensor.config.temp_sensor1.critlimit.get
elif [ $2 = "status" ]; then
../bin/occmd sync.gps.status.gps_lock.get
../bin/occmd sync.sensor.status.temp_sensor1.temperature.get
else
        echo "Invalid Option"
fi
elif [ $1 = "testmodule" ]; then
if [ $2 = "status" ]; then
../bin/occmd testmodule.2gsim.status.imei.get
../bin/occmd testmodule.2gsim.status.imsi.get
../bin/occmd testmodule.2gsim.status.mfg.get
../bin/occmd testmodule.2gsim.status.model.get
../bin/occmd testmodule.2gsim.status.rssi.get
../bin/occmd testmodule.2gsim.status.ber.get
../bin/occmd testmodule.2gsim.status.registration.get
../bin/occmd testmodule.2gsim.status.network_operatorinfo.get
../bin/occmd testmodule.2gsim.status.cellid.get
../bin/occmd testmodule.2gsim.status.bsic.get
../bin/occmd testmodule.2gsim.status.lasterror.get
else
        echo "Invalid Option"
fi
elif [ $1 = "obc" ]; then
if [ $2 = "status" ]; then
../bin/occmd obc.iridium.status.imei.get
../bin/occmd obc.iridium.status.mfg.get
../bin/occmd obc.iridium.status.model.get
../bin/occmd obc.iridium.status.signal_quality.get
../bin/occmd obc.iridium.status.registration.get
../bin/occmd obc.iridium.status.numberofoutgoingmessage.get
../bin/occmd obc.iridium.status.lasterror.get
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
#../bin/occmd hci.led.config.ledstate get
else
        echo "Invalid Option"
fi
elif [ $1 = "system" ]; then
if [ $2 = "status" ]; then
../bin/occmd system.comp_all.status.eeprom_sid.gbcboardinfo.get
../bin/occmd system.comp_all.status.eeprom_sid.ocserialinfo.get
../bin/occmd system.comp_all.config.eeprom_mac.address.get
../bin/occmd system.comp_all.echo
../bin/occmd system.comp_all.reset

../bin/occmd testmodule.2gsim.disconnect_nw
../bin/occmd testmodule.2gsim.connect_nw
../bin/occmd testmodule.2gsim.send 9789799425 hi
../bin/occmd testmodule.2gsim.dial 9789799425
../bin/occmd testmodule.2gsim.answer_call
../bin/occmd testmodule.2gsim.hangup_call
../bin/occmd testmodule.comp_all.reset
../bin/occmd obc.iridium.reset
../bin/occmd ethernet.port0.reset
../bin/occmd ethernet.port1.reset
../bin/occmd ethernet.port2.reset
../bin/occmd ethernet.port3.reset
../bin/occmd power.pse.reset
../bin/occmd rffe.comp_all.reset
../bin/occmd gpp.ap.reset
../bin/occmd hci.led.fw.set 1
../bin/occmd rffe.ch1_fe.enable
../bin/occmd rffe.ch1_fe.disable
../bin/occmd rffe.ch2_fe.disable
../bin/occmd rffe.ch2_fe.enable
../bin/occmd testmodule.2gsim.enable
../bin/occmd testmodule.2gsim.disable
../bin/occmd sdr.comp_all.reset
../bin/occmd sdr.fx3.reset
../bin/occmd ethernet.port0.en_loopBk 0
../bin/occmd ethernet.port0.en_pktGen 8374
../bin/occmd ethernet.port0.dis_pktGen
../bin/occmd ethernet.port0.dis_loopBk 0
../bin/occmd ethernet.port1.en_loopBk 0
../bin/occmd ethernet.port1.en_pktGen 8374
../bin/occmd ethernet.port1.dis_pktGen
../bin/occmd ethernet.port1.dis_loopBk 0
../bin/occmd ethernet.port2.en_loopBk 0
../bin/occmd ethernet.port2.en_pktGen 8374
../bin/occmd ethernet.port2.dis_pktGen
../bin/occmd ethernet.port2.dis_loopBk 0
../bin/occmd ethernet.port3.en_loopBk 0
../bin/occmd ethernet.port3.en_pktGen 8374
../bin/occmd ethernet.port3.dis_pktGen
../bin/occmd ethernet.port3.dis_loopBk 0
../bin/occmd ethernet.port4.en_loopBk 0
../bin/occmd ethernet.port4.en_pktGen 8374
../bin/occmd ethernet.port4.dis_pktGen
../bin/occmd ethernet.port4.dis_loopBk 0
else
        echo "Invalid Option"
fi
elif [ $1 = "debug" ]; then
if [ $2 = "I2C" ]; then
../bin/occmd debug.I2C.bus0.get 104 2 58
../bin/occmd debug.I2C.bus1.get 24 2 2
../bin/occmd debug.I2C.bus2.get 29 1 0
../bin/occmd debug.I2C.bus3.get 68 2 255
../bin/occmd debug.I2C.bus4.get 65 2 254
../bin/occmd debug.I2C.bus6.get 64 2 5
../bin/occmd debug.I2C.bus7.get 69 2 5
../bin/occmd debug.I2C.bus8.get 26 2 6
elif [ $2 = "GPIO" ]; then
../bin/occmd debug.ec.PA.get 1
../bin/occmd debug.ec.PB.get 1
../bin/occmd debug.ec.PC.get 1
../bin/occmd debug.ec.PD.get 7
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
../bin/occmd debug.sdr.ioexpanderx1E.get 1
../bin/occmd debug.fe.ioexpanderx18.get 1
../bin/occmd debug.fe.ioexpanderx1A.get 0
../bin/occmd debug.fe.ioexpanderx1B.get 1
../bin/occmd debug.fe.ioexpanderx1C.get 1
../bin/occmd debug.fe.ioexpanderx1D.get 1
../bin/occmd debug.sync.ioexpanderx71.get 1
elif [ $2 = "MDIO" ]; then
../bin/occmd debug.ethernet.port0.get 1
../bin/occmd debug.ethernet.port1.get 1
../bin/occmd debug.ethernet.port2.get 1
../bin/occmd debug.ethernet.port3.get 1
../bin/occmd debug.ethernet.port4.get 1
../bin/occmd debug.ethernet.global1.get 1
../bin/occmd debug.ethernet.global2.get 1
../bin/occmd debug.ethernet.swport0.get 1
../bin/occmd debug.ethernet.swport1.get 1
../bin/occmd debug.ethernet.swport2.get 1
../bin/occmd debug.ethernet.swport3.get 1
../bin/occmd debug.ethernet.swport4.get 1
../bin/occmd debug.ethernet.swport5.get 1
../bin/occmd debug.ethernet.swport6.get 1
else
        echo "Invalid Option"
fi
else
        echo "Invalid Option"
fi

echo "Done..."
