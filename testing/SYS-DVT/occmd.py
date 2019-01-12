import os
import sys
import json

DB_FILE = "data.json"

class dvt():

    def __init__(self, dbfile):
        self.module = ''
        self.operation = ''
        self.param = ''
        self.value = ''
        self.dbfile = dbfile
        self.db = ''

    def get(self):
        pass

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

    def print_cmds(self):
        pass;

def check_args(args, a):
    
    # check for module name
    try:
        if((len(args) < 1) or (args[1] == 'help')):
            return -1

        for x in a.db['module']:
            if(x == args[1]):
                a.module = x
                break

        if a.module == '':
            return -1
        else:
            # Check for get/set
            if(len(args) < 2 and (args[2] == 'get' or args[2] == 'set')):
                return -1
            else:
                a.operation = args[2]
                if(a.operation == 'set'):
                    if(len(args) < 4):
                        return -1
                    else:
                        a.param = args[3]
                        a.value = args[4]
                else: # get
                    if(len(args) < 3):
                        return -1
                    else:
                        a.param = args[3]
        return 0

    except:
        return -1

def main():
    a = dvt(DB_FILE)
    a.load_db()
    if(check_args(sys.argv, a)):
         print a.db['module']['help']
         sys.exit(-1)

   # We have a valid request request process it
    a.print_vars() 

    sys.exit(0)


if __name__ == '__main__': main()
