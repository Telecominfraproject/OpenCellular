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
VAL_FNAME=valid_schema.json
SCHEMAPATH=../../ec/platform/oc-sdr/schema
JSON_FNAME=sys_schema.json
C_FNAME=auto_schema.c
INPUT_FNAME=$SCHEMAPATH/$JSON_FNAME
OUTPUT_FNAME=$SCHEMAPATH/$C_FNAME

gen_schema()
{
  python3 sdtester.py -j $INPUT_FNAME -c $OUTPUT_FNAME
}

if [ ! -e $INPUT_FNAME ]; then
   echo "ERROR: $JSON_FNAME does not exist" 1>&2
   exit 1
fi

if [ ! -e $OUTPUT_FNAME ]; then
   echo "Generating $C_FNAME because it does not exist"
   gen_schema
elif [ ! -e $VAL_FNAME ]; then
   echo "Generating $C_FNAME because validation file does not exist"
   gen_schema
elif [ $INPUT_FNAME -nt $VAL_FNAME ] || [ $INPUT_FNAME -nt $OUTPUT_FNAME ]; then
   echo "Generating $C_FNAME because $JSON_FNAME has been updated"
   gen_schema
else
   echo "$C_FNAME does not need an update"
fi
