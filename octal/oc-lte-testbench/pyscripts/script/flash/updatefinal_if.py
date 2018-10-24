import sys, os
import opentest
import opentest.script.testrunner as testrunner
from opentest.interface import filexfer
from tools.lte import lte_calibtools

class UpdateFinal(testrunner.TestSuite):

    @testrunner.testcase("Update final env if was before test", critical=True)
    def CM_CLN003(self, context):

        env_file_path = os.path.join(context.TEST_PACKAGE_PATH, context.UBOOT_FINALFILEENV)
        tftp_file = os.path.join(context.TEST_PACKAGE_PATH, context.UBOOT_SETUPFILEENV)

        desired_final_state = context.UPDATEFINAL_ACTION_REQUIRED
        was_e2e_and_must_keep = (context.WAS_FINALENV and desired_final_state == 'keep')
        must_e2e = (desired_final_state == 'e2e')
        if was_e2e_and_must_keep or must_e2e:
            context.FLASH_REBOOT_VALIDATION = True
            context.logger.info("Setup final env in progress")
            tftp_file_path = os.path.join
            if context.FLASH_CLEAR_ENV:
                context.logger.info("Clearing all unessential env vars and setup final env")
                # This will clear all variables which are not in the specified files
                context.server.env_var.clear_all_and_setup_from_files(tftp_file, env_file_path)
            else:
                context.server.env_var.setup_from_file(env_file_path)
        else:
            if context.FLASH_CLEAR_ENV:
                context.logger.info("Clearing all unessential env vars and setup TFTP env")
                # This will clear all variables which are not in the specified file
                context.server.env_var.clear_all_and_setup_from_files(tftp_file)
            context.FLASH_REBOOT_VALIDATION = False

    @testrunner.testcase("Validate boot from flash")
    def SY_TSC050(self, context):
        if context.FLASH_REBOOT_VALIDATION:
            context.server.tftp.kill()#Closes the TFTP server
            context.server.ssh.reboot()
            context.server.ssh.wait_for_initializing(context)
        else:
            context.logger.info("No reboot required")
