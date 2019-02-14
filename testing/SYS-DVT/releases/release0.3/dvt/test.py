#!/usr/bin/env python

import sys, paramiko

hostname = "192.168.1.2"
password = "cavium.lte"
command = "bb get all"

username = "root"
port = 22

try:
    client = paramiko.SSHClient()
    client.load_system_host_keys()
    client.set_missing_host_key_policy(paramiko.WarningPolicy)
    client.connect(hostname, port=port, username=username, password=password)
    stdin, stdout, stderr = client.exec_command(command)
    print stdout.read(),

finally:
    client.close()
