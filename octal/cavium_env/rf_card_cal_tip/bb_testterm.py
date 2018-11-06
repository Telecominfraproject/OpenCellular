#!/usr/bin/python

# Very simple serial terminal
# (C)2002-2004 Chris Liechti <cliecht@gmx.net>

import test_config
#weison import sys, os, serial, threading, getopt
import sys, os, threading, getopt
#weison import enhancedserial
from time import sleep
from im_calibration import Calibration

EXITCHARCTER = '\x04'   #ctrl+D
FUNCCHARCTER = '\x01'   #ctrl+A
CONVERT_CRLF = 2
CONVERT_CR   = 1
CONVERT_LF   = 0

if os.name == 'nt':
    print "OS = Windows"
    import msvcrt
elif os.name == 'posix':
    print "OS = Linux"
    import termios
else:
    raise "Sorry no implementation for your platform (%s) available." % sys.platform
        
class TestTerm(Calibration):

    def __init__(self, osname):

        #initialize with defaults
        #self.port  = 'COM11' #'/dev/ttyUSB0'
        #self.baudrate = 115200
        #self.pingip = '10.18.104.189'   #'192.168.166.202' #'10.102.81.5'
        self.port = type(self).cfg.bb_port
        self.baudrate = type(self).cfg.bb_baudrate
        self.pingip = type(self).cfg.bb_pingip
        self.echo = 0
        self.convert_outgoing = CONVERT_CR
        self.newline = '';
        self.rtscts = 0
        self.xonxoff = 0
        self.repr_mode = 0
        self.os_name = osname
    
    def getkey(self):
        
        return raw_input()
    
    def getkey_org(self):
        
        if self.os_name == 'nt':
            
            while True:
                if self.echo:
                    z = msvcrt.getche()
                else:
                    z = msvcrt.getch()
                    
                if z == '\0' or z == '\xe0':    #functions keys
                    msvcrt.getch()
                else:
                    if z == '\r':
                        return '\n'
                    return z
                
        elif self.os_name == 'posix':
            
            fd = sys.stdin.fileno()
            old = termios.tcgetattr(fd)
            new = termios.tcgetattr(fd)
            new[3] = new[3] & ~termios.ICANON & ~termios.ECHO
            new[6][termios.VMIN] = 1
            new[6][termios.VTIME] = 0
            termios.tcsetattr(fd, termios.TCSANOW, new)
            self.ts = ''    # We'll save the characters typed and add them to the pool.
    
        def clenaup_console():
            
            if self.os_name == 'posix':
                termios.tcsetattr(fd, termios.TCSAFLUSH, old)
    
    def reader(self):
        """loop forever and copy serial->console"""
        
        while 1:
            try:
                data = self.ts.read()
            except:
                e = sys.exc_info()[0]
                print "--- Exception ignored: ", sys.exc_info()[0]
    
            if self.repr_mode:
                sys.stdout.write(repr(data)[1:-1])
            else:
                sys.stdout.write(data)
                
            sys.stdout.flush()
    
    def writer(self):
        """loop and copy console->serial until EOF character is found"""
        
        c = FUNCCHARCTER
        while True:
            if c == EXITCHARCTER:
                break                       #exit app
            elif c == FUNCCHARCTER:
                self.menu()
                return
            elif c == '\n':
                self.ts.write(self.newline)
            else:
                self.ts.write(c)                  #send character
                
            c = self.getkey()
            #print(":".join(a.encode('hex') for a in c))
    
    def menu(self):
        
        ret=' ';
        while True:
            print("--- Starting a new test...")
            print("--- Reset Zen to initial test procedure...")
            self.ts.wait_until("Clearing DRAM......", timeout=0);
            self.ts.write(self.newline + self.newline);
            self.ts.wait_until("Octeon zen=>","Octeon titan_cnf7130=>", timeout=0);
            
            ret = self.uboot_menu(ret)
            if 1 == ret: return;
            self.ts.wait_until("LSM login:", timeout=0);
            self.ts.write('root' + self.newline);
            
            ret = self.kernel_menu(ret)
            if 1 == ret: return;
    
    def kernel_menu(self, batch_cmd = ' '):
        
        sleep(0.1)
        bac=batch_cmd;
        
        while 1:
            print("\n")
            print("---  1. Network: Ping "+self.pingip)
            print("---  2. GPIO LED: blink(GPIO 0, 6, 7)")
            print("---  3. Flash write/read test(/dev/mtdblock7)")
            #print("---  4. SD/MMC write/read test(/dev/mmcblk0)")
            print("---  4. GPS query test")
            print("---  r. Restart test")
            print("---  q. Quit")
            print("--- Please select function:")
            
            if len(bac) > 1:
                c = bac[0]
                bac = bac[1:]
            else: 
                c = self.getkey()
            
            #c = self.getkey()
             
            if c == '1' :
                
                print("\n--- Starting Ping test...")
                self.ts.write(self.newline+'ping -c3 -q '+self.pingip+self.newline);
                ret=self.ts.wait_until("0 received","1 received","2 received","3 received",timeout=5);
                sleep(0.1)
                
                if ret > 0 : 
                    print("\n\n--- RESULT : Ping test PASSED\n")
                else:
                    print("\n\n--- RESULT : Ping test FAILED!!\n")
                    bac=' '
                    
            elif c == '2':
                
                for a in [0,6,7]:
                    print("\n--- GPIO LED %d blink test..."%a)
                    self.ts.write(self.newline+'toggle 1 %d '%a+self.newline);
                    sleep(0.1)
                    print("--- Please check if GPIO %d LED are blinking..."%a)
                    print("--- Press Y/y if the LED is blinking, others mean failed...")
                    c = self.getkey()
                    
                    if c == 'y' or c == 'Y':
                        print("\n\n--- RESULT: LED %d test PASSED"%a)
                    else:
                        print("\n\n--- RESULT: LED %d test FAILED"%a)
                        self.ts.write('\x03');
                        
            elif c == '3':
                
                print("\n--- Starting flash test...")
                self.ts.write(self.newline + 
                              'DEV=/dev/mtdblock7; dd if=/dev/urandom of=test bs=1M count=1; \
                              dd if=test of=$DEV bs=1M count=1; dd if=$DEV of=test1 bs=1M count=1;\
                               cmp test test1 && echo Files are the same!!;' + self.newline);
                ret = self.ts.wait_until("Files are the same!!","differ:",timeout=30);
                sleep(0.1)
                
                if ret == 0: 
                    print ("\n\n--- RESULT : flash test PASSED\n")
                else:
                    print("\n\n--- RESULT : flash test FAILED\n")
                    bac=' '
                    
            elif c == '4':
                
                print("\n--- Starting GPS query test...")
                self.ts.write(self.newline+' cat > /gps_test.sh << EOF'+self.newline+
                '#!/bin/sh'+self.newline+
                'i=0'+self.newline+
                'cat /dev/ttyS1 |grep -a -m 1 "GP" > /tmp/gps_result &'+self.newline+
                'while true'+self.newline+
                'do'+self.newline+
                '    sleep 1'+self.newline+
                '    result=\`cat /tmp/gps_result\`'+self.newline+
                '        if [ -z "\$result" ]; then '+self.newline+
                '            if [ "\$i" -ge "5" ];then echo "gps module test is failed!!"; break; fi '+self.newline+
                '            '+self.newline+
                '        else'+self.newline+
                '            echo "gps module test is passed!!";'+self.newline+
                '            break;'+self.newline+
                '        fi'+self.newline+
                '        true \$(( i++ ))'+self.newline+
                'done'+self.newline+
                'exit 0'+self.newline+
                'EOF'+self.newline);
                ret=self.ts.wait_until("EOF");
                ret=self.ts.wait_until("EOF");
                self.ts.write('sh /gps_test.sh'+self.newline);
                ret=self.ts.wait_until("gps module test is failed","gps module test is passed");
                sleep(0.1)
                
                if ret == 1: print ("\n\n--- RESULT : GPS query test PASSED!\n")
                elif ret == 0: print("\n\n--- RESULT : GPS query test FAILED\n")
                else:
                    print("\n\n--- RESULT : GPS query test FAILED (timeout) \n")
                    bac=' '
                    
            elif c == 'r' or c == 'R':
                print("--- Restarting test!!")
                return ' '
            
            elif c == 'q' or c == 'Q':
                print("--- Quit test mode!!")
                return 1
            
            else:
                print("--- Error: unknown function!!")
                
        return ' '
    
    def uboot_menu(self, batch_cmd = ' '):
        
        sleep(0.1)
        bac = batch_cmd;
        
        while 1:
            print("\n\n")
            print("---  1. Run all the tests")
            print("---  2. Reset button")
            print("---  3. Memory: u-boot mtest")
            print("---  4. BB/RF Temperature sensor test")
            print("---  5. BB EEPROM test")
            print("---  6. RF EEPROM test")
            print("---  7. DSP test")
            print("---  k. Boot to kernel")
            print("---  r. Reset u-boot environment variables")
            print("---  q. Quit")
            print("--- Please select function:")
            
            if len(bac) > 1:
                c = bac[0]
                bac = bac[1:]
            else: 
                c = self.getkey()
            
            print "selection = " + c
            
            if c == '1' :
                print("\n--- Run all the tests")
                #bac='234567k1234r '
                bac='234567k1234 '
                
            elif c == '2':
                print("\n--- Starting Reset button test...\n")
                print("Please press reset button in 10 seconds...\n")
                ret=self.ts.wait_until("Clearing DRAM......",timeout=20);
                if ret>=0 :
                    self.ts.write(self.newline+self.newline);
                    ret=self.ts.wait_until("Octeon zen=>","Octeon titan_cnf7130=>", timeout=10);
                    if ret<0 : print("\n\n--- RESULT : Reset button test FAILED (for wait timeout)\n")
                    else : print("\n\n--- RESULT : Reset button test PASSED\n")
                    
            elif c == '3':
                print("\n--- Starting memroy test...")
                self.ts.write('echo '+self.newline+'mtest 80100000 80ffffff 5a5aa5a5 a'+self.newline);
                ret=self.ts.wait_until("0 errors","errors",timeout=15);
                sleep(0.1)
                if ret==0 : print("\n\n--- RESULT : Memory test PASSED\n")
                elif ret==1 : print("\n\n--- RESULT : Memory test FAILED\n")
                elif ret<0 : print("\n\n--- RESULT : Memory test FAILED (for wait timeout)\n")
                
            elif c == '4':
                print("\n--- Starting BB/RF Temperature sensor test...\n")
                self.ts.write('echo '+self.newline+'dtt'+self.newline);
                ret=self.ts.wait_until("DTT0: Failed init!", "DTT1: Failed init!", "C")
                sleep(0.5)
                
                if ret==2 : print("\n\n--- RESULT : BB/RF Temperature sensor test PASSED\n")
                elif ret>=0 : print("\n\n--- RESULT : BB/RF Temperature sensor test FAILED\n")
                else : print("\n\n--- RESULT : BB/RF Temperature sensor test FAILED (for wait timeout)\n")
                
            elif c == '5':
                print("\n--- Starting BB EEPROM test...\n")
                self.ts.write('echo '+self.newline+'i2c md 50 0.2 0x100'+self.newline);
                ret=self.ts.wait_until("00f0:","Error");
                sleep(0.1)
                
                if ret==0 : print("\n\n--- RESULT : BB EEPROM test PASSED\n")
                elif ret==1 : print("\n\n--- RESULT : BB EEPROM test FAILED\n")
                elif ret<0 : print("\n\n--- RESULT : BB EEPROM test FAILED (for wait timeout)\n")
                
            elif c == '6':
                print("\n--- Starting RF EEPROM test...\n")
                self.ts.write('echo '+self.newline+'i2c md 51 0.2 0x100'+self.newline);
                ret=self.ts.wait_until("00f0:","Error");
                sleep(0.1)
                
                if ret==0 : print("\n\n--- RESULT : RF EEPROM test PASSED\n")
                elif ret==1 : print("\n\n--- RESULT : RF EEPROM test FAILED\n")
                elif ret<0 : print("\n\n--- RESULT : RF EEPROM test FAILED (for wait timeout)\n")
                
            elif c == '7':
                print("\n--- Starting DSP query test...\n")
                self.ts.write('echo '+self.newline+'echo CVMX_EOI_CTL_STA; md64 0x0001180013000000 1;'+self.newline);
                ret=self.ts.wait_until("8001180013000000:");
                
                if ret < 0 :
                    print("\n\n--- RESULT : DSP query test for reading CVMX_EOI_CTL_STA FAILED (for wait timeout)\n")
                    break
                    sleep(0.1)
                    self.ts.write('echo '+self.newline+'mw64 0x0001180013000000 0x800 1;echo CVMX_EOI_CTL_STA; md64 0x0001180013000000 1;'+self.newline);
                    ret=self.ts.wait_until("8001180013000000: 0000000000000800");
                    
                if ret < 0 :
                    print("\n\n--- RESULT : DSP query test for writing CVMX_EOI_CTL_STA FAILED 1 (for wait timeout)\n")
                    break
                    sleep(0.1)
                    self.ts.write('echo '+self.newline+'mw64 0x0001180013000000 0x802 1;echo CVMX_EOI_CTL_STA; md64 0x0001180013000000 1;'+self.newline);
                    ret=self.ts.wait_until("8001180013000000: 0000000000000802");
                    
                if ret < 0 :
                    print("\n\n--- RESULT : DSP query test for writing CVMX_EOI_CTL_STA FAILED 2 (for wait timeout)\n")
                    break
                    sleep(0.1)
                    self.ts.write('echo '+self.newline+'echo BBP_RSTCLK_CLKENB1_STATE 0x00010F0000844430; read64l 0x00010F0000844430;'+self.newline);
                    ret=self.ts.wait_until("0x80010f0000844430:");
                    
                if ret < 0 :
                    print("\n\n--- RESULT : DSP query test for reading BBP_RSTCLK_CLKENB1_STATE FAILED (for wait timeout)\n")
                    break
                    sleep(0.1)
                    self.ts.write('echo '+self.newline+'write64l 0x00010F0000844438 0xff; echo BBP_RSTCLK_CLKENB1_STATE 0x00010F0000844430; read64l 0x00010F0000844430;'+self.newline);
                    ret=self.ts.wait_until("0x80010f0000844430: 0x0");
                    
                if ret < 0 :
                    print("\n\n--- RESULT : DSP query test for clearing BBP_RSTCLK_CLKENB1_STATE FAILED (for wait timeout)\n")
                    break
                    sleep(0.1)
                    self.ts.write('echo '+self.newline+'write64l 0x00010F0000844434 0xff; echo BBP_RSTCLK_CLKENB1_STATE 0x00010F0000844430; read64l 0x00010F0000844430;'+self.newline);
                    ret=self.ts.wait_until("0x80010f0000844430: 0xff");
                    
                if ret < 0 :
                    print("\n\n--- RESULT : DSP query test for setting BBP_RSTCLK_CLKENB1_STATE FAILED (for wait timeout)\n")
                    break
                    sleep(0.1)
                    print("\n\n--- RESULT : DSP query test PASSED\n")
                    
            elif c == 'k':
                print("\n--- Running bootcmd...\n")
                self.ts.write('echo '+self.newline+'run bootcmd'+self.newline);
                return bac;
            
            elif c == 'r':
                print("\n--- Resetting u-boot environment variables...")
                self.ts.write('echo '+self.newline+'setenv blabla xxx; savenv'+self.newline);
                
            elif c == 'q' or c == 'Q':
                print("--- Quit test mode!!")
                return 1
            
            else:
                print("--- Error: unknown function!!")
                
        return 0
    
    
    #print a short help message
    def usage(self):
        
        sys.stderr.write("""USAGE: %s [options]
        Miniterm - A simple terminal program for the serial port.
    
        options:
        -p, --port=PORT: port, a number, default = 0 or a device name
        -b, --baud=BAUD: baudrate, default 9600
        -r, --rtscts:    enable RTS/CTS flow control (default off)
        -x, --xonxoff:   enable software flow control (default off)
        -e, --echo:      enable local echo (default off)
        -c, --cr:        do not send CR+LF, send CR only
        -n, --newline:   do not send CR+LF, send LF only
        -D, --debug:     debug received data (escape nonprintable chars)
    
        """ % (sys.argv[0], ))

    def run(self):
        
        #parse command line options
        try:
            opts, args = getopt.getopt(sys.argv[1:],
                "hp:b:rxecnD",
                ["help", "port=", "baud=", "rtscts", "xonxoff", "echo",
                "cr", "newline", "debug"]
            )
        except getopt.GetoptError:
            # print help information and exit:
            self.usage()
            sys.exit(2)
        
        for o, a in opts:
            if o in ("-h", "--help"):       #help text
                self.usage()
                sys.exit()
            elif o in ("-p", "--port"):     #specified port
                try:
                    self.port = int(a)
                except ValueError:
                    self.port = a
            elif o in ("-b", "--baud"):     #specified baudrate
                try:
                    self.baudrate = int(a)
                except ValueError:
                    raise ValueError, "Baudrate must be a integer number, not %r" % a
            elif o in ("-r", "--rtscts"):
                self.rtscts = 1
            elif o in ("-x", "--xonxoff"):
                self.xonxoff = 1
            elif o in ("-e", "--echo"):
                self.echo = 1
            elif o in ("-c", "--cr"):
                self.convert_outgoing = CONVERT_CR
            elif o in ("-n", "--newline"):
                self.convert_outgoing = CONVERT_LF
            elif o in ("-D", "--debug"):
                self.repr_mode = 1
    
        if self.convert_outgoing == CONVERT_CRLF:
            self.newline = '\r\n'         #make it a CR+LF
        elif self.convert_outgoing == CONVERT_CR:
            self.newline = '\r'           #make it a CR
        elif self.convert_outgoing == CONVERT_LF:
            self.newline = '\n'           #make it a LF
    
        #open the port
        try:
            self.ts = enhancedserial.EnhancedSerial(self.port, self.baudrate, 
                                                    rtscts=self.rtscts, xonxoff=self.xonxoff)
        except:
            sys.stderr.write("Could not open port %s \n"%self.port)
            sys.exit(1)
            
        #sys.stderr.write("--- type Ctrl-D or Ctrl-C to quit\n")
        #sys.stderr.write("--- type Ctrl-A to enter test mode\n")
        
        #start serial->console thread
        r = threading.Thread(target=self.reader)
        r.setDaemon(True)
        r.start()
        #and enter console->serial loop
        self.writer()
        r.join(0)
        #sys.stderr.write("\n--- exit ---\n")

def main():

    tt = TestTerm(os.name)
    tt.run()

if __name__ == '__main__':
    main()

