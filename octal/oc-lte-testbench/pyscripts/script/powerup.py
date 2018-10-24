import os
import time
import sys
import opentest.script.testrunner as testrunner

class Powerup(testrunner.TestSuite):

    # @testrunner.testcase("Launch DCSupply Service", critical=True)
    def CM_PRP016(self, context):
        context.server.dcsupply.create_service()

    # @testrunner.testcase("Power-up", critical=True)
    def CM_PRP030(self, context):
        time.sleep(0.5)

        context.server.dcsupply.setvoltage(5.0)
        context.server.dcsupply.setenable(0)
        context.logger.info("Power Off")

        time.sleep(2)

        context.logger.info("Power On")
        context.server.dcsupply.setvoltage(context.BOARD_VOLTAGE)
        context.server.dcsupply.setenable(1)
        time.sleep(1)
