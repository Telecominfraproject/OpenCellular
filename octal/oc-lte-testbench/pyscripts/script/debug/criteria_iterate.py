from nute.core import *

FREQ_VALUES = [1970, 1980]
TX_VALUES = [0,1]

@iterate_testsuite_param('freq', FREQ_VALUES, print_format='Frequency= %s MHz')
@iterate_testsuite_param('other_freq', FREQ_VALUES)
@format_name_prefix('%d_', 'freq')
@format_name_suffix('_%d', 'other_freq')
@declare_configuration(ConfigurationDeclaration(past_one=True, tx_values=TX_VALUES))
class ExampleCriteriaIterator(TestSuite):
    criteria = CriteriaDeclaration(CRITERIA1= 'value == 17')

    @iterate_param('tx', 'context.tx_values')
    @iterate_param('tm', [17])
    @declare_criteria(criteria)
    @format_name('TSC0%d1%d', ('tx', 'tm'))
    @suite_critical
    @testcase("Hi")
    def TSC0x1(self, context, tx, tm):
        if context.past_one:
            context.past_one = False
            raise Exception('Hello')
        context.logger.info('MODIFIED')
        self.freq
        self.other_freq
        context.criteria.evaluate('CRITERIA1', tm)
