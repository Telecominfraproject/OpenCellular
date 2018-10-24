import cStringIO, codecs, sys
import errno
import os
import shutil
import ConfigParser
import importlib
import logging
import inspect
import subprocess
import datetime
import tempfile
import collections
import hashlib

from StringIO import StringIO
from safe_eval import safe_eval




##region Path operations

import nute
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(nute.__file__)))

def nute_root():
    # head, tail = os.path.split()
    # while tail.strip() != 'nute' and head:
    #     head, tail = os.path.split(head)
    return ROOT

def is_sub_path(sub, sup):
    return sub.startswith(os.path.abspath(sup) + os.sep)

def get_path_relative_to(path, relative):
    norm_path = os.path.normcase(path)
    norm_relative = os.path.normcase(relative)
    assert is_sub_path(norm_path, norm_relative), "%s is not a subpath of %s" % (path, relative)

    # This goes one file after the other and only compares the head
    # of the path with the normalized OS case
    # This makes it so that case is conserved from path argument.
    temp_path, temp_ext = os.path.split(path)
    norm_path = os.path.normcase(temp_path)
    result_path = temp_ext
    while(is_sub_path(norm_path, norm_relative)):
        temp_path, temp_ext = os.path.split(temp_path)
        norm_path = os.path.normcase(temp_path)
        result_path = os.path.join(temp_ext, result_path)
    result_path = os.path.join('.', result_path)
    assert result_path.startswith('.')
    return result_path

def path_to_array(path):
    folders = []
    while 1:
        path, folder = os.path.split(path)
        if folder == '.':
            pass
        elif folder != "":
            folders.append(folder)
        else:
            if path != "":
                folders.append(path)
            break

    folders.reverse()
    return folders

##endregion

##region Class reflection

class ClassFinder(object):
    """
    Used primarily to search for a certain class name.
    Files are added through:
    import_path(path) : for all python files in directory (recursive=True will scan all sub folders also)
    import_file(file) : for a single python file
    import_module(mod): for a single python module

    The class if found with:
    find_first_subclass_with_name(root_cls, name): Returns the first class which is a child of root_cls with the specified name
    """
    def __init__(self, logger=None, ignore_import_errors=False):
        self.modules = []
        self.ignore_import_errors = ignore_import_errors
        if logger is not None:
            self.logger = logger
        else:
            self.logger = logging.getLogger('classfinder')
        self.import_errors = []

    def import_path(self, path, import_root=nute_root(), recursive=False, ignore_files= []):
        if not import_root:
            import_root = path
        import_root = os.path.abspath(import_root)
        for py_file in get_python_files_in_dir(path, recursive=recursive):
            mod_name,file_ext = os.path.splitext(os.path.split(py_file)[-1])
            if mod_name not in ignore_files:
                self.import_file(os.path.join(path, py_file), import_root=import_root)

    def set_catch_import_errors_module_name(self, name):
        self._current_module_name = name

    def catch_import_errors(function):
        def _catch_import_errors(self, *args, **kwargs):
            try:
                return function(self, *args, **kwargs)
            except ImportError as e:
                #raise
                if self.ignore_import_errors:
                    file_path = str(args[0])
                    self.import_errors.append('%s: %s' % (file_path, str(e)))
                else:
                    raise
        return _catch_import_errors

    @catch_import_errors
    def import_file(self, file_path, import_root=nute_root()):
        module = import_file_path_from_root(file_path, import_root)
        self.logger.debug("Added module %s" % module)
        self.add_module(module)

    @catch_import_errors
    def import_module(self, module_name):
        self.add_module(importlib.import_module(module_name))

    def add_module(self, module):
        self.modules = self.modules + [module]

    def yield_subclasses(self, root_cls, yield_root=False):
        for claz in class_childs_generator_from_modules(root_cls=root_cls, modules=self.modules):
            if yield_root or not is_class_name_the_same(claz, root_cls):
                yield claz

    def find_first_subclass_with_name(self, root_cls, name):
        self.logger.debug("Looking for child of %s with name %s" % (root_cls, name))
        for claz in self.yield_subclasses(root_cls=root_cls, yield_root=True):
            if claz.__name__ ==  name:
                return claz
        else:
            return None

def class_child_generator_from_file_name(root_cls, file_name, import_root):
    """
    Returns a generator of all subclasses of root_cls in the specified file_name.
    import_root specifies the directory from where to import the file_name (see import_file_path_from_root)
    """
    module = import_file_path_from_root(file_name, import_root)
    return class_childs_generator_from_module(root_cls, module, order_classes=True, import_root=import_root)

def import_file_path_from_root(file_path, import_root):
    """
    Import the file_path from import_root. For example:
    some/path/to/script.py
    >>> module = import_file_path_from_root('some/path/to/script.py', 'some/path')
    module -> to.script
    >>> module = import_file_path_from_root('some/path/to/script.py', '.')
    module -> some.path.to.script
    """
    abs_file_path = os.path.abspath(file_path)
    abs_import_path = os.path.abspath(import_root)
    with stack_sys_path(abs_import_path):
        file_path = get_path_relative_to(abs_file_path, abs_import_path)
        path_array = path_to_array(file_path)
        file_name = path_array[-1]

        file_mod_name, file_ext = os.path.splitext(file_name)
        mod_name_array = path_array[:-1] + [file_mod_name]
        mod_name = '.'.join(mod_name_array)
        try:
            return importlib.import_module(mod_name)
        except Exception as e:
            raise_exception_with_message(e, 'Unable to import %s from %s' % (mod_name, import_root), reverse=True)

def class_childs_generator_from_modules(root_cls, modules):
    """
    Returns a generator of all subclasses of root_cls in the list of provided modules.
    """
    yielded_classes = set()
    for module in modules:
        for clz in class_childs_generator_from_module(root_cls, module, order_classes=False):
            if clz not in yielded_classes:
                yielded_classes.add(clz)
                yield clz

def class_childs_generator_from_module(root_cls, module, order_classes=False, import_root=nute_root()):
    """
    Returns a generator of all subclasses of root_cls in module.
    Classes maybe be ordered as they appear in the source file using order_classes = True.
    HOWEVER; this requires a correct import root relative to the module name.
    """
    yielded_classes = set()
    with stack_chdir(import_root):
        clsmembers = inspect.getmembers(module, inspect.isclass)
        if order_classes:
            clsmembers = order_classes_by_line_number(clsmembers)
        for name, clz in clsmembers:
            #Do not allow the root class
            if is_subclass_of(clz, root_cls) and clz not in yielded_classes and not is_class_name_the_same(clz, root_cls):
                yielded_classes.add(clz)
                yield clz

def order_classes_by_line_number(members):
    import inspect
    def linenumber_of_member(m):
        try:
            #print(m)
            return inspect.findsource(m[1])[1]
        except IOError:
            pass
        except AttributeError:
            return -1
    members.sort(key=linenumber_of_member)
    return members

def import_qualified_name(qual_name):
    split_name = qual_name.split('.')
    module_name = '.'.join(split_name[:-1])
    attribute_name = split_name[-1]
    module = importlib.import_module(module_name)
    return getattr(module, attribute_name)

def is_subclass_of(sub, clz):
    try:
        mro = sub.__mro__
        for mro_clz in mro:
            if is_class_name_the_same(mro_clz, clz):
                return True
        return False
    except:
        #No __mro__ probably -> no super class
        return False

def is_class_name_the_same(clz1, clz2):
    if not clz1 or not clz2:
        return False
    return clz1.__name__ == clz2.__name__

def get_python_files_in_dir(dir_path, recursive=False):
    py_files = []
    with stack_chdir(dir_path):
        dir_path = os.path.abspath(dir_path)
        for root, dirs, files in os.walk('.'):
            root = os.path.abspath(root)
            if not recursive and root != dir_path:
                continue

            for fil in files:
                if fil.endswith('.py'):
                    py_files = py_files + [os.path.join(root, fil)]
    return py_files


##endregion

##region Capture print

def capture_print(function):
    def print_to_string(*args, **kwargs):
        capture = False
        if 'capture_print' in kwargs:
            capture = kwargs.pop('capture_print')

        #override std.out  and call back autoTransmit with captureOutput false
        if(capture):
            bufferout = cStringIO.StringIO()
            codecinfo = codecs.lookup("utf8")
            wrapperout = codecs.StreamReaderWriter(bufferout,
                    codecinfo.streamreader, codecinfo.streamwriter)

            oldout = sys.stdout
            try:
                sys.stdout = bufferout
                function(*args, **kwargs)
            finally:
                sys.stdout = oldout
            return wrapperout.getvalue()
        else:
            return function(*args, **kwargs)
    return print_to_string

##endregion

##region Directory functions

def merge_dir(dir_temp, dir_dest):
    '''
    Merge dir_temp in dir_dest
    '''
    arrayfile=[]
    arraydir=[]
    for (dirpath, dirnames, filenames) in os.walk(dir_dest):
        for name in dirnames:
            arraydir.append(_path_name(dirpath, name, dir_dest))

        for name in filenames:
            arrayfile.append(_path_name(dirpath, name, dir_dest))

    for (dirpath, dirnames, filenames) in os.walk(dir_temp):
        for name in dirnames:
            dir = _path_name(dirpath, name, dir_temp)
            if dir not in arraydir:
                if not os.path.exists(dir_dest+dir):
                    os.mkdir(dir_dest+dir)

        for name in filenames:
            file = _path_name(dirpath, name, dir_temp)
            if file in arrayfile:
                log_combiner(dir_dest+file, dir_temp+file, dir_dest+file)
            else :
                shutil.copyfile(dir_temp+file, dir_dest+file)#Copy missing file

def _path_name(path, name, name_dir):
    substract_name = os.path.join(path, name)[len(name_dir):]#Will sub the original directory form the path
    return substract_name

def log_combiner(file_dest, *files):
    content = []

    for file_path in files:
        with open(file_path) as f:
            content = content + f.readlines()

    cleaned_content = []
    last_timestamped_line_index = 0
    for i in range(len(content)):
        if _log_combiner_is_timestamped_line(content[i]):
            cleaned_content.append(content[i])
            last_timestamped_line_index = len(cleaned_content) - 1
        else:
            cleaned_content[last_timestamped_line_index] = cleaned_content[last_timestamped_line_index] + content[i]

    with open(file_dest, 'w+') as f:
        #Sorted with 24 first chars, its the datetime
        f.writelines(sorted(cleaned_content, key=lambda x:x[:24]))


def _log_combiner_is_timestamped_line(string):
    #Starts with the year
    stripped_str = string.strip()
    try:
        int(stripped_str[0:4])
        return True
    except:
        return False

##endregion


##region Expand ranges standard

def _number_range_list_from_match(number_match):
    begin = int(number_match[1])
    end = int(number_match[2])
    return range(begin, end)

def _word_range_list_from_match(word_match):
    return word_match[1].split(',')

NUMBER_RANGE_MATCH = (r'({(\d+)-(\d+)})', _number_range_list_from_match) # Replaces number ranges {0-4} -> [0, 1, 2, 3]
WORD_LIST_MATCH = (r'({((?:\w+,?){0,})})', _word_range_list_from_match) # Replaces word lists {0,a,hello} -> [0, a, hello]

def expand_number_ranges(strings):
    MATCH_FUNCTIONS = [
    NUMBER_RANGE_MATCH,
    ]
    return recursive_regex_expansion(string, MATCH_FUNCTIONS)

def expand_ranges(string):
    MATCH_FUNCTIONS = [
    NUMBER_RANGE_MATCH,
    WORD_LIST_MATCH,
    ]
    return recursive_regex_expansion(string, MATCH_FUNCTIONS)

def recursive_regex_expansion(string, match_functions):
    results = [string]

    for regex, fn in match_functions:
        new_results = []
        for string in results:
            new_results = new_results + _expand_range_list(string, regex, fn)
        results = new_results

    return results

def _expand_range_list(string, regex, list_from_match_fn):
    results = []
    import re
    compiled_regex = re.compile(regex)
    match = compiled_regex.findall(string)

    if not match:
        return [string]
    else:
        match = match[0]
        for replacement in list_from_match_fn(match):
            replaced = compiled_regex.sub(str(replacement), string, count=1)
            results = results + _expand_range_list(replaced, regex, list_from_match_fn)

    return results

def load_ini_expand_range(file_path):
    config = ConfigParser.SafeConfigParser()
    config.optionxform = str
    config.read(file_path)
    loaded_params = collections.OrderedDict()
    for section in config.sections():
        for section_name in expand_ranges(section):
            for name, value in config.items(section):
                attr_name = "%s_%s" % (section_name, name)
                loaded_params[attr_name] = value
    return loaded_params

##endregion

##region logging

class capture_logger():
    def __init__(self, logger, *handlers):
        self.logger = logger
        self.handlers = handlers

    def __enter__(self):
        for handler in self.handlers:
            self.logger.addHandler(handler)

    def __exit__(self, *args):
        for handler in self.handlers:
            handler.close()
            self.logger.removeHandler(handler)

class LoggingFormat:
    LEVEL_NAME_MESSAGE = logging.Formatter('%(levelname)8s: %(name)s: %(message)s')

##endregion

##region Other
from pid_controller.pid import PID
def stabilize_output(get_value_fn, stable_diff, average_count=2, poll_rate=0.05, timeout=10, logger=None):
    import timeit, time
    pid = PID(p=1, d=0.5)
    pid.target = 0
    stable = False
    before = None
    start_time = timeit.default_timer()
    elapsed = 0
    afters = []
    values = []
    while elapsed < timeout:
        after = get_value_fn()
        values.append(after)
        if before is not None:
            diff = pid(after-before)
            if abs(diff) < stable_diff:
                afters.append(after)
            else:
                afters = []

        before = after
        time.sleep(poll_rate)
        elapsed = timeit.default_timer() - start_time
        if len(afters) >= average_count:
            break
    if logger:
        logger.debug('Stabilized %s[ <%.2f: avg %d: \/%.2fs]. Took %.2fs(timeout=%.0fs) Values=%s' % (get_value_fn.__name__, stable_diff, average_count, poll_rate, elapsed, timeout, str(values)))

    if not afters:
        #If empty return last value
        return after
    else:
        return float(sum(afters)) / float(len(afters))


class stack_chdir():
    def __init__(self, new_chdir):
        self.new_chdir = new_chdir
        self.old_chdir = os.getcwd()

    def __enter__(self):
        os.chdir(self.new_chdir)

    def __exit__(self, *args):
        os.chdir(self.old_chdir)

class stack_sys_path():
    def __init__(self, path, pos=-1):
        self.path = path
        self.pos = pos

    def __enter__(self):
        sys.path.insert(self.pos, self.path)

    def __exit__(self, *args):
        sys.path.remove(self.path)

def check_output(args):
    cmd = ' '.join(args)
    process_file = os.popen(cmd)
    output = process_file.read()
    exit_status = process_file.close()
    assert exit_status != 0, 'Command %s returned non-zero exit code %d' % (cmd, exit_status)
    return output

def git_describe_current_dir():
    try:
        return check_output(["git", "describe", "--always"]).strip()
    except AssertionError:
        return '[Not a git repo]'

def git_is_clean_2():
    output = check_output(["git", "diff-index", "HEAD", "--"]).strip()
    return output.strip() == ""

def git_is_clean():
    return git_is_clean_2()

def git_is_clean_string(if_clean="CLEAN", if_not_clean="DIRTY"):
    clean = git_is_clean()
    return if_clean if clean else if_not_clean

import pickle
def save_obj(obj, file_name):
    with open(file_name, 'wb') as f:
        pickle.dump(obj, f, pickle.HIGHEST_PROTOCOL)

def load_obj(file_name):
    with open(file_name, 'rb') as f:
        return pickle.load(f)

def create_shortcut(*args, **kwargs):
    """
    Default create_shortcut
    """
    pass

if os.name == 'nt':
    try:
        sys.coinit_flags = 0 # multithreaded coinit
        import pythoncom
        import win32com.client # Reference PyWin32
        def create_shortcut(path, target='', wDir='', icon=''):
            _rest, ext = os.path.splitext(path)
            pythoncom.CoInitialize()
            if ext == 'url':
                shortcut = file(path, 'w')
                shortcut.write('[InternetShortcut]\n')
                shortcut.write('URL=%s' % target)
                shortcut.close()
            else:
                if ext != 'lnk':
                    path = os.path.join(_rest, 'lnk')
                print(path)
                shell = win32com.client.Dispatch('WScript.Shell')
                shortcut = shell.CreateShortCut(path)
                shortcut.Targetpath = target
                shortcut.WorkingDirectory = wDir
                if icon == '':
                    pass
                else:
                    shortcut.IconLocation = icon
                shortcut.save()
    except ImportError as e:
        import warnings
        warnings.warn('Shortcut imports failed: %s' % str(e), ImportWarning)


def chop_microseconds(delta):
    return delta - datetime.timedelta(microseconds=delta.microseconds)

##endregion

def gen_valid_identifier(seq):
    # get an iterator
    itr = iter(seq)
    # pull characters until we get a legal one for first in identifer
    for ch in itr:
        if ch == '_' or ch.isalpha():
            yield ch
            break
    # pull remaining characters and yield legal ones for identifier
    for ch in itr:
        if ch == '_' or ch.isalpha() or ch.isdigit():
            yield ch

def sanitize_identifier(name):
    return ''.join(gen_valid_identifier(name))

def raise_exception_with_message(e, message, raise_type=None, reverse=False):
    if raise_type is None:
        raise_type = type(e)

    message = str(message)
    e.message = str(e.message)
    if reverse:
        message = message + ': ' + e.message
    else:
        message = e.message + ': ' + message
    raise raise_type, raise_type(message), sys.exc_info()[2]


def clean_user_path(path):
    components = path.split(os.sep)
    home = os.path.expanduser('~')
    home_tail, home_head = os.path.split(home)
    for index, comp in enumerate(components):
        if '~' in comp:
            start = comp.split('~')[0]
            if home_head.startswith(start) and home_tail.lower() == os.sep.join(components[0:index]).lower():
                path = os.path.join(home, os.sep.join(components[index+1:]))
    return path

def get_temp_dir():
    tempdir = tempfile.mkdtemp()
    return clean_user_path(tempdir)

def as_dict(**kwargs):
    return kwargs

def print_dict_to_logger(_dict, logger, level=logging.DEBUG):
    for attr_name, value in _dict.items():
        logger.log(level, '%s= %s' % (attr_name, value))

class SysPathEntriesContext(object):
    def __init__(self, *entries):
        self.entries = entries

    def __enter__(self):
        sys.path = list(self.entries) + sys.path

    def __exit__(self, *args, **kwargs):
        for entry in self.entries:
            sys.path.remove(entry)

def temporary_sys_path_entries(*entries):
    return SysPathEntriesContext(*entries)

def md5_hash(string):
    return hashlib.md5(string).hexdigest()
