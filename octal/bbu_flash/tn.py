#!/usr/bin/python

import telnetlib
import subprocess
import os
import time

tStart = time.time()

tn = telnetlib.Telnet("192.168.53.72")
print tn.read_until('terminate the Telnet session')

tn.write('info' + b'\r\n')
print tn.read_until('0xDEADBEEF', 2)

tn.write('reset halt' + b'\r\n')
print tn.read_until('processing target startup passed')

tn.write('info' + b'\r\n')
print tn.read_until('0xDEADBEEF', 3)

tn.write('load 0x400000 bdk-minimal.bin BIN' + b'\r\n')
print tn.read_until('Loading program file passed')

tn.write('go 0x400000' + b'\r\n')
tn.write('info' + b'\r\n')
print tn.read_until('0xDEADBEEF', 3)

tn.close()

cwd = os.getcwd()
os.chdir("~/octeon-bdk-2012.12/bin")
output = subprocess.call(["bdk-remote", "flash", "write", "u-boot.bin", "0"])
print output
time.sleep(3)  # can't write without wait
output = subprocess.call(["bdk-remote", "flash", "write", "u-boot.bin", "0xc0000"])
print output
os.chdir(cwd)

tEnd = time.time()

print 'Time elapsed %f sec\n' % (tEnd - tStart)
print 'Unplug BDI\n'
print 'Cycle power on BBU\n'
print 'Run teraterm.py\n'
