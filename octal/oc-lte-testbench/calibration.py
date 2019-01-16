from configuration_loader import ConfigurationLoader
from pprint import pprint
from tools.lte import lte_comport, lte_calibtools, lte_calib, lte_software
import logging
from opentest.server import serverinterface
import os
from nute import utils

#SOFTWARE_VERSION = 'v2.1_mod_us'
SOFTWARE_VERSION = 'v2.1_mod_us'
ROOT_FOLDER = 'pyscripts'
SCRIPT_FOLDER = 'pyscripts/script'
#MAC_ADDR = '00-D0-CC-20-14-2E'
MAC_ADDR = '00-D0-CC-20-14-40'

def create_logger():
    logger = logging.getLogger('recover')
    logger.setLevel(logging.DEBUG)
    stream_handler = logging.StreamHandler()
    stream_handler.setLevel(logging.DEBUG)
    logger.addHandler(stream_handler)
    return logger


if __name__ == '__main__':
    config_loader = ConfigurationLoader(ROOT_FOLDER)

    # config_script_loader = ConfigurationLoader(SCRIPT_FOLDER)
    # config_script_loader.load_config('CALIBRATION', '$DEFAULT')

    context = config_loader.get_conf()

    context.path_loss = raw_input("What is the path loss?")
    print("Using a path loss of " + context.path_loss + "db")

    context.analyzer_name = "TCPIP0::K-N9030B-80108.local::hislip0::INSTR"
    se = 'n'
    while (se != 'y'):
        print("\nSelected analyzer " + context.analyzer_name)
        se = raw_input("Is this the correct analyzer?(y/n):")
        if (se == 'y') or (se == 'Y'):
            se = 'y'
            print("Using analyzer " + context.analyzer_name)
            break
        else:
            context.analyzer_name = raw_input("Please enter the correct analyzer name:")

    context.user_band = "0"
    while (context.user_band != "3") or (context.user_band != "5") or (context.user_band != "28"):
        context.user_band = raw_input("\nWhich band are you running?(3/5/28):")
        if (context.user_band == "3") or (context.user_band == "5") or (context.user_band == "28"):
            print ("Using Band " + context.user_band)
            break
        else:
            print context.user_band + " is not a supported band."

    config_loader.load_config('PRE-TEST', '$DEFAULT', context.user_band)

    # context = config_script_loader.get_conf()
    context.logger = create_logger()
    context.server = serverinterface.LocalLaunchedServerInterface(port=5050)
    context.server.logger = context.logger

    # TFTP
    # context.server.tftp = context.server.get_service_client_from_name('TFTPServiceClient', url='tftp')
    # tftp_folder = os.path.abspath(os.path.join('versions', SOFTWARE_VERSION))
    # lte_software.choose_env_files(context, tftp_folder)
    # context.server.tftp.create_service(context.STATION_LOCALIP, tftp_folder)
    # context.UPDATEFINAL_FILE_LOCAL_PATH = tftp_folder

    # This clears /mnt/app if True
    context.FLASH_CLEAR_APP = False

    # COM PORT
    from tools.lte.lte_comport import LTECOMServiceClient
    # context.server.com = context.server.get_service_client_from_name('tools.lte.lte_comport.LTECOMServiceClient', url='com')
    #commenting out to test the 'closed system' bringup
    #for tests can always try to start com port first to have access to boot logs just in case
    #first boot of baseband need to access uboot
    #context.server.com.create_service(context.UUT_COMPORT, baudrate=context.UUT_BAUDRATE) #this will return error if com port not accessible, do try except

    # SSH AND SCP
    context.server.ssh = context.server.get_service_client_from_name('tools.lte.lte_ssh_client.LTESSHServiceClient', url='ssh')
    context.server.scp = context.server.get_service_client_from_name('SCPServiceClient', url='scp')
    context.server.rfswitch = context.server.get_service_client_from_name('LossRFSwitchServiceClient', url='ssh')
    # context.server.rfswitch.create_service()
    context.server.env_var = context.server.get_service_client_from_name('tools.lte.lte_ssh_client.EnvVarSSHServiceClient',
                                                                      url='ssh')
    context.server.sensor = context.server.get_service_client_from_name('tools.lte.lte_ssh_client.SensorSSHServiceClient',
                                                                      url='ssh')


    context.server.spectrum = context.server.get_service_client_from_name('KeysightEXASpectrumClient', url='spectrum')
    # commenting out for now to test on analyzer later
    # hislip is working for borrowed
    #context.server.spectrum.create_service("TCPIP0::K-N9030B-80108.local::hislip0::INSTR")
    # NRG
    # context.server.spectrum.create_service("TCPIP0::K-N9030B-41982.local::inst0::INSTR")
    # system near door\
    # context.server.spectrum.create_service("TCPIP0::K-N9010B-10686.local::hislip0::INSTR")
    context.server.spectrum.create_service(context.analyzer_name)




    with utils.stack_chdir(ROOT_FOLDER):
        context.BB_MAC_ADDR = MAC_ADDR
        # context.LOGS_PATH = 'pyscripts'
        from pyscripts.script.firstboot import Firstboot
        firstboot_testsuite = Firstboot()

        # Reset all env vars and update u-boot
        # could try to see if com port connection made, if yes then do uboot (open system), else do this via ssh & have to wait for boot
        #firstboot_testsuite.CM_PRP040(context)
        #firstboot_testsuite.CM_PRP041(context)

        # Wait for boot
        # this wait for initializing was needed to be able to send echo commands during ocxo
        context.server.ssh.wait_for_initializing(context)
        lte_calibtools.launchscp(context)
        # #

        from pyscripts.script.system.calib_prep import PrepareCalibration
        calib_prep = PrepareCalibration()
        calib_prep.SY_PRP011(context)

        from pyscripts.script.system.system_rx import PrepareCalibration as SystemRxPrepareCalibration
        prepare_calibration = SystemRxPrepareCalibration()
        prepare_calibration.SY_PRP040(context)
        prepare_calibration.SY_TSC140(context)
        # prepare_calibration.SY_TSC240(context)

        # from pyscripts.script.board.ocxo import OCXOCalibration
        # ocxo_calibration = OCXOCalibration()
        # ocxo_calibration.BB_TSC020(context)
        #
        # from pyscripts.script.board.rfpal import RFPALCalibration
        # rfpal_calibration = RFPALCalibration()
        # rfpal_calibration.FE_TSC120(context)
        # # rfpal_calibration.FE_TSC220(context)

        from pyscripts.script.system.system_tx import SystemCalib10MHz
        system_calibration_10MHz = SystemCalib10MHz()
        #system_calibration_10MHz.SY_TSC110(context)
        #115 is for b5
        #system_calibration_10MHz.SY_TSC115(context)
        # 128 is for b28
        system_calibration_10MHz.SY_TSC128(context)
        # system_calibration_10MHz.SY_TSC210(context)
        system_calibration_10MHz.SY_TSCX11(context)

        from pyscripts.script.system.write_calib import WriteCalibration
        write_calibration = WriteCalibration()
        # FIXME trying to send the calibration file over doesn't work
        # write_calibration.SY_CLN040(context)
