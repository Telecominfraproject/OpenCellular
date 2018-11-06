#!/usr/bin/python

"""
Setup:
1. Agilent MXA / Angritsu MT8870A, connect to primary antenna port of enodeB
2. Set TFTP server, address and port as test_config
"""

import sys
import test_config
import im_calibration
from time import sleep
from im_calibration import Calibration
#import common

class CalTxPwr(Calibration):
    
    def __init__(self, rpt_hndl, rec_num):
        
        self.rpt = rpt_hndl
        self.rn = rec_num
        self.slope_1 = ''
        self.offset_1 = ''
        self.slope_2 = ''
        self.offset_2 = ''
        self.attn_gain_val = []
        self.input_power = -20  # input power in dBm
        self.signal_bw = type(self).cfg.cal_bandwidth
        
        if (self.cfg.cal_bandwidth == 5):
            self.intbw = '4.5'
            self.spabw = '10'
        elif (self.cfg.cal_bandwidth == 10):
            self.intbw = '9'
            self.spabw = '20'
        elif (self.cfg.cal_bandwidth == 15):
            self.intbw = '13.5'
            self.spabw = '25'
        elif (self.cfg.cal_bandwidth == 20):
            self.intbw = '18'
            self.spabw = '30'
        
    def mxa_setup(self):
        
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        type(self).mxa.send_msg_to_server(':INST:SEL SA')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(test_config.dl_freq) + ' MHz')
        type(self).mxa.send_msg_to_server(':CONF:CHP')
        type(self).mxa.send_msg_to_server(':POW:ATT 30')
        type(self).mxa.send_msg_to_server(':DISP:CHP:VIEW:WIND:TRAC:Y:RLEV 20 dBm')
        type(self).mxa.send_msg_to_server(':CHP:BAND:INT ' + self.intbw + ' MHz')
        type(self).mxa.send_msg_to_server(':CHP:FREQ:SPAN ' + self.spabw + ' MHz')
        type(self).mxa.send_msg_to_server(':CHP:AVER:COUN 25')
        type(self).mxa.send_msg_to_server(':CHP:AVER ON')
        
    def mt8870a_setup(self, freq_center):
        
        print 'Start VSA...'
        type(self).mt8870a.send_msg_to_server('*RST')
        type(self).mt8870a.send_msg_to_server('*IDN?')
        in_msg = type(self).mt8870a.recv_msg_frm_server()
        print('recv ' + type(self).cfg.mt8870a_ipaddr + '= ' + in_msg)
        
        type(self).mt8870a.send_msg_to_server('INST SMALLCELL')
        type(self).mt8870a.send_msg_to_server('INST:SYST 3GLTE_DL,ACT')
        type(self).mt8870a.send_msg_to_server(':ROUT:PORT:CONN:DIR PORT3,PORT4')
        
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
        #	  type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:DMOD FDD")
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
            
    def do_txpwr_cal(self):
        
        type(self).enb.enb_cd_usr_bin()
        
        #cmd = "a 1 " + str(type(self).cfg.attn1) + " q"
        #type(self).enb.enb_rf_drv_call_cmd(cmd)
        #sleep(2)
        
        type(self).enb.enb_rf_drv_call()
        type(self).enb.tn_write("loss", "T")   # select tx cal
        type(self).enb.tn_write("antenna", str(type(self).cfg.txpwr_cable_loss))
        sleep(2)
        
        if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
            self.rpt.write('\nTX Power Calibration:\n')
        
        for ant in range(2):
            
            # the RF driver already switch the TX ports
            print('')
            print("Testing antenna port " + str(ant+1))
            type(self).enb.tn.write("\n")          # connect ant
            sleep(2)    # wait for second round stable
            
            if (ant == 0):
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:LTE:MEAS:ENB:SCENario:SALone RFAC, RX1")
            else:
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:LTE:MEAS:ENB:SCENario:SALone RFBC, RX1")
                    
            
            for st in range(0,3):
                
                res = type(self).enb.tn.read_until(" ".encode("ascii"))
                
                # measure channel power
                if (type(self).cfg.test_set == 'agilent'):
                    
                    sleep(1) #(5)    # wait for average
                    type(self).mxa.send_msg_to_server(':FETC:CHP:CHP?')
                    in_msg = type(self).mxa.recv_msg_frm_server()
                    chpwr = str(round(float(in_msg), 2))
                    print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + chpwr)
                    
                elif (type(self).cfg.test_set == 'anritsu'):
                    
                    sleep(3)    # wait for average
                    self.input_power = self.input_power + 10*st
                    type(self).mt8870a.send_msg_to_server(':BATC:BAND:POW:RANG:ILEV ' + str(self.input_power))
                    type(self).mt8870a.send_msg_to_server(':INIT:MODE:SING')
                    type(self).mt8870a.send_msg_to_server('*WAI')
                    type(self).mt8870a.send_msg_to_server(':FETC:BATC1?')
                    type(self).mt8870a.send_msg_to_server('*WAI')
                    in_msg = type(self).mt8870a.recv_msg_frm_server()
                    #print in_msg
                    line = in_msg.split(',')
                    chpwr = str(round(float(line[5]), 2))
                    print "Tx Power (Average) = " + chpwr + " dBm"
                    
                elif (type(self).cfg.test_set == 'rs'):
                    
                    if (st == 0):
                        self.input_power = 5
                        
                    self.input_power = self.input_power + 10
                    type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:ENP " + str(self.input_power))
                    type(self).cmw500.send_msg_to_server("INIT:LTE:MEAS:ENB:MEV")
                    type(self).cmw500.send_msg_to_server("*WAI")
                    type(self).cmw500.send_msg_to_server("FETC:LTE:MEAS:ENB:MEV:MOD:AVER?")
                    in_msg = type(self).cmw500.recv_msg_frm_server()
                    line = in_msg.split(',')
                    if (int(line[0]) != 0):
                        print in_msg
                    chpwr = str(round(float(line[17]), 2))
                    print "Tx Power (Average) = " + chpwr + " dBm"
                    
                type(self).enb.tn.write((chpwr+'\n').encode("ascii"))
                sleep(3)    # wait for second round stable
                
        try:
            res = type(self).enb.tn.read_until("calibration", 5)
            #type(self).enb.tn_write("Option:", "y") # fsetenv variables
            type(self).enb.tn.write("q\n".encode("ascii"))
            
            start = 0
            while (True):
                start = res.find("slope_1=".encode("ascii"), start)
                if (start == -1): break
                start += len("slope_1=")
                self.slope_1 = res[start:res.find(", ".encode("ascii"), start)]
                
                start = res.find("offset_1=".encode("ascii"), start)
                if (start == -1): break
                start += len("offset_1=")
                self.offset_1 = res[start:res.find('\n'.encode("ascii"), start)]
                
                start = res.find("slope_2=".encode("ascii"), start)
                if (start == -1): break
                start += len("slope_2=")
                self.slope_2 = res[start:res.find(", ".encode("ascii"), start)]
                
                start = res.find("offset_2=".encode("ascii"), start)
                if (start == -1): break
                start += len("offset_2=")
                self.offset_2 = res[start:res.find('\n'.encode("ascii"), start)]
                
                print ""
                print("slope_1=" + self.slope_1 + ", offset_1=" + self.offset_1)
                print("slope_2=" + self.slope_2 + ", offset_2=" + self.offset_2)
                print ""
                
                if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                    self.rpt.write("slope_1=" + self.slope_1 + ", offset_1=" + self.offset_1 + '\n')
                    self.rpt.write("slope_2=" + self.slope_2 + ", offset_2=" + self.offset_2 + '\n')
                    
                break
            
            if test_config.wr_var_to_uboot == True:
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv tx_slope_prim " \
                                        + str(self.slope_1), 3)
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv tx_offset_prim " \
                                        + str(self.offset_1), 3)
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv tx_slope_sec " \
                                        + str(self.slope_2), 3)
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv tx_offset_sec " \
                                        + str(self.offset_2), 3)
            
            if (type(self).cfg.en_eeprom_write == True):
                self.enb.editUbootenv('TX_SLOPE_PRIM', str(self.slope_1))
                self.enb.editUbootenv('TX_OFFSET_PRIM', str(self.offset_1))
                self.enb.editUbootenv('TX_SLOPE_SEC', str(self.slope_2))
                self.enb.editUbootenv('TX_OFFSET_SEC', str(self.offset_2))
                
                if (im_calibration.is_test_all == True):
                    sleep(2)
                    self.write_txpwr_to_eeprom()
                else:
                    se = raw_input("Write result to EEPROM?(y/n):")
                    if (se == 'y') or (se == 'Y'):
                        self.write_txpwr_to_eeprom()
            
        except:
            print "Unexpected error:", sys.exc_info()[0]
            
        # stop transmit
        #type(self).enb.enb_cd_tmpfs()
        #type(self).enb.enb_stop_transmit()
        
    def write_txpwr_to_eeprom(self):
        
        type(self).enb.enb_cd_usr_bin()

        print "edit EEPROM record"
        type(self).enb.enb_eeprom_edit_record('wtsp', self.rn, self.slope_1)
        type(self).enb.enb_eeprom_edit_record('wtop', self.rn, self.offset_1)
        type(self).enb.enb_eeprom_edit_record('wtss', self.rn, self.slope_2)
        type(self).enb.enb_eeprom_edit_record('wtos', self.rn, self.offset_2)   
        sleep(0.5)    # wait EEPROM data wrote
        
    def run(self):
        
        if (type(self).cfg.test_set == 'agilent'):
            self.mxa_setup()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(test_config.dl_freq)
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_setup(test_config.dl_freq)
            
        self.start_enodeb_tx()
        sleep(3)    # wait spectrum comes out
        
        self.do_txpwr_cal()

    def get_attenuation_gain_value(self):
        
        self.attn_gain_val = []
        type(self).enb.enb_cd_usr_bin()
        type(self).enb.tn.write("oncpu 0 /usr/bin/" + self.cfg.rf_driver + '\n')
        _ = type(self).enb.tn.read_until("Option:".encode("ascii"), 5)
        
        sleep(2) # prevent the rag command send too fast
        type(self).enb.tn.write("rag\n".encode("ascii"))
        res = type(self).enb.tn.read_until("gain1".encode("ascii"), 5)
        type(self).enb.tn.write("q\n".encode("ascii"))
        self.attn_gain_val.append(self.parse_attn_gain(res, 'attn1 '))
        self.attn_gain_val.append(self.parse_attn_gain(res, 'attn2 '))

    def parse_attn_gain(self, message, headstr):
        
        start = 0
        while (True):
            start = message.find(headstr, start)
            if (start == -1): break
            start += len(headstr)
            res = message[start:message.find(" dB".encode("ascii"), start)]
            return int(res)

    def set_attenuation(self, ch):
        
        self.get_attenuation_gain_value()
        
        if  (ch == 1):
            if  (len(self.attn_gain_val) == 2):
                print "Current primary attenuation = " + \
                        str(self.attn_gain_val[0])
            self.att = raw_input("TX1 power attenuation(5-45 dB): ")
            cmd = "a 1 " + str(self.att) + " q"
            type(self).enb.enb_rf_drv_call_cmd(cmd)
            
        else:
            if  (len(self.attn_gain_val) == 2):
                print "Current secondary attenuation = " + \
                        str(self.attn_gain_val[1])
            self.att = raw_input("TX2 power attenuation(5-45 dB): ")
            cmd = " a 2 " + str(self.att) + " q"
            type(self).enb.enb_rf_drv_call_cmd(cmd)
            
    def tx_test(self):
        
        self.start_enodeb_tx()
        sleep(3)    # wait spectrum comes out
        
        if (type(self).cfg.test_set == 'agilent'):
            self.mxa_setup()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(test_config.dl_freq)
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_setup(test_config.dl_freq)
        
        while (True):
            print("(1) Primary port only")
            print("(2) Second port only")
            print("(3) All ports turn on")
            print("(q) Quit")
            
            self.mod = raw_input("Select: ")
            
            if (self.mod == '1'):
                type(self).enb.enb_disable_TX2()
                self.set_attenuation(1)
            
            elif (self.mod == '2'):
                type(self).enb.enb_disable_TX1()
                self.set_attenuation(2)
                
            elif (self.mod == '3'):
                type(self).enb.enb_enable_all_TX()
                self.set_attenuation(1)
                self.set_attenuation(2)
            
            elif (self.mod == 'q') or (self.mod == 'Q'):
                break
            
            else:
                print "unknown option"
                continue
                
        type(self).enb.enb_stop_transmit()
        