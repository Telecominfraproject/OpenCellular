
import unittest
from nute.utils import observers, flask_utils
import os

class TestFlaskSocketioStream(unittest.TestCase):

    def test_simple(self):
        called = ['False']
        UPDATE_PATH = ['test', 'test2']
        UPDATE_CONTENT = 6

        ROOT_PATH = []
        ROOT_NAME = '__root'
        ROOT_CONTENT = {'wtf': 'is_this_shit'}
        super_self = self
        class fake_socketio():
            def emit(self, *args, **kwargs):
                super_self.assertTrue(args[0] == 'update')
                root = args[1]['path'] == []
                update = args[1]['path'] == UPDATE_PATH
                super_self.assertTrue(root or update)
                if root:
                    super_self.assertEqual(args[1]['content'], ROOT_CONTENT)
                if update:
                    super_self.assertEqual(args[1]['content'], UPDATE_CONTENT)
                called[0] = True

        dto = observers.DataTransferObject()

        dto._get_dict()['test'] = {'test2':4, 'test1': {'test3': 6}}

        flask_utils.socketio_subscribe_dto_stream(fake_socketio(), dto, event_name='update', room_name='')

        dto.update(path=UPDATE_PATH, data=UPDATE_CONTENT)
        dto.update(path=ROOT_PATH, data=ROOT_CONTENT)

        self.assertTrue(called[0])
