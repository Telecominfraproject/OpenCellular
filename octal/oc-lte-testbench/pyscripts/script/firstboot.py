# import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools
import os

class Firstboot():

    # @testrunner.testcase("Setup tftp boot in u-boot", critical=True)
    def CM_PRP040(self, context):
        lte_calibtools.stop_in_uboot(context)

        context.logger.info("U-boot setup default env in progress")
        setdefaultubootvar_result = bool(context.server.com.setdefaultubootvar(context.UBOOT_PROMPT, save=False))
        assert setdefaultubootvar_result, "Unable to setup default u-boot variables"

        env_file_path = os.path.join(context.UBOOT_FILEENVCLEANUP);
        setupubootwithfile_result = bool(context.server.com.setupubootwithfile(env_file_path, context.UBOOT_PROMPT, save=False))
        assert setupubootwithfile_result, "Unable to setup u-boot from clean file"
        context.logger.info("U-boot set setup env in progress")

        myenv = {}
        if hasattr(context, 'BB_MAC_ADDR'):
            myenv["ethaddr"] = context.BB_MAC_ADDR.replace('-',':')
            # mac_file = os.path.join(context.STATION_MACDBFOLDER, '%s.csv' % context.SERIAL_NO)
            # with open(mac_file, 'wb') as f:
            #     f.write(context.BB_MAC_ADDR.replace('-',':') + '\n')

        env_file_path = os.path.join(context.UBOOT_SETUPFILEENV);
        setupubootwithfile_result = bool(context.server.com.setupubootwithfile(env_file_path, context.UBOOT_PROMPT, save=False, additionnal_vars=myenv))
        assert setupubootwithfile_result, "Unable to setup u-boot env from tftp setup file"



        setuptftpboot_result = bool(context.server.com.setupuboot(myenv, context.UBOOT_PROMPT, save=True))
        assert setuptftpboot_result, "Unable to setup tftp boot in u-boot"
        context.logger.info("U-boot setup for tftp boot successful")

        lte_calibtools.reset_from_uboot(context)

    # @testrunner.testcase("Update u-boot from u-boot", critical=True)
    def CM_PRP041(self, context):
        lte_calibtools.stop_in_uboot(context)

        context.logger.info("U-boot update in progress")
        updateuboot_result = bool(context.server.com.updateuboot(context.UBOOT_UPDATEFILE, context.UBOOT_PROMPT, context.UBOOT_UPDATEDHCP))
        assert updateuboot_result, "Unable to update u-boot"
        context.logger.info("U-boot update successful")

        lte_calibtools.reset_from_uboot(context)

    # @testrunner.testcase("Validate 1000 MHz operation")
    def BB_TSC004(self, context):
        clock = context.server.com.extract_coreclock_from_boot()
        context.criteria.evaluate("BB_TSC004_CORE_CLOCK", clock)
