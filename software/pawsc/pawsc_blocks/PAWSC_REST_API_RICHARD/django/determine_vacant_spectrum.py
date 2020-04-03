'''
Summary:

'''

import sqlite3
from sqlite3 import Error
#from interval import Interval, IntervalSet
#from interval import Interval, IntervalSet
import intervals as interval #requires: pip install python-intervals

import threading
import time
from time import gmtime, strftime

'''
Global constants
'''
#databases
spectrum_data_base = "openspectrum.db"
#database tables
spectrum_assignment_long_term = "freqAssignment" #occupied spectrum
spectrum_unassigned = "unassigned_freq" #vacant spetrum
spectrum_assignment_dynamic = "dynamic_freq_assignment" #dynamically assigned spectrum or short-term spectrum lease
spectrum_assignment_changes_flag = "flags" # containts binary values (0/1) to indicate whether or not there's been a change in frequency assignment
spectrum_operators = "operators"
spectrum_license = "spectrumLicense"

#frequency bands
nine_hundred_MHz_band_start = 880
nine_hundred_MHz_band_end = 960

eighteen_hundred_MHz_band_start = 1710
eighteen_hundred_MHz_band_end = 1880

twenty_one_hundred_MHz_band_start = 1920
twenty_one_hundred_MHz_band_end = 2170

twenty_six_hundred_MHz_band_start = 2500
twenty_six_hundred_MHz_band_end = 2690

"""
Attemping to combine above frequency ranges into one list
"""
bands = [(nine_hundred_MHz_band_start, nine_hundred_MHz_band_end), 
         (eighteen_hundred_MHz_band_start, eighteen_hundred_MHz_band_end), 
         (twenty_one_hundred_MHz_band_start, twenty_one_hundred_MHz_band_end ), 
         (twenty_six_hundred_MHz_band_start, twenty_six_hundred_MHz_band_end)] 
#Other
boundary = 1
spectrum_info_update_interval = 5 #in seconds
    
def spectrum_assignment_changes_monitor(conn):    
    '''
    Periodically check if there any changes in frequence assignment
    '''
    while True:
        #keep checking for changes in spectrum assignment
        conn = create_connection(spectrum_data_base)
        cur = conn.cursor()
        sql = "SELECT fixed_frequency_assignment, dynamic_frequency_assignment FROM " + spectrum_assignment_changes_flag
        cur.execute(sql)  
        
        val2 = cur.fetchone() #returns first row, which is all there is
        
        print strftime("%Y-%m-%d %H:%M:%S", gmtime()), 'Flag status:', val2[0], val2[1] 
        #print 'Flag status:', val2[0], val2[1]
      
        if (val2[0] == 1 or val2[1] == 1):
            print 'Refreshing unassigned frequency'
            #recalculate vacant spetrum
            vacant_spectrum_finder(conn)
       
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
    
def vacant_spectrum_finder(conn):
    '''
    -Let A be total spectrum in a given frequency band.
    -Let B be assigned frequencies (nationally and long term).
    -Unassigned frequencies C = A - B
    -Further let D be dynamic and possibly short term frequency assignments
    -Vacant spectrum C' = C - D    
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
    # do a table join and use the country ID for Mozambique
    #based on database schema: https://dbdiagram.io/d/5c719df7f7c5bb70c72f1a9a
    sql = 'SELECT freqStart, freqEnd FROM '+spectrum_assignment_long_term+ \
    ' INNER JOIN spectrumLicense ON freqAssignment.license_ID = spectrumLicense.ID  \
    INNER JOIN '+ spectrum_operators +' ON ' +spectrum_operators+'.ID = '+ spectrum_license +'.Operator_ID \
    WHERE ' +spectrum_operators+'.country_ID = 152'
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
    #r = interval.closed(eighteen_hundred_MHz_band_start, eighteen_hundred_MHz_band_end) #1800 MHz band
    #r = interval.closed(twenty_one_hundred_MHz_band_start, twenty_one_hundred_MHz_band_end) #2100 MHz band
    #r = interval.closed(twenty_six_hundred_MHz_band_start, twenty_six_hundred_MHz_band_end) #2600 MHz band       
        
    '''
    substract assigned frequencies (long term) from total frequency range i.e A-B
    ''' 
    for band in bands:
        r = interval.closed(band[0], band[1])
        for row in  static_frequency_assignment:
            #cur.execute(sql + str(row)) 
            #vacant_spectrum.append(row)
            '''
            The logic "interval.closed(row[x], row[y])" denotes start and end points of a range.
            'x' and 'y' are the columns in the table. 
            For example, since we only selected `freqStart` and `freqEnd` from the database,  the values are in column number 0 & 1  
            '''
            r1 = interval.closed(row[0]-boundary, row[1]+boundary) # -,+ 'boundary'  ensures overlapingbounds are excluded
            temp = r - r1
            r = temp     
     
        '''
        Subtract dynamically assigned frequencies i.e. C- D
        '''    
        for row in dynamic_frequency_assignment:
            r1 = interval.closed(row[2]-boundary, row[3]+boundary)
            temp = r - r1
            r = temp
            
        vacant_spectrum = [] #list of rows
        vacant_spectrum = temp
        
        '''
        Save the newly calculated unoccupied frequencies to database if vacant_spectrum != empty
        '''
        freq_band = ""
        if (check_if_list_empty(temp)==True ): #needs fixing, currently breaks when list is empty [4 July 2019]
            print 'No vacant spectrum found'
        else:
            cur = conn.cursor()            
            for item in vacant_spectrum:
                '''
                Determine frequency band
                '''
                if item in interval.closed(nine_hundred_MHz_band_start, nine_hundred_MHz_band_end):
                    freq_band = "900" 
                elif item in interval.closed(eighteen_hundred_MHz_band_start, eighteen_hundred_MHz_band_end):
                    freq_band = "1800"
                elif item in interval.closed(twenty_one_hundred_MHz_band_start, twenty_one_hundred_MHz_band_end):
                    freq_band = "2100"
                elif item in interval.closed(twenty_six_hundred_MHz_band_start, twenty_six_hundred_MHz_band_end):
                   freq_band = "2600"         
                             
                #The ID field is set to auto-increment and is also the primary key
                sql = 'INSERT INTO '+spectrum_unassigned+' (freqStart, freqEnd, band) VALUES (' + str(item).strip("( ) []")+ ','+freq_band +')' #strip off the brackets if any 
                cur.execute(sql)            
            print (temp)
    
    '''
    Reset the flags
    '''
    cur = conn.cursor()
    sql = 'UPDATE '+spectrum_assignment_changes_flag + ' SET fixed_frequency_assignment = 0, dynamic_frequency_assignment = 0' 
    cur.execute(sql) 
    conn.commit() #commit changes
        
    return None 
  

def check_if_list_empty(seq):
     try:
         return all(map(check_if_list_empty, seq))
     except TypeError:
         return False

    
def main():     
    
    conn = create_connection(spectrum_data_base) #create a database connection
        
    with conn:    
        
        print("Attempting to  find unassigned spectrum:")
        '''
        Initial calculation of unassigned frequencies
        '''
        vacant_spectrum_finder(conn)        
        
    '''
    Monitor frequency assignment status 
    '''
    threading.Thread(target = spectrum_assignment_changes_monitor, args =(conn,)).start()     
 
 
if __name__ == '__main__':
    main()

