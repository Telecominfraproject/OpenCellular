from opentest.server import serverinterface


from nute import utils
context = utils.EmptyObject()
## Starting the server
context.server = serverinterface.LocalLaunchedServerInterface(port=5050)

OCLTE_IPADDR = "10.102.81.61"
## Creating a com port
# com_client = context.server.get_service_client_from_name('COMServiceClient', url='com')
# com_client.create_service('COM10', baudrate=115200)

context.server.ssh = context.server.get_service_client_from_name('SSHServiceClient', url='ssh')
context.server.scp = context.server.get_service_client_from_name('SCPServiceClient', url='scp')

context.server.ssh.create_service(OCLTE_IPADDR, 'root', 'cavium.lte')
context.server.scp.create_service(OCLTE_IPADDR, 'root', 'cavium.lte')

import logging
context.logger = logging

from tools.lte import lte_software

context.UPDATEFINAL_FILE_LOCAL_PATH = r'versions\v2.1'
lte_software.update_software(context)
## Loading an ini file


utils.load_ini_in_obj('test.ini', context)
assert context.SECTION_A == 'A'
assert context.SECTION_B == 'B'
section = context.get_as_dict('SECTION')
assert section['A'] == 'A'
assert section['B'] == 'B'
