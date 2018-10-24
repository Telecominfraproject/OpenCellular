import sys
import os
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools


class Preparation(testrunner.TestSuite):

    @testrunner.testcase("Hardware setup", critical=True)
    def SY_PRP060(self, context):
        context.wait_tester_feedback("Setup is as follow",image=os.path.join(context.IMAGE_FOLDER, r"e2e_setup.jpg"))
        context.wait_tester_feedback("Plug The ETHERNET cable and the 2 antennas in the OC-LTE\nDO NOT plug the power yet",image=os.path.join(context.IMAGE_FOLDER, r"device_oc_lte.png"))

    @testrunner.testcase("Launch TFTP Service", critical=True)
    def CM_PRP011(self, context):
        context.server.tftp.create_service(context)

    @testrunner.testcase("Initial setup", critical=True)
    def SY_PRP061(self, context):
        context.wait_tester_feedback("Launch the Virtual machine named OC-LTE-EPC",image=os.path.join(context.IMAGE_FOLDER, r"vm.png"))

    @testrunner.testcase("Validate connection PC/VM", critical=True)
    def SY_PRP062(self, context):
        context.server.e2e_ssh.launch_service(context)

    @testrunner.testcase("MME setup", critical=True)
    def SY_PRP063(self, context):
        context.server.e2e_ssh.mme_setup()
        context.wait_tester_feedback("Plug the power in the OC-LTE",image=os.path.join(context.IMAGE_FOLDER, r"device_oc_lte.png"))
