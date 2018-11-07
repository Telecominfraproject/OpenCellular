#!/usr/bin/python

"""
Setup:
1. Agilent MXA, connect to primary antenna port of enodeB
2. Set TFTP server, address and port as test_config
"""

import common
import test_config
from im_calibration import Calibration

class TestTxCw(Calibration):
    
    def __init__(self, rpt_hndl):
        
        self.fp = rpt_hndl
        self.slope_1 = ''
        self.offset_1 = ''
        self.slope_2 = ''
        self.offset_2 = ''
        
    def mxa_setup(self):
        
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        type(self).mxa.send_msg_to_server(':INST:SEL SA')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(test_config.dl_freq) + ' MHz')
        type(self).mxa.send_msg_to_server(':CONF:CHP')
        type(self).mxa.send_msg_to_server(':POW:ATT 30')
        type(self).mxa.send_msg_to_server(':DISP:CHP:VIEW:WIND:TRAC:Y:RLEV 20 dBm')
        type(self).mxa.send_msg_to_server(':CHP:BAND:INT 9MHz')
        type(self).mxa.send_msg_to_server(':CHP:FREQ:SPAN 20 MHz')
        type(self).mxa.send_msg_to_server(':CHP:AVER:COUN 25')
        type(self).mxa.send_msg_to_server(':CHP:AVER ON')
        
    def start_enodeb_tx(self):
        
        type(self).enb.enb_login()
        #type(self).enb.get_macaddr()
        type(self).enb.enb_set_1pps('tx')

        type(self).enb.enb_cd_usr_bin()
        type(self).enb.enb_set_rf_drv_rf_card()
        
        if (type(self).cfg.tcxo_ctrl == "pwm"):
            print("load pwm clock control")
            type(self).enb.enb_load_pwm()   # do PWM control
        elif (type(self).cfg.tcxo_ctrl == "dax"):
            print("load ext dac clock control")
            type(self).enb.enb_load_dax()   # do ext DAC control
        
    def run(self):
        
        self.mxa_setup()
        self.start_enodeb_tx()
        common.hit_continue()
        