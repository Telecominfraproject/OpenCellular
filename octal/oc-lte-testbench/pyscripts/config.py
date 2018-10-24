from nute.core import *

config = TestLoaderConfiguration(
    name= 'OC-LTE Testbench',
    import_root= r'..',
    product_index= r'\\ravel\Production\Outils\ConfigFiles\products.ini',
    product_sub_path= r'Test\TestLogs',
    fallback_log_destination= "logs",
    test_function_regex= r'(\w+)[_-](\w+?)([\dxX])(\d+)',
    enforce_test_fn_uniqueness= True,
    hook_webapp_service_server= False,
    import_yml= [
        r'C:\GIT_Files\nute_config\nute.yml',
        r'C:\GIT_Files\nute_config\oc_lte_family.yml',
        r'.config.yml']
)
