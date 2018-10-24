
from tools.lte import lte_scan

from .preparation import Preparation
from . import impedance
from . import impedance_revE
from .. import cleanup

def main(test, context):
    lte_scan.validate_fe_scan(context, context.ORIGINAL_UNIT_SCAN)
    if context.FE_REV == 'E':
        test.suite(impedance_revE.FEImpedance)
    else:
        test.suite(Preparation)
        test.suite(impedance.FEImpedance)
        test.suite(cleanup.Cleanup)
