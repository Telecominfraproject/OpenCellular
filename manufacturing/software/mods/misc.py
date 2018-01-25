#!/usr/bin/python3
# -*- coding: utf-8 -*-
#
#	File: misc.py
#
#	Miscellaneous support routines.
#
import socket
import os, os.path
import time

#	This function opens a UNIX socket in the role of server,
#	launches a child process, accepts the child's connection,
#	and finally returns the connection and the socket file name.
class open_sock():
	fn = ''
	sf = ''
	pid = ''
	conn = ''
	addr = ''
	lfn = ''
	olfn = ''
	lfh = ''

	def __init__(self):
		def child():
			pn = '/usr/local/fbin/fbt'
			os.execl(pn, pn)
			os._exit(99)

		self.fn = '/tmp/fbt_sock'
		if os.path.exists(self.fn):
			os.remove(self.fn)
	
		server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
		server.bind(self.fn)
		server.listen(1)
	
		self.pid = os.fork()
		if self.pid == 0:
			child()
	
		self.conn, self.addr = server.accept()
		self.lfn = '/var/log/fbt/gui.log'
		self.olfn = '/var/log/fbt/gui.log.old'
		if os.path.isfile(self.olfn):
			os.unlink(self.olfn)
		if os.path.isfile(self.lfn):
			os.rename(self.lfn, self.olfn)
		self.lfh = open(self.lfn, mode='wt')
		# self.lfh.truncate()
		self.sf = self.conn.makefile()
		

	def get_conn(self):
		return self.conn

	def get_fn(self):
		return self.fn

	def get_pid(self):
		return self.pid

	def send_command(self, cmd):
		self.conn.send((cmd + '\n').encode('utf-8'))
		self.lfh.write('> ' + cmd + '\n')

	def sock_reply(self):
		sf = self.sf
		line = sf.readline().strip()
		self.lfh.write('< ' + line + '\n')
		return line

	def sock_close(self):
		self.lfh.close()
		self.conn.close()
		return os.waitpid(self.pid, 0)

def mk_sock():
	osock = open_sock()
	return osock

