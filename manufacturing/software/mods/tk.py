#!/usr/bin/python3
#
#	File: tk.py
#

import tkinter as tk
from tkinter import *
from tkinter import ttk
from tkinter import scrolledtext
from mods.misc import mk_sock
from multiprocessing import Queue
import os
import time
# from threading import Thread
# t_result = Queue(maxsize = 1)
gif = []
sts = [	#12345678901234567890
	'Wait<10%: ..........',	# 0
	'Wait 10%: |.........',	# 1
	'Wait 20%: ||........',	# 2
	'Wait 30%: |||.......',	# 3
	'Wait 40%: ||||......',	# 4
	'Wait 50%: |||||.....',	# 5
	'Wait 60%: ||||||....',	# 6
	'Wait 70%: |||||||...',	# 7
	'Wait 80%: ||||||||..',	# 8
	'Wait 90%: |||||||||.',	# 9
	'Done:     ||||||||||',	# 10
	]
	

#
#	Widget Layout Row,Column
#		0,0 ent1, ent2, but1	0,1 start	0,2 tv
#		1,0 but2		1,1 Sess	1,2 tv
#		2,0 but3 		2,1 PS1		2,2 tv
#					3,1 PS2
#					4,1 PS3
#					5,1 Setup
#	Frame Layout Row,Column
#		0,0 ftxt		0,1 ssfrm	0,2 tfrm
#		1,0 f2but		1,1 ssfrm	1,2 tfrm
#		2,0 f3but		2,1 ssfrm	2,2 tfrm
#					3,1 ssfrm
#					4,1 ssfrm
#					5,1 ssfrm
#

class Tkwin(object):
	root = ''
	d = dict()				# Test key, addr in text.
	rtps = dict()				# Run time ps changes.
	su = dict()				# Setup steps
	ssfd =	[
		{ 'loc': 1, 'txt': 'Socket'},	# 0
		{ 'loc': 2, 'txt': 'PS1'},	# 1
		{ 'loc': 3, 'txt': 'PS2'},	# 2
		{ 'loc': 4, 'txt': 'PS3'},	# 3
		{ 'loc': 5, 'txt': 'Setup'},	# 4
		{ 'loc': 6, 'txt': 'ssh'}	# 5
		]
	tfrm = ''	# Frame for text box
	ssfrm = ''	# Frame for start and status buttons
	tv = ''		# Text box widget
	sb = ''		# Scrollbar widget
	lno = 0
	ent1 = ''
	lab1 = ''
	ent2 = ''
	lab2 = ''
	ftxt = ''	# Text entry frame
	f2but = ''	# Run button frame
	f3but = ''	# Exit button frame
	but1 = ''	# Submit button
	but2 = ''	# Run button
	but3 = ''	# Exit button
	sbut = ''	# Start button
	btxt = ''	# Text for button 5
	osock = 0
	pfn = '/usr/local/fbin/mods/pass2_icon.gif'
	ffn = '/usr/local/fbin/mods/fail2_icon.gif'
	fn = ''
	pbar = ''
	
	def __init__(self):
		# tk.Frame.__init__(self, root)
		self.root = tk.Tk()
		# top = Toplevel(self.root)
		# top.protocol("WM_DELETE_WINDOW", self.root.destroy)
		self.init_widgets()
		self.root.title('init_widgets here')
		self.root.lift()
		self.root.attributes('-topmost', True)
		self.root.after_idle(self.root.attributes, '-topmost', False)
		self.root.mainloop()

	def init_widgets(self):
		# top = Toplevel(self.root)
		# top.protocol("WM_DELETE_WINDOW", self.root.destroy)
		# self.root = tk.Tk()
		self.root.geometry('1440x900+50+50')
		self.tfrm = ttk.Frame(self.root)
		self.tfrm.grid(row = 0, column = 2, rowspan = 3)

		self.tfrm.config(height = 500, width = 900, relief = RIDGE)
		self.tfrm.config(padding = (30, 15))
		self.tv = Text(self.tfrm, width=80, height=55)
		self.tv.config(wrap = 'none')
		self.tv.grid(row=0, column=0, sticky='n')

		# Add scrollbar to right of text window.
		self.sb = ttk.Scrollbar(self.root, orient = VERTICAL,
				command = self.tv.yview)
		self.sb.grid(row = 0, column = 2, rowspan = 3, sticky = 'nse')
		self.tv.config(yscrollcommand = self.sb.set)

		# Start and status frame.
		self.ssfrm = ttk.Frame(self.root)
		self.ssfrm.grid(row = 0, column = 1, rowspan = 3, sticky='n')
		self.ssfrm.config(height = 500, width = 300, relief = RIDGE)
		self.sbut = ttk.Button(self.ssfrm, text = 'Start',
			style="X.TButton")
		self.sbut.config(command = self.start_action)
		self.sbut.state(['!disabled'])
		self.sbut.grid(row=0, column=1, padx = 3, pady = 5)

		# Setup dashboard in ssfrm.
		for n in range(0, 6):
			b = self.ssfd[n]
			b['lab'] = ttk.Label(self.ssfrm, text=b['txt'])
			b['lab'].grid(row=b['loc'], padx = 3, pady = 5)
			if (n == 5):
				# self.btxt = tk.StringVar()
				# b['bttn'] = Button(self.ssfrm,
				# 	textvariable=self.btxt,
				# 	height=2, width=2, bg='black')
				b['bttn'] = tk.Label(self.ssfrm,
						text = b['txt'])
				b['bttn'].config(width = 20, bg = 'black',
						font = ('Courier', 12, 'bold'))
			else:
				b['bttn'] = Button(self.ssfrm, 
					height=2, width=2, bg='black')
			b['bttn'].grid(row=b['loc'], 
				column=1, padx = 3, pady = 5)

		# Text Entry Frame
		self.ftxt = ttk.Frame(self.root)
		self.ftxt.grid(row = 0, column = 0, padx = 25, pady = 25)

		self.ftxt.config(height = 100, width = 500, relief = RIDGE)
		self.ftxt.config(padding = (45, 30))
		self.lab1 = ttk.Label(self.ftxt, text='Operator Name:', 
			width = 15)
		self.lab1.grid(row=0, padx = 3, pady = 5)
		self.ent1 = ttk.Entry(self.ftxt, width = 24)
		self.ent1.grid(row=0,column=1, pady = 5)
		self.lab2 = ttk.Label(self.ftxt, text='Serial Number:', 
			width = 15)
		self.lab2.grid(row=1, padx = 3, pady = 5)
		self.ent2 = ttk.Entry(self.ftxt, width = 24)
		self.ent2.grid(row=1,column=1, pady = 5)
		self.ent1.focus()
		style = ttk.Style()
		style.map("X.TButton",
			foreground=[('disabled', 'slate gray'), 
			('!disabled', 'black')],
			background=[('disabled', 'dim gray'),
			('!disabled', 'cadetblue')]
			)
		style.configure("X.TButton",
			font=('Helvetica', 18, 'bold')
			)
		# print(ttk.Style().lookup("X.TButton", "font"))
		self.but1 = ttk.Button(self.ftxt, text = 'Submit',
			style="X.TButton")
		self.but1.config(command = self.submit_action)
		self.but1.grid(row = 2, column = 0, columnspan = 2, pady = 5)
		self.but1.state(['disabled'])

		# Begin Run button frame
		# Position the button 2 frame under the button 1 frame.
		self.f2but = ttk.Frame(self.root)
		self.f2but.grid(row = 1, column = 0, padx = 25, pady = 25)

		self.f2but.config(height = 100, width = 300, relief = RIDGE)
		self.f2but.config(padding = (45, 30))
		self.but2 = ttk.Button(self.f2but, text = 'Run',
			style="X.TButton")
		self.but2.config(command = self.run_action)
		self.but2.grid(row = 0, column = 0, columnspan = 2, pady = 5)
		self.but2.state(['disabled'])

		# Begin Exit button frame
		# Position the button 3 frame under the button 1 frame.
		self.f3but = ttk.Frame(self.root)
		self.f3but.grid(row = 2, column = 0, padx = 25, pady = 25)

		self.f3but.config(height = 100, width = 300, relief = RIDGE)
		self.f3but.config(padding = (45, 30))
		self.but3 = ttk.Button(self.f3but, text = 'Exit',
			style="X.TButton")
		self.but3.config(command = self.exit_action)
		self.but3.grid(row = 0, column = 0, columnspan = 2, pady = 5)
		self.but3.state(['!disabled'])
		self.pbar = ttk.Progressbar(self.f2but, orient = HORIZONTAL,
			length = 200)
		ttk.Label(self.f2but,text = 'Progress').grid(row = 2,column = 0)
		self.pbar.grid(row = 3, column = 0, pady = 5)
		self.pbar.config(mode = 'determinate', maximum = 10, value = 0)

	def tree_update(self, item, text):
		tv = self.tv
		# print("I'm updating", item, text)
		pfre = re.compile(r'pass')
		if (pfre.match(text)):
			fn = self.pfn
		else:
			fn = self.ffn
		n = self.lno
		addr = self.d[item]
		p = (float(addr) / n) * 10.0
		self.pbar.config(value = p)
		self.pbar.update()
		n = int(round(float(addr)))
		addr = addr + ' lineend'
		# print('My hoped for insert addr is ' + addr + '.')
		tv.mark_set('my_mark', addr)
		tv.mark_gravity('my_mark', 'right')
		tv.see('my_mark')
		# The image variable used by image_create must be a global.
		tv.insert(addr, ' ' + text + ' ')
		tv.mark_set('my_mark', addr)
		tv.mark_gravity('my_mark', 'right')
		global gif
		# print("My file name is", fn)
		gif.append(PhotoImage(file=fn).subsample(16, 16))
		# gif.append(PhotoImage(file = fn))
		# print("My last gif is", gif[-1])
		tv.image_create('my_mark', image = gif[-1])
		tv.grid()
		pfre = re.compile(r'pass_NOPS')
		if (pfre.match(text)):
			return

		if (item in self.rtps):
			psdict = self.rtps[item]
			self.set_button(psdict['psn'], psdict['color'])
		return

	def final_update(self, nerr):
		self.pbar.config(value = 10)
		self.pbar.update()
		tv = self.tv
		# print('Total errors: ', nerr)
		n = self.lno
		addr = str(n) + '.0 lineend'
		tv.mark_set('my_mark', addr)
		tv.mark_gravity('my_mark', 'right')
		tv.insert(addr, ' ' + nerr)
		if (int(nerr) > 0):
			fn = '/usr/local/fbin/config/test.errs.txt'
			self.send_txt(fn, tv, 'no')
		tv.see('end')

	def tree_insert(self, item, text):
		tv = self.tv
		self.lno += 1
		n = self.lno
		addr = str(n) + '.0'
		tv.insert(addr, item + ': ' + text + '\n')
		self.d[item] = addr
		psre = re.compile(r'PS(\d) to (ON|OFF)', re.I)
		mobj = psre.match(text)
		if (mobj):
			psn = int(mobj.group(1))
			onoff = mobj.group(2)
			if (onoff.lower() == 'on'):
				color = 'green2'
			else:
				color = 'black'

			self.rtps[item] = { 'psn' :  psn, 'color' : color }
		else:
			return
			

	def setup_insert(self, item, text):
		self.su[item] = text

	def last(self, text):
		tv = self.tv
		ffn = self.ffn
		self.lno += 1
		n = self.lno
		addr = str(n) + '.0'
		# print('Last addr: ' + addr + '.')
		tv.insert(addr, text + '\n')
		self.d['lastline'] = addr

	def set_uname(self, name):
		ent = self.ent1
		ent.delete(0, END)
		ent.insert(0, name)
		ent.select_range(0, END)

	def set_sn(self, name):
		ent = self.ent2
		ent.delete(0, END)
		ent.insert(0, name)

	def submit_action(self):
		usr = self.ent1
		sn = self.ent2
		osock = self.osock
		osock.send_command('w_uname: ' + usr.get())
		osock.sock_reply()
		osock.send_command('w_serno: ' + sn.get())
		osock.sock_reply()
		self.but1.state(['disabled'])
		self.but2.state(['!disabled'])

	def send_txt(self, fn, tobj, delf):
		fh = open(fn, 'r')
		txt = fh.read()
		if (delf == 'yes'):
			tobj.delete('1.0', END)
			tobj.insert('1.0', txt)
		else:
			tobj.insert(END, txt)

		fh.close()

	def run_action(self):
		s = self.osock
		tv = self.tv
		time.sleep(1.0)

		self.but2.state(['disabled'])
		self.but3.state(['!disabled'])

		self.pbar.config(value = 0)
		self.pbar.update()
		cmd = 'pon'
		s.send_command(cmd)
		ret = s.sock_reply()
		# If I get na or on or ok, I can start testing.
		# Otherwise, I need to check the setup steps.
		err = re.compile(r'na|on|ok', re.IGNORECASE)
		while not err.match(ret):
			# print('pon_reply: ', ret)
			item, value = ret.split(': ')
			pre = re.compile(r'pass', re.IGNORECASE)
			if (pre.match(value)):
				psre = re.compile(r'PS(\d)')
				mobj = psre.match(self.su[item])
				if (mobj):
					psn = int(mobj.group(1))
					self.set_button(psn, 'green2')

			else:	# Error during setup, we are done.
				# I want to set setup button to red.
				# I want to blacken the rest.
				# Finally, I send an error message
				# to the text window and disable
				# all buttons except Exit.
				self.set_button(4, 'red')
				for n in range(0, 4):
					self.set_button(n, 'black')
				fn = '/usr/local/fbin/config/setup.err.txt'
				self.send_txt(fn, self.tv, 'yes')
				self.tv.see('end')
				self.but1.state(['disabled'])
				self.sbut.state(['disabled'])
				self.but3.state(['!disabled'])
				self.but3.config(command = self.exit_error)
				return

			ret = s.sock_reply()

		self.set_setup_pwr()

		cmd = 'mk_sess'
		s.send_command(cmd)
		self.set_ssh_warn()
		while True:
			try:
				ret = s.sock_reply()
			except socket.error as e:
				print ("Socket fatal error: ",  e)
				sys.exit(9)
			err = re.compile(r'waiting (\d+) seconds of (\d+)', 
				re.IGNORECASE)
			n = err.match(ret)
			if n:
				# Ignore the waiting messages.
				self.update_b5_txt(n.group(1), n.group(2))
				continue
			err = re.compile(r'ok\.', re.IGNORECASE)
			if err.match(ret):
				break
			else:
				print('Bad return from mk_sess: ', ret)
				sys.exit(10)
		self.update_b5_txt('1', '1')
		self.set_setup_ok()
		self.set_ssh_ok()

		cmd = 'status'
		s.send_command(cmd)
		ret = s.sock_reply()
		err = re.compile(r'state: 2', re.IGNORECASE)
		if not err.match(ret):
			print("I'm not in state 2: ", ret)

		cmd = 'run_tests'
		s.send_command(cmd)
		ret = s.sock_reply()
		wat = re.compile(r'waiting (\d+) part of (\d+)')
		err = re.compile(r'^Tests complete.')
		while not err.match(ret):
			# print('sock_reply: ', ret)
			wait = wat.match(ret)
			if wait:
				self.update_b5_txt(wait.group(1), 
					wait.group(2))
			else:
				item, value = ret.split(': ')
				self.tree_update(item, value)
			ret = s.sock_reply()

		nerr = re.compile(r'Total errors: (\d+)\.')
		# print('My ret is', ret)
		m = nerr.search(ret)
		n = m.group(1)
		self.final_update(n)
		
		cmd = 'poff'
		s.send_command(cmd)
		ret = s.sock_reply()
		err = re.compile(r'^ok', re.IGNORECASE)
		if not err.match(ret):
			print('Bad return from poff: ', ret)
		for n in range(1, 6):
			if (n == 5):
				self.set_ssh_off()
			else:
				self.set_button(n, 'black')

		cmd = 'report'
		s.send_command(cmd)
		ret = s.sock_reply()
		err = re.compile(r'ok\.', re.IGNORECASE)
		if not err.match(ret):
			print('Bad return from report: ', ret)
		
		s.sock_close()
		del s
		self.osock = 0
		self.set_sess_off()
		self.set_setup_off()
		self.sbut.state(['!disabled'])
		
	def exit_error(self):
		if os.path.exists(self.fn):
			os.remove(self.fn)

		sys.exit(os.EX_DATAERR)

	def exit_action(self):
		self.but3.state(['disabled'])
		self.but1.state(['disabled'])
		self.sbut.state(['disabled'])

		if self.osock != 0:
			cmd = 'exit'
			self.osock.send_command(cmd)
			# ret = self.osock.sock_reply()
			self.osock.sock_close()
			del self.osock
		
		if os.path.exists(self.fn):
			os.remove(self.fn)

		sys.exit(os.EX_OK)

	def start_action(self):
		self.sbut.state(['disabled'])
		self.osock = mk_sock()
		s = self.osock
		self.set_sess_on()
		self.tv.delete('1.0', END)
		self.lno = 0
		keys = list(self.d.keys())
		for k in keys:
			del self.d[k]

		global gif
		for g in gif:
			try: gif.remove(g)
			except: pass

		# This is a Unix socket, so it has a Unix file name.
		self.fn = s.get_fn()
		cmds = ['id', 'uname', 'serno']
		d = dict(ver = 'na', uname = 'na', serno = 'na')

		for cmd in cmds:
			s.send_command(cmd)
			data = s.sock_reply()
			if not data:
				break
			else:
				d[cmd] = data

		self.set_uname(d['uname'])
		self.set_sn(d['serno'])

		mode = os.environ.get('GUI_TEST_MODE')
		if mode is None: 
			cmd = 'test_mode: 0'
		else:
			cmd = 'test_mode: ' + mode

		s.send_command(cmd)
		line = s.sock_reply()
	
		cmd = 'test_list'
		s.send_command(cmd)

		test_seq = True
		while True:
			line = s.sock_reply()
			if line == 'test_list end.':
				break
			elif line == 'test_list still needs power sequence.':
				test_seq = False
				continue
			elif test_seq:
				item, text = line.split(": ")
				self.tree_insert(item, text)
			elif not test_seq:
				item, text = line.split(": ")
				self.setup_insert(item, text)

		self.last('total errors:')
		self.but1.state(['!disabled'])

		
	def run_gui(self):
		self.init_widgets()
		# self.root.attributes('-topmost', True)
		self.root.mainloop()

	def set_button(self, indx, color):
		# print('In set_button(), indx: ', indx, 'color', color)
		b = self.ssfd[indx]
		b['bttn'] = Button(self.ssfrm, bg=color)
		b['bttn'].grid(row=b['loc'], column=1, padx = 3, pady = 5)
		b['bttn'].update()

	def set_ssh_off(self):
		# self.set_button(5, 'yellow')
		b = self.ssfd[5]
		b['bttn'].config(bg = 'black')

	def set_ssh_warn(self):
		# self.set_button(5, 'yellow')
		b = self.ssfd[5]
		b['bttn'].config(background = 'yellow', foreground = 'black')
		b['bttn'].update()

	def set_ssh_ok(self):
		# self.set_button(5, 'green2')
		b = self.ssfd[5]
		b['bttn'].config(bg = 'green2')

	def set_setup_pwr(self):
		self.set_button(4, 'yellow')

	def set_setup_ok(self):
		self.set_button(4, 'green2')

	def set_sess_on(self):
		# self.set_button(0, '#000fff000')
		self.set_button(0, 'green2')

	def set_sess_off(self):
		self.set_button(0, 'black')

	def set_setup_off(self):
		self.set_button(4, 'black')

	def update_b5_txt(self, n, d):
		# self.btxt.set(char)
		global sts
		n = int(n)
		d = int(d)
		i = int((float((n/d)*10.0)))
		t = sts[i]
		b = self.ssfd[5]
		b['bttn'].config(text = t)
		b['bttn'].update()


# def mk_tk(root):
def mk_tk():
	tko = Tkwin()
	return tko

