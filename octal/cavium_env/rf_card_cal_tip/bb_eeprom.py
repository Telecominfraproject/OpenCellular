#!/usr/bin/python

import sys
import os.path
import test_config
from time import sleep
from im_calibration import Calibration


class BBEepromAccess(Calibration):
    
    def __init__(self):
        
        self.cfg = test_config.EnbConfig()
        self.cfg_file = 'bb_eeprom.txt'
        
        self.SerialNum = '12345678'     # 12 bytes
        self.boardname = 'zen'          # ('refkit1', 'zen') default 'zen'
        self.boardversion = 7           # (1 to 254)
        self.NumETHPort = 1             # (1 or 2) default 1
        self.ExtPTPHWPresent = 1        # (0 not present, 1 present)
        self.ExtPTPHWUARTPort = 'ttyS1' # (port name) default 'ttyS1'
        self.ExtPTPHWUARTBaud = 6       # (1: 4800 2: 9600 3: 19200 4: 38400 5: 57600 
                                        # 6: 115200 7: 230400); default 6
        self.ExtPTPHWUARTEcho = 0       # (0 disable, 1 enable) default 0
        self.ExtGPSHWPresent = 0        # (0 not present, 1 present)
        self.ExtGPSHWUARTPort = 'ttyS1' # (port name) default 'ttyS1'
        self.ExtGPSHWUARTBaud = 6       # (1: 4800 2: 9600 3: 19200 4: 38400 5: 57600 
                                        # 6: 115200 7: 230400); default 6
        self.ExtGPSHWUARTEcho = 0       # (0 disable, 1 enable) default 0
        self.SimCardPresent = 0         # (0 not present, 1 present)
        self.SDCardPresent = 0          # (0 not present, 1 present)
        self.USBPortPresent = 0         # (0 not present, 1 present)
        self.PCIEPresent = 0            # (0 not present, 1 present)
        self.EthSwitchPresent = 0       # (0 not present, 1 present)
        
        self.PtpVcoDriftPresent = 0     # (0 not present, 1 present)
        self.PtpVcoDriftValue = 0       # signed integer
        
        self.read_bb_config_file()
        
    def start_enodeb(self):
        
        type(self).enb.enb_login()
        type(self).enb.enb_cd_usr_bin()
        #type(self).enb.enb_load_rf_driver()
        
    def read_line(self, strline):
        param = ""
        value = ""
        sn = strline
        
        if ((sn[0] != '#') and (sn[0] != ' ') and 
            (sn[0] != '\n') and (sn[0] != '\r') and
            (sn[0] != '\t')):
            result = sn.split('=')
            param = result[0]
            value = result[1]
            for i in range(len(value)):
                if ((value[i] == '\n') or (value[i] == '\r')):
                    value = value[:i]
                    break
                elif ((value[i] == " ") or (value[i] == "#") or (value[i] == '\t')):
                        value = value[:i]
                        break
            value = value.strip("\'")
            #print("read_line(): param=%s, value=%s" % (param, value))
            
        return param, value
        
    def read_bb_config_file(self):
        
        #if not os.path.isfile(self.cfg_file):
        #    print 'error, bb_eeprom.txt not exists'
        #    exit(-1)
        
        cfgfile = open(self.cfg_file, 'r')
        cfgln = cfgfile.readlines()
        cfgfile.close()
        
        for cl in cfgln:
            par, val = self.read_line(cl)
            if (par == 'SerialNum'):
                self.SerialNum = val
            elif (par == 'boardname'):
                self.boardname = val
            elif (par == 'boardversion'):
                self.boardversion = int(val)
            elif (par == 'NumETHPort'):
                self.NumETHPort = int(val)
            elif (par == 'ExtPTPHWUARTPort'):
                self.ExtPTPHWUARTPort = val
            elif (par == 'ExtGPSHWUARTPort'):
                self.ExtGPSHWUARTPort = val
            elif (par == 'ExtPTPHWPresent'):
                self.ExtPTPHWPresent = int(val)
            elif (par == 'ExtPTPHWUARTBaud'):
                self.ExtPTPHWUARTBaud = int(val)
            elif (par == 'ExtPTPHWUARTEcho'):
                self.ExtPTPHWUARTEcho = int(val)
            elif (par == 'ExtGPSHWPresent'):
                self.ExtGPSHWPresent = int(val)
            elif (par == 'ExtGPSHWUARTBaud'):
                self.ExtGPSHWUARTBaud = int(val)
            elif (par == 'ExtGPSHWUARTEcho'):
                self.ExtGPSHWUARTEcho = int(val)
            elif (par == 'SimCardPresent'):
                self.SimCardPresent = int(val)
            elif (par == 'SDCardPresent'):
                self.SDCardPresent = int(val)
            elif (par == 'USBPortPresent'):
                self.USBPortPresent = int(val)
            elif (par == 'PCIEPresent'):
                self.PCIEPresent = int(val)
            elif (par == 'EthSwitchPresent'):
                self.EthSwitchPresent = int(val)
            elif (par == 'PtpVcoDriftPresent'):
                self.PtpVcoDriftPresent = int(val)
            elif (par == 'PtpVcoDriftValue'):
                self.PtpVcoDriftValue = int(val)
            elif (par == ''):
                continue
            else:
                pass

    def read_bb_eeprom(self):
        
        _ = self.enb.tn.read_until('login'.encode("ascii"), 5)
        print ''
        print 'Serial Number: ' + self.do_driver_reading('rsn')
        print 'Board Name: ' + self.do_driver_reading('rbn')
        print 'Board Version: ' + self.do_driver_reading('rbv')
        print 'Number ETH Port: ' + self.do_driver_reading('rne')
        print 'EXT PTP Present: ' + self.do_driver_reading('rpp')
        print 'EXT PTP UART Port: ' + self.do_driver_reading('rpo')
        print 'EXT PTP UART Baud: ' + self.do_driver_reading('rpb')
        print 'EXT PTP UART Echo: ' + self.do_driver_reading('rpe')
        print 'EXT GPS Present: ' + self.do_driver_reading('rgp')
        print 'EXT GPS UART Port: ' + self.do_driver_reading('rgo')
        print 'EXT GPS UART Baud: ' + self.do_driver_reading('rgb')
        print 'EXT GPS UART Echo: ' + self.do_driver_reading('rge')
        print 'SimCard Present: ' + self.do_driver_reading('rsp')
        print 'SD Card Present: ' + self.do_driver_reading('rdp')
        print 'USB Port Present: ' + self.do_driver_reading('rup')
        print 'PCIE Present: ' + self.do_driver_reading('rcp')
        print 'ETH Switch Present: ' + self.do_driver_reading('rwp')
        print 'PTP VCO Drift Present: ' + self.do_driver_reading('rvp')
        print 'PTP VCO Drift Value: ' + self.do_driver_reading('rvv')
        print ''
    
    def convert_baud_rate(self, baudNum):
        
        res = 0
        
        if baudNum == 1:
            res = 4800
        elif baudNum == 2:
            res = 9600
        elif baudNum == 3:
            res = 19200
        elif baudNum == 4:
            res = 38400
        elif baudNum == 5:
            res = 57600
        elif baudNum == 6:
            res = 115200
        elif baudNum == 7:
            res = 230400
        else:
            res = -1
            
        return str(res).encode('ascii')
    
    def do_driver_reading(self, cmd):
        
        self.enb.tn_write('Option', "oncpu 0 /usr/bin/" + self.cfg.rf_driver)
        self.enb.tn_write('option', "be")
        self.enb.tn_write(':', cmd)
        res = self.enb.tn.read_until('*'.encode("ascii"), 5)
        self.enb.tn_write('/usr/bin', "q")
        #print 'get:' + res + ':got'
        start = 0
        start = res.find("= ".encode("ascii"), start)
        start += len("= ")
        data = res[start:res.find('*'.encode("ascii"), start)].strip()
        return data

    def write_bb_serial_number(self, serialNum):
        
        self.enb.enb_bb_eeprom_edit_record('wsn', serialNum)

    def write_bb_eeprom(self):
        
        #self.enb.enb_bb_eeprom_edit_record('wsn', self.SerialNum)
        self.enb.enb_bb_eeprom_edit_record('wbn', self.boardname)
        self.enb.enb_bb_eeprom_edit_record('wbv', str(self.boardversion))
        self.enb.enb_bb_eeprom_edit_record('wne', str(self.NumETHPort))
        self.enb.enb_bb_eeprom_edit_record('wpp', str(self.ExtPTPHWPresent))
        self.enb.enb_bb_eeprom_edit_record('wpo', self.ExtPTPHWUARTPort)
        self.enb.enb_bb_eeprom_edit_record('wpb', str(self.ExtPTPHWUARTBaud))
        self.enb.enb_bb_eeprom_edit_record('wpe', str(self.ExtPTPHWUARTEcho))
        self.enb.enb_bb_eeprom_edit_record('wgp', str(self.ExtGPSHWPresent))
        self.enb.enb_bb_eeprom_edit_record('wgo', self.ExtGPSHWUARTPort)
        self.enb.enb_bb_eeprom_edit_record('wgb', str(self.ExtGPSHWUARTBaud))
        self.enb.enb_bb_eeprom_edit_record('wge', str(self.ExtGPSHWUARTEcho))
        self.enb.enb_bb_eeprom_edit_record('wsp', str(self.SimCardPresent))
        self.enb.enb_bb_eeprom_edit_record('wdp', str(self.SDCardPresent))
        self.enb.enb_bb_eeprom_edit_record('wup', str(self.USBPortPresent))
        self.enb.enb_bb_eeprom_edit_record('wcp', str(self.PCIEPresent))
        self.enb.enb_bb_eeprom_edit_record('wwp', str(self.EthSwitchPresent))
        self.enb.enb_bb_eeprom_edit_record('wvp', str(self.PtpVcoDriftPresent))
        self.enb.enb_bb_eeprom_edit_record('wvv', str(self.PtpVcoDriftValue))
        
    def erase_bb_eeprom(self):
        
        answer = raw_input("Are you sure erase EEPROM?(y or n):")
        
        if (answer == 'y') or (answer == 'Y'):
            print 'earsing baseboard EEPROM...'
            self.enb.tn_write('Option', "oncpu 0 /usr/bin/" + self.cfg.rf_driver)
            self.enb.tn_write('option', "be")
            self.enb.tn_write(':', 'eei')
            self.enb.tn_write('option', 'y', 60)
            self.enb.tn_write('/usr/bin', "q", 3)
            print 'erase done'
