#!/usr/bin/python

import sys
import serial
import serial.tools.list_ports
import time

all = serial.tools.list_ports.comports()
port = None
for i in all:
  if 'ttyUSB0' in i[0]:
    port = i[0]

if port is None:
  print('Device not found\n')
  sys.exit
else:
  print(port)

# configure the serial connections
ser = serial.Serial(
    port,
    baudrate=115200,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    xonxoff=False
)

if not ser.isOpen:
  print('Failed to open port\n');
  sys.exit

data = []
data.append('setenv ethaddr AA-BB-CC-DD-EE-FF')
data.append('setenv ipaddr 10.102.81.61')
data.append('setenv netmask 255.255.255.0')
data.append('setenv serverip 10.102.81.50')
data.append('setenv bootby tftp')
data.append('setenv cfgloadby tftp')
data.append('setenv swloadby tftp')
data.append('setenv i2cinit \"i2c dev 0; i2c probe; i2c dev 1; i2c probe\"')
data.append('setenv enable_eth \"i2c dev 1;i2c mw 0x21 7.1 0x7f\"')
data.append('setenv bootcmd \"run i2cinit; run enable_eth; run namedalloc; run bootcby${bootby}\"')
data.append('setenv bootcbytftp \"tftp 0x21000000 lsm_os.gz; gunzip 0x21000000 0x20000000 0x1000000; tftp 0x30800000 lsm_rd.gz; bootoctlinux 0x20000000 coremask=0x7 endbootargs rd_name=initrd mem=512M;\"')
data.append('setenv namedalloc \"namedalloc dsp-dump 0x400000 0x7f4D0000; namedalloc cazac 0x630000 0x7f8D0000; namedalloc cpu-dsp-if 0x100000 0x7ff00000; namedalloc dsp-log-buf 0x4000000 0x80000000; namedalloc initrd 0x2800000 0x30800000;\"')
data.append('setenv mk_ubootenv 1')
data.append('saveenv')

for i in range(len(data)):
  ser.write(data[i] + '\r\n')
  out = ''
  time.sleep(1)   # 1 second delay between messages
  while ser.inWaiting() > 0:
    out += ser.read(1)

  if out != '' :
    print ">>" + out

ser.close()

