import os
import sys
import json
from subprocess import Popen, PIPE
import shlex

DB_FILE = "data.json"
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

    def get(self):
        d = self.db['module'][self.module]['get']
        for x in d:
            if((x == self.arglist[PARAM]) or (self.arglist[PARAM] == 'all') ):
                if(x == 'all'):
                    print "all\n"
                else:
                    d = d[x]
                    buf = self.run_command(d['cmd']+ ' ' + d['path'] + d['exe'])
                    print x + ' : ' + buf.rstrip('\n') + ' ' + d['unit']

    def run_command(self, command):
        buf = ""
        process = Popen(shlex.split(command), stdout=PIPE)
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                buf  = buf + output
                #print buf
        rc = process.poll()
        return buf

    def set(self):
        pass

    def print_vars(self):
        print("module_name: %s" %(self.module))
        print("operation: %s" %(self.operation))
        print("param: %s" %(self.param))
        print("value: %s" %(self.value))
        print("DB file: %s" %(self.dbfile))


    def load_db(self):
        try:
            f = open(self.dbfile)
            self.db = json.load(f)
            f.close()
        except:
            print "File error %s" %(self.dbfile)
            sys.exit(-1);

        return 0
   
    def parse(self):
        d = self.db['module']

        try:
            # Find module name
            for x in d:
                if(x == self.arglist[MODULE_NAME]):
                    if((x == 'help') or (x == 'version')):
                        print(d[x])
                        return 0
                    else:
                        # Find operation
                        self.module = x
                        d = d[x]
                        for y in d:
                            if(y == self.arglist[OPERATION]):
                                eval('self.'+y+'()')
                                return 0

                        return -1
                            
            return -1

        except:
                print("Failure during parse\n")
                sys.exit(-1)
                


    # Store the args for processing
    def load_args(self, args):
       
        self.arglist = args
        self.arglist = self.arglist[1:]

    def print_cmds(self):
        pass;


def main():
    a = dvt(DB_FILE)
    a.load_db()
    a.load_args(sys.argv)

    if(a.parse()):
        print a.db['module']['help'] 
                        
    sys.exit(0)


if __name__ == '__main__': main()
