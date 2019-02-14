import os
import sys
import json
import subprocess
import time
from dvt import *

def run_reliability(a):

    print("Running reliability n returning ..\n");
    #monitor_bb_current_temp(a)
    return
    
def monitor_bb_current_temp(a):

    print("\nMonitoring BB Current/Temperature interrupt\n")
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
    bb_enable_alert = setdb['bb_alert_enable']['cmd']+' ' + str(setdb['bb_alert_enable']['init']) + \
                      ' > '+ setdb['bb_alert_enable']['path']+setdb['bb_alert_enable']['file']

    #Get the Alert status flag to decide the Alert type 
    alert_status = getdb['alert_status']['cmd']+' ' + \
                   getdb['alert_status']['path']+getdb['alert_status']['file']

    #Enable the FE Alert flag to get one more alert 
    fe_enable_alert = setdb['fe_alert_enable']['cmd']+' ' + str(setdb['fe_alert_enable']['init']) + \
                      ' > '+ setdb['fe_alert_enable']['path']+setdb['fe_alert_enable']['file']
    
    #Enable the DVT Alert flag to get one more alert
    dvt_enable_alert = setdb['dvt_alert_status']['cmd']+' ' + str(setdb['dvt_alert_status']['init']) + \
                               ' > '+ setdb['dvt_alert_status']['path']+setdb['dvt_alert_status']['file']
    a.run_command(bb_enable_alert)
    a.run_command(fe_enable_alert)
    print("Running reliabilty ..\n")

    #checking for the Interrupt occurence 
    while 1:
     
        d = a.run_command("./dvtmod")
        if(d == "intr"):
            status = int(a.run_command(alert_status))
            if((status == 1) or (status == 3)):
                c = int(a.run_command(input_current))
                l = int(a.run_command(get_current_limit))
                i = int(a.run_command(get_board_temp))
                m = int(a.run_command(get_temp_limit))
        	if(c > l):
                    print("BB CURRENT ALERT: Limit %d Actual board current  %d\n" %(c, l))
                if( i > m):
                    print("BB TEMPERATURE ALERT: Limit %d Actual board temp %d\n" %(m, i))
            if(status == 2):
                print("FE TEMPERATURE  Alert tenp is > 80 degrees \n")
                a.run_command(fe_enable_alert)
        time.sleep(3)   
        a.run_command(dvt_enable_alert)
        a.run_command(fe_enable_alert)
        a.run_command(bb_enable_alert)

