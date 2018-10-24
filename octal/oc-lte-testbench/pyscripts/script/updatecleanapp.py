import sys, os
import opentest
import opentest.script.testrunner as testrunner
from opentest.interface import filexfer
from tools.lte import lte_calibtools

class UpdateCleanApp(testrunner.TestSuite):

    @testrunner.testcase("Update Clean App mount", critical=True)
    def CM_CLN001(self, context):
        context.logger.info("Updating and cleaning app mount...")
        configfile = os.path.join(context.TEST_PACKAGE_PATH, context.UPDATECLEANAPP_FILE)
        if context.UPDATECLEANAPP_FILE_LOCAL_TEST_PATH:
            localprepath = os.path.join(context.TEST_PACKAGE_PATH, "")
        else:
            localprepath = os.path.join(context.UPDATECLEANAPP_FILE_LOCAL_PATH, "")
        remoteprepath = os.path.join(context.UPDATECLEANAPP_FILE_REMOTE_PATH, "")
        filexfer.filexferwithfile(context, configfile, localprepath, remoteprepath)
    	context.server.com.com_send(msg="reboot\n")
        # makes sure we log in again after the reboot
        lte_calibtools.login_from_com(context)
        lte_calibtools.launchssh(context)
        lte_calibtools.launchscp(context)
        context.logger.info("Updating and cleaning app mount... Done")
