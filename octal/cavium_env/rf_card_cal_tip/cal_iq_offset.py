#!/usr/bin/python

"""
Setup:
1. Agilent MXA, connect to primary antenna port of enodeB
2. Set TFTP server, address and port as test_config
"""

import operator
import test_config
import im_calibration
from time import sleep
from im_calibration import Calibration

class CalIQOffset(Calibration):
    
    def __init__(self, rpt_hndl, rec_num):
        
        self.rpt = rpt_hndl
        self.rn = rec_num
        self.iq_step = 5 #3
        self.local_freq_tolerance = 50000 # tolerance of local frequency deviation
        self.offs_low = 0xF0    # IQ offset lower limit #0xC0
        self.offs_up = 0xFF     # IQ offset upper limit
        
        self.reg_tx1_i = 0x92
        self.reg_tx1_q = 0x93
        self.reg_tx2_i = 0x94
        self.reg_tx2_q = 0x95
        self.reg_en_offset = 0x9F #0x9A
        
        self.results = {}
        self.offsets = []
        
    def mxa_setup(self):
        
        type(self).mxa.send_msg_to_server('*RST')
        #type(self).mxa.send_msg_to_server('*IDN?')
        #in_msg = type(self).mxa.recv_msg_frm_server()
        #print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)
        
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        type(self).mxa.send_msg_to_server(':INST:SEL SA')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(test_config.dl_freq) + ' MHz')
        type(self).mxa.send_msg_to_server(':FREQ:SPAN 300 kHz')
        type(self).mxa.send_msg_to_server(':BAND 100 Hz')
        type(self).mxa.send_msg_to_server(':POW:ATT 38')
        type(self).mxa.send_msg_to_server(':DISP:WIND:TRAC:Y:RLEV 20 dBm')
        
    def start_enodeb_tx(self):
        
        type(self).enb.enb_login()
        
        # default is PWM control in booting
        if (type(self).cfg.tcxo_ctrl == "dax"):
            print("load ext dac clock control")
            type(self).enb.enb_cd_usr_bin()
            type(self).enb.enb_load_dax()   # do ext DAC control
    
        if test_config.band > 32:
            type(self).enb.enb_cd_usr_bin()
            type(self).enb.enb_set_zen_tdd_tx()
            
        type(self).enb.enb_cd_tmpfs()
        type(self).enb.enb_run_dsp_app_dl()
            
    def do_iq_offset_cal(self):
        
        self.reg_tx_i = self.reg_tx1_i
        self.reg_tx_q = self.reg_tx1_q
        
        type(self).enb.enb_cd_usr_bin()
        
        if (type(self).cfg.cal_bandwidth >= 15):
            cmd = "w 0x02 0x4e q"
        else:
            cmd = "w 0x02 0x5e q"
            
        type(self).enb.enb_rf_drv_call_cmd(cmd)
        sleep(2)
        
        type(self).enb.enb_adi_write_reg(str(hex(self.reg_en_offset)), str(hex(0x0C)))
        
        try:
            for ant in range(2):
                print('')
                print("Testing antenna port " + str(ant+1))
                
                if (ant == 1):
                    self.reg_tx_i = self.reg_tx2_i
                    self.reg_tx_q = self.reg_tx2_q
                    type(self).enb.enb_disable_TX1()
                else:
                    type(self).enb.enb_disable_TX2()
                    
                # 1. fix Q offset to zero
                print("\n1. fix Q offset to initial value")
                type(self).enb.enb_adi_write_reg(str(hex(self.reg_tx_q)), str(hex(0xC0)))
                
                # 2. optimize I offset
                print("\n2. optimize I offset")
                self.results = {}
                for offs in xrange(self.offs_low, self.offs_up + 1, self.iq_step):
                    type(self).enb.enb_adi_write_reg(str(hex(self.reg_tx_i)), str(hex(offs)))
                    #sleep(self.delay)    # wait for average
                    lopwr = self.get_local_leakage_power()
                    if (lopwr == 0):
                        print 'IQ offset test error'
                        return -1
                    self.results[str(hex(offs))] = lopwr
                    print('I offset ' + str(hex(offs)) + ' power ' + str(lopwr) + ' dBm')
                    
                sort_res = sorted(self.results.iteritems(), key=operator.itemgetter(1))
                try:
                    self.offsets.append(sort_res[0][0])
                except:
                    print("error")
                    
                # 3. fix I offset to new value
                print("\n3. fix I offset to new value")
                type(self).enb.enb_adi_write_reg(str(hex(self.reg_tx_i)), str(sort_res[0][0]))
                
                # 4. optimize Q offset
                print("\n4. optimize Q offset")
                self.results = {}
                for offs in xrange(self.offs_low, self.offs_up + 1, self.iq_step):
                    type(self).enb.enb_adi_write_reg(str(hex(self.reg_tx_q)), str(hex(offs)))
                    #sleep(self.delay)    # wait for average
                    lopwr = self.get_local_leakage_power()
                    self.results[str(hex(offs))] = lopwr
                    print('Q offset ' + str(hex(offs)) + ' power ' + str(lopwr) + ' dBm')
                    
                sort_res = sorted(self.results.iteritems(), key=operator.itemgetter(1))
                self.offsets.append(sort_res[0][0])
                
                # 5. fix Q offset to new value
                print("\n5. fix Q offset to new value")
                type(self).enb.enb_adi_write_reg(str(hex(self.reg_tx_q)), str(sort_res[0][0]))
                
                # 6. optimize I offset
                print("\n6. optimize I offset")
                self.results = {}
                for offs in xrange(self.offs_low, self.offs_up + 1, self.iq_step):
                    type(self).enb.enb_adi_write_reg(str(hex(self.reg_tx_i)), str(hex(offs)))
                    #sleep(self.delay)    # wait for average
                    lopwr = self.get_local_leakage_power()
                    self.results[str(hex(offs))] = lopwr
                    print('I offset ' + str(hex(offs)) + ' power ' + str(lopwr) + ' dBm')
                    
                sort_res = sorted(self.results.iteritems(), key=operator.itemgetter(1))
                self.offsets.append(sort_res[0][0])
                
                # 7. fix I offset to new value
                print("\n7. fix I offset to new value")
                type(self).enb.enb_adi_write_reg(str(hex(self.reg_tx_i)), str(sort_res[0][0]))

                if (ant == 0):
                    se = 'n'
                    while (se != 'y'):
                        se = raw_input("Are you ready to switch to ant2?(y):")
                        if (se == 'y') or (se == 'Y'):
                            se = 'y'
                            print("\nNow testing ant2")
                            break
        
        except:
            #print Exception, e
            print("\n8. save dummy offset results")
            self.offsets.append(hex(255))
            self.offsets.append(hex(255))
            self.offsets.append(hex(255))
            self.offsets.append(hex(255))
            self.offsets.append(hex(255))
            self.offsets.append(hex(255))
        else:    
            # 8. save offset results
            print('\nIQ Offset Calibration Results:\n')
            print('tx1_i_offset = ' + str(self.offsets[2]))
            print('tx1_q_offset = ' + str(self.offsets[1]))
            print('tx2_i_offset = ' + str(self.offsets[5]))
            print('tx2_q_offset = ' + str(self.offsets[4]))
            print("\n8. save offset results")
            
        
        if test_config.wr_var_to_uboot == True:
            type(self).enb.tn_write(im_calibration.pp_base, "fsetenv tx1_q_offset " \
                                    + str(self.offsets[1]), 3)
            print("fsetenv TX1_Q_OFFSET " + str(self.offsets[1]))
            type(self).enb.tn_write(im_calibration.pp_base, "fsetenv tx1_i_offset " \
                                    + str(self.offsets[2]), 3)
            print("fsetenv TX1_I_OFFSET " + str(self.offsets[2]))
            type(self).enb.tn_write(im_calibration.pp_base, "fsetenv tx2_q_offset " \
                                    + str(self.offsets[4]), 3)
            print("fsetenv TX2_Q_OFFSET " + str(self.offsets[4]))
            type(self).enb.tn_write(im_calibration.pp_base, "fsetenv tx2_i_offset " \
                                    + str(self.offsets[5]), 3)
            print("fsetenv TX2_I_OFFSET " + str(self.offsets[5]))
        
        if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
            self.rpt.write('\nIQ Offset Calibration:\n')
            self.rpt.write('tx1_i_offset = ' + str(self.offsets[2]) + '\n')
            self.rpt.write('tx1_q_offset = ' + str(self.offsets[1]) + '\n')
            self.rpt.write('tx2_i_offset = ' + str(self.offsets[5]) + '\n')
            self.rpt.write('tx2_q_offset = ' + str(self.offsets[4]) + '\n')
            
        if (type(self).cfg.en_eeprom_write == True):
            self.enb.editUbootenv('TX1_I_OFFSET', str(self.offsets[2]))
            self.enb.editUbootenv('TX1_Q_OFFSET', str(self.offsets[1]))
            self.enb.editUbootenv('TX2_I_OFFSET', str(self.offsets[5]))
            self.enb.editUbootenv('TX2_Q_OFFSET', str(self.offsets[4]))

            if (im_calibration.is_test_all == True):
                self.write_iqoffset_to_eeprom()
            else:
                se = raw_input("Write result to EEPROM?(y/n):")
                if (se == 'y') or (se == 'Y'):
                    self.write_iqoffset_to_eeprom()
            
        # stop transmit
        #type(self).enb.enb_cd_tmpfs()
        #type(self).enb.enb_stop_transmit()
        return 0
        
        
    def write_iqoffset_to_eeprom(self):
        
        type(self).enb.enb_cd_usr_bin()
        
        print "edit EEPROM record"
        type(self).enb.enb_eeprom_edit_record('wiop', self.rn, str(self.offsets[2]))
        type(self).enb.enb_eeprom_edit_record('wqop', self.rn, str(self.offsets[1]))
        type(self).enb.enb_eeprom_edit_record('wios', self.rn, str(self.offsets[5]))
        type(self).enb.enb_eeprom_edit_record('wqos', self.rn, str(self.offsets[4]))
        sleep(0.5)    # wait EEPROM data wrote
        
    def get_local_leakage_power(self):
        
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(test_config.dl_freq) + ' MHz')
        type(self).mxa.send_msg_to_server(':CALC:MARK1:CPS ON')    # mark peak
        
        if (self.is_peak_at_local() == False):
            print "local leakage not detected"
            return 0
            
        type(self).mxa.send_msg_to_server(':CALC:MARK1:Y?')        # get power
        in_msg = type(self).mxa.recv_msg_frm_server()
        cur_pwr = round(float(in_msg), 2)
        #print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)
        
        return cur_pwr
    
    def is_peak_at_local(self):
        
        type(self).mxa.send_msg_to_server(':CALC:MARK1:X?')        # get mark frequency
        in_msg = type(self).mxa.recv_msg_frm_server()
        fpeak = float(in_msg)
        #print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)
        diff = abs(test_config.dl_freq*1000000 - fpeak)
        
        if (diff < self.local_freq_tolerance):
            return True
        else:
            return False
        
        
    def run(self):
        
        self.mxa_setup()
        self.start_enodeb_tx()
        sleep(8)    # wait for leakage appears
        
        res = self.do_iq_offset_cal()
        return res
