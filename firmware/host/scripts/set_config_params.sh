#!/bin/sh
if [ -z "$1" ]; then
        echo "Invalid Option"
    echo "Usage : ./set_config_params <subsystem>"
        exit
elif [ $1 = "post" ]; then
../bin/occmd system.comp_all.post.test.enable.set
elif [ $1 = "bms" ]; then
../bin/occmd bms.ec.config.temp_sensor1.lowlimit.set -19
../bin/occmd bms.ec.config.temp_sensor1.highlimit.set 76
../bin/occmd bms.ec.config.temp_sensor1.critlimit.set 85
../bin/occmd bms.ec.config.current_sensor1.currlimit.set 1500
../bin/occmd bms.ec.config.current_sensor2.currlimit.set 1500
elif [ $1 = "gpp" ]; then
../bin/occmd gpp.ap.config.temp_sensor1.lowlimit.set -19
../bin/occmd gpp.ap.config.temp_sensor1.highlimit.set 81
../bin/occmd gpp.ap.config.temp_sensor1.critlimit.set 86
../bin/occmd gpp.ap.config.temp_sensor2.lowlimit.set -19
../bin/occmd gpp.ap.config.temp_sensor2.highlimit.set 81
../bin/occmd gpp.ap.config.temp_sensor2.critlimit.set 86
../bin/occmd gpp.ap.config.current_sensor1.currlimit.set 2010
../bin/occmd gpp.ap.config.temp_sensor3.lowlimit.set -19
../bin/occmd gpp.ap.config.temp_sensor3.highlimit.set 81
../bin/occmd gpp.ap.config.temp_sensor3.critlimit.set 86
../bin/occmd gpp.msata.config.current_sensor1.currlimit.set 2010
elif [ $1 = "sdr" ]; then
../bin/occmd sdr.comp_all.config.current_sensor1.currlimit.set 2010
../bin/occmd sdr.fpga.config.temp_sensor1.lowlimit.set -19
../bin/occmd sdr.fpga.config.temp_sensor1.highlimit.set 76
../bin/occmd sdr.fpga.config.temp_sensor1.critlimit.set 81
../bin/occmd sdr.fpga.config.current_sensor1.currlimit.set 510
elif [ $1 = "rf" ]; then
../bin/occmd rffe.ch1_sensor.config.temp_sensor1.lowlimit.set -19
../bin/occmd rffe.ch1_sensor.config.temp_sensor1.highlimit.set 76
../bin/occmd rffe.ch1_sensor.config.temp_sensor1.critlimit.set 81
../bin/occmd rffe.ch1_sensor.config.current_sensor1.currlimit.set 1500
../bin/occmd rffe.ch2_sensor.config.temp_sensor1.lowlimit.set -19
../bin/occmd rffe.ch2_sensor.config.temp_sensor1.highlimit.set 76
../bin/occmd rffe.ch2_sensor.config.temp_sensor1.critlimit.set 81
../bin/occmd rffe.ch2_sensor.config.current_sensor1.currlimit.set 510
../bin/occmd rffe.ch1_fe.config.ch1_band.band.set 3
../bin/occmd rffe.ch1_fe.config.tx.atten.set 60
../bin/occmd rffe.ch1_fe.config.rx.atten.set 20
../bin/occmd rffe.ch2_fe.config.tx.atten.set 60
../bin/occmd rffe.ch2_fe.config.rx.atten.set 20
../bin/occmd rffe.ch2_fe.config.ch2_band.band.set 3

elif [ $1 = "power" ]; then
../bin/occmd power.leadacid_sensor.config.temp_sensor1.lowlimit.set -19
../bin/occmd power.leadacid_sensor.config.temp_sensor1.highlimit.set 76
../bin/occmd power.leadacid_sensor.config.temp_sensor1.critlimit.set 81
../bin/occmd power.leadacid.config.battery.batteryVoltageLow.set 12500
../bin/occmd power.leadacid.config.battery.batteryVoltageHigh.set 16500
../bin/occmd power.leadacid.config.battery.batteryCurrentLow.set 900
../bin/occmd power.leadacid.config.battery.inputVoltageLow.set 16210
../bin/occmd power.leadacid.config.battery.inputCurrentHigh.set 17400
../bin/occmd power.leadacid.config.battery.inputCurrentLimit.set 15000
#../bin/occmd power.leadacid.config.battery.maxcurr_chargesetting.set 10000
../bin/occmd power.leadacid.config.battery.vcharge.set 10660
../bin/occmd power.leadacid.config.battery.icharge.set 12000
../bin/occmd power.lion.config.battery.batteryVoltageLow.set 9010
../bin/occmd power.lion.config.battery.batteryVoltageHigh.set 12700
../bin/occmd power.lion.config.battery.batteryCurrentLow.set 110
../bin/occmd power.lion.config.battery.inputVoltageLow.set 16210
../bin/occmd power.lion.config.battery.inputCurrentHigh.set 5010
../bin/occmd power.lion.config.battery.inputCurrentLimit.set 3580
../bin/occmd power.lion.config.battery.vcharge.set 10660
../bin/occmd power.lion.config.battery.icharge.set 12000
../bin/occmd power.pse.config.operatingMode.set 3
../bin/occmd power.pse.config.detectEnable.set 64
../bin/occmd power.pse.config.interruptMask.set 195
../bin/occmd power.pse.config.interruptEnable.set 128
../bin/occmd power.pse.config.enableHighpower.set 17
elif [ $1 = "ethernet" ]; then
../bin/occmd ethernet.port0.config.speed.set 0
../bin/occmd ethernet.port0.config.duplex.set 2
../bin/occmd ethernet.port0.config.powerDown.set 0
../bin/occmd ethernet.port0.config.enable_sleepMode.set 1
../bin/occmd ethernet.port0.config.enable_interrupt.set 10
../bin/occmd ethernet.port1.config.speed.set 1
../bin/occmd ethernet.port1.config.duplex.set 0
../bin/occmd ethernet.port1.config.powerDown.set 1
../bin/occmd ethernet.port1.config.enable_sleepMode.set 0
../bin/occmd ethernet.port1.config.enable_interrupt.set 1
../bin/occmd ethernet.port2.config.speed.set 1
../bin/occmd ethernet.port2.config.duplex.set 1
../bin/occmd ethernet.port2.config.powerDown.set 1
../bin/occmd ethernet.port2.config.enable_sleepMode.set 1
../bin/occmd ethernet.port2.config.enable_interrupt.set 0
../bin/occmd ethernet.port3.config.speed.set 1
../bin/occmd ethernet.port3.config.duplex.set 0
../bin/occmd ethernet.port3.config.powerDown.set 0
../bin/occmd ethernet.port3.config.enable_sleepMode.set 1
../bin/occmd ethernet.port3.config.enable_interrupt.set 1
../bin/occmd ethernet.port4.config.speed.set 1
../bin/occmd ethernet.port4.config.duplex.set 1
../bin/occmd ethernet.port4.config.powerDown.set 1
../bin/occmd ethernet.port4.config.enable_sleepMode.set 1
../bin/occmd ethernet.port4.config.enable_interrupt.set 1
elif [ $1 = "sync" ]; then
../bin/occmd sync.sensor.config.temp_sensor1.lowlimit.set -19
../bin/occmd sync.sensor.config.temp_sensor1.highlimit.set 81
../bin/occmd sync.sensor.config.temp_sensor1.critlimit.set 86
elif [ $1 = "hci" ]; then
../bin/occmd hci.led.config.temp_sensor1.lowlimit.set -19
../bin/occmd hci.led.config.temp_sensor1.highlimit.set 76
../bin/occmd hci.led.config.temp_sensor1.critlimit.set 85
elif [ $1 = "debug" ]; then
if [ $2 = "I2C" ]; then
if [ $3 = "stub" ]; then
../bin/occmd debug.I2C.bus0.set 1 1 1 1
../bin/occmd debug.I2C.bus1.set 1 1 1 1024
../bin/occmd debug.I2C.bus2.set 1 1 1 2
../bin/occmd debug.I2C.bus3.set 1 1 1 1025
../bin/occmd debug.I2C.bus4.set 1 1 1 3
../bin/occmd debug.I2C.bus6.set 1 1 1 4
../bin/occmd debug.I2C.bus7.set 1 1 1 1027
../bin/occmd debug.I2C.bus8.set 1 1 1 5
elif [ $3 = "FW" ]; then
../bin/occmd debug.I2C.bus0.set 104 2 58 7000
../bin/occmd debug.I2C.bus1.set 24 2 2 1024
../bin/occmd debug.I2C.bus2.set 29 1 0 100
../bin/occmd debug.I2C.bus3.set 68 2 255 8000
../bin/occmd debug.I2C.bus4.set 65 2 254 21500
../bin/occmd debug.I2C.bus6.set 64 2 5 25500
../bin/occmd debug.I2C.bus7.set 69 2 5 25500
../bin/occmd debug.I2C.bus8.set 26 2 6 4000
else
echo "Invalid Option"
fi
elif [ $2 = "GPIO" ]; then
if [ $3 = "stub" ]; then
../bin/occmd debug.ec.PA.set 1 1
../bin/occmd debug.ec.PB.set 1 1
../bin/occmd debug.ec.PC.set 1 1
../bin/occmd debug.ec.PD.set 1 0
../bin/occmd debug.ec.PE.set 1 0
../bin/occmd debug.ec.PF.set 1 1
../bin/occmd debug.ec.PG.set 1 1
../bin/occmd debug.ec.PJ.set 1 1
../bin/occmd debug.ec.PK.set 1 1
../bin/occmd debug.ec.PL.set 1 1
../bin/occmd debug.ec.PM.set 1 0
../bin/occmd debug.ec.PN.set 1 0
../bin/occmd debug.ec.PP.set 1 0
../bin/occmd debug.ec.PQ.set 1 0

../bin/occmd debug.gbc.ioexpanderx70.set 1 0
../bin/occmd debug.gbc.ioexpanderx71.set 1 0
../bin/occmd debug.sdr.ioexpanderx1E.set 1 0
../bin/occmd debug.fe.ioexpanderx18.set 1 0
../bin/occmd debug.fe.ioexpanderx1A.set 1 1
../bin/occmd debug.fe.ioexpanderx1B.set 1 0
../bin/occmd debug.fe.ioexpanderx1C.set 1 0
../bin/occmd debug.fe.ioexpanderx1D.set 1 1
../bin/occmd debug.sync.ioexpanderx71.set 1 1
elif [ $3 = "FW" ]; then
../bin/occmd debug.ec.PA.set 1 1
../bin/occmd debug.ec.PB.set 1 1
../bin/occmd debug.ec.PC.set 1 1
../bin/occmd debug.ec.PD.set 7 0
../bin/occmd debug.ec.PE.set 1 0
../bin/occmd debug.ec.PF.set 1 1
../bin/occmd debug.ec.PG.set 1 1
../bin/occmd debug.ec.PH.set 1 1
../bin/occmd debug.ec.PJ.set 1 1
../bin/occmd debug.ec.PK.set 1 1
../bin/occmd debug.ec.PL.set 5 1
../bin/occmd debug.ec.PM.set 1 0
../bin/occmd debug.ec.PN.set 1 0
../bin/occmd debug.ec.PP.set 3 0
../bin/occmd debug.ec.PQ.set 1 0

../bin/occmd debug.gbc.ioexpanderx70.set 1 0
../bin/occmd debug.gbc.ioexpanderx71.set 0 0
../bin/occmd debug.sdr.ioexpanderx1E.set 1 0
../bin/occmd debug.fe.ioexpanderx18.set 1 0
../bin/occmd debug.fe.ioexpanderx1A.set 0 1
../bin/occmd debug.fe.ioexpanderx1B.set 1 0
../bin/occmd debug.fe.ioexpanderx1C.set 1 0
../bin/occmd debug.fe.ioexpanderx1D.set 1 1
../bin/occmd debug.sync.ioexpanderx71.set 1 1
else
echo "Invalid Option"
fi
elif [ $2 = "MDIO" ]; then
../bin/occmd debug.ethernet.port0.set 18 1024
../bin/occmd debug.ethernet.global1.set 4 128
../bin/occmd debug.ethernet.global2.set 1 31
../bin/occmd debug.ethernet.swport0.set 1 1
else
echo "Invalid Option"
fi

else
        echo "Invalid Option"
fi
echo "Done..."
