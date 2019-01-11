import os
import sys
import json

DB_FILE = "data.json"
db = '';
module = ''
operation = ''
param = ''
value = ''

def check_args(args):
    global module
    global operation
    global param
    global value
    
    # check for module name
    if((len(args) < 2) or (args[1] == 'help')):
        return -1

    for x in db['module']:
        if(x == args[1]):
            module = x
            # Check for get/set
            if(len(args) < 3 and (args[2] == 'get' or args[2] == 'set')):
                return -1
            else:
                operation = args[2]
                if(operation == 'set'):
                    if(len(args) < 5):
                        return -1
                    else:
                        param = args[3]
                        value = args[4]
                else: # get
                    if(len(args) < 4):
                        return -1
                    else:
                        param = args[3]
   
    return 0

def load_database():
    global db
    with open(DB_FILE) as json_data:
        db = json.load(json_data)
    return 0

def main():
    load_database()
    if(check_args(sys.argv)):
         print db['module']['help']
         sys.exit(-1)

   # We have a valid request request process it
    print "Request valid" 

    sys.exit(0)


if __name__ == '__main__': main()
