#!/usr/bin/env python3
#
#  Copyright (c) 2018-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

import re
import json
import jsonschema as js


class SchemaUtils:
    draftFiles = {'d4' : 'meta_sys_schema_d4.json',
    'd6' : 'meta_sys_schema_d6.json'}

    def __init__(self, md, json_fname, c_fname, draft):
        """
        Load meta schema and configuration schema
        """
        self.draft = draft
        self.valid = False
        self.json_fname = json_fname
        self.c_fname = c_fname
        self.sys_schema = ''
        self.md = md
        # Meta schema: this shouldn't change except when either a new JSON
        # draft requires it, or when the hardware is redesigned
        self.metaschema = json.load(open(SchemaUtils.draftFiles[draft], 'r'))

        if md != 'g':
            # JSON definition file exists and we want to produce the
            # corresponding C-file

            # Configuration schema: this can change any time device
            # configuration changes
            self.cfgschema = json.load(open(self.json_fname, 'r'))
        else:
            # Want to create JSON definition file from a C-file
            # containing a schema declared as a structure
            self.cfgschema = []

    def generate(self):
        new_schema = []
        with open(self.c_fname, 'r') as f:
            in_schema = False
            # Find start of schema declaration and extract
            for line in f:
                # find start of schema declaration
                if (not in_schema) and ('sys_schema' in line):
                    in_schema = True
                    # Skip first line
                    new_schema.append('[\n')
                    continue
                if in_schema:
                    new_schema.append(line)
            schema = ''.join(str(line) for line in new_schema)

        # Remove all C language features
        schema = SchemaUtils.stripC(schema)
        # Find terminations to array declarations
        schema = SchemaUtils.fixarray(schema)
        # JSON line formatting
        schema = SchemaUtils.parse(schema, 4)
        # JSON punctuation
        self.sys_schema = SchemaUtils.punct(schema)

    def auto_file(self):
        """
        Write C-file from validated schema
        """
        # Remove all JSON format features
        schema, decls = SchemaUtils.strip(self.sys_schema)
        # Format values as required for each data type
        schema = SchemaUtils.typeformat(schema)
        # Write C-file
        SchemaUtils.writeSchema(decls, schema, self.c_fname)

    def dump(self):
        """
        Save the validated schema
        """
        if self.md != 'g':
            # convert the data dictionary to a string
            if self.valid:
                self.sys_schema = json.dumps(self.cfgschema, indent=4)
            #
            v_fname = 'valid_schema.json'
            msg = 'Saved validated schema to ' + v_fname
        else:
            v_fname = self.json_fname
            msg = 'Saved auto generated schema to ' + v_fname

        with open(v_fname, 'w') as f:
            f.write(self.sys_schema)
        f.close()
        print(msg)

    def validate(self):
        """
        Check for errors in either instance or schema
        """
        # Sample errors:
        #  self.metaschema['definitions']['Subcomponent']['required'] = []
        #  self.cfgschema[0]['name'] = []
        try:
            try:
                if self.draft == 'd4':
                    js.Draft4Validator.check_schema(self.metaschema)
                else:
                    # not supported until jsonschema 3.0
                    js.Draft6Validator.check_schema(self.metaschema)
                print('Meta-schema OK')
            except js.SchemaError as e:
                print('Error in meta-schema:' + str(e))
            except Exception():
                raise

            try:
                js.validate(self.cfgschema, self.metaschema)
                print('Configuration schema validated')
            except js.ValidationError as e:
                print('Error in schema configuration:' + str(e))
            except Exception():
                raise
        except Exception() as e:
            print(e)
            self.valid = False
        else:
            self.valid = True
        return self.valid

    @staticmethod
    def declares(text):
        """
        Select fields for which either a declaration or a prototype is necessary
        """
        setA = set()
        setC = set()
        setP = set()
        for line in text.split('\n'):
            words = line.split()
            if 'driver_cfg' in line:
                setA.add(words[-1])
            if 'factory_config' in line:
                setC.add(words[-1])
            if 'null' not in line.lower():
                if 'preInitFxn' in line:
                    setP.add(words[-1])
                if 'postInitFxn' in line:
                    setP.add(words[-1])
                if 'cb_cmd' in line:
                    setP.add(words[-1])

        qual1 = 'SCHEMA_IMPORT DriverStruct '
        lines1 = [qual1 + value + ';' for value in iter(setA)]
        lines1.sort()
        str1 = '\n'.join(str(line) for line in lines1)

        qual2 = 'SCHEMA_IMPORT const DriverStruct '
        lines2 = [qual2 + value + ';' for value in iter(setC)]
        lines2.sort()
        str2 = '\n'.join(str(line) for line in lines2)

        qual3 = 'SCHEMA_IMPORT bool '
        qual4 = '(void *, void *)'
        lines3 = [qual3 + value + qual4 + ';' for value in iter(setP)]
        lines3.sort(key=str.lower)
        str3 = '\n'.join(str(line) for line in lines3)

        decls = str1 + '\n\n' + str2 + '\n\n' + str3 + '\n\n'
        return decls

    @staticmethod
    def strip(text):
        """
        Remove JSON format features
        """
        # Find and remove all double quotes
        p = re.compile(r'\"')
        result = p.sub('', text)

        # Find and replace all colons
        result = re.sub(r':', ' =', result)

        # Find and replace all commas
        result = re.sub(r',', '', result)

        # Find values for which a declaration is required
        decls = SchemaUtils.declares(result)

        # Format structure elements
        result = re.sub(r'name', '.name', result)
        result = re.sub(r'components', '.components', result)
        result = re.sub(r'\bdriver\b', '.driver', result)
        result = re.sub(r'\bdriver_cfg\b', '.driver_cfg', result)
        result = re.sub(r'factory_config', '.factory_config', result)
        result = re.sub(r'commands', '.commands', result)
        result = re.sub(r'ssHookSet', '.ssHookSet', result)
        result = re.sub(r'postDisabled', '.postDisabled', result)
        result = re.sub(r'cb_cmd', '.cb_cmd', result)
        result = re.sub(r'preInitFxn', '.preInitFxn', result)
        result = re.sub(r'postInitFxn', '.postInitFxn', result)
        return result, decls

    @staticmethod
    def typeformat(text):
        """
        Format each element type as required
        """
        newschema = []
        for line in text.split('\n'):
            words = line.split()
            if '.name' in line:
                newline = line.replace(words[-1], '"' + words[-1] + '"')
            elif '.driver' in line:
                newline = line.replace(words[-1], '&' + words[-1])
            elif '.factory_config' in line:
                newline = line.replace(words[-1], '&' + words[-1])
            elif '.components' in line:
                newline = line.replace('[', '(Component[]) {')
            elif '.commands' in line:
                newline = line.replace('[', '(Command[]) {')
            elif '.ssHookSet' in line:
                newline = line.replace('{', '&(SSHookSet) {')
            elif '.preInitFxn' in line and 'NULL' not in line:
                newline = line.replace(words[-1], '(ssHook_Cb)' + words[-1])
            elif '.postInitFxn' in line and 'NULL' not in line:
                newline = line.replace(words[-1], '(ssHook_Cb)' + words[-1])
            elif ']' in line:
                n = line.find(']') + 4
                newschema.append(' ' * n + '{}')
                newline = line.replace(']', '}')
            else:
                newline = line

            if '{' in newline:
                newschema.append(newline)
            else:
                newschema.append(newline + ',')

        # Set first line correctly
        newschema[0] = 'const Component sys_schema[] = {'
        # Set last line correctly
        newschema[-1] = '};'

        return '\n'.join(str(line) for line in newschema)

    @staticmethod
    def writeSchema(decls, schema, c_fname):
        """
        Save schema to .c file
        """
        f = open(c_fname, 'w')
        cpw = []
        cpw.append('/**')
        cpw.append('* Copyright (c) 2018-present, Facebook, Inc.')
        cpw.append('* All rights reserved.')
        cpw.append('*')
        cpw.append('* This source code is licensed under the BSD-style license')
        cpw.append('* found in the LICENSE file in the root directory of this')
        cpw.append('* source tree. An additional grant of patent rights can be')
        cpw.append('* found in the PATENTS file in the same directory.')
        cpw.append('*')
        cpw.append('* WARNING: Do not modify this file by hand.  It is auto')
        cpw.append('*          generated from the json schema definition.')
        cpw.append('*          Refer to sdtester.py')
        cpw.append('*/\n')
        cpw.append('#include "auto_schema.h"\n\n')
        f.write('\n'.join(line for line in cpw))
        f.write(decls)
        f.write('\n')
        f.write(schema)
        f.write('\n')
        f.close()
        print('Saved schema to ' + c_fname)

    @staticmethod
    def stripC(text):
        """
        Using regular expressions, strip schema struct from
        C-languange features
        """
        # Find and remove C-style comments, single line and multiline
        p = re.compile(r'/\*([^*]|[\r\n]|(\*+([^*/]|[\r\n])))*\*+/')
        result = p.sub('', text)
        # Find and remove all dots
        result = re.sub(r'\.', '', result)
        # Find and remove all double quotes
        result = re.sub(r'\"', '', result)
        # Find and remove all commas
        result = re.sub(r',', '', result)
        # Find and replace all =
        result = re.sub(r'=', ' : ', result)
        # Find and remove all ampersands
        result = re.sub(r'&', '', result)
        # Find and remove all {}
        result = re.sub(r'{}', '', result)
        # Find and remove all semicolons
        result = re.sub(r';', '', result)
        # Find and remove all empty lines
        result = re.sub(r'\n\s*\n', '\n', result)
        # Find and remove all array declarations
        p = re.compile(r'\(\w*\[\]\)\s?\{')
        result = p.sub('[', result)
        # Find and remove all type casts
        p = re.compile(r'\(\w*\)')
        result = p.sub('', result)
        #
        return result

    @staticmethod
    def fixarray(text):
        """
        Use a stack to find where array declarations should end
        """
        opers = []
        schema = []
        for line in text.split('\n'):
            modline = line
            if '{' in line:
                opers.append('{')
            if '[' in line:
                opers.append('[')
            if '}' in line:
                if opers.pop() == '[':
                    modline = line.replace('}', ']')
            schema.append(modline)

        return '\n'.join(str(line) for line in schema)

    @staticmethod
    def parse(text, ident):
        """
        Apply indentation levels by parsing through schema
        """
        tab = ident * ' '
        depth = 0
        schema = []
        for line in text.split('\n'):
            indent, depth = SchemaUtils.stack(line, depth)
            modline = SchemaUtils.fields(line)
            newline = indent * tab + modline
            schema.append(newline)

        return '\n'.join(str(line) for line in schema)

    @staticmethod
    def punct(text):
        """
        JSON punctuation
        """
        schema = []
        lines = text.split('\n')
        n = len(lines) - 1
        for i in range(n):
            # No comma in starting bracket or brace
            if ('{' in lines[i]) or ('[' in lines[i]):
                modline = lines[i]
            # Comma in objects, except last in a group
            elif (':' in lines[i]) and ('}' not in lines[i + 1]):
                modline = lines[i] + ','
            # No comma in a last array member
            elif ('}' in lines[i]) and \
                  (('}' in lines[i + 1]) or (']' in lines[i + 1])):
                modline = lines[i]
            # Comma after object member of an array
            # Comma after array member of an object
            elif (('}' in lines[i]) or (']' in lines[i])) and \
                   (('{' in lines[i + 1]) or ('[' in lines[i + 1]) or
                    (':' in lines[i + 1])):
                modline = lines[i] + ','
            else:
                modline = lines[i]
            schema.append(modline)
        schema.append(lines[-1])
        return '\n'.join(str(line) for line in schema)

    @staticmethod
    def stack(line, depth):
        """
        Compute indentation level
        """
        if ('{' in line) or ('[' in line):
            indent = depth
            depth += 1
        elif ('}' in line) or (']' in line):
            depth -= 1
            indent = depth
        else:
            depth = depth
            indent = depth
        return indent, depth

    @staticmethod
    def fields(line):
        """
        Apply JSON formatting to the keys and values
        """
        modline = ''
        for word in line.split():
            modline = modline + SchemaUtils.compose(word)
        return modline

    @staticmethod
    def compose(word):
        """
        Correct use of quotes and spaces
        """
        if SchemaUtils.wordOK(word):
            newword = '"' + word + '"'
        elif word == ':':
            newword = ' : '
        else:
            newword = word
        return newword

    @staticmethod
    def wordOK(word):
        """
        Check for non-words
        """
        return word != '{' and word != '}' and \
               word != '[' and word != ']' and word != ':'
