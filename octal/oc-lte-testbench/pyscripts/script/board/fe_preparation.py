
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools

class FrontEndPreparation(testrunner.TestSuite):

    @testrunner.testcase("Enable FE", critical=True)
    def FE_PRP030(self, context):
        lte_calibtools.enable_fe_all_on(context)

    @testrunner.testcase("Program RFPAL", critical=True)
    def FE_PRP0301(self, context):
        if context.FE_REV == 'E':
            lte_calibtools.rfpal_program_freq_range(context, band=context.RF_BAND)
        else:
            context.logger.info('RFPAL programming skipped, only supported by RevE +')
