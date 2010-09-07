# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class firmware_VbootCrypto(test.test):
    """
    Tests for correctness of verified boot reference crypto implementation.
    """
    version = 1
    preserve_srcdir = True

    # TODO(gauravsh): Disable this autotest until we have a way
    # of running these in a 64-bit environment (since for x86, this
    # code is run in 64-bit mode.
    #
    # This issue is tracked as Issue 3792 on the Chromium OS Bug Tracker.
    # http://code.google.com/p/chromium-os/issues/detail?id=3792
    def setup_Disabled(self):
        os.chdir(self.srcdir)
        utils.make('clean all')


    # Parses the [result] and output the key-value pairs.
    def __output_result_keyvals(self, results):
        for keyval in results.splitlines():
            if keyval.strip().startswith('#'):
                continue
            key, val = keyval.split(':')
            self.keyvals[key.strip()] = float(val)


    def __generate_test_cases(self):
        gen_test_case_cmd = os.path.join(self.srcdir, "tests",
                                         "gen_test_cases.sh")
        return_code = utils.system(gen_test_case_cmd, ignore_status = True)
        if return_code == 255:
            return False
        if return_code == 1:
            raise error.TestError("Couldn't generate test cases")
        return True


    def __sha_test(self):
        sha_test_cmd = os.path.join(self.srcdir, "tests", "sha_tests")
        return_code = utils.system(sha_test_cmd, ignore_status=True)
        if return_code == 255:
            return False
        if return_code == 1:
            raise error.TestError("SHA Test Error")
        return True


    def __rsa_test(self):
        os.chdir(self.srcdir)
        rsa_test_cmd = os.path.join(self.srcdir, "tests",
                                    "run_rsa_tests.sh")
        return_code = utils.system(rsa_test_cmd, ignore_status=True)
        if return_code == 255:
            return False
        if return_code == 1:
            raise error.TestError("RSA Test Error")
        return True


    def __image_verification_test(self):
        image_verification_cmd = "cd %s && ./run_image_verification_tests.sh" \
                                 % os.path.join(self.srcdir, "tests")
        return_code = utils.system(image_verification_cmd,
                                   ignore_status=True)
        if return_code == 255:
            return False
        if return_code == 1:
            raise error.TestError("Image Verification Test Error")
        return True


    def __sha_benchmark(self):
        sha_benchmark_cmd = os.path.join(self.srcdir, "tests",
                                         "sha_benchmark")
        self.results = utils.system_output(sha_benchmark_cmd,
                                           retain_output=True)
        self.__output_result_keyvals(self.results)


    def __rsa_benchmark(self):
        rsa_benchmark_cmd = "cd %s && ./rsa_verify_benchmark" % \
                            os.path.join(self.srcdir, "tests")
        self.results = utils.system_output(rsa_benchmark_cmd,
                                           retain_output=True)
        self.__output_result_keyvals(self.results)


    def __verify_image_benchmark(self):
        firmware_benchmark_cmd = "cd %s && ./firmware_verify_benchmark" % \
                                 os.path.join(self.srcdir, "tests")
        kernel_benchmark_cmd = "cd %s && ./kernel_verify_benchmark" % \
                                 os.path.join(self.srcdir, "tests")
        self.results = utils.system_output(firmware_benchmark_cmd,
                                           retain_output=True)
        self.__output_result_keyvals(self.results)
        self.results = utils.system_output(kernel_benchmark_cmd,
                                           retain_output=True)
        self.__output_result_keyvals(self.results)


    def __rollback_tests(self):
        firmware_rollback_test_cmd = "cd %s && ./firmware_rollback_tests" % \
                                     os.path.join(self.srcdir, "tests")
        kernel_rollback_test_cmd = "cd %s && ./kernel_rollback_tests" % \
                                     os.path.join(self.srcdir, "tests")
        return_code = utils.system(firmware_rollback_test_cmd,
                                   ignore_status=True)
        if return_code == 255:
            return False
        if return_code == 1:
            raise error.TestError("Firmware Rollback Test Error")

        return_code = utils.system(kernel_rollback_test_cmd,
                                   ignore_status=True)
        if return_code == 255:
            return False
        if return_code == 1:
            raise error.TestError("KernelRollback Test Error")
        return True


    def __splicing_tests(self):
        firmware_splicing_test_cmd = "cd %s && ./firmware_splicing_tests" % \
                                     os.path.join(self.srcdir, "tests")
        kernel_splicing_test_cmd = "cd %s && ./kernel_splicing_tests" % \
                                     os.path.join(self.srcdir, "tests")
        return_code = utils.system(firmware_splicing_test_cmd,
                                   ignore_status=True)
        if return_code == 255:
            return False
        if return_code == 1:
            raise error.TestError("Firmware Splicing Test Error")

        return_code = utils.system(kernel_splicing_test_cmd,
                                   ignore_status=True)
        if return_code == 255:
            return False
        if return_code == 1:
            raise error.TestError("Kernel Splicing Test Error")
        return True


    def run_crypto(self):
        success = self.__sha_test()
        if not success:
            raise error.TestFail("SHA Test Failed")
        success = self.__rsa_test()
        if not success:
            raise error.TestFail("RSA Test Failed")


    def run_verification(self):
        success = self.__image_verification_test()
        if not success:
            raise error.TestFail("Image Verification Test Failed")


    def run_benchmarks(self):
        self.keyvals = {}
        self.__sha_benchmark()
        self.__rsa_benchmark()
        self.__verify_image_benchmark()
        self.write_perf_keyval(self.keyvals)


    def run_rollback(self):
        success = self.__rollback_tests()
        if not success:
            raise error.TestFail("Rollback Tests Failed")


    def run_splicing(self):
        success = self.__splicing_tests()
        if not success:
            raise error.TestFail("Splicing Tests Failed")


    def run_once(self, suite='crypto'):
        self.__generate_test_cases()
        getattr(self, 'run_' + suite)()
