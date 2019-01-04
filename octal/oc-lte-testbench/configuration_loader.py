import ConfigParser
import os
from nute import utils
import platform

INI_LOADER_KWARGS = {
'overwrite_existing_attributes': True
}

class ConfigurationLoader():
    def __init__(self, root_folder):
        self.root_folder = root_folder
        config = ConfigParser.SafeConfigParser()
        config.optionxform = str
        lookup_file = os.path.join(root_folder, '.lookup.ini')
        config.read(lookup_file)

        self.ini_content = config
        self.conf_obj = utils.EmptyObject()

    def get_conf(self):
        return self.conf_obj

    def load_config(self, test_type, product):
        section = self._get_test_type(test_type)
        product_entries = self._get_product_entries(section, product)
        product_folder_and_config = eval(product_entries).split(',')
        product_folder = product_folder_and_config[0]
        config_files = product_folder_and_config[1:]

        self.load_config_file(os.path.join(self.root_folder, '.test.ini'))
        base_path = os.path.join(self.root_folder, 'script')
        for additionnal_path in config_files:
            self.load_config_file(os.path.join(base_path, additionnal_path))
        self.load_config_file(os.path.join(self.root_folder, 'testbench', platform.node() + '.ini'), must_exist=False)
        self.load_config_file(os.path.join(base_path, product_folder, '.test.ini'), must_exist=False)
        # DSTest manually adding the below for calibration
        #self.load_config_file(os.path.join(base_path, '.calib_const.ini'))
        self.load_config_file(os.path.join(base_path, '.calib_const28.ini'))
        #self.load_config_file(os.path.join(base_path, '.rf_band3.ini'))
        #self.load_config_file(os.path.join(base_path, '.rf_band5.ini'))
        self.load_config_file(os.path.join(base_path, '.rf_band28.ini'))
        #self.load_config_file(os.path.join(base_path, 'system', '.test.ini'))
        self.load_config_file(os.path.join(base_path, 'system', '.test28.ini'))

    def _get_test_type(self, test_type):
        return self.ini_content.items(test_type)

    def _get_product_entries(self, section, product):
        for product_name, entries in section:
            if product_name == product:
                return entries

    def load_config_file(self, filepath, must_exist=True):
        if not os.path.exists(filepath):
            if must_exist:
                raise Exception('File does not exists: %s' % filepath)
            else:
                return
        utils.load_ini_in_obj(filepath, self.conf_obj, overwrite_existing_attributes=True)
