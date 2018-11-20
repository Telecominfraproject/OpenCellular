#!/usr/bin/env python3
#
#  Copyright (c) 2018-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.

"""

  Structured Data Tester

  This module runs the schema utilities.  Two use cases are supported:
  1. Given a JSON schema declaration, validate and generate the C-files
  2. Given a C-file with a schema declaration, generate the JSON schema

  Example 1:
      Check the JSON schema declaration and generate the C-file
      required by the firmware and middleware
      $ python3 sdtester.py -j ../../ec/platform/oc-sdr/schema/sys_schema.json -c ../../ec/platform/oc-sdr/schema/auto_schema.c -d d4

  Example 2:
      Same as previous example but with all defaults
      $ python3 sdtester.py

  Example 3:
      Generate the JSON schema from a given C-file
      $ python3 sdtester.py -g -c ../../ec/platform/oc-sdr/schema/schema.c -j ../../ec/platform/oc-sdr/schema/sys_schema.json

  Example 4:
      Same as previous example but with all defaults
      $ python3 sdtester.py -g

  Example 5:
      Check that any arbitrary JSON schema declaration generates a C-file.
      Use d0 validation for development purposes
      $ python3 sdtester.py -j test_schema.json -c test_schema.c -d d0

"""

import sys
import os
import schemautils as sc
import argparse


def useCases(md, json_fname, c_fname, draft):
    # Error check
    if not os.path.isfile(json_fname) and md != 'g':
        print('Missing source JSON file\n')
        sys.exit()
    elif not os.path.isfile(c_fname) and md == 'g':
        print('Missing source C-file\n')
        sys.exit()

    if md != 'g':
        schema = sc.SchemaUtils('', json_fname, c_fname, draft)
        # Validate schema
        if not schema.validate():
            sys.exit()
        # Write the valid schema to disk
        schema.dump()
        # Generate auto_schema.c
        schema.auto_file()
    else:
        schema = sc.SchemaUtils('g', json_fname, c_fname, draft)
        schema.generate()
        schema.dump()


def main(args):
    if args.generate:
        useCases('g', args.json_fname, args.c_fname, args.draft)
    else:
        useCases('', args.json_fname, args.c_fname, args.draft)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    # JSON Source file
    parser.add_argument(
        '-j',
        dest='json_fname',
        action='store',
        default='../../ec/platform/oc-sdr/schema/sys_schema.json',
        type=str,
        help='JSON file where configuration schema is declared',
    )
    # C-file name
    parser.add_argument(
        '-c',
        dest='c_fname',
        action='store',
        default='../../ec/platform/oc-sdr/schema/auto_schema.c',
        type=str,
        help='Name of C-file to be read from or written to',
    )
    # Draft version
    parser.add_argument(
        '-d',
        dest='draft',
        action='store',
        default='d4',
        type=str,
        help='JSON draft version, e.g. \'d4\' or \'d6\' for build; \'d0\' for general purpose',
    )
    # Schema source C-file to convert into JSON schema
    parser.add_argument(
        '-g',
        dest='generate',
        action='store_true',
        help='If specified, generates JSON schema from C-file',
    )

    main(parser.parse_args())
