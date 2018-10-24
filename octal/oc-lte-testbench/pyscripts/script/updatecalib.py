import sys, os
import opentest
import opentest.script.testrunner as testrunner
from opentest.interface import filexfer
from tools.lte import lte_calibtools

class UpdateCalib(testrunner.TestSuite):

    @testrunner.testcase("Update Calib Final", critical=True)
    def CM_CLN004(self, context):
        context.logger.info("Updating calib file list...")
        configfile = os.path.join(context.TEST_PACKAGE_PATH, context.UPDATECALIB_FILE)
        if context.UPDATECALIB_FILE_LOCAL_LOGS_PATH:
            localprepath = os.path.join(context.LOGS_PATH, "")
        else:
            localprepath = os.path.join(context.UPDATECALIB_FILE_LOCAL_PATH, "")
        remoteprepath = os.path.join(context.UPDATECALIB_FILE_REMOTE_PATH, "")

        # !!! M.T. this will fake creating dummy calibration files (needs 7-zip to be installed in its default install path)
        if not os.path.isfile(localprepath + "calib\\" + "bbrxatten_b3_10_ant1.cal"):
            mytest='"C:\\Program Files\\7-Zip\\7z" e E:\\Projet_OC-LTE\\Other\\Init\\tftpboot\\cal.tar -r * -o'
            mytest=mytest + localprepath + "calib\\"
            os.system('"' + mytest + '"')

        filexfer.filexferwithfile(context, configfile, localprepath, remoteprepath)
        context.logger.info("Updating calib file list... Done")
