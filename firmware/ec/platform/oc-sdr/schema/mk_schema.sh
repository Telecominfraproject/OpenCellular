#!/bin/sh -e
#
# Description
#   The purpose of this script is to automatically generate
#   the schema C-file when it is deemed necessary.
#
# Notes
#   1. To force an update, touch sys_schema.json
#   2. See usage notes in sdtester.py
#
# Usage
#   Error check
#     Exit if JSON schema source file does not exist
#   Run sdtester.py when (in order of precedence):
#     1. Auto schema C-file does not exist
#     2. JSON validation schema file does not exist
#     3. JSON schema source file has been updated
#   Else do nothing
#
VAL_FNAME="valid_schema.json"
INPUT_FNAME="sys_schema.json"
OUTPUT_FNAME="auto_schema.c"

gen_schema()
{
  python3 sdtester.py -j $INPUT_FNAME -c $OUTPUT_FNAME
}

if [ ! -e $INPUT_FNAME ]; then
   echo "ERROR: $INPUT_FNAME does not exist" 1>&2
   exit 1
fi

if [ ! -e $OUTPUT_FNAME ]; then
   echo "Generating $OUTPUT_FNAME because it does not exist"
   gen_schema
elif [ ! -e $VAL_FNAME ]; then
   echo "Generating $OUTPUT_FNAME because validation file does not exist"
   gen_schema
elif [ $INPUT_FNAME -nt $VAL_FNAME ] || [ $INPUT_FNAME -nt $OUTPUT_FNAME ]; then
   echo "Generating $OUTPUT_FNAME because $INPUT_FNAME has been updated"
   gen_schema
else
   echo "$OUTPUT_FNAME does not need an update"
fi
