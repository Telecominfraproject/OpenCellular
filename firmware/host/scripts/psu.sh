
#!/bin/sh

#temp sensors

echo "temp sensors"
../bin/occmd psuCore.sensors.status.temp_sensor1.temperature.get 
../bin/occmd psuCore.sensors.config.temp_sensor1.critlimit.get
../bin/occmd psuCore.sensors.config.temp_sensor1.highlimit.get
../bin/occmd psuCore.sensors.config.temp_sensor1.lowlimit.get
../bin/occmd psuCore.sensors.config.temp_sensor1.critlimit.set 100
../bin/occmd psuCore.sensors.config.temp_sensor1.highlimit.set  90
../bin/occmd psuCore.sensors.config.temp_sensor1.lowlimit.set   20

../bin/occmd psuCore.sensors.status.temp_sensor2.temperature.get
../bin/occmd psuCore.sensors.config.temp_sensor2.critlimit.get
../bin/occmd psuCore.sensors.config.temp_sensor2.highlimit.get
../bin/occmd psuCore.sensors.config.temp_sensor2.lowlimit.get
../bin/occmd psuCore.sensors.config.temp_sensor2.critlimit.set 101
../bin/occmd psuCore.sensors.config.temp_sensor2.highlimit.set   91
../bin/occmd psuCore.sensors.config.temp_sensor2.lowlimit.set   21

../bin/occmd psuCore.sensors.status.temp_sensor3.temperature.get
../bin/occmd psuCore.sensors.config.temp_sensor3.critlimit.get
../bin/occmd psuCore.sensors.config.temp_sensor3.highlimit.get
../bin/occmd psuCore.sensors.config.temp_sensor3.lowlimit.get
../bin/occmd psuCore.sensors.config.temp_sensor2.critlimit.set 101
../bin/occmd psuCore.sensors.config.temp_sensor2.highlimit.set 91
../bin/occmd psuCore.sensors.config.temp_sensor2.lowlimit.set 21

# ina for GBC board 

echo "ina for GBC board "

../bin/occmd psubms.gbc.status.current_sensor1.busvoltage.get
../bin/occmd psubms.gbc.status.current_sensor2.busvoltage.get
../bin/occmd psubms.gbc.status.current_sensor3.busvoltage.get

../bin/occmd psubms.gbc.status.current_sensor1.power.get
../bin/occmd psubms.gbc.status.current_sensor2.power.get
../bin/occmd psubms.gbc.status.current_sensor3.power.get


../bin/occmd psubms.gbc.status.current_sensor1.shuntvoltage.get
../bin/occmd psubms.gbc.status.current_sensor2.shuntvoltage.get
../bin/occmd psubms.gbc.status.current_sensor3.shuntvoltage.get


../bin/occmd psubms.gbc.status.current_sensor1.current.get
../bin/occmd psubms.gbc.status.current_sensor2.current.get
../bin/occmd psubms.gbc.status.current_sensor3.current.get


../bin/occmd psubms.gbc.config.current_sensor1.currlimit.get
../bin/occmd psubms.gbc.config.current_sensor2.currlimit.get
../bin/occmd psubms.gbc.config.current_sensor3.currlimit.get

../bin/occmd psubms.gbc.config.current_sensor1.currlimit.set 2000
../bin/occmd psubms.gbc.config.current_sensor2.currlimit.set 2100
../bin/occmd psubms.gbc.config.current_sensor3.currlimit.set 2200

../bin/occmd psubms.gbc.config.current_sensor1.currlimit.get
../bin/occmd psubms.gbc.config.current_sensor2.currlimit.get
../bin/occmd psubms.gbc.config.current_sensor3.currlimit.get

# ina for BB board 
echo "ina for bb board "
../bin/occmd psubms.bb.status.current_sensor.busvoltage.get
../bin/occmd psubms.bb.status.current_sensor.power.get
../bin/occmd psubms.bb.status.current_sensor.current.get
../bin/occmd psubms.bb.status.current_sensor.shuntvoltage.get
../bin/occmd psubms.bb.config.current_sensor.currlimit.get
../bin/occmd psubms.bb.config.current_sensor.currlimit.set 2000
../bin/occmd psubms.bb.config.current_sensor.currlimit.get


# inasensor for fe 
echo "ina for fe board "

../bin/occmd psubms.fe1.status.current_sensor1.busvoltage.get
../bin/occmd psubms.fe1.status.current_sensor2.busvoltage.get
../bin/occmd psubms.fe1.status.current_sensor3.busvoltage.get
../bin/occmd psubms.fe2.status.current_sensor4.busvoltage.get
../bin/occmd psubms.fe2.status.current_sensor5.busvoltage.get
../bin/occmd psubms.fe2.status.current_sensor6.busvoltage.get

../bin/occmd psubms.fe1.status.current_sensor1.power.get
../bin/occmd psubms.fe1.status.current_sensor2.power.get
../bin/occmd psubms.fe1.status.current_sensor3.power.get
../bin/occmd psubms.fe2.status.current_sensor4.power.get
../bin/occmd psubms.fe2.status.current_sensor5.power.get
../bin/occmd psubms.fe2.status.current_sensor6.power.get

../bin/occmd psubms.fe1.status.current_sensor1.shuntvoltage.get
../bin/occmd psubms.fe1.status.current_sensor2.shuntvoltage.get
../bin/occmd psubms.fe1.status.current_sensor3.shuntvoltage.get
../bin/occmd psubms.fe2.status.current_sensor4.shuntvoltage.get
../bin/occmd psubms.fe2.status.current_sensor5.shuntvoltage.get
../bin/occmd psubms.fe2.status.current_sensor6.shuntvoltage.get

../bin/occmd psubms.fe1.status.current_sensor1.current.get
../bin/occmd psubms.fe1.status.current_sensor2.current.get 
../bin/occmd psubms.fe1.status.current_sensor3.current.get
../bin/occmd psubms.fe2.status.current_sensor4.current.get
../bin/occmd psubms.fe2.status.current_sensor5.current.get
../bin/occmd psubms.fe2.status.current_sensor6.current.get

../bin/occmd psubms.fe1.config.current_sensor1.currlimit.get
../bin/occmd psubms.fe1.config.current_sensor2.currlimit.get
../bin/occmd psubms.fe1.config.current_sensor3.currlimit.get
../bin/occmd psubms.fe2.config.current_sensor4.currlimit.get
../bin/occmd psubms.fe2.config.current_sensor5.currlimit.get
../bin/occmd psubms.fe2.config.current_sensor6.currlimit.get

../bin/occmd psubms.fe1.config.current_sensor1.currlimit.set 2000
../bin/occmd psubms.fe1.config.current_sensor2.currlimit.set 2100
../bin/occmd psubms.fe1.config.current_sensor3.currlimit.set 2200
../bin/occmd psubms.fe2.config.current_sensor4.currlimit.set 2300
../bin/occmd psubms.fe2.config.current_sensor5.currlimit.set 2400
../bin/occmd psubms.fe2.config.current_sensor6.currlimit.set 2500

../bin/occmd psubms.fe1.config.current_sensor1.currlimit.get
../bin/occmd psubms.fe1.config.current_sensor2.currlimit.get
../bin/occmd psubms.fe1.config.current_sensor3.currlimit.get
../bin/occmd psubms.fe2.config.current_sensor4.currlimit.get
../bin/occmd psubms.fe2.config.current_sensor5.currlimit.get
../bin/occmd psubms.fe2.config.current_sensor6.currlimit.get
#battery 
echo "lion "

#../bin/occmd psuCore.lion.status.battery.batteryCurrent.get
#../bin/occmd psuCore.lion.status.battery.ichargeDAC.get
#../bin/occmd psuCore.lion.status.battery.systemVoltage.get
#../bin/occmd psuCore.lion.status.battery.batteryVoltage.get
#../bin/occmd psuCore.lion.status.battery.inputCurrent.get
#../bin/occmd psuCore.lion.status.battery.dieTemperature.get
#../bin/occmd psuCore.lion.status.battery.inputVoltage.get

#../bin/occmd psuCore.lion.config.battery.batteryCurrentLow.get
#../bin/occmd psuCore.lion.config.battery.inputCurrentHigh.get
#../bin/occmd psuCore.lion.config.battery.batteryVoltageHigh.get
#../bin/occmd psuCore.lion.config.battery.inputCurrentLimit.get
#../bin/occmd psuCore.lion.config.battery.batteryVoltageLow.get
#../bin/occmd psuCore.lion.config.battery.inputVoltageLow.get
#../bin/occmd psuCore.lion.config.battery.dieTemperature.get
#../bin/occmd psuCore.lion.config.battery.vcharge.get
#../bin/occmd psuCore.lion.config.battery.icharge.get

#pd ltc4295
echo "ltc4295 sensor "
../bin/occmd psuCore.pd.status.class.get
../bin/occmd psuCore.pd.status.powerGoodState.get

#pse ltc4274
echo "ltc4274 sensor"
../bin/occmd psuCore.pse.status.class.get
../bin/occmd psuCore.pse.status.detection.get
../bin/occmd psuCore.pse.status.powerGood.get

../bin/occmd psuCore.pse.config.detectEnable.get 
../bin/occmd psuCore.pse.config.interruptEnable.get
../bin/occmd psuCore.pse.config.operatingMode.get
../bin/occmd psuCore.pse.config.enableHighpower.get
../bin/occmd psuCore.pse.config.interruptMask.get

#debug 
echo "debugGPIO"

../bin/occmd psubms.debugGPIO.PA.get 0
../bin/occmd psubms.debugGPIO.PB.get 0
../bin/occmd psubms.debugGPIO.PC.get 0
../bin/occmd psubms.debugGPIO.PD.get 0
../bin/occmd psubms.debugGPIO.PE.get 0 
../bin/occmd psubms.debugGPIO.PF.get 0 
../bin/occmd psubms.debugGPIO.PG.get 0 

../bin/occmd psubms.debugIOexpander.ioexpander.get 2

echo "debugI2C get"

../bin/occmd psubms.debugI2C.bus0.get 86 1 0
#21577 INA mfg id
../bin/occmd psubms.debugI2C.bus1.get 64 2 254

#21577 INA mfg id
../bin/occmd psubms.debugI2C.bus2.get 65 2 254

#../bin/occmd psubms.debugI2C.bus3.get

#21577 INA mfg id
../bin/occmd psubms.debugI2C.bus4.get 64 2 254
../bin/occmd psubms.debugI2C.bus5.get 86 1 0




