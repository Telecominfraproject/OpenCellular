import collections
import ConfigParser
from safe_eval import safe_eval
from all_utils import expand_ranges, expand_number_ranges

def file_exist_check(file_name):
    with open(file_name) as f:
        pass

class IniLoader(object):
    def __init__(self):
        self.conf_count = 0
        self.overwrite_count = 0

    def load_ini_in_obj(self, init_file_path, obj, file_must_exist=True, overwrite_existing_attributes=False, use_eval=True, additionnal_symbols={}):
        """
        Loads an init file into the specified object. Example:
        [SECTION1]
        ATTR1: "foo"
        ATTR2: ATTR1 + "50"

        [SECTION2]
        ATTR1: SECTION1_ATTR1 + "bar"

        Would set attributes in obj as the following python code:
        obj.SECTION1_ATTR1 = "foo"
        obj.SECTION1_ATTR2 = "foo" + "50"

        obj.SECTION2_ATTR1 = "foo" + "bar"

        Additionnally, the following methods will be added to the obj:

        section_array = obj.get_as_array('SECTION1')
        >>> [("ATTR1", "foo"), ("ATTR2", 50)]
        section_dict = obj.get_as_dict('SECTION1')
        >>> {"ATTR1": "foo", ATTR2: 50}
        sections = obj.get_sections()
        >>> ["SECTION1", "SECTION2"]
        """

        try:
            file_exist_check(init_file_path)
        except IOError:
            if file_must_exist:
                raise
            else:
                return None
        # Combine locals and globals

        config = ConfigParser.SafeConfigParser()
        config.optionxform = str
        config.read(init_file_path)
        section_names = []
        if not hasattr(obj, '__ALL_ATTRIBUTES__'):
            obj.__ALL_ATTRIBUTES__ = []

        for section in config.sections():
            for section_name in expand_ranges(section):
                section_dict = {}
                section_array = []
                section_names = section_names + [section_name]
                for name, value in config.items(section):
                    attr_name = "%s_%s" % (section_name, name)
                    if attr_name not in obj.__ALL_ATTRIBUTES__:
                        obj.__ALL_ATTRIBUTES__.append(attr_name)
                    #Use obj as globals, so previously declared variables can be used
                    #Use section_dict as locals
                    if use_eval:
                        temp_dict = {}
                        temp_dict.update(section_dict)
                        temp_dict.update(obj.__dict__)
                        temp_dict.update(additionnal_symbols)

                        evalued_value = safe_eval(value, temp_dict, file_name=init_file_path)
                    else:
                        evalued_value = value

                    if not hasattr(obj, attr_name) or overwrite_existing_attributes:

                        if hasattr(obj, attr_name):
                            self.overwrite_count = self.overwrite_count + 1
                        self.conf_count = self.conf_count + 1

                        setattr(obj, attr_name, evalued_value)

                        section_dict[name] = evalued_value
                        section_array = section_array + [(name, evalued_value)]


                merge_array(obj, section_name + "__ARRAY__", section_array)
                merge_dict(obj, section_name + "__DICT__", section_dict)

        merge_array(obj, '__SECTIONS__', section_names)

        add_getters(obj)

        return self.conf_count

class EmptyObject(object):
    def __init__(self, fill_with_dict=None):
        if isinstance(fill_with_dict, collections.Mapping):
            self.__dict__.update(fill_with_dict)
        elif fill_with_dict is not None:
            raise Exception('EmptyObject can only be initialized with mappings.')


def load_ini_in_obj(*args, **kwargs):
    ini_loader = IniLoader()
    return ini_loader.load_ini_in_obj(*args, **kwargs)

def merge_dict(obj, dict_name, new_dict):
    if hasattr(obj, dict_name):
        temp_new_dict = new_dict
        new_dict = getattr(obj, dict_name).copy()
        new_dict.update(temp_new_dict)

    setattr(obj, dict_name, new_dict)

def merge_array(obj, array_name, new_array):
    if hasattr(obj, array_name):
        temp_array = getattr(obj, array_name) + new_array
        new_array = []
        #Remove duplicates
        for section in temp_array:
            if section not in new_array:
                new_array.append(section)

    setattr(obj, array_name, new_array)

def add_getters(obj):
    def _get_as_array(self, section_name):
        return getattr(self, section_name + "__ARRAY__", [])

    def _get_as_dict(self, section_name):
        return getattr(self, section_name + "__DICT__", {})

    def _get_sections(self):
        return self.__SECTIONS__

    def _get_attributes(self):
        return self.__ALL_ATTRIBUTES__

    obj.get_as_array = _get_as_array.__get__(obj)
    obj.get_as_dict = _get_as_dict.__get__(obj)
    obj.get_sections = _get_sections.__get__(obj)
    obj.get_attributes = _get_attributes.__get__(obj)
