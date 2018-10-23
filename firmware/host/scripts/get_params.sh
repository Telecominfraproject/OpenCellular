#!/bin/bash

bmsParam=("bms.ec.config.temp_sensor1.lowlimit.get"
            "bms.ec.config.temp_sensor1.highlimit.get"
            "bms.ec.config.temp_sensor1.critlimit.get"
            "bms.ec.config.current_sensor1.currlimit.get"
            "bms.ec.config.current_sensor2.currlimit.get")

defaultBms=("bms.ec.config.temp_sensor1.lowlimit.get=-20"
               "bms.ec.config.temp_sensor1.highlimit.get=75"
               "bms.ec.config.temp_sensor1.critlimit.get=80"
               "bms.ec.config.current_sensor1.currlimit.get=1000"
               "bms.ec.config.current_sensor2.currlimit.get=1000")

setBmsVal=("bms.ec.config.temp_sensor1.lowlimit.get=-19"
               "bms.ec.config.temp_sensor1.highlimit.get=76"
               "bms.ec.config.temp_sensor1.critlimit.get=85"
               "bms.ec.config.current_sensor1.currlimit.get=1498"
               "bms.ec.config.current_sensor2.currlimit.get=1498")

bmsStatusParam=("bms.ec.status.temp_sensor1.temperature.get"
                "bms.ec.status.current_sensor1.busvoltage.get"
                "bms.ec.status.current_sensor1.shuntvoltage.get"
                "bms.ec.status.current_sensor1.current.get"
                "bms.ec.status.current_sensor1.power.get"
                "bms.ec.status.current_sensor2.busvoltage.get"
                "bms.ec.status.current_sensor2.shuntvoltage.get"
                "bms.ec.status.current_sensor2.current.get"
                "bms.ec.status.current_sensor2.power.get")

bmsStatusMinVal=(-20 11400 250 100 1100 2970 800 400 1200)

bmsStatusMaxVal=(75 12600 1400 600 7500 3630 2000 800 2900)

gppParam=("gpp.ap.config.temp_sensor1.lowlimit.get"
            "gpp.ap.config.temp_sensor1.highlimit.get"
            "gpp.ap.config.temp_sensor1.critlimit.get"
            "gpp.ap.config.temp_sensor2.lowlimit.get"
            "gpp.ap.config.temp_sensor2.highlimit.get"
            "gpp.ap.config.temp_sensor2.critlimit.get"
            "gpp.ap.config.temp_sensor3.lowlimit.get"
            "gpp.ap.config.temp_sensor3.highlimit.get"
            "gpp.ap.config.temp_sensor3.critlimit.get"
            "gpp.ap.config.current_sensor1.currlimit.get"
            "gpp.msata.config.current_sensor1.currlimit.get")

defaultGpp=("gpp.ap.config.temp_sensor1.lowlimit.get=-20"
            "gpp.ap.config.temp_sensor1.highlimit.get=75"
            "gpp.ap.config.temp_sensor1.critlimit.get=80"
            "gpp.ap.config.temp_sensor2.lowlimit.get=-20"
            "gpp.ap.config.temp_sensor2.highlimit.get=75"
            "gpp.ap.config.temp_sensor2.critlimit.get=80"
            "gpp.ap.config.temp_sensor3.lowlimit.get=-20"
            "gpp.ap.config.temp_sensor3.highlimit.get=75"
            "gpp.ap.config.temp_sensor3.critlimit.get=80"
            "gpp.ap.config.current_sensor1.currlimit.get=1498"
            "gpp.msata.config.current_sensor1.currlimit.get=1498")

setGppVal=("gpp.ap.config.temp_sensor1.lowlimit.get=-19"
            "gpp.ap.config.temp_sensor1.highlimit.get=81"
            "gpp.ap.config.temp_sensor1.critlimit.get=86"
            "gpp.ap.config.temp_sensor2.lowlimit.get=-19"
            "gpp.ap.config.temp_sensor2.highlimit.get=81"
            "gpp.ap.config.temp_sensor2.critlimit.get=86"
            "gpp.ap.config.temp_sensor3.lowlimit.get=-19"
            "gpp.ap.config.temp_sensor3.highlimit.get=81"
            "gpp.ap.config.temp_sensor3.critlimit.get=86"
            "gpp.ap.config.current_sensor1.currlimit.get=2010"
            "gpp.msata.config.current_sensor1.currlimit.get=2010")


gppStatusParam=("gpp.ap.status.temp_sensor1.temperature.get"
                "gpp.ap.status.temp_sensor2.temperature.get"
                "gpp.ap.status.temp_sensor3.temperature.get"
                "gpp.ap.status.current_sensor1.busvoltage.get"
                "gpp.ap.status.current_sensor1.shuntvoltage.get"
                "gpp.ap.status.current_sensor1.current.get"
                "gpp.ap.status.current_sensor1.power.get"
                "gpp.msata.status.current_sensor1.busvoltage.get"
                "gpp.msata.status.current_sensor1.shuntvoltage.get"
                "gpp.msata.status.current_sensor1.current.get"
                "gpp.msata.status.current_sensor1.power.get")

gppStatusMinVal=(-20 -20 -20 11400 700 400 5000 3135 140 50 250)

gppStatusMaxVal=(80 80 80 12600 1400 1200 8300 3465 500 300 800)

hciParam=("hci.led.config.temp_sensor1.lowlimit.get"
            "hci.led.config.temp_sensor1.highlimit.get"
            "hci.led.config.temp_sensor1.critlimit.get")

defaultHci=("hci.led.config.temp_sensor1.lowlimit.get=-20"
                "hci.led.config.temp_sensor1.highlimit.get=80"
                "hci.led.config.temp_sensor1.critlimit.get=85")

setHciVal=("hci.led.config.temp_sensor1.lowlimit.get=-19"
                "hci.led.config.temp_sensor1.highlimit.get=76"
                "hci.led.config.temp_sensor1.critlimit.get=85")

hciStatusParam=("hci.led.status.temp_sensor1.temperature.get")

hciStatusMinVal=(-20)

hciStatusMaxVal=(80)

sdrParam=("sdr.comp_all.config.current_sensor1.currlimit.get"
            "sdr.fpga.config.temp_sensor1.lowlimit.get"
            "sdr.fpga.config.temp_sensor1.highlimit.get"
            "sdr.fpga.config.temp_sensor1.critlimit.get"
            "sdr.fpga.config.current_sensor1.currlimit.get")

defaultSdr=("sdr.comp_all.config.current_sensor1.currlimit.get=2998"
            "sdr.fpga.config.temp_sensor1.lowlimit.get=-20"
            "sdr.fpga.config.temp_sensor1.highlimit.get=75"
            "sdr.fpga.config.temp_sensor1.critlimit.get=85"
            "sdr.fpga.config.current_sensor1.currlimit.get=500")

setSdrVal=("sdr.comp_all.config.current_sensor1.currlimit.get=2010"
            "sdr.fpga.config.temp_sensor1.lowlimit.get=-19"
            "sdr.fpga.config.temp_sensor1.highlimit.get=76"
            "sdr.fpga.config.temp_sensor1.critlimit.get=81"
            "sdr.fpga.config.current_sensor1.currlimit.get=510")

sdrStatusParam=("sdr.comp_all.status.current_sensor1.busvoltage.get"
                "sdr.comp_all.status.current_sensor1.shuntvoltage.get"
                "sdr.comp_all.status.current_sensor1.current.get"
                "sdr.comp_all.status.current_sensor1.power.get"
                "sdr.fpga.status.temp_sensor1.temperature.get"
                "sdr.fpga.status.current_sensor1.busvoltage.get"
                "sdr.fpga.status.current_sensor1.shuntvoltage.get"
                "sdr.fpga.status.current_sensor1.current.get"
                "sdr.fpga.status.current_sensor1.power.get")

sdrStatusMinVal=(11400 2762 1381 15742 0 11400 60 30 342)

sdrStatusMaxVal=(12600 4173 2087 26292 80 12600 240 120 1512)

rfParam=("rffe.ch1_sensor.config.temp_sensor1.lowlimit.get"
            "rffe.ch1_sensor.config.temp_sensor1.highlimit.get"
            "rffe.ch1_sensor.config.temp_sensor1.critlimit.get"
            "rffe.ch1_sensor.config.current_sensor1.currlimit.get"
            "rffe.ch2_sensor.config.temp_sensor1.lowlimit.get"
            "rffe.ch2_sensor.config.temp_sensor1.highlimit.get"
            "rffe.ch2_sensor.config.temp_sensor1.critlimit.get"
            "rffe.ch2_sensor.config.current_sensor1.currlimit.get"
            "rffe.ch1_fe.config.ch1_band.band.get"
            "rffe.ch1_fe.config.tx.atten.get"
            "rffe.ch1_fe.config.rx.atten.get"
            "rffe.ch2_fe.config.ch2_band.band.get"
            "rffe.ch2_fe.config.tx.atten.get"
            "rffe.ch2_fe.config.rx.atten.get")

defaultRf=("rffe.ch1_sensor.config.temp_sensor1.lowlimit.get=-20"
            "rffe.ch1_sensor.config.temp_sensor1.highlimit.get=80"
            "rffe.ch1_sensor.config.temp_sensor1.critlimit.get=85"
            "rffe.ch1_sensor.config.current_sensor1.currlimit.get=2000"
            "rffe.ch2_sensor.config.temp_sensor1.lowlimit.get=-20"
            "rffe.ch2_sensor.config.temp_sensor1.highlimit.get=80"
            "rffe.ch2_sensor.config.temp_sensor1.critlimit.get=85"
            "rffe.ch2_sensor.config.current_sensor1.currlimit.get=2000"
            "rffe.ch1_fe.config.ch1_band.band.get=3"
            "rffe.ch1_fe.config.tx.atten.get=63"
            "rffe.ch1_fe.config.rx.atten.get=31"
            "rffe.ch2_fe.config.ch2_band.band.get=3"
            "rffe.ch2_fe.config.tx.atten.get=63"
            "rffe.ch2_fe.config.rx.atten.get=31")

setRfVal=("rffe.ch1_sensor.config.temp_sensor1.lowlimit.get=-19"
            "rffe.ch1_sensor.config.temp_sensor1.highlimit.get=76"
            "rffe.ch1_sensor.config.temp_sensor1.critlimit.get=81"
            "rffe.ch1_sensor.config.current_sensor1.currlimiti.get=1467"
            "rffe.ch2_sensor.config.temp_sensor1.lowlimit.get=-19"
            "rffe.ch2_sensor.config.temp_sensor1.highlimit.get=76"
            "rffe.ch2_sensor.config.temp_sensor1.critlimit.get=81"
            "rffe.ch2_sensor.config.current_sensor1.currlimit.get=510"
            "rffe.ch1_fe.config.ch1_band.band.get=3"
            "rffe.ch1_fe.config.tx.atten.get=60"
            "rffe.ch1_fe.config.rx.atten.get=20"
            "rffe.ch2_fe.config.ch2_band.band.get=3"
            "rffe.ch2_fe.config.tx.atten.get=60"
            "rffe.ch2_fe.config.rx.atten.get=20")

rfStatusParam=("rffe.ch1_sensor.status.temp_sensor1.temperature.get"
               "rffe.ch1_sensor.status.current_sensor1.busvoltage.get"
               "rffe.ch1_sensor.status.current_sensor1.shuntvoltage.get"
               "rffe.ch1_sensor.status.current_sensor1.current.get"
               "rffe.ch1_sensor.status.current_sensor1.power.get"
               "rffe.ch2_sensor.status.temp_sensor1.temperature.get"
               "rffe.ch2_sensor.status.current_sensor1.busvoltage.get"
               "rffe.ch2_sensor.status.current_sensor1.shuntvoltage.get"
               "rffe.ch2_sensor.status.current_sensor1.current.get"
               "rffe.ch2_sensor.status.current_sensor1.power.get"
               "rffe.ch1_fe.status.power.forward.get"
               "rffe.ch1_fe.status.power.reverse.get"
               "rffe.ch2_fe.status.power.forward.get"
               "rffe.ch2_fe.status.power.reverse.get")

rfStatusMinVal=(10 5500 2800 1400 7700 10 5500 2800 1400 7700 0 0 0 0)

rfStatusMaxVal=(80 5900 4200 2100 12390 80 5900 4200 2100 12390 10 10 10 10)

powerParam=("power.leadacid_sensor.config.temp_sensor1.lowlimit.get"
            "power.leadacid_sensor.config.temp_sensor1.highlimit.get"
            "power.leadacid_sensor.config.temp_sensor1.critlimit.get"
            "power.leadacid.config.battery.batteryVoltageLow.get"
            "power.leadacid.config.battery.batteryVoltageHigh.get"
            "power.leadacid.config.battery.batteryCurrentLow.get"
            "power.leadacid.config.battery.inputVoltageLow.get"
            "power.leadacid.config.battery.inputCurrentHigh.get"
            "power.leadacid.config.battery.inputCurrentLimit.get"
            "power.lion.config.battery.batteryVoltageLow.get"
            "power.lion.config.battery.batteryVoltageHigh.get"
            "power.lion.config.battery.batteryCurrentLow.get"
            "power.lion.config.battery.inputVoltageLow.get"
            "power.lion.config.battery.inputCurrentHigh.get"
            "power.lion.config.battery.inputCurrentLimit.get"
            "power.pse.config.operatingMode.get"
            "power.pse.config.detectEnable.get"
            "power.pse.config.interruptMask.get"
            "power.pse.config.interruptEnable.get"
            "power.pse.config.enableHighpower.get")

defaultPower=("power.leadacid_sensor.config.temp_sensor1.lowlimit.get=-20"
                "power.leadacid_sensor.config.temp_sensor1.highlimit.get=75"
                "power.leadacid_sensor.config.temp_sensor1.critlimit.get=80"
                "power.leadacid.config.battery.batteryVoltageLow.get=12666"
                "power.leadacid.config.battery.batteryVoltageHigh.get=16799"
                "power.leadacid.config.battery.batteryCurrentLow.get=999"
                "power.leadacid.config.battery.inputVoltageLow.get=16199"
                "power.leadacid.config.battery.inputCurrentHigh.get=17499"
                "power.leadacid.config.battery.inputCurrentLimit.get=16000"
                "power.lion.config.battery.batteryVoltageLow.get=9499"
                "power.lion.config.battery.batteryVoltageHigh.get=12599"
                "power.lion.config.battery.batteryCurrentLow.get=99"
                "power.lion.config.battery.inputVoltageLow.get=16199"
                "power.lion.config.battery.inputCurrentHigh.get=4999"
                "power.lion.config.battery.inputCurrentLimit.get=4571"
                "power.pse.config.operatingMode.get=17"
                "power.pse.config.detectEnable.get=17"
                "power.pse.config.interruptMask.get=17"
                "power.pse.config.interruptEnable.get=17"
                "power.pse.config.enableHighpower.get=17")

setPowerVal=("power.leadacid_sensor.config.temp_sensor1.lowlimit.get=-19"
                "power.leadacid_sensor.config.temp_sensor1.highlimit.get=76"
                "power.leadacid_sensor.config.temp_sensor1.critlimit.get=81"
                "power.leadacid.config.battery.batteryVoltageLow.get=12499"
                "power.leadacid.config.battery.batteryVoltageHigh.get=16499"
                "power.leadacid.config.battery.batteryCurrentLow.get=899"
                "power.leadacid.config.battery.inputVoltageLow.get=16209"
                "power.leadacid.config.battery.inputCurrentHigh.get=17399"
                "power.leadacid.config.battery.inputCurrentLimit.get=15000"
                "power.lion.config.battery.batteryVoltageLow.get=9009"
                "power.lion.config.battery.batteryVoltageHigh.get=12699"
                "power.lion.config.battery.batteryCurrentLow.get=109"
                "power.lion.config.battery.inputVoltageLow.get=16209"
                "power.lion.config.battery.inputCurrentHigh.get=5009"
                "power.lion.config.battery.inputCurrentLimit.get=3571"
                "power.pse.config.operatingMode.get=3"
                "power.pse.config.detectEnable.get=64"
                "power.pse.config.interruptMask.get=195"
                "power.pse.config.interruptEnable.get=128"
                "power.pse.config.enableHighpower.get=17")

ethParam=("ethernet.port0.config.speed.get"
            "ethernet.port0.config.duplex.get"
            "ethernet.port0.config.powerDown.get"
            "ethernet.port0.config.enable_sleepMode.get"
            "ethernet.port0.config.enable_interrupt.get"
            "ethernet.port1.config.speed.get"
            "ethernet.port1.config.duplex.get"
            "ethernet.port1.config.powerDown.get"
            "ethernet.port1.config.enable_sleepMode.get"
            "ethernet.port1.config.enable_interrupt.get"
            "ethernet.port2.config.speed.get"
            "ethernet.port2.config.duplex.get"
            "ethernet.port2.config.powerDown.get"
            "ethernet.port2.config.enable_sleepMode.get"
            "ethernet.port2.config.enable_interrupt.get"
            "ethernet.port3.config.speed.get"
            "ethernet.port3.config.duplex.get"
            "ethernet.port3.config.powerDown.get"
            "ethernet.port3.config.enable_sleepMode.get"
            "ethernet.port3.config.enable_interrupt.get"
            "ethernet.port4.config.speed.get"
            "ethernet.port4.config.duplex.get"
            "ethernet.port4.config.powerDown.get"
            "ethernet.port4.config.enable_sleepMode.get"
            "ethernet.port4.config.enable_interrupt.get")

defaultEth=("ethernet.port0.config.speed.get=2"
            "ethernet.port0.config.duplex.get=2"
            "ethernet.port0.config.powerDown.get=0"
            "ethernet.port0.config.enable_sleepMode.get=0"
            "ethernet.port0.config.enable_interrupt.get=0"
            "ethernet.port1.config.speed.get=2"
            "ethernet.port1.config.duplex.get=2"
            "ethernet.port1.config.powerDown.get=0"
            "ethernet.port1.config.enable_sleepMode.get=0"
            "ethernet.port1.config.enable_interrupt.get=0"
            "ethernet.port2.config.speed.get=2"
            "ethernet.port2.config.duplex.get=2"
            "ethernet.port2.config.powerDown.get=0"
            "ethernet.port2.config.enable_sleepMode.get=0"
            "ethernet.port2.config.enable_interrupt.get=0"
            "ethernet.port3.config.speed.get=2"
            "ethernet.port3.config.duplex.get=2"
            "ethernet.port3.config.powerDown.get=0"
            "ethernet.port3.config.enable_sleepMode.get=0"
            "ethernet.port3.config.enable_interrupt.get=0"
            "ethernet.port4.config.speed.get=2"
            "ethernet.port4.config.duplex.get=2"
            "ethernet.port4.config.powerDown.get=0"
            "ethernet.port4.config.enable_sleepMode.get=0"
            "ethernet.port4.config.enable_interrupt.get=0")

setEthVal=("ethernet.port0.config.speed.get=2"
            "ethernet.port0.config.duplex.get=2"
            "ethernet.port0.config.powerDown.get=0"
            "ethernet.port0.config.enable_sleepMode.get=1"
            "ethernet.port0.config.enable_interrupt.get=0"
            "ethernet.port1.config.speed.get=1"
            "ethernet.port1.config.duplex.get=0"
            "ethernet.port1.config.powerDown.get=1"
            "ethernet.port1.config.enable_sleepMode.get=0"
            "ethernet.port1.config.enable_interrupt.get=0"
            "ethernet.port2.config.speed.get=1"
            "ethernet.port2.config.duplex.get=1"
            "ethernet.port2.config.powerDown.get=1"
            "ethernet.port2.config.enable_sleepMode.get=1"
            "ethernet.port2.config.enable_interrupt.get=0"
            "ethernet.port3.config.speed.get=1"
            "ethernet.port3.config.duplex.get=0"
            "ethernet.port3.config.powerDown.get=0"
            "ethernet.port3.config.enable_sleepMode.get=1"
            "ethernet.port3.config.enable_interrupt.get=0"
            "ethernet.port4.config.speed.get=1"
            "ethernet.port4.config.duplex.get=1"
            "ethernet.port4.config.powerDown.get=1"
            "ethernet.port4.config.enable_sleepMode.get=1"
            "ethernet.port4.config.enable_interrupt.get=0")

ethernetStatusParam=("ethernet.port0.status.speed.get"
                     "ethernet.port0.status.duplex.get"
                     "ethernet.port1.status.speed.get"
                     "ethernet.port1.status.duplex.get"
                     "ethernet.port2.status.speed.get"
                     "ethernet.port2.status.duplex.get"
                     "ethernet.port3.status.speed.get"
                     "ethernet.port3.status.duplex.get"
                     "ethernet.port4.status.speed.get"
                     "ethernet.port4.status.duplex.get")

ethernetStatusMinVal=(1 1 1 1 1 1 1 1 1 1)

ethernetStatusMaxVal=(1 1 1 1 1 1 1 1 1 1)

syncParam=("sync.sensor.config.temp_sensor1.lowlimit.get"
            "sync.sensor.config.temp_sensor1.highlimit.get"
            "sync.sensor.config.temp_sensor1.critlimit.get")

defaultSync=("sync.sensor.config.temp_sensor1.lowlimit.get=-20"
                "sync.sensor.config.temp_sensor1.highlimit.get=80"
                "sync.sensor.config.temp_sensor1.critlimit.get=85")

setSyncVal=("sync.sensor.config.temp_sensor1.lowlimit.get=-19"
                "sync.sensor.config.temp_sensor1.highlimit.get=81"
                "sync.sensor.config.temp_sensor1.critlimit.get=86")

syncStatusParam=("sync.sensor.status.temp_sensor1.temperature.get")

syncStatusMinVal=(-20)

syncStatusMaxVal=(80)

debugI2CParam=("debug.I2C.bus0.get 104 2 58"
            "debug.I2C.bus1.get 24 2 2"
            "debug.I2C.bus2.get 29 1 0"
            "debug.I2C.bus3.get 68 2 255"
            "debug.I2C.bus4.get 65 2 254"
            "debug.I2C.bus6.get 64 2 5"
            "debug.I2C.bus7.get 69 2 5"
            "debug.I2C.bus8.get 26 2 6")


defaultDebugI2C=("debug.I2C.bus0(slaveaddress:104noOfBytes:2RegisterAddress:58)get=9984"
            "debug.I2C.bus1(slaveaddress:24noOfBytes:2RegisterAddress:2)get=1216"
            "debug.I2C.bus2(slaveaddress:29noOfBytes:1RegisterAddress:0)get=115"
            "debug.I2C.bus3(slaveaddress:68noOfBytes:2RegisterAddress:255)get=8800"
            "debug.I2C.bus4(slaveaddress:65noOfBytes:2RegisterAddress:254)get=21577"
            "debug.I2C.bus6(slaveaddress:64noOfBytes:2RegisterAddress:5)get=25600"
            "debug.I2C.bus7(slaveaddress:69noOfBytes:2RegisterAddress:5)get=25600"
            "debug.I2C.bus8(slaveaddress:26noOfBytes:2RegisterAddress:6)get=0")

setDebugI2CVal=("debug.I2C.bus0(slaveaddress:104noOfBytes:2RegisterAddress:58)get=7000"
            "debug.I2C.bus1(slaveaddress:24noOfBytes:2RegisterAddress:2)get=1024"
            "debug.I2C.bus2(slaveaddress:29noOfBytes:1RegisterAddress:0)get=100"
            "debug.I2C.bus3(slaveaddress:68noOfBytes:2RegisterAddress:255)get=8000"
            "debug.I2C.bus4(slaveaddress:65noOfBytes:2RegisterAddress:254)get=21500"
            "debug.I2C.bus6(slaveaddress:64noOfBytes:2RegisterAddress:5)get=25500"
            "debug.I2C.bus7(slaveaddress:69noOfBytes:2RegisterAddress:5)get=25500"
            "debug.I2C.bus8(slaveaddress:26noOfBytes:2RegisterAddress:6)get=4000")

debugGpioParam=("debug.ec.PA.get 1"
               "debug.ec.PB.get 1"
               "debug.ec.PC.get 1"
               "debug.ec.PD.get 7"
               "debug.ec.PE.get 1"
               "debug.ec.PF.get 1"
               "debug.ec.PG.get 1"
               "debug.ec.PH.get 1"
               "debug.ec.PJ.get 1"
               "debug.ec.PK.get 1"
               "debug.ec.PL.get 5"
               "debug.ec.PM.get 1"
               "debug.ec.PN.get 1"
               "debug.ec.PP.get 3"
               "debug.ec.PQ.get 1"
               "debug.gbc.ioexpanderx70.get 1"
               "debug.gbc.ioexpanderx71.get 0"
               "debug.sdr.ioexpanderx1E.get 1"
               "debug.fe.ioexpanderx18.get 1"
               "debug.fe.ioexpanderx1A.get 0"
               "debug.fe.ioexpanderx1B.get 1"
               "debug.fe.ioexpanderx1C.get 1"
               "debug.fe.ioexpanderx1D.get 1"
               "debug.sync.ioexpanderx71.get 1")

defaultDebugGpio=("debug.ec.PA(PinNo:1)get=0"
                "debug.ec.PB(PinNo:1)get=0"
                "debug.ec.PC(PinNo:1)get=0"
                "debug.ec.PD(PinNo:7)get=1"
                "debug.ec.PE(PinNo:1)get=1"
                "debug.ec.PF(PinNo:1)get=0"
                "debug.ec.PG(PinNo:1)get=0"
                "debug.ec.PH(PinNo:1)get=0"
                "debug.ec.PJ(PinNo:1)get=0"
                "debug.ec.PK(PinNo:1)get=0"
                "debug.ec.PL(PinNo:5)get=0"
                "debug.ec.PM(PinNo:1)get=0"
                "debug.ec.PN(PinNo:1)get=0"
                "debug.ec.PP(PinNo:3)get=0"
                "debug.ec.PQ(PinNo:1)get=1"
                "debug.gbc.ioexpanderx70(PinNo:1)get=1"
                "debug.gbc.ioexpanderx71(PinNo:0)get=1"
                "debug.sdr.ioexpanderx1E(PinNo:1)get=1"
                "debug.fe.ioexpanderx18(PinNo:1)get=1"
                "debug.fe.ioexpanderx1A(PinNo:0)get=1"
                "debug.fe.ioexpanderx1B(PinNo:1)get=1"
                "debug.fe.ioexpanderx1C(PinNo:1)get=1"
                "debug.fe.ioexpanderx1D(PinNo:1)get=0"
                "debug.sync.ioexpanderx71(PinNo:1)get=0")

setDebugGpioVal=("debug.ec.PA(PinNo:1)get=1"
                "debug.ec.PB(PinNo:1)get=1"
                "debug.ec.PC(Pin No:1)get=1"
                "debug.ec.PD(Pin No:7)get=0"
                "debug.ec.PE(Pin No:1)get=0"
                "debug.ec.PF(PinNo:1)get=1"
                "debug.ec.PG(Pin No:1)get=1"
                "debug.ec.PH(PinNo:1)get=1"
                "debug.ec.PJ(PinNo:1)get=1"
                "debug.ec.PK(PinNo:1)get=1"
                "debug.ec.PL(PinNo:5)get=1"
                "debug.ec.PM(PinNo:1)get=0"
                "debug.ec.PN(PinNo:1)get=0"
                "debug.ec.PP(PinNo:3)get=0"
                "debug.ec.PQ(PinNo:1)get=0"
                "debug.gbc.ioexpanderx70(PinNo:1)get=0"
                "debug.gbc.ioexpanderx71(PinNo:0)get=0"
                "debug.sdr.ioexpanderx1E(PinNo:1)get=0"
                "debug.fe.ioexpanderx18(PinNo:1)get=0"
                "debug.fe.ioexpanderx1A(PinNo:0)get=0"
                "debug.fe.ioexpanderx1B(PinNo:1)get=0"
                "debug.fe.ioexpanderx1C(PinNo:1)get=0"
                "debug.fe.ioexpanderx1D(PinNo:1)get=1"
                "debug.sync.ioexpanderx71(PinNo:1)get=1")

debugMdioParam=("debug.ethernet.port0.get 18"
                "debug.ethernet.global1.get 4"
                "debug.ethernet.global2.get 1"
                "debug.ethernet.swport0.get 1")

defaultDebugMdio=("debug.ethernet.port0(RegisterAddress:18)get=9216"
                "debug.ethernet.global1(RegisterAddress:4)get=1"
                "debug.ethernet.global2(RegisterAddress:1)get=0"
                "debug.ethernet.swport0(RegisterAddress:1)get=3")

setDebugMdioVal=("debug.ethernet.port0(RegisterAddress:18)get=1024"
                "debug.ethernet.global1(RegisterAddress:4)get=128"
                "debug.ethernet.global2(RegisterAddress:1)get=31"
                "debug.ethernet.swport0(RegisterAddress:1)get=1")

function compareValue() {
    local -n  paramArray=$1
    local -n valueArray=$2
    i=0
    for index in "${paramArray[@]}"
    do
    val=${valueArray[$i]}
    value=$(../bin/occmd $index)
    valueStr=$value
    printf -v value '%s' $value
    if [ "$value" = "$val" ]; then
        echo "${paramArray[$i]}   Passed"
        echo "$valueStr"
    else
        echo "${paramArray[$i]}   FAILED"
        if [[ "$value" =~ "Failed" ]]; then
           echo "Failed from TIVA"
        else
            echo "Value Mismatch"
            echo "$valueStr"
        fi
    fi
    ((i++))
    done
}

function errorPrint() {

   if [[ "$1" =~ "Failed" ]]; then
    echo "Failed from TIVA"
   else
    echo "Value not in range: Max=$2 Min=$3 value=$4"
   fi
}

function compareStatusValue() {
    local -n  paramStatusArray=$1
    local -n minArray=$2
    local -n maxArray=$3
    i=0
    for index in "${paramStatusArray[@]}"
    do
    minVal=${minArray[$i]}
    maxVal=${maxArray[$i]}
    value=$(../bin/occmd $index)
    valueStr=$value
    printf -v value '%s' $value
    IFS='=' read -ra token <<< "$value"
    statusVal=${token[1]}
    if [[ ("$value" =~ "ethernet") ]]; then
        if [[ ($statusVal -eq $maxVal) ]]; then
       echo "$valueStr   Passed"
        else
           echo "$valueStr   Failed"
           errorPrint $value $maxVal $minVal $statusVal
        fi
    elif [[ ($statusVal -le $maxVal) ]]; then
         if [[ ($statusVal -ge $minVal) ]]; then
            echo "$valueStr   Passed"
         else
           echo "$valueStr   Failed"
       errorPrint $value $maxVal $minVal $statusVal
        fi
   else
        echo "$valueStr   Failed"
    errorPrint $value $maxVal $minVal $statusVal
   fi
    ((i++))
    done
}

if [ -z "$1" ]; then
        echo "Invalid Option"
    echo "Usage : ./get_params <subsystem> <class> <verify/default>"
        exit
elif [ $1 = "bms" ]; then
if [ $2 = "config" ]; then
if [ $3 = "default" ]; then
        compareValue bmsParam defaultBms
elif [ $3 = "verify" ]; then
        compareValue bmsParam setBmsVal
else
        echo "Invalid Option"
fi
elif [ $2 = "status" ]; then
       compareStatusValue bmsStatusParam bmsStatusMinVal bmsStatusMaxVal
else
        echo "Invalid Option"
fi
elif [ $1 = "gpp" ]; then
if [ $2 = "config" ]; then
if [ $3 = "default" ]; then
        compareValue gppParam defaultGpp
elif [ $3 = "verify" ]; then
        compareValue gppParam setGppVal
else
        echo "Invalid Option"
fi
elif [ $2 = "status" ]; then
    compareStatusValue gppStatusParam gppStatusMinVal gppStatusMaxVal
else
        echo "Invalid Option"
fi
elif [ $1 = "sdr" ]; then
if [ $2 = "config" ]; then
if [ $3 = "default" ]; then
        compareValue sdrParam defaultSdr
elif [ $3 = "verify" ]; then
        compareValue sdrParam setSdrVal
else
        echo "Invalid Option"
fi
elif [ $2 = "status" ]; then
    compareStatusValue sdrStatusParam sdrStatusMinVal sdrStatusMaxVal
../bin/occmd sdr.comp_all.status.eeprom.dev_id.get
else
        echo "Invalid Option"
fi
elif [ $1 = "rf" ]; then
if [ $2 = "config" ]; then
if [ $3 = "default" ]; then
        compareValue rfParam defaultRf
elif [ $3 = "verify" ]; then
        compareValue rfParam setRfVal
else
        echo "Invalid Option"
fi
elif [ $2 = "status" ]; then
    compareStatusValue rfStatusParam rfStatusMinVal rfStatusMaxVal
../bin/occmd rffe.comp_all.status.eeprom.dev_id.get
else
        echo "Invalid Option"
fi
elif [ $1 = "power" ]; then
if [ $2 = "config" ]; then
if [ $3 = "default" ]; then
        compareValue powerParam defaultPower
elif [ $3 = "verify" ]; then
        compareValue powerParam setPowerVal
else
        echo "Invalid Option"
fi
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
../bin/occmd power.pse.status.detection.get
../bin/occmd power.pse.status.class.get
../bin/occmd power.pse.status.powerGood.get
# Commented as of now as PD Driver doesnt have enum values defined
#../bin/occmd power.pd.status.class.get
#../bin/occmd power.pd.status.powerGoodState.get
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
if [ $3 = "default" ]; then
        compareValue ethParam defaultEth
elif [ $3 = "verify" ]; then
        compareValue ethParam setEthVal
else
        echo "Invalid Option"
fi
elif [ $2 = "status" ]; then
    compareStatusValue ethernetStatusParam ethernetStatusMinVal ethernetStatusMaxVal

../bin/occmd ethernet.port0.status.sleep_mode_en.get
../bin/occmd ethernet.port0.status.autoneg_on.get
../bin/occmd ethernet.port0.status.autoneg_complete.get
../bin/occmd ethernet.port0.status.link_up.get
../bin/occmd ethernet.port1.status.sleep_mode_en.get
../bin/occmd ethernet.port1.status.autoneg_on.get
../bin/occmd ethernet.port1.status.autoneg_complete.get
../bin/occmd ethernet.port1.status.link_up.get
../bin/occmd ethernet.port2.status.sleep_mode_en.get
../bin/occmd ethernet.port2.status.autoneg_on.get
../bin/occmd ethernet.port2.status.autoneg_complete.get
../bin/occmd ethernet.port2.status.link_up.get
../bin/occmd ethernet.port3.status.sleep_mode_en.get
../bin/occmd ethernet.port3.status.autoneg_on.get
../bin/occmd ethernet.port3.status.autoneg_complete.get
../bin/occmd ethernet.port3.status.link_up.get
../bin/occmd ethernet.port4.status.sleep_mode_en.get
../bin/occmd ethernet.port4.status.autoneg_on.get
../bin/occmd ethernet.port4.status.autoneg_complete.get
../bin/occmd ethernet.port4.status.link_up.get
else
        echo "Invalid Option"
fi
elif [ $1 = "sync" ]; then
if [ $2 = "config" ]; then
if [ $3 = "default" ]; then
        compareValue syncParam defaultSync
elif [ $3 = "verify" ]; then
        compareValue syncParam setSyncVal
else
        echo "Invalid Option"
fi
elif [ $2 = "status" ]; then
    compareStatusValue syncStatusParam syncStatusMinVal syncStatusMaxVal
../bin/occmd sync.gps.status.gps_lock.get
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
if [ $3 = "default" ]; then
        compareValue hciParam defaultHci
elif [ $3 = "verify" ]; then
        compareValue hciParam setHciVal
else
        echo "Invalid Option"
fi
elif [ $2 = "status" ]; then
    compareStatusValue hciStatusParam hciStatusMinVal hciStatusMaxVal
else
        echo "Invalid Option"
fi
elif [ $1 = "system" ]; then
if [ $2 = "status" ]; then
../bin/occmd system.comp_all.status.eeprom_sid.gbcboardinfo.get
../bin/occmd system.comp_all.status.eeprom_sid.ocserialinfo.get
../bin/occmd system.comp_all.config.eeprom_mac.address.get
elif [ $2 = "post" ]; then
../bin/occmd system.comp_all.post.results.get
../bin/occmd system.comp_all.post.enable.set
else
        echo "Invalid Option"
fi
elif [ $1 = "debug" ]; then
if [ $2 = "I2C" ]; then
if [ $3 = "default" ]; then
        compareValue debugI2CParam defaultDebugI2C
elif [ $3 = "verify" ]; then
        compareValue debugI2CParam setDebugI2CVal
else
        echo "Invalid Option"
fi
elif [ $2 = "GPIO" ]; then
if [ $3 = "default" ]; then
        compareValue debugGpioParam defaultDebugGpio
elif [ $3 = "verify" ]; then
        compareValue debugGpioParam setDebugGpioVal
else
        echo "Invalid Option"
fi
elif [ $2 = "MDIO" ]; then
if [ $3 = "default" ]; then
        compareValue debugMdioParam defaultDebugMdio
elif [ $3 = "verify" ]; then
        compareValue debugMdioParam setDebugMdioVal
else
        echo "Invalid Option"
fi
else
        echo "Invalid Option"
fi
elif [ $1 = "command" ]; then
../bin/occmd system.comp_all.echo
../bin/occmd ethernet.port0.reset
../bin/occmd ethernet.port1.reset
../bin/occmd ethernet.port2.reset
../bin/occmd ethernet.port3.reset
../bin/occmd ethernet.port4.reset
#../bin/occmd power.pse.reset
../bin/occmd rffe.comp_all.reset
../bin/occmd hci.led.fw.set 1
../bin/occmd rffe.ch1_fe.enable
../bin/occmd rffe.ch1_fe.disable
../bin/occmd rffe.ch2_fe.disable
../bin/occmd rffe.ch2_fe.enable
../bin/occmd testmodule.2gsim.enable
../bin/occmd testmodule.2gsim.disable
../bin/occmd testmodule.2gsim.disconnect_nw
../bin/occmd testmodule.2gsim.connect_nw
../bin/occmd testmodule.2gsim.send 9789799425 hi
../bin/occmd testmodule.2gsim.dial 9789799425
../bin/occmd testmodule.2gsim.answer
../bin/occmd testmodule.2gsim.hangup
../bin/occmd testmodule.comp_all.reset
../bin/occmd sdr.comp_all.reset
../bin/occmd sdr.fx3.reset
../bin/occmd obc.iridium.reset
../bin/occmd sync.comp_all.reset
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
echo "Done..."

