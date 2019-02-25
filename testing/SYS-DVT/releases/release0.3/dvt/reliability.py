import os
import sys
import json
import subprocess
import time
from dvt import *

def parse_data(buff):
    current = -1
    current_alert = -1
    temperature = -1
    temperature_alert = -1;
    fe_temp_alert = -1
 
    print buff
    lines = buff.split('\n')
    for each_line in lines:
        clean_line = each_line.strip()
        if 'current' in clean_line:
            field = clean_line.split(' ')
            if field[2] == 'current':
                current = int(field[4])
            if field[2] == 'current_alert_limit':
                   current_alert = int(field[4])

        if 'temperature' in clean_line:
            field = clean_line.split(' ')
            if field[2] == 'temperature':
                temperature = int(field[4])
            if field[2] == 'temperature_alert_limit':
                   temperature_alert = int(field[4])

        if 'fe_temp_alert' in clean_line:
            field = clean_line.split(' ')
            if field[2] == 'fe_temp_alert':
                fe_temp_alert = int(field[4])

   
     # Check for any Alert trigger
    if((current > current_alert) and (current != -1) and (current_alert != -1)):
        print("***** BB CURRENT ALERT: Limit %d Actual board current  %d\n" %(current, current_alert))

    if((temperature > temperature_alert) and (temperature != -1) and (temperature_alert != -1)):
        print("***** BB TEMPERATURE ALERT: Limit %d Actual board temp %d\n" %(temperature, temperature_alert))
       
    if(fe_temp_alert != -1 and fe_temp_alert == 0):
       print("***** FE TEMPERATURE ALERT  > 80 degrees \n")
    
def run_reliability(a):
    t = a.db['module']['monitor']['delay']
    print ("Monitoring system every %d seconds ..." %(t+3))
    
    try:
        while True:
            buff =  a._command("python occmd.py bb get all")
            parse_data(buff)
            time.sleep(t)
    except Exception as e:
        pass
        #print(e)
    return 0
         
