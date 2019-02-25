import os
import sys
import json
import subprocess
import time
from dvt import *
from reliability import *

DB_FILE = "band28.json"

def main():
    a = dvt(DB_FILE)
    a.load_db()
    a.load_args(sys.argv)

    if(a.parse()):
        print a.db['module']['help'] 
                        
    sys.exit(0)


if __name__ == '__main__': main()
