'''modus operandi:) 29 June 2019
1)create table in sqlite [done]
2)connect to sqlite db and display table contents [done]
3) create another table, say table2 [done]
4) do some computation on table 1 and overwrite table 2 with computation output
5) 
'''

import sqlite3
from sqlite3 import Error
#from interval import Interval, IntervalSet
#from interval import Interval, IntervalSet
import intervals as interval #requires: pip install python-intervals

#import intervals as IntervalSet

import threading
import time

'''
Global constants
'''
#databases
spectrum_data_base = "test.db"
#database tables
spectrum_assignment_long_term = "table1" #occupied spectrum
spectrum_unassigned = "table2" #vacant spetrum
spectrum_assignment_dynamic = "table3" #dynamically assigned spectrum or short-term spectrum lease
spectrum_assignment_changes_flag = "flags" # containts binary values (0/1) to indicate whether or not there's been a change in frequency assignment

#frequency bands
nine_hundred_MHz_band_start = 880
nine_hundred_MHz_band_end = 960

eighteen_hundred_MHz_band_start = 1710
eighteen_hundred_MHz_band_end = 1880

twenty_one_hundred_MHz_band_start = 1920
twenty_one_hundred_MHz_band_end = 2170

twenty_six_hundred_MHz_band_start = 2500
twenty_six_hundred_MHz_band_end = 2690

#Other
boundary = 1
spectrum_info_update_interval = 5
    
def spectrum_assignment_changes_monitor(conn):    
    '''
    Periodically check if there any changes in frequence assignment
    '''
    while True:
        #keep checking for changes in spectrum assignment
        conn = create_connection(spectrum_data_base)
        cur = conn.cursor()
        sql = "SELECT fixed_frequency_assignment, dynamic_frequency_assignment FROM flags" #+ spectrum_assignment_changes_monitor
        cur.execute(sql)    
        #row = cur.fetchall()
        #val1 = cur.fetchone()[0] #returns single value fron first column
        val2 = cur.fetchone() #returns first row
        print 'Flag status: '
        print val2[0], val2[1]
        if (val2[0] == 1 or val2[1] == 1):
            #recalculate vacant spetrum
            vacant_spectrum_finder(conn)
        #print val2[0], val2[1]
        #hello()
        time.sleep (spectrum_info_update_interval)
        
    

def create_connection(db_file):
    """ create a database connection to the SQLite database
        specified by the db_file
    :param db_file: database file
    :return: Connection object or None
    """
    try:
        conn = sqlite3.connect(db_file)
        return conn
    except Error as e:
        print(e)
 
    return None

def display_table_contents(conn):
    cur = conn.cursor()
    sql = 'SELECT * FROM '+spectrum_assignment_long_term
    cur.execute(sql)
    
    rows = cur.fetchall()
    
    for row in rows:
        print(row) 
    return rows

def save_data_to_db(conn, records):
    sql = 'DELETE FROM table3'
    cur = conn.cursor()
    cur.execute(sql) #delete all previous table contents
    
    cur = conn.cursor()
    sql = 'INSERT INTO table3 (ID, freqStart, freqEnd) VALUES '# + str(row)
    
    for row in records:
        cur.execute(sql + str(row)) 
        #print row
    return cur.lastrowid
    
def vacant_spectrum_finder(conn):
    '''
    Let A be total spectrum.
    Let B be assigned frequencies (nationally and long term)
    Unassigned frequencies C = A - B
    Further let D be dynamic and possibly short term frequency assignments
    Vacant spectrum C' = C - D
    ----------------------------------------------------
    '''    
    '''
    start by erasing previous records of unassigned spectrum
    '''
    sql = 'DELETE FROM '+spectrum_unassigned
    cur = conn.cursor()
    cur.execute(sql) #delete all previous table contents
        
    '''
    fetch statically assigned frequencies  
    '''
    cur = conn.cursor()
    sql = 'SELECT * FROM '+spectrum_assignment_long_term
    cur.execute(sql)
        
    static_frequency_assignment = cur.fetchall()      
    
    '''
    fetch dynamic frequency assignment
    '''
    cur = conn.cursor()
    sql = 'SELECT * FROM '+spectrum_assignment_dynamic
    cur.execute(sql)
        
    dynamic_frequency_assignment = cur.fetchall()      
      
    '''
    Specify total spectrum in terms of START and STOP frequency
    '''
    #r = interval.closed(nine_hundred_MHz_band_start, nine_hundred_MHz_band_end) #900 MHz band
    r = interval.closed(twenty_six_hundred_MHz_band_start, twenty_six_hundred_MHz_band_end) #2600 MHz band
    
    '''
    substract assigned frequencies (long term) from total frequency range i.e A-B
    '''   
    for row in  static_frequency_assignment:
        #cur.execute(sql + str(row)) 
        #vacant_spectrum.append(row)
        r1 = interval.closed(row[1]-boundary, row[2]+boundary) # -,+ guardband  ensures overlapingbounds are excluded
        temp = r - r1
        r = temp
    
    '''
    Subtract dynamically assigned frequencies i.e. C- D
    '''
    for row in dynamic_frequency_assignment:
        r1 = interval.closed(row[1]-boundary, row[2]+boundary)
        temp = r - r1
        r = temp
        
    #print("Append output: ")
    #for item in vacant_spectrum:
    #    print(item[1], item[2])
    #r1 = interval.closed(3,7)
    #r2 = interval.closed(1,20)
    #r3 = interval.closed(13, 16)
    #temp = r2 - r1
    #print(temp - r3)
    
    vacant_spectrum = [] #list of rows
    vacant_spectrum = temp
    
    '''
    Save the newly calculated unoccupied frequencies to database
    '''
    cur = conn.cursor()
    
    for item in vacant_spectrum:
        #The ID field is set to auto-increment and is also the primary key
        sql = 'INSERT INTO '+spectrum_unassigned+'(freqStart, freqEnd) VALUES (' + str(item).strip("( ) []") + ')' #strip off the brackets if any 
        cur.execute(sql)
    print (temp)
    
    '''
    Reset the flags
    '''
    cur = conn.cursor()
    sql = 'UPDATE '+spectrum_assignment_changes_flag + ' SET fixed_frequency_assignment = 0, dynamic_frequency_assignment = 0' 
    cur.execute(sql) 
    conn.commit() #commit changes
        
    return cur.lastrowid   
'''
 cur = conn.cursor()
    sql = 'SELECT * FROM '+spectrum_assignment_long_term
    cur.execute(sql)
    
    rows = cur.fetchall()
    
    for row in rows:
        print(row) 
    return rows
'''
    
def spectrum_response(conn):
    cur = conn.cursor()
    sql = 'SELECT * FROM '+ spectrum_unassigned
    cur.execute(sql)
    
    rows = cur.fetchall()
    response = ""
    for row in rows:
        response += " freqStart " + str(row[1])  + ", freqEnd " + str(row[2]) + "\n"
    print (response) #"freqStart", row[1], "freqEnd", row[2]
        
    #do stuff
    return 0

def hello():
    print ("hello world")
    
def main():
    database = spectrum_data_base
 
    # create a database connection
    conn = create_connection(database)
    
    #x = 
    #x.start() #launch eternal thread
    with conn:    
        print("1. Display table1 contents:")
        data = display_table_contents(conn)
        
        '''
        print("2. Attempting to save write data to table2:")
        msg = save_data_to_db(conn, data)
        print(msg)
        '''
        
        print("3. Attempting to  find unassigned spectrum:")
        '''
        Initial calculation of unassigned frequencies
        '''
        vacant_spectrum_finder(conn)
        
        print("4. Attempting to provide spectrum availability response")
        spectrum_response(conn)
    '''
    Monitor frequency assignment status
    '''
    threading.Thread(target = spectrum_assignment_changes_monitor, args =(conn,)).start()
        
 
 
if __name__ == '__main__':
    main()

