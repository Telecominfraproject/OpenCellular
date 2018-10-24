import os
import yaml

USER_CONFIG_PATH = os.path.expanduser(r'~\nute-config')

if not os.path.exists(USER_CONFIG_PATH):
    os.mkdir(USER_CONFIG_PATH)

def get_config_file_path(key):
    return os.path.join(USER_CONFIG_PATH, key + '.yml')

class UserConfig(object):
    def __init__(self, key):
        self.config_key = key

    def save(self):
        filepath = get_config_file_path(self.config_key)
        with open(filepath, mode='wb') as f:
            f.write(yaml.dump(self.__dict__))

    def load(self):
        filepath = get_config_file_path(self.config_key)
        if os.path.exists(filepath):
            with open(filepath, mode='r') as f:
                string = f.read()
                self.__dict__.update(yaml.load(string))
