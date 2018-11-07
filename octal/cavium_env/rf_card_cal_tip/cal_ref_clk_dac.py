#!/usr/bin/python

"""
Setup:
1. Agilent MXA, connect cable to one of the antenna port of enodeB
2. Set TFTP server, address and port as test_config
"""

import test_config
import im_calibration
from time import sleep
from im_calibration import Calibration

class CalRefClk(Calibration):
    
    def __init__(self, rpt_hndl, rec_num):
        
        self.rpt = rpt_hndl
        self.rn = rec_num
        self.cur_freq = 0.0
        self.ref_freq = 30720000
        self.dif_freq = 0
        self.dac_cur = 0x0
        self.dac_val = 0x0
        self.limit_cal = 1
        self.got_ref = False
        self.narrow_span = False
        
    def mxa_setup(self):
        
        type(self).mxa.send_msg_to_server('*RST')
        type(self).mxa.send_msg_to_server('*IDN?')
        in_msg = type(self).mxa.recv_msg_frm_server()
        print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)
        
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        type(self).mxa.send_msg_to_server(':INST:SEL SA')
        type(self).mxa.send_msg_to_server(':CONF:SAN:NDEF')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(self.ref_freq) + ' Hz')
        type(self).mxa.send_msg_to_server(':FREQ:SPAN 1 KHz')
        type(self).mxa.send_msg_to_server(':DISP:WIND:TRAC:Y:RLEV -50 dBm')
        type(self).mxa.send_msg_to_server(':POW:ATT 10')
        
    def start_enodeb(self):
        
        type(self).enb.enb_login()
        print 'enb get mac address'
        type(self).enb.get_macaddr()
        print 'enb set 1pps'
        type(self).enb.enb_set_1pps('tx')

        print 'enb cd usr bin'
        type(self).enb.enb_cd_usr_bin()
        print 'enb load rf drv'
        type(self).enb.enb_load_rf_drv()
        print 'env load rf init'
        type(self).enb.enb_load_rf_init()
        type(self).enb.enb_set_rf_drv_rf_card()
        
        if test_config.band > 32:
            type(self).enb.enb_set_zen_tdd_tx()
        
        if im_calibration.is_dsp_running:
            type(self).enb.enb_cd_tmpfs()
            type(self).enb.enb_stop_transmit()
        
    def do_refclk_cal_dac(self):
        
        type(self).enb.enb_rf_drv_call()
        type(self).enb.tn_write("Value", "C")   # select clock cal
        type(self).enb.tn_write("clock", str(self.dac_val))
        type(self).mxa.send_msg_to_server(':CALC:MARK1:CPS ON')    # mark peak
        sleep(3)    # wait for frequency stable
        
        for rn in range(50):
            print("\ncalibration round: " + str(rn+1))
            self.check_curr_freq()
            
            self.dif_freq = int(round(self.ref_freq - self.cur_freq))
            
            if rn > 30:
                self.limit_cal = 5;
            
            if (abs(self.dif_freq) < 10) and (not self.narrow_span):
                type(self).mxa.send_msg_to_server(':FREQ:SPAN 100 Hz')
                self.narrow_span = True
                sleep(3)
                continue
            elif (abs(self.dif_freq) >= self.limit_cal):
                self.calc_dac_value()
            else:
                self.got_ref = True
                tmp = hex(self.dac_val)
                self.dac_val = tmp
                print("Got target reference frequency, DAC value= " + str(self.dac_val))
                if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                    self.rpt.write('\nReference Clock Calibration:\n')
                    self.rpt.write('Frequency = ' + str(self.ref_freq) + '\n')
                    self.rpt.write('dac_offset = ' + str(self.dac_val) + '\n')
                break
                
            type(self).enb.tn_write("Value", "n")
            type(self).enb.tn_write("clock", str(hex(self.dac_val)))
            
            if self.narrow_span:
                sleep(2)
            else:
                sleep(1)
        
        if (self.got_ref):
            type(self).enb.tn_write("calibration", "y")
            #type(self).enb.tn_write("Option", "y") # y, fsetenv dac_offset
            type(self).enb.tn.write("q\n".encode("ascii"))
        else:
            print("Reference clock calibration failed")
          
        if test_config.wr_var_to_uboot == True:
            print("fsetenv dac_offset " + str(self.dac_val))
            type(self).enb.tn_write(im_calibration.pp_base, "fsetenv dac_offset " \
                                    + str(self.dac_val), 3)
        
        
    def check_curr_freq(self):
        
        type(self).mxa.send_msg_to_server(':CALC:MARK1:X?')
        in_msg = type(self).mxa.recv_msg_frm_server()
        self.cur_freq = float(in_msg)
        print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)        
        
    def calc_dac_value(self):

        self.dac_val = self.dac_cur + self.dif_freq*2
        
        print("cur_val=" + str(hex(self.dac_cur)) + " diff=" + str(self.dif_freq) 
              + " dac_val=" + str(hex(self.dac_val)))
        self.dac_cur = self.dac_val    
    
    def write_cal_temp_to_eeprom(self, cal_temp):
        
        cal_temp = type(self).enb.enb_get_temperature()
        
        type(self).enb.enb_cd_usr_bin()
        num_rec = type(self).enb.enb_eeprom_get_record_num()
        
        print "record calibration temperature in EEPROM"
        type(self).enb.enb_eeprom_edit_record('wct', num_rec, hex(cal_temp))
        
    def run(self):
        
        self.mxa_setup()
        self.start_enodeb()
        self.do_refclk_cal_dac()
        
        #if (type(self).cfg.eeprom_new_rec == True):
        #    self.write_cal_temp_to_eeprom()