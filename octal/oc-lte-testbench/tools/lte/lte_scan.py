
from nute.uut.uut import UUT, Format, UUTError
import re

from view_utils import append_unit_info

def try_get_mac(scan):
    _uut = UUT(form=[Format.MRP, Format.SN, Format.MAC_ADDR])
    try:
        _uut.scan_parse(scan)
        return _uut.MAC_ADDR
    except:
        return None


def validate_fe_scan(context, scan):
    _uut = UUT(form=[Format.MRP, Format.SN])
    _uut.scan_parse(scan)

    context.FE_MRP = _uut.MRP
    context.FE_SERIAL_NO = _uut.SN
    # with context.criteria.evaluate_block() as block:
    #     block.evaluate("UUT_FE_MRP", context.FE_MRP)
    #     block.evaluate("UUT_FE_SERIAL_NO", context.FE_SERIAL_NO)

    rev_regex = r'PRD-854(?:[-\d]{4})(\w)[\d-]*'
    context.FE_REV = re.match(rev_regex, _uut.MRP).group(1)
    append_unit_info(context, 'Frontend: MRP: %s' % context.FE_MRP)
    context.add_future_link_to_test(_uut.MRP, _uut.SN)

def validate_bb_scan(context, scan):
    _uut = UUT(form=[Format.MRP, Format.SN, Format.MAC_ADDR])
    _uut.scan_parse(scan)

    context.BB_MRP = _uut.MRP
    context.BB_SERIAL_NO = _uut.SN
    context.BB_MAC_ADDR = _uut.MAC_ADDR
    with context.criteria.evaluate_block() as block:
        block.evaluate("UUT_BB_MRP", context.BB_MRP)
        block.evaluate("UUT_BB_SERIAL_NO", context.BB_SERIAL_NO)
        block.evaluate("UUT_BB_MAC", context.BB_MAC_ADDR)

    append_unit_info(context, 'Baseboard: MRP: %s' % (context.BB_MRP))
    context.add_future_link_to_test(_uut.MRP, _uut.SN)

def validate_unit_scan(context, scan):
    validate_sys_scan(context, scan)
    # _uut = UUT(form=[Format.MRP, Format.SN])
    # _uut.scan_parse(scan)
    #
    # context.UNIT_MRP = _uut.MRP
    # context.UNIT_SERIAL_NO = _uut.SN
    # with context.criteria.evaluate_block() as block:
    #     block.evaluate("UUT_UNIT_MRP", context.UNIT_MRP)
    #     block.evaluate("UUT_UNIT_SERIAL_NO", context.UNIT_SERIAL_NO)


def validate_sys_scan(context, scan):
    try:
        _uut = UUT(form=[Format.MRP, Format.SN])
        _uut.scan_parse(scan)
    except UUTError:
        _uut = UUT(form=[Format.MRP, Format.SN, Format.MAC_ADDR])
        _uut.scan_parse(scan)

    context.SYS_MRP = _uut.MRP
    context.SYS_SERIAL_NO = _uut.SN
    #with context.criteria.evaluate_block() as block:
        #block.evaluate("UUT_SYS_MRP", context.SYS_MRP)
        #block.evaluate("UUT_SYS_SERIAL_NO", context.SYS_SERIAL_NO)

    append_unit_info(context, 'System: MRP: %s' % (context.SYS_MRP))
    context.add_future_link_to_test(_uut.MRP, _uut.SN)
