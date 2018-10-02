#!/usr/bin/env python

"""
Start the process and dump the documentation to the doc dir
"""

import socket, subprocess, time,os

env = os.environ
env['L1FWD_BTS_HOST'] = '127.0.0.1'

bts_proc = subprocess.Popen(["./src/osmo-bts-sysmo/sysmobts-remote",
		"-c", "./doc/examples/sysmo/osmo-bts.cfg"], env = env,
		stdin=None, stdout=None)
time.sleep(1)

try:
	sck = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sck.setblocking(1)
	sck.connect(("localhost", 4241))
	sck.recv(4096)

	# Now send the command
	sck.send("show online-help\r")
	xml = ""
	while True:
		data = sck.recv(4096)
		xml = "%s%s" % (xml, data)
		if data.endswith('\r\nOsmoBTS> '):
			break

	# Now write everything until the end to the file
	out = open('doc/vty_reference.xml', 'w')
	out.write(xml[18:-11])
	out.close()
finally:
	# Clean-up
	bts_proc.kill()
	bts_proc.wait()

