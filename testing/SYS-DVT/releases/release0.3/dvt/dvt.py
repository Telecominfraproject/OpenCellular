import os
import sys
import json
import subprocess
import time
from reliability import *
import sys, paramiko

# cmdline format ModuleName/Operation<get/set>/Param/Value
MODULE_NAME = 0
OPERATION = 1
PARAM = 2
VALUE = 3

class dvt():

    def __init__(self, dbfile):
        self.module = ''
        self.operation = ''
        self.param = ''
        self.value = ''
        self.dbfile = dbfile
        self.db = ''
        self.arglist = ''

    def _get(self):
        d = self.db['module'][self.module]['get']
        for x in d:
            #print x
            if((x == self.arglist[PARAM]) or (self.arglist[PARAM] == 'all') ):
                try:
                    if(d[x]['pre']):
                        #run pre command
                        buf = self.run_command(d[x]['pre'])
                except:
                    pass
                buf = self.run_command(d[x]['cmd']+ ' ' + d[x]['path'] + d[x]['file'])
                print self._ts() + ' ' + x + ' : ' + buf.rstrip('\n') + ' ' + d[x]['unit'] 
                try:
                    if(d[x]['post']):
                        #run pre command
                        buf = self.run_command(d[x]['post'])
                except:
                    pass

                if(self.arglist[PARAM] != 'all'):
                    break
            
        return 0

    def _set(self):
        max_val = 0
        min_val = 0
        d = self.db['module'][self.module]['set']
        # Validate the value
        if(self.arglist[VALUE] == ''):
            return -1
        for x in d:
            if( x == self.arglist[PARAM]):
                max_val = int(d[x]['max'])
                min_val = int(d[x]['min'])
                if(int(self.arglist[VALUE]) <= max_val and int(self.arglist[VALUE] >= min_val)):
                    # Value good; Now continue
                    buf = (d[x]['cmd'] + " " + self.arglist[VALUE] + " > "+ d[x]['path'] + d[x]['file'])
                    print buf
                    self.run_command(buf)
                    break
                else:
                    print("error: Value out of range\n")

        return 0

    def _ts(self):
        buf = self.run_command("echo $(date +%x_%r)")
        return buf.rstrip('\n')
        
    def _command(self, command):
        buf = ""
        #process = Popen(shlex.split(command), stdout=PIPE)
        process = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                buf  = buf + output
        rc = process.poll()
        return buf
     
    def run_command(self, command):
        
        hostname = "192.168.1.2"
        password = "cavium.lte"
        username = "root"
        port = 22

        try:
            client = paramiko.SSHClient()
            client.load_system_host_keys()
            client.set_missing_host_key_policy(paramiko.WarningPolicy)
            client.connect(hostname, port=port, username=username, password=password)
            stdin, stdout, stderr = client.exec_command(command)
            return str(stdout.read())

        finally:
            client.close() 
        
        return 0

    def load_db(self):
        try:
            f = open(self.dbfile)
            self.db = json.load(f)
            f.close()
        except:
            print "File error %s" %(self.dbfile)
            sys.exit(-1);

        return 0

    # Does first level parsing 
    def parse(self):
        d = self.db['module']
        
        try:
            # Find module name
            for x in d:
                if(x == self.arglist[MODULE_NAME]):
                    if((x == 'help') or (x == 'version')):
                        print(d[x])
                        return 0
                    if(x == 'monitor'):
                        try:
                            run_reliability(self)
                        except:
                            sys.exit(0) 
                    else:
                        # Find operation
                        self.module = x
                        d = d[x]
                        for y in d:
                            if(y == self.arglist[OPERATION]):
                                eval('self._'+y+'()')
                                return 0

                        return -1
                            
            return -1

        except Exception as e:
                print(e)
                return -1

    # Parse the json and print the command lines for help
    def print_cmdline(self):
        pass        

    # Store the args for processing
    def load_args(self, args):
       
        self.arglist = args
        self.arglist = self.arglist[1:]

    def print_cmds(self):
        pass;
