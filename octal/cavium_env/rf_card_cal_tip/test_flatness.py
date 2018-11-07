#!/usr/bin/python

#import common
import test_config
from time import sleep
from im_calibration import Calibration

class TestFlatness(Calibration):
    
    def __init__(self, rpt_hndl, bandwidth, dl_freq):
        
        self.rpt = rpt_hndl
        self.input_power = -20  # input power in dBm
        self.test_bw = bandwidth
        self.dl_freq = dl_freq
        self.curr_obw = 0.0
        self.ripple_data = []
        self.curr_freq_start = 0.0
        self.curr_freq_stop = 0.0
        self.sample_freq_step = 100 # kHz
        
        self.max_ripple_less_than_3mhz = 8
        self.max_ripple_greater_than_3mhz = 4
        
        if (self.test_bw == 5):
            self.intbw = '4.5'
            self.spabw = '10'
            self.obw_std = 'B5M'
        elif (self.test_bw == 10):
            self.intbw = '9'
            self.spabw = '20'
            self.obw_std = 'B10M'
        elif (self.test_bw == 20):
            self.intbw = '18'
            self.spabw = '30'
            self.obw_std = 'B20M'
        else:
            print "bandwidth is not in configuration"
            exit(1)
        
    def mxa_flatness_setup(self):
        
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        type(self).mxa.send_msg_to_server(':INST:SEL SA')
        #type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(self.dl_freq) + ' MHz')
        #type(self).mxa.send_msg_to_server(':FREQ:SPAN ' + self.curr_obw + ' MHz')
        type(self).mxa.send_msg_to_server(':FREQ:START ' + str(self.curr_freq_start - 1) + ' MHz')
        type(self).mxa.send_msg_to_server(':FREQ:STOP ' + str(self.curr_freq_stop + 1) + ' MHz')
        #type(self).mxa.send_msg_to_server(':BAND:RES 1 kHz')
        type(self).mxa.send_msg_to_server(':BAND:VID 1 kHz')
        type(self).mxa.send_msg_to_server(':POW:ATT 38')
        type(self).mxa.send_msg_to_server(':DISP:WIND:TRAC:Y:RLEV 20 dBm')
        type(self).mxa.send_msg_to_server(':AVER:COUN 10')
        
    def mxa_obw_setup(self):
        
        type(self).mxa.send_msg_to_server('*PSC')
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        type(self).mxa.send_msg_to_server(':INST LTE')
        type(self).mxa.send_msg_to_server(':POW:ATT 30') # attenuation
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(self.dl_freq) + ' MHz')
        type(self).mxa.send_msg_to_server(':RAD:STAN:PRES ' + self.obw_std) # LTE preset
        type(self).mxa.send_msg_to_server(':CONF:OBW ' + str(self.test_bw) + ' MHz')
        type(self).mxa.send_msg_to_server(':INIT:OBW')
        
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
        
    def collect_ripple_data(self):
        
        self.ripple_data = []
        mark_freq = self.curr_freq_start
        type(self).mxa.send_msg_to_server(':CALC:MARK1:MODE POS')
        
        while (mark_freq <= self.curr_freq_stop):
            type(self).mxa.send_msg_to_server(':CALC:MARK1:X ' + str(mark_freq) + ' MHz')
            sleep(0.5)
            type(self).mxa.send_msg_to_server(':CALC:MARK1:Y?')        # get power
            in_msg = type(self).mxa.recv_msg_frm_server()
            cur_pwr = round(float(in_msg), 2)
            #self.ripple_data.append([mark_freq, cur_pwr])
            self.ripple_data.append(cur_pwr)
            self.rpt.write('\t' + str(mark_freq) + ' MHz\t\t' + str(cur_pwr) + ' dBm\n')
            mark_freq = mark_freq + float(self.sample_freq_step)/1000.0
            print str(mark_freq) + ' MHz ' + str(cur_pwr) + ' dBm'
            
        type(self).mxa.send_msg_to_server(':CALC:MARK1:MODE OFF')
        
    def calcFlatness(self):
        
        data = sorted(self.ripple_data)
        flatness = abs(round(float(data[-1] - data[0]), 2))
        
        self.rpt.write('max = ' + str(round(float(data[-1]), 2)) + ' dBm\n')
        self.rpt.write('min = ' + str(round(float(data[0]), 2)) + ' dBm\n')
        self.rpt.write('flatness = ' + str(flatness) + ' dBm\n\n')
        self.rpt.flush()
        
        print ''
        print 'max = ' + str(round(float(data[-1]), 2)) + ' dBm'
        print 'min = ' + str(round(float(data[0]), 2)) + ' dBm'
        print 'flatness = ' + str(flatness) + ' dBm'
        print ''
        
    def do_flatness_measurement(self):
        
        # measure OBW
        obw_loop = 6
        self.mxa_obw_setup()
        
        for cnt in range(0, obw_loop):   # prevent get carrier only
            sleep(5) # wait for spectrum stable
            type(self).mxa.send_msg_to_server(':FETC:OBW?')
            in_msg = type(self).mxa.recv_msg_frm_server()
            line = in_msg.split(',')
            self.curr_obw = round(float(line[0])/1000000, 6)
            if (self.curr_obw > 3.0):
                break
            elif (cnt == (obw_loop - 1)):
                print 'transmit signal error, obw measure failed'
                exit(1)
        
        print('OBW = ' + str(self.curr_obw) + ' MHz')
        self.rpt.write('OBW = ' + str(self.curr_obw) + ' MHz\n')
        
        # set MXA in new span
        obw_offset = round(float(self.curr_obw/2), 6)
        self.curr_freq_start = self.dl_freq - obw_offset
        self.curr_freq_stop = self.dl_freq + obw_offset
        self.rpt.write('frequency start = ' + str(self.curr_freq_start) + ' MHz\n')
        self.rpt.write('frequency stop = ' + str(self.curr_freq_stop) + ' MHz\n\n')
        self.mxa_flatness_setup()
        sleep(3)
        
        # collect frequency vs power list [freq, pow]
        self.collect_ripple_data()
        
        # analysis data
        self.calcFlatness()
        
    def run(self):
        
        self.start_enodeb_tx()
        sleep(3)    # wait spectrum comes out
        self.do_flatness_measurement()
        
