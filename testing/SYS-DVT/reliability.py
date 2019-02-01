import os
import sys
import json
import subprocess
import time
from dvt import *

def run_reliabilty(a):

    monitor_bb_current_temp(a):
    return
    
def monitor_bb_current_temp(a):

    getdb = a.db['module']['bb']['get']
    setdb = a.db['module']['bb']['set']
    
    # setting the Alert limit Register for the Current sensor  
    set_current_limit = setdb['current_alert']['cmd']+' ' + \
                  str(setdb['current_alert']['init']) \
                  +' > ' + setdb['current_alert']['path'] \
                  + setdb['current_alert']['file']
    a.run_command(set_current_limit)

    # Get the current from the current Register
    input_current = getdb['current']['cmd'] + \
                   ' ' + getdb['current']['path'] \
                   + getdb['current']['file']

    # Get the value from the Alert limit Register 
    get_current_limit = getdb['current_alert']['cmd'] + ' ' + \
                        getdb['current_alert']['path'] + \
                        getdb['current_alert']['file']
    # Setting the Temperature Limit for the Temperature sensor     
    set_temp_limit = setdb['temperature_alert']['cmd']+' ' + \
               str(setdb['temperature_alert']['init']) + \
                ' > '+ setdb['temperature_alert']['path'] + \
               setdb['temperature_alert']['file']
    a.run_command(set_temp_limit)

    # Get the current temperature  
    get_board_temp = getdb['temperature']['cmd'] + ' ' + \
                       getdb['temperature']['path'] + getdb['temperature']['file']
    # Get the limit of the Temperature
    get_temp_limit = getdb['temperature_alert']['cmd'] + ' ' + \
                     getdb['temperature_alert']['path'] + \
                     getdb['temperature_alert']['file']

    # Enable the Alert_enable flag to get one more Alert .	
    bb_enable_alert = setdb['dvt']['cmd']+' ' + str(setdb['dvt']['init']) + \
                      ' > '+ setdb['dvt']['path']+setdb['dvt']['file']
    a.run_command(bb_enable_alert)

    print("Running reliabilty ..\n")

    #checking for the Interrupt occurence 
    while 1:
       
        d = a.run_command("./dvtmod");
        if(d == "intr"):
            while 1: 
                c = int(a.run_command(input_current))
                l = int(a.run_command(get_current_limit))
                i = int(a.run_command(get_board_temp))
                m = int(a.run_command(get_temp_limit))
                # Check for current alert
        	    if(c > l):
                    print("CURRENT ALERT: Limit %d Actual input current %d\n" %(l, c))    
                #check for temperature alert
                if( i > m):
                    print("TEMPERATURE ALERT: Limit %d Actual board temp %d\n" %(m, i))    

                time.sleep(3)
	
                if(c < l and i < m):
                    break;	
           
            a.run_command(bb_enable_alert)

def main():
    a = dvt(DB_FILE)
    a.load_db()
    a.load_args(sys.argv)

   
    if(a.arglist[MODULE_NAME] == "reliability"):
        run_reliabilty(a)
        sys.exit(0)

    if(a.parse()):
        print a.db['module']['help'] 
                        
    sys.exit(0)


if __name__ == '__main__': main()
