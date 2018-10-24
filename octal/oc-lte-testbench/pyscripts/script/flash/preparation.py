import sys
import os
import opentest.script.testrunner as testrunner
from opentest.integration import rfmeasure

class Preparation(testrunner.TestSuite):

    @testrunner.testcase("Launch TFTP Service", critical=True)
    def CM_PRP011(self, context):
        context.server.tftp.create_service(context)
