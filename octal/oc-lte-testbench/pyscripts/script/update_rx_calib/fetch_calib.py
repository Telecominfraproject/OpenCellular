import sys, os
import opentest
from nute.core import *
from opentest.script import utils
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure

from nute.core import processors, testresults
from nute.uut import logpaths
from nute.repositories import product

import time

class FetchCalib(TestSuite):

    @critical
    @testcase("Find up-to-date calib files")
    def SY_TSC260(self, context):
        replaced_product = context.PRODUCT_NAME.replace('603', '503')
        try:
            fetch_calib(context, replaced_product)
        except Exception as e:
            context.logger.debug('Error with no R1: ' + str(e))
            fetch_calib(context, replaced_product + 'R1')

def fetch_calib(context, product_name):
    replaced_product = product_name
    context.logger.debug('Product: %s replaced by %s' % (context.PRODUCT_CODE, replaced_product))
    current_test = context.TEST_
    target_product = product.find_product(current_test.config, replaced_product, logger=context.logger)
    base_logs_path = target_product.get_logs_destination()

    testtype_path = processors.TestRunnerLoggerMaster.get_logs_path_from_base(
        base_logs_path, test_type='SYSTEM', product_name=replaced_product, serial_no=context.SERIAL_NO)

    all_tests = logpaths.get_datetime_ordered_tests_in_testtype_folder(testtype_path,
        allow_debug=True, allowed_results=[testresults.GlobalTestPassState.TEST_PASS, testresults.GlobalTestPassState.TEST_FAIL])

    # test are in oldest-first order, reverse.
    for test in reversed(all_tests):
        path = os.path.join(testtype_path, test['folder'])
        context.logger.debug('Checking path %s' % path)
        calib_folder = os.path.join(path, 'calib')
        if not os.path.exists(calib_folder):
            context.logger.debug('No calib folder')
            continue
        file_count = len([name for name in os.listdir(calib_folder) if os.path.isfile(os.path.join(calib_folder, name))])
        if file_count < 31:
            context.logger.debug('Not enough files in calib folder (%d files)' % file_count)
            continue
        # Test looks like it has a full calib, break loop
        context.logger.info('Test chosen is at path %s' % path)
        break
    else:
        raise Exception('Unit does not seem to have a previous test with a full calib')
    context.PREVIOUS_CALIB_TEST_PATH = path
