import sys
import os
import opentest.script.testrunner as testrunner
from opentest.integration import rfmeasure

class Preparation(testrunner.TestSuite):

    @testrunner.testcase("Launch DCSupply Service", critical=True)
    def CM_PRP016(self, context):
        try:
            context.server.dcsupply.create_service()
        except Exception as e:
            context.logger.warning("Unable to create DC Supply service: %s" % str(e))
