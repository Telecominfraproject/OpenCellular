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
    config_loader.load_config('PRE-TEST', '$DEFAULT')



    context = config_loader.get_conf()
    context.logger = create_logger()
    context.server = serverinterface.LocalLaunchedServerInterface(port=5050)
    context.server.logger = context.logger






    # SSH AND SCP
    context.server.ssh = context.server.get_service_client_from_name('tools.lte.lte_ssh_client.LTESSHServiceClient', url='ssh')
    context.server.scp = context.server.get_service_client_from_name('SCPServiceClient', url='scp')
    context.server.rfswitch = context.server.get_service_client_from_name('LossRFSwitchServiceClient', url='ssh')
    context.server.env_var = context.server.get_service_client_from_name('tools.lte.lte_ssh_client.EnvVarSSHServiceClient',
                                                                      url='ssh')
    context.server.sensor = context.server.get_service_client_from_name('tools.lte.lte_ssh_client.SensorSSHServiceClient',
                                                                      url='ssh')


    context.server.spectrum = context.server.get_service_client_from_name('KeysightEXASpectrumClient', url='spectrum')
    context.server.spectrum.create_service("TCPIP0::K-N9010B-10686.local::hislip0::INSTR")

    with utils.stack_chdir(ROOT_FOLDER):
        context.BB_MAC_ADDR = MAC_ADDR

        from pyscripts.script.firstboot import Firstboot
        firstboot_testsuite = Firstboot()


        # Wait for boot
        context.server.ssh.wait_for_initializing(context)
        lte_calibtools.launchscp(context)

        from pyscripts.script.system.calib_prep import PrepareCalibration
        calib_prep = PrepareCalibration()
        calib_prep.SY_PRP011(context)

        from pyscripts.script.system.system_rx import PrepareCalibration as SystemRxPrepareCalibration
        prepare_calibration = SystemRxPrepareCalibration()
        prepare_calibration.SY_PRP040(context)
        prepare_calibration.SY_TSC140(context)
		#FIXME add ant2
        # prepare_calibration.SY_TSC240(context)

        from pyscripts.script.board.ocxo import OCXOCalibration
        ocxo_calibration = OCXOCalibration()
        ocxo_calibration.BB_TSC020(context)

        from pyscripts.script.board.rfpal import RFPALCalibration
        rfpal_calibration = RFPALCalibration()
        rfpal_calibration.FE_TSC120(context)
		#FIXME add ant2
        # rfpal_calibration.FE_TSC220(context)

        from pyscripts.script.system.system_tx import SystemCalib5MHz
        system_calibration_5MHz = SystemCalib5MHz()
        system_calibration_5MHz.SY_TSC120(context)
        # FIXME add ant2
        # system_calibration_5MHz.SY_TSC220(context)
        system_calibration_5MHz.SY_TSCX21(context)

        from pyscripts.script.system.system_tx import SystemCalib10MHz
        system_calibration_10MHz = SystemCalib10MHz()
        system_calibration_10MHz.SY_TSC110(context)
		#FIXME add ant2
        # system_calibration_10MHz.SY_TSC210(context)
        system_calibration_10MHz.SY_TSCX11(context)

        from pyscripts.script.system.system_tx import SystemCalib20MHz
        system_calibration_20MHz = SystemCalib20MHz()
        system_calibration_20MHz.SY_TSC130(context)
        # FIXME add ant2
        # system_calibration_20MHz.SY_TSC230(context)
        system_calibration_20MHz.SY_TSCX31(context)

        from pyscripts.script.system.write_calib import WriteCalibration
        write_calibration = WriteCalibration()
        write_calibration.SY_CLN040(context)
