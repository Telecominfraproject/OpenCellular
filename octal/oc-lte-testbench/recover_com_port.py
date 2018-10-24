from configuration_loader import ConfigurationLoader
from pprint import pprint
from tools.lte import lte_comport, lte_calibtools, lte_calib, lte_software
import logging
from opentest.server import serverinterface
import os
from nute import utils

SOFTWARE_VERSION = 'v2.1'
ROOT_FOLDER = 'pyscripts'
MAC_ADDR = '00-D0-CC-20-14-2E'

def create_logger():
    logger = logging.getLogger('recover')
    logger.setLevel(logging.DEBUG)
    stream_handler = logging.StreamHandler()
    stream_handler.setLevel(logging.DEBUG)
    logger.addHandler(stream_handler)
    return logger


if __name__ == '__main__':
    config_loader = ConfigurationLoader(ROOT_FOLDER)
    config_loader.load_config('PRE-TEST', '$DEFAULT')

    context = config_loader.get_conf()
    context.logger = create_logger()
    context.server = serverinterface.LocalLaunchedServerInterface(port=5050)
    context.server.logger = context.logger

    # TFTP
    context.server.tftp = context.server.get_service_client_from_name('TFTPServiceClient', url='tftp')
    tftp_folder = os.path.abspath(os.path.join('versions', SOFTWARE_VERSION))
    lte_software.choose_env_files(context, tftp_folder)
    context.server.tftp.create_service(context.STATION_LOCALIP, tftp_folder)
    context.UPDATEFINAL_FILE_LOCAL_PATH = tftp_folder

    # This clears /mnt/app if True
    context.FLASH_CLEAR_APP = True

    # COM PORT
    from tools.lte.lte_comport import LTECOMServiceClient
    context.server.com = context.server.get_service_client_from_name('tools.lte.lte_comport.LTECOMServiceClient', url='com')
    context.server.com.create_service(context.UUT_COMPORT, baudrate=context.UUT_BAUDRATE) #this will return error if com port not accessible, do try except

    # SSH AND SCP
    context.server.ssh = context.server.get_service_client_from_name('tools.lte.lte_ssh_client.LTESSHServiceClient', url='ssh')
    context.server.scp = context.server.get_service_client_from_name('SCPServiceClient', url='scp')
    context.server.env_var = context.server.get_service_client_from_name('tools.lte.lte_ssh_client.EnvVarSSHServiceClient',
                                                                     url='ssh')

    with utils.stack_chdir(ROOT_FOLDER):
        context.BB_MAC_ADDR = MAC_ADDR
        from pyscripts.script.firstboot import Firstboot
        firstboot_testsuite = Firstboot()

        # Reset all env vars and update u-boot
        # could try to see if com port connection made, if yes then do uboot (open system), else do this via ssh & have to wait for boot
        firstboot_testsuite.CM_PRP040(context)
        firstboot_testsuite.CM_PRP041(context)

        # Wait for boot
        context.server.ssh.wait_for_initializing(context)
        lte_calibtools.launchscp(context)
        #

        from pyscripts.script.flash.flash_app import FlashApp
        flash_app_testsuite = FlashApp()
        flash_app_testsuite.CM_CLN002(context)

        # this does the e2eenv.txt
        env_file_path = context.UBOOT_FINALFILEENV
        context.server.env_var.setup_from_file(env_file_path)

        #context.server.ssh.execute_command('command goes here') #will return return of command
        #would need to kill tftp server to test boot by flash
        context.server.tftp.kill()  # Closes the TFTP server
        context.server.ssh.reboot()
        context.server.ssh.wait_for_initializing(context)