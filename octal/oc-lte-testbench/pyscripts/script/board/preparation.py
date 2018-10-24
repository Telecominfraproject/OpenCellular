import sys
import os
import opentest.script.testrunner as testrunner
from opentest.integration import rfmeasure

class Preparation(testrunner.TestSuite):

    @testrunner.testcase("Launch TFTP Service", critical=True)
    def CM_PRP011(self, context):
        context.server.tftp.create_service(context)

    #@testrunner.testcase("Launch DHCP Service", critical=True)
    def CM_PRP012(self, context):
        context.server.dhcp.create_service(context.STATION_LOCALIP)

    @testrunner.testcase("Launch COM Service", critical=True)
    def CM_PRP013(self, context):
        context.server.com.create_service(context.UUT_COMPORT, baudrate=context.UUT_BAUDRATE)

    @testrunner.testcase("Launch ComponentTester", critical=True)
    def CM_PRP014(self, context):
        context.server.component_tester = rfmeasure.ComponentTester(spec=context.server.spectrum, gen=context.server.generator)
        rfmeasure.create_generator(context)
        rfmeasure.create_spectrum(context)

    @testrunner.testcase("Launch RFSwitch Service", critical=True)
    def CM_PRP015(self, context):
        rfmeasure.create_rfswitch(context)
