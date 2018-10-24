from nute.core import testrunner
import os

if __name__ == "__main__":
    tb_name="OC-LTE Testbench"
    path = os.path.dirname(os.path.realpath(__file__))
    testrunner.setup(tb_name, path)
    testrunner.main()
