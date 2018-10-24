import sys, os
import opentest.script.testrunner as testrunner

from tools.lte import lte_scan

class UpdateEnv(testrunner.TestSuite):

    @testrunner.testcase("Update env", critical=True)
    def CM_CLN008(self, context):
        final_update = context.server.env_var.get_value("mode").lower() == "e2e"
        context.server.env_var.setup_from_file(context.UBOOT_SETUPFILEENV)
        if final_update:
            context.server.env_var.setup_from_file(context.UBOOT_FINALFILEENV)
        mac_addr = lte_scan.try_get_mac(context.ORIGINAL_UNIT_SCAN)
        if mac_addr is not None:
            context.wait_tester_feedback("Write MAC address ? %s" % mac_addr, fb_type= testrunner.FeedbackType.PASS_FAIL)
            context.server.env_var.set_value('ethaddr', mac_addr)
