import os
import sys
import json
import subprocess
import time
from dvt import *

def run_reliability(a):

    monitor_bb_current_temp(a)
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

    #Get the GPIO1 value for the BB Alert. 
    bb_alert_gpio = getdb['bb_alert_gpio']['cmd']+' ' + \
                   getdb['bb_alert_gpio']['path']+getdb['bb_alert_gpio']['file']

    #Get the GPIO13 value for the FE Alert.
    fe_alert_gpio = getdb['fe_alert_gpio']['cmd']+' ' + \
                   getdb['fe_alert_gpio']['path']+getdb['fe_alert_gpio']['file']
    print("Running reliabilty ..\n")


    #checking for the Interrupt occurence 
    while 1:
        bb = int(a.run_command(bb_alert_gpio))
        fe =int(a.run_command(fe_alert_gpio))
        if(bb == 0):
            c = int(a.run_command(input_current))
            l = int(a.run_command(get_current_limit))
            i = int(a.run_command(get_board_temp))
            m = int(a.run_command(get_temp_limit))
            if(c > l):
                print("BB CURRENT ALERT: Limit %d Actual board current  %d\n" %(c, l))
            if(i > m):
                print("BB TEMPERATURE ALERT: Limit %d Actual board temp %d\n" %(m, i))
        if(fe == 0):
                print("FE TEMPERATURE  Alert tenp is > 80 degrees \n")
        time.sleep(3)   



