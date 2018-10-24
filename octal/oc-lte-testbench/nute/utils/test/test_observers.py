
import unittest
from nute.utils import *
import os

class TestDataTransferObject(unittest.TestCase):

    def setUp(self):
        self.dto = DataTransferObject()
        self.PATH_A_B = ['a', 'b']
        self.PATH_A = ['a']

    def test_set_inexistant_nest(self):
        SOME_DATA = 'hello'
        self.dto.update(path=self.PATH_A_B, data=SOME_DATA)

        self.assertEqual(self.dto.get_path(self.PATH_A_B), SOME_DATA)

    def test_fire_updates(self):
        SOME_DATA = 'hello'
        SOME_PATH = self.PATH_A_B
        called = ['False']
        def callback(event):
            self.assertEqual(event.path, SOME_PATH)
            self.assertEqual(event.content, SOME_DATA)
            called[0] = True

        self.dto.subscribe(callback)
        self.dto.update(path=SOME_PATH, data=SOME_DATA)
        self.assertTrue(called[0])

    def test_nested_dto(self):
        dto_inside = DataTransferObject()
        dto_inside['inside'] = 'Hello'
        self.dto.update(dto_inside, ['outside'])
        self.dto.update('Hello2', ['outside', 'inside'])
        self.assertEqual(self.dto['outside']['inside'], 'Hello2')

    def test_assign_empty_dict(self):
        self.dto['root_id'] = ''
        self.dto['tree'] = {}
