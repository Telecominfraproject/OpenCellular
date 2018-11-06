#!/usr/bin/python

"""
Setup:
1. Agilent MXA, connect cable to one of the antenna port of enodeB
2. Set TFTP server, address and port as test_config
3. Test after TX Power calibration and reference clock calibration are done
"""

import sys
import test_config
import im_calibration
from time import sleep
from im_calibration import Calibration
#import common

class TestTxEvm(Calibration):
    
    def __init__(self, rpt_hndl, rec_num):
        
        self.rpt = rpt_hndl
        self.rn = rec_num
        self.got_evm_pwr = False
        self.val_evm_pwr = 0.0
        
        # TX power & EVM
        self.data = []
        self.elems = []
        self.opt_res = []
        self.first_loop = True
        # end of TX power & EVM
        
        self.input_power = -10  # input power in dBm
        self.signal_bw = type(self).cfg.cal_bandwidth   # input signal bandwidth
        
        if (self.cfg.cal_bandwidth == 5):
            self.ltebw = 'B5M'
            self.intbw = '4.5'
            self.spabw = '10'
        elif (self.cfg.cal_bandwidth == 10):
            self.ltebw = 'B10M'
            self.intbw = '9'
            self.spabw = '20'
        elif (self.cfg.cal_bandwidth == 15):
            self.ltebw = 'B15M'
            self.intbw = '13.5'
            self.spabw = '25'
        elif (self.cfg.cal_bandwidth == 20):
            self.ltebw = 'B20M'
            self.intbw = '18'
            self.spabw = '30'
        
    def mxa_setup(self):
        
        type(self).mxa.send_msg_to_server('*RST')
        #type(self).mxa.send_msg_to_server('*PSC')
        type(self).mxa.send_msg_to_server(':INST LTE')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(test_config.dl_freq) + ' MHz')
        type(self).mxa.send_msg_to_server(':DISP:MON:VIEW:WIND:TRAC:Y:RLEV 20')   # reference level
        type(self).mxa.send_msg_to_server(':POW:ATT 40')               # attenuation
        type(self).mxa.send_msg_to_server(':RAD:STAN:BAND ' + self.ltebw)   # LTE bandwidth
        type(self).mxa.send_msg_to_server(':MON:FREQ:SPAN ' + self.spabw + ' MHz')     # frequency span
        type(self).mxa.send_msg_to_server(':RAD:STAN:DIR DLIN')        # downlink
        type(self).mxa.send_msg_to_server(':CONF:CEVM')
        #type(self).mxa.send_msg_to_server(':CONF:EVM')
        type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:ANT:NUMB ANT2')
            
    def set_mxa_chp_measure(self):
        
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        #type(self).mxa.send_msg_to_server(':INST:SEL SA')
        type(self).mxa.send_msg_to_server(':INST:SEL LTE')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(test_config.dl_freq) + ' MHz')
        type(self).mxa.send_msg_to_server(':CONF:CHP')
        type(self).mxa.send_msg_to_server(':POW:ATT 40')
        type(self).mxa.send_msg_to_server(':DISP:CHP:VIEW:WIND:TRAC:Y:RLEV 20 dBm')
        #type(self).mxa.send_msg_to_server(':CHP:BAND:INT ' + self.intbw + ' MHz')
        #type(self).mxa.send_msg_to_server(':CHP:FREQ:SPAN ' + self.spabw + ' MHz')
        #type(self).mxa.send_msg_to_server(':CHP:AVER:COUN 10')
        #type(self).mxa.send_msg_to_server(':CHP:AVER ON')
        type(self).mxa.send_msg_to_server(':RAD:STAN:PRES ' + self.ltebw)   # LTE bandwidth
        type(self).mxa.send_msg_to_server(':MON:FREQ:SPAN ' + self.spabw + ' MHz')
        
    def mt8870a_setup(self, freq_center):
        
        print 'Start VSA...'
        type(self).mt8870a.send_msg_to_server(':BATC:BAND:FREQ:CENT ' + str(freq_center) + 'MHZ')
        type(self).mt8870a.send_msg_to_server(':BATC:BAND:POW:RANG:ILEV ' + str(self.input_power))
        type(self).mt8870a.send_msg_to_server(':BATC:CC:RAD:CBAN ' + str(self.signal_bw))
        type(self).mt8870a.send_msg_to_server(':BATC:CC:RAD:TMOD TM1_1')
        
        type(self).mt8870a.send_msg_to_server(':TRIG:STAT OFF')
        type(self).mt8870a.send_msg_to_server(':TRIG:SOUR IMM')
        
        type(self).mt8870a.send_msg_to_server(':BATC:CAPT:TIME:STAR 0')
        type(self).mt8870a.send_msg_to_server(':BATC:CAPT:TIME:LENG 1')
        
        type(self).mt8870a.send_msg_to_server(':BATC:CAPT:TIME:UWEM:STAR 0')
        type(self).mt8870a.send_msg_to_server(':BATC:CAPT:TIME:UWEM:LENG 140')
        type(self).mt8870a.send_msg_to_server(':BATC:CC:PDCC:SYMB:NUMB 2')
        
        type(self).mt8870a.send_msg_to_server(':BATC:EVM ON')
        type(self).mt8870a.send_msg_to_server('*WAI')
        
    def cmw500_setup(self, freq_center):
        
        print 'Start VSA...'
        #if test_config.band > 32:
        #    type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:DMOD TDD")
        #else:
        #    type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:DMOD FDD")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:DMOD FDD")    
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:FREQ " + str(freq_center) + " MHz")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:ENP 35")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:UMAR 0")
        
        if (self.cfg.cal_bandwidth == 5):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B050")
        elif (self.cfg.cal_bandwidth == 10):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B100")
        elif (self.cfg.cal_bandwidth == 15):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B150")
        elif (self.cfg.cal_bandwidth == 20):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B200")
        
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:RES:ALL ON,OFF,OFF,OFF,OFF,OFF,ON,OFF,OFF")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:SCO:MOD 10")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:MOEX ON")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:REP SING")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:ETES ETM11")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:PLC 1")
        type(self).cmw500.send_msg_to_server("TRIG:LTE:MEAS:ENB:MEV:SOUR 'Free Run (Fast Sync)'")
        
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
    
    # Find max TX power and check EVM
    def get_tx_evm_power(self):
        """
        type(self).enb.enb_cd_usr_bin()
        type(self).enb.enb_set_rf_drv_rf_card()
        sleep(3)
        
        if (type(self).cfg.cal_bandwidth >= 15):
            cmd = "w 0x02 0x4e q"
        else:
            cmd = "w 0x02 0x5e q"
            
        type(self).enb.enb_rf_drv_call_cmd(cmd)
        sleep(2)
        """
        if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
            self.rpt.write('\nTX Max Available Power:\n')
        
        for ant in range(2):
            print('')
            print("Testing antenna port " + str(ant+1))
            self.first_loop = True
            self.elems = []
            
            if (ant == 0):
                type(self).enb.enb_disable_TX2()
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:LTE:MEAS:ENB:SCENario:SALone RFAC, RX1")
            else:
                type(self).enb.enb_disable_TX1()
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:LTE:MEAS:ENB:SCENario:SALone RFBC, RX1")
                #sleep(3)
            
            #self.mxa_setup()
            sleep(2)
            
            for att in range(type(self).cfg.max_atten - 5):
                
                att2 = (type(self).cfg.max_atten - 1) - att
                cmd = "a 1 " + str(att2) + " a 2 " + str(att2) + " q"
                #print "cmd: a 1 " + str(att2) + " a 2 " + str(att2) + " q"
                type(self).enb.enb_rf_drv_call_cmd(cmd)
                
                if (type(self).cfg.test_set == 'agilent'):
                    
                    self.set_mxa_chp_measure()
                    sleep(2) #(5)
                    type(self).mxa.send_msg_to_server(':FETC:CHP:CHP?')
                    in_msg = type(self).mxa.recv_msg_frm_server()
                    chpwr = round(float(in_msg), 2) + type(self).cfg.txpwr_cable_loss
                    #print "chpwr = " + str(chpwr) + "dBm"
                    #common.hit_continue("check 2")
                elif (type(self).cfg.test_set == 'anritsu'):
                    if (type(self).cfg.cal_bandwidth >= 15):
                        self.input_power = 30 - att2
                    else:
                        self.input_power = 25 - att2
                        
                    type(self).mt8870a.send_msg_to_server(':BATC:BAND:POW:RANG:ILEV ' + str(self.input_power))
                    type(self).mt8870a.send_msg_to_server(':INIT:MODE:SING')
                    type(self).mt8870a.send_msg_to_server('*WAI')
                    sleep(2)
                    type(self).mt8870a.send_msg_to_server(':FETC:BATC1?')
                    type(self).mt8870a.send_msg_to_server('*WAI')
                    in_msg = type(self).mt8870a.recv_msg_frm_server()
                    #print in_msg
                    line = in_msg.split(',')
                    chpwr = round(float(line[5]), 2)  + type(self).cfg.txpwr_cable_loss
                    #print('recv ' + type(self).cfg.mt8870a_ipaddr + '= ' + str(chpwr))
                
                elif (type(self).cfg.test_set == 'rs'):
                    
                    type(self).cmw500.send_msg_to_server("INIT:LTE:MEAS:ENB:MEV")
                    type(self).cmw500.send_msg_to_server("*WAI")
                    #sleep(3)
                    #type(self).cmw500.send_msg_to_server("FETC:LTE:MEAS:ENB:MEV:STAT?")
                    type(self).cmw500.send_msg_to_server("FETC:LTE:MEAS:ENB:MEV:MOD:AVER?")
                    in_msg = type(self).cmw500.recv_msg_frm_server()
                    line = in_msg.split(',')
                    if (int(line[0]) != 0):
                        print in_msg
                    chpwr = round(float(line[17]), 2)  + type(self).cfg.txpwr_cable_loss
                    print('recv ' + type(self).cfg.cmw500_ipaddr + '= ' + str(chpwr))
                
                # check if TX can reach TX power criteria
                if (chpwr < type(self).cfg.cr_txpwr_min):

                    if (att2 == 5):
                        print "Fail, power " + str(chpwr) + " dBm lower than MIN TX power criteria"
                        if (ant == 1):
                            self.end_sys()
                            sys.exit()
                        else:
                            break
                        
                # check if EVM less than criteria
                else:
                    
                    if (type(self).cfg.test_set == 'agilent'):
                        #type(self).enb.enb_enable_all_TX()
                        self.mxa_setup()
                        if (ant == 0):
                            type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:ANT:PORT P0')
                            type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:SS:ANT:PORT P0')
                        else:
                            type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:ANT:PORT P1')
                            type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:SS:ANT:PORT P1')
                        sleep(2)
                        
                        type(self).mxa.send_msg_to_server(':READ:CEVM?')
                        #type(self).mxa.send_msg_to_server(':READ:EVM?')
                        in_msg = type(self).mxa.recv_msg_frm_server()
                        #print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)
                        
                        line = in_msg.split(',')
                        evm = round(float(line[0]), 2)
                    
                    elif (type(self).cfg.test_set == 'anritsu'):
                        """
                        type(self).mt8870a.send_msg_to_server(':INIT:MODE:SING')
                        type(self).mt8870a.send_msg_to_server('*WAI')
                        type(self).mt8870a.send_msg_to_server(':FETC:BATC1?')
                        type(self).mt8870a.send_msg_to_server('*WAI')
                        in_msg = type(self).mt8870a.recv_msg_frm_server()
                        print in_msg
                        line = in_msg.split(',')
                        """
                        evm = round(float(line[3]), 2)
                        
                    elif (type(self).cfg.test_set == 'rs'):
                        evm = round(float(line[3]), 2)
                    
                    #print('')
                    print("ANT" + str(ant+1) + ": TX Power=" + str(chpwr) + " dBm" + \
                          ", EVM=" + str(evm) + "%, ATT=" + str(att2))
                    if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                        self.rpt.write("ANT" + str(ant+1) + ": TX Power=" + str(chpwr) + " dBm" + \
                          ", EVM=" + str(evm) + "%, ATT=" + str(att2) + '\n')
                    
                    # if smaller than the EVM limit
                    if (evm <= type(self).cfg.cr_txevm_max):
                        # if bigger than max power
                        if (chpwr > type(self).cfg.cr_txpwr_max):
                            if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                                self.rpt.write('\n')
                            break
                        else:
                            self.elems.append([chpwr, evm, att2])
                            self.first_loop = False

                    # larger than EVM limit
                    else:
                        if self.first_loop:
                            self.elems.append([chpwr, evm, att2])
                        if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                            self.rpt.write('\n')
                        break
            
            # save data for each antenna
            self.data.append(self.elems)        
            
        self.get_optimized_txpwr()
        #type(self).enb.killPltD()
        
        print('')
        for ant in range(2):      
            print("TX" + str(ant+1) + " power=" + str(self.opt_res[ant][0]))
            print("    EVM=" + str(self.opt_res[ant][1]))
            print("    ATT=" + str(self.opt_res[ant][2]))
            if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                self.rpt.write("TX" + str(ant+1) + " power=" + str(self.opt_res[ant][0]) + '\n')
                self.rpt.write("    EVM=" + str(self.opt_res[ant][1]) + '\n')
                self.rpt.write("    ATT=" + str(self.opt_res[ant][2]) + '\n')
            print('')
        
        if (type(self).cfg.en_eeprom_write == True):
            if (im_calibration.is_test_all == True):
                self.write_max_txpwr_to_eeprom()
            else:
                se = raw_input("Write result to EEPROM?(y/n):")
                if (se == 'y') or (se == 'Y'):
                    self.write_max_txpwr_to_eeprom()
        
        self.chk_configure_attenuation()
        
        # stop transmit
        #type(self).enb.enb_cd_tmpfs()
        #type(self).enb.enb_stop_transmit()
        
    def get_optimized_txpwr(self):
        
        ant1pwr = int(self.data[0][-1][0])
        ant2pwr = int(self.data[1][-1][0])
        elem1sz = len(self.data[0])
        elem2sz = len(self.data[1])
        
        if (ant1pwr > ant2pwr):
            for cn in range(elem1sz, 0, -1):
                if (ant2pwr == int(self.data[0][cn-1][0])):
                    self.opt_res.append(self.data[0][cn-1])
                    self.opt_res.append(self.data[1][-1])
                    break
                if (cn == 1):
                    print "cannot find equal output power in both ports"
                    self.opt_res.append(self.data[0][-1])
                    self.opt_res.append(self.data[1][-1])
                    break
                    
        elif (ant1pwr < ant2pwr):
            for cn in range(elem2sz, 0, -1):
                if (ant1pwr == int(self.data[1][cn-1][0])):
                    self.opt_res.append(self.data[0][-1])
                    self.opt_res.append(self.data[1][cn-1])
                    break
                if (cn == 1):
                    print "cannot find equal output power in both ports"
                    self.opt_res.append(self.data[0][-1])
                    self.opt_res.append(self.data[1][-1])
                    break
        else:
            self.opt_res.append(self.data[0][-1])
            self.opt_res.append(self.data[1][-1])
            
    def write_max_txpwr_to_eeprom(self):
        
        type(self).enb.enb_cd_usr_bin()
        
        print "edit EEPROM record"
        type(self).enb.enb_eeprom_edit_record('wmpp', self.rn, str(self.opt_res[0][0]))
        type(self).enb.enb_eeprom_edit_record('wmps', self.rn, str(self.opt_res[1][0]))
        
        #print "write MD5 information"
        md5Val = type(self).enb.calc_rf_eeprom_md5()
        if (md5Val != 'non'):
            type(self).enb.enb_eeprom_edit_record('wrm', self.rn, md5Val)
            print "wrote MD5 record"
        else:
            print "MD5 not found"

        sleep(1)    # wait EEPROM data wrote
        
    # check if test configure attenuation available; do after optimized value found
    def chk_configure_attenuation(self):
        
        if (self.opt_res[0][2] > type(self).cfg.attn1) or (self.opt_res[1][2] > type(self).cfg.attn2):
            print "test configure attenuation is lower than optimized result, overwrite by optimized result"
            type(self).enb.tn_write(self.im_calibration.pp_base, "fsetenv atten1 " \
                                    + str(self.opt_res[0][2]), 3)
            type(self).enb.tn_write(self.im_calibration.pp_base, "fsetenv atten2 " \
                                    + str(self.opt_res[1][2]), 3)

    def run(self):
        
        self.start_enodeb_tx()
        
        if (type(self).cfg.test_set == 'agilent'):
            self.mxa_setup()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(test_config.dl_freq)
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_setup(test_config.dl_freq)
            
        sleep(3)
        
        #self.set_mxa_chp_measure()
        self.get_tx_evm_power()
        
