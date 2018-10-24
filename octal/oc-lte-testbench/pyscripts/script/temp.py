import os
import time
import sys
import opentest.script.testrunner as testrunner

class temp(testrunner.TestSuite):

    @testrunner.testcase("EEPROM test", critical=False)
    def TSC100(self, context):
        pass
        #context.logger.info("test")
        #result = context.server.ssh_eeprom.eeprom_test()
        #context.logger.info(result)<
    @testrunner.testcase("RMS ADC Script", critical=False)
    def rms_adc(self, context):

        result1,result2,result3,result4 = context.server.ssh_rms.read_all()
        result = False

        context.logger.info("Trx0 Forward Voltage: {}".format(result1))
        context.logger.info("Trx0 Reverse Voltage: {}".format(result2))
        context.logger.info("Trx1 Forward Voltage: {}".format(result3))
        context.logger.info("Trx1 Reverse Voltage: {}".format(result4))

        if result1 != "" and result2 != "" and result3 != "" and result4 != "":
            result = True # test failed
        assert result, "At least one ADC is missing"
