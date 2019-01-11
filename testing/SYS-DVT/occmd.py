import os
import sys
import json

DB_FILE = "data.json"
db = '';

def parse_it(args):
    for 
    return 0

def load_database():
    global db
    with open(DB_FILE) as json_data:
        db = json.load(json_data)
    return 0

def main():
    load_database()
    print db
    parse_it(sys.argv)
    sys.exit(0)


if __name__ == '__main__': main()
