#!/usr/bin/env python3
#
#  Copyright (c) 2018-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.

""""
Script to convert lcov generated coverage data to Jenkins readable Cobertura 
 XML coverage formatted data.
"""

import argparse
import glob
import os
import sys
from lcov_cobertura import LcovCobertura


def main(args):
    # Auto set arguments if none were provided

    # If no sourcefile was provided, find the test-coverage.info file.
    #  This assumes that the first file found is the desired one, so if multiple
    #  exist then the sourceFile must be specified on the command line.
    if not args.sourceFile:
        f = glob.glob('**/test-coverage.info', recursive=True)
        if f:
            sourceFile = f[0]
        else:
            sys.exit("No lcov output file found below current directory.")
    else:
        sourceFile = args.sourceFile

    # If no output file was provided, then place it in the same directory as
    #  the source file.
    if not args.outFile:
        outFile = os.path.dirname(sourceFile) + '/test-coverage.xml'
    else:
        outFile = args.outFile

    # Read all the data from the lcov output file
    with open(sourceFile) as fr:
        data = fr.read()

    # Create a converter and convert coverage data
    converter = LcovCobertura(data)
    res = converter.convert()

    # Write all output data to the destination file.
    with open(outFile, 'w') as fw:
        fw.write(res)

if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    # lcov data source file
    parser.add_argument(
        '-i',
        dest='sourceFile',
        action='store',
        default='',
        type=str,
        help='lcov data file to extract coverage information from. If not \
              provided, will recursively search from cwd for test-coverage.info\
              to use. If it finds multiple, will use the first one found',
    )

    # Output file name
    parser.add_argument(
        '-o',
        dest='outFile',
        action='store',
        default='',
        type=str,
        help='Name of file to write xml coverage data to. If not provided, will\
               default to test-coverage.xml in the same directory as sourceFile',
    )

    main(parser.parse_args())
