##
## Copyright (c) Microsoft Corporation. All rights reserved.
## Licensed under the MIT License.
##


import sys
import os
import argparse
import time
import json
import sqlite3


#############################################################################################################
## args

parser = argparse.ArgumentParser()
parser.add_argument('--schema', help='Schema file (json)', required=True )
parser.add_argument('--data_l', help='Space-seperated list of data file (json)', required=True )
parser.add_argument('--o', help='Output file', default="paws.db" )
args = parser.parse_args()
print args

class bcolors:
    BLACK = '\033[40m'
    RED = '\033[41m'
    GREEN = '\033[42m'
    YELLOW = '\033[43m'
    BLUE = '\033[44m'
    MAGENTA = '\033[45m'
    CYAN = '\033[46m'
    WHITE = '\033[47m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    ERROR = '\033[41m'    ##  red 
   
#############################################################################################################
def printerr(l):
    print bcolors.ERROR + l + bcolors.ENDC


#############################################################################################################
def read_jsonfile(file_) :
    try:
        if os.path.exists(file_) :
            with open(file_, 'r') as json_data:
                d = json.load(json_data)
                return d
        return {}
    except:
        printerr('Exception reading : {}'.format(file_))
        sys.exit(0)



#############################################################################################################
def create_schema(c, conn, schema_file) :
    schema_ = read_jsonfile(schema_file)
    if schema_ != {} :
        for tbl,d_ in schema_.items() :
            try:
                # Creating the tableSQLite table with 1 column
               cmd = 'CREATE TABLE {} '.format(tbl)
               fields = ','.join(["{} {}".format(k,v) for k,v in d_.items()])
               cmd += '( ' + fields + ' )'
               c.execute(cmd)
               conn.commit()
            except:
                printerr('Exception creating schema for table:{}'.format(tbl))
                sys.exit(0)
        return schema_
    return {}

  



#############################################################################################################
def populate_db(c, conn, schema_, data_files) :

    data_file_list = data_files.strip().split(' ')

    print 'data_file_list = ',data_file_list

    for f in data_file_list:
        f = f.strip()
        if len(f) == 0:     ## handles multiple spaces betwene filenames
            continue
        data_ =  read_jsonfile(f)
        print 'Processing {}'.format(f)
        if data_ == {} :
            continue
        ## parse each table in the data files
        for tbl,d_ in data_.items():
            try:
                if tbl not in schema_.keys() :
                    printerr('Error: Table "{}" not in schema'.format(tbl))
                tbl_schema = schema_[tbl]
                tbl_schema_keys = tbl_schema.keys()   ## create an ordered list

                if not isinstance(d_, list):
                    d_ = [d_]
                   
                for d_item in d_:
                    cmd = 'INSERT INTO {} '.format(tbl)
                    fields = ','.join(["{}".format(k) for k in tbl_schema.keys() if k in d_item.keys()])
                    cmd += '( ' + fields + ' )'
                    cmd += ' VALUES ' 

                    fields = []
                    for k in tbl_schema_keys:
                        if k in d_item.keys() :
                            if tbl_schema[k].find('VARCHAR')>=0:
                                field = " '{}'".format(d_item[k])
                            elif tbl_schema[k].find('BOOLEAN')>=0:
                                field = ' 1' if d_item[k] else ' 0'
                            else:
                                field = ' {}'.format(d_item[k])
                            fields.append(field)
                    fields_str = ','.join(fields)
                    cmd += '( ' + fields_str + ' )'
                    c.execute(cmd)
                    conn.commit()
            except:
                printerr('Exception populating data for file:{} table:{}'.format(f, tbl))
                sys.exit(0)
 

#############################################################################################################
## main

conn = sqlite3.connect(args.o)
c = conn.cursor()

schema_ = create_schema(c, conn, args.schema)
if schema_ == {} :
    printerr('Problem setting up schema')
    sys.exit(0)

populate_db(c, conn, schema_, args.data_l)

conn.close()






