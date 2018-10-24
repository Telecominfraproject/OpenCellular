
import unittest
from nute import utils
import os

FOLDER = os.path.dirname(__file__)



class TestExpandRanges(unittest.TestCase):

    def test_expand_ranges_detects_string(self):
        string = "Hello"
        result = utils.expand_ranges(string)

        self.assertIn(string, result)

    def test_expand_ranges(self):
        TESTS = {
        "{0-3}": ['0', '1', '2'],
        "TEST{0-2}": ['TEST0', 'TEST1'],
        "TEST{8-10}": ['TEST8', 'TEST9'],
        "TEST{11-12}": ['TEST11'],
        "TEST{6,7,8}": ['TEST6', 'TEST7', 'TEST8'],
        "TEST{ONE,TWO}": ['TESTONE', 'TESTTWO'],
        "TEST{ONE,TWO}{5-7}": ['TESTONE5', 'TESTTWO5', 'TESTONE6', 'TESTTWO6'],
        "TEST{5-7}{ONE,TWO}": ['TEST5ONE', 'TEST5TWO', 'TEST6ONE', 'TEST6TWO'],
        "TEST{5}": ['TEST5']
        }
        for test, expected_result in TESTS.iteritems():
            result = utils.expand_ranges(test)
            self.assertEqual(set(result), set(expected_result), "expand_ranges('%s') result=%s, expected=%s" % (test, result, expected_result))

class TestLoadIniObject(unittest.TestCase):
    MERGE1_SECTIONS = ['SECTION1', 'SECTION4']

    MERGE1_SECTION1 = {
    'A' : 'SECTION1_A',
    'B' : 'SECTION1_B'
    }
    MERGE1_SECTION4 = {
    'Z' : 'SECTION4_Z'
    }

    MERGE2_SECTIONS = ['SECTION1', 'SECTION2', 'SECTION3']
    MERGE2_SECTION1 = {
    'A' : 'OVERWRITTEN',
    'C' : 'SECTION1_C'
    }
    MERGE2_SECTION2 = {
    'A' : 'SECTION2_A'
    }
    MERGE2_SECTION3 = {
    'A' : 'SECTION3_A'
    }


    def setUp(self):
        self.merge1 = utils.EmptyObject()
        self.merge2 = utils.EmptyObject()
        self.merge1_2 = utils.EmptyObject()
        self.merge1_2_ow = utils.EmptyObject()

        with utils.stack_chdir(FOLDER):
            utils.load_ini_in_obj('merge1.ini', self.merge1)
            utils.load_ini_in_obj('merge2.ini', self.merge2)

            utils.load_ini_in_obj('merge1.ini', self.merge1_2_ow)
            utils.load_ini_in_obj('merge2.ini', self.merge1_2_ow, overwrite_existing_attributes=True)

            utils.load_ini_in_obj('merge1.ini', self.merge1_2)
            utils.load_ini_in_obj('merge2.ini', self.merge1_2)

    def test_single_sections_array(self):
        self.assertEqual(TestLoadIniObject.MERGE1_SECTIONS, self.merge1.get_sections())
        self.assertEqual(TestLoadIniObject.MERGE2_SECTIONS, self.merge2.get_sections())

    def test_attributes_set(self):
        attributes = set(self.merge1_2.get_attributes())
        expected_attributes = {'SECTION1_A', 'SECTION1_B', 'SECTION1_C','SECTION2_A','SECTION3_A','SECTION4_Z'}
        self.assertEqual(attributes, expected_attributes)

    def test_merge_sections_array(self):
        for section in TestLoadIniObject.MERGE1_SECTIONS:
            self.assertIn(member=section, container=self.merge1_2.get_sections())
        for section in TestLoadIniObject.MERGE2_SECTIONS:
            self.assertIn(member=section, container=self.merge1_2.get_sections())

    def test_single_section_attributes(self):
        section_dict = self.merge1.get_as_dict('SECTION1')
        self.assert_dict_contains_second(section_dict, TestLoadIniObject.MERGE1_SECTION1)
        section_dict = self.merge1.get_as_dict('SECTION4')
        self.assert_dict_contains_second(section_dict, TestLoadIniObject.MERGE1_SECTION4)

    def test_merge_section_attributes(self):
        section_dict = self.merge1_2.get_as_dict('SECTION1')
        self.assert_dict_contains_second(section_dict, TestLoadIniObject.MERGE1_SECTION1)
        self.assert_dict_contains_second(section_dict, TestLoadIniObject.MERGE2_SECTION1)

    def test_overwrite_dict(self):
        overwritten = self.merge1_2_ow.get_as_dict('SECTION1')['A']
        not_overwritten = self.merge1_2.get_as_dict('SECTION1')['A']

        self.assertIs(overwritten, TestLoadIniObject.MERGE2_SECTION1['A'])
        self.assertIs(not_overwritten, TestLoadIniObject.MERGE1_SECTION1['A'])

    def test_no_section_duplicates(self):
        count_set = set()
        sections = self.merge1_2.get_sections()
        for section in sections:
            count_set.add(section)

        self.assertEqual(len(sections), len(count_set))

    def assert_dict_contains_second(self, di, second):
        for key,value in second.iteritems():
            self.assertIn(member=key, container=di)

    def test_single_section(self):
        obj = utils.EmptyObject()
        with utils.stack_chdir(FOLDER):
            utils.load_ini_in_obj('one_section.ini', obj)
        self.assertTrue(hasattr(obj, 'SECTION_ONE'))
