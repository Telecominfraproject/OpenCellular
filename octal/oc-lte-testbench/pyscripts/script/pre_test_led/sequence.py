import subprocess
import click
import time
import signal
import os
import numpy
import time, sys, os, traceback, types
from opentest.script import bringup_scripts

import opentest.script.testrunner as testrunner
import opentest.equipment.i2cexedirectory as i2cpod_dir
from threading import Thread


CURRENT_DIR = i2cpod_dir.directory()
STREAM_TIME = 0.2
LED_OFF = 255
led_colors = ['G', 'R']
D11 = {'SA':"0x3E", 'GG':"0x11",'GR':"0x11",'G':247,'R':251}
D9  = {'SA':"0x3E", 'GG':"0x10",'GR':"0x10",'G':223,'R':239}
D1 = {'SA':"0x3E", 'GG':"0x11",'GR':"0x11",'G':253,'R':254}
D2 = {'SA':"0x3E", 'GG':"0x10",'GR':"0x11",'G':127,'R':191}
D3  = {'SA':"0x3E", 'GG':"0x10",'GR':"0x10",'G':253,'R':254}
D4  = {'SA':"0x3E", 'GG':"0x10",'GR':"0x10",'G':247,'R':251}

D5  = {'SA':"0x3F", 'GG':"0x11",'GR':"0x11",'G':253,'R':247}
D6  = {'SA':"0x3F", 'GG':"0x11",'GR':"0x11",'G':239,'R':251}
D7  = {'SA':"0x3F", 'GG':"0x11",'GR':"0x11",'G':223,'R':254}
D8  = {'SA':"0x3F", 'GG':"0x11",'GR':"0x11",'G':127,'R':191}
D10  = {'SA':"0x3F", 'GG':"0x10",'GR':"0x10",'G':253,'R':254}
D12  = {'SA':"0x3F", 'GG':"0x10",'GR':"0x10",'G':247,'R':251}

DIO_DICT =[D11, D9, D1, D2, D3, D4, D5, D6, D7, D8, D10, D12]
IMPEDANCE_LED_TEST = [(1, '3_3V', 'P4', r"impedance_test.jpg" )]


class BringUp(testrunner.TestSuite):

    @testrunner.testcase("Impedance Test", critical=True)
    def LED_TSC001(self, context):
        bringup_scripts.impedance_test(context, IMPEDANCE_LED_TEST, 'TSC001', base_path=context.IMAGE_FOLDER)

    @testrunner.testcase("Power-Up", critical=True)
    def LED_PRP001(self, context):
        context.wait_tester_feedback("Please connect the LED board to the Diolan")

    @testrunner.testcase("Temperature sensor test", critical=True)
    def LED_TSC002(self, context):
        with context.criteria.evaluate_block() as criteria_block:
            temperature = get_temp()
            criteria_block.evaluate('TSC002_TEMPERATURE', temperature)
            context.logger.info(temperature)

class Sequence(testrunner.TestSuite):

    @testrunner.testcase("Led test sequence")
    def LED_TSC003(self, context):
        reset()
        start_u2c_12()
        all_led_off()
        context.wait_tester_feedback("Confirm that all LEDs are OFF.",fb_type=testrunner.FeedbackType.PASS_FAIL)
        all_led_on()
        context.wait_tester_feedback("Confirm that all LEDs are ORANGE.",fb_type=testrunner.FeedbackType.PASS_FAIL)
        all_led_green()
        context.wait_tester_feedback("Confirm that all LEDs are GREEN.",fb_type=testrunner.FeedbackType.PASS_FAIL)
        all_led_red()
        context.wait_tester_feedback("Confirm that all LEDs are RED.",fb_type=testrunner.FeedbackType.PASS_FAIL)
        all_led_off()
        context.wait_tester_feedback("The test will cycle all LED and turn them GREEN and then RED. Watch carefully for errors")
        while True:
            try :
                cycle('G')
                all_led_off()
                cycle('R')
                all_led_off()
                context.wait_tester_feedback("Confirm that all GREEN and RED LED are working",fb_type= testrunner.FeedbackType.PASS_FAIL_RETRY)
                break
            except testrunner.TesterRetry:
                pass
        all_led_off()

        time.sleep(1)
        all_led_off()

    @testrunner.testcase("Led test cleanup")
    def LED_CLN001(self, context):
        all_led_off()


def cycle(led_color):
    led_position = 0
    while led_position < 12:
        led_stream(led_position,led_color)
        led_position = led_position + 1
        time.sleep(STREAM_TIME)



def start_u2c_12():
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x0A', 'FF']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x0B', 'FF']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x0A', 'FF']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x0B', 'FF']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    all_led_off()

def end_u2c_12():
    all_led_off()
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x0A', '00']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x0B', '00']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x0A', '00']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x0B', '00']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)

def all_led_on():
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x10', '00']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x11', '00']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x10', '00']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x11', '00']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)


def all_led_red():
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x11', 'BA']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x10', 'EA']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x11', 'B2']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x10', 'FA']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)


def all_led_green():
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x11', 'F5']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x10', '55']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x11', '4D']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x10', 'F5']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)

def all_led_off():
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x10', str(hex(LED_OFF))]
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x11', str(hex(LED_OFF))]
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x10', str(hex(LED_OFF))]
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x11', str(hex(LED_OFF))]
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    time.sleep(0.1)

def led_stream(diode,led_color):
    D1,D2,D3 = get_led(diode)
    xE11, xE10, xF11, xF10 = get_valeur(led_color, D1, D2, D3)
    set_valeur(xE11, xE10, xF11, xF10)

def get_led(diode):
    DIO1,DIO2,DIO3 = diode, (diode -1),(diode -2)
    DIO2, DIO3 = DIO1,DIO1
    return DIO1, DIO2, DIO3

def get_valeur(led_color, D1, D2, D3):
    xE11,xE10,xF11,xF10= 255,255,255,255
    position = [D1,D2,D3]
    for diode in position:
        if led_color == 'G':
            general_address =DIO_DICT[diode]['GG']
        if led_color == 'R':
            general_address =DIO_DICT[diode]['GR']

        slave_address = DIO_DICT[diode]['SA']

        if slave_address == "0x3E" and general_address == "0x11":
            xE11 = xE11 & DIO_DICT[diode][led_color]
        if slave_address == "0x3E" and general_address == "0x10":
            xE10 = xE10 & DIO_DICT[diode][led_color]
        if slave_address == "0x3F" and general_address == "0x11":
            xF11 = xF11 & DIO_DICT[diode][led_color]
        if slave_address == "0x3F" and general_address == "0x10":
            xF10 = xF10 & DIO_DICT[diode][led_color]
    return xE11, xE10, xF11, xF10

def set_valeur(xE11, xE10, xF11, xF10):
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x10', str(hex(xE10))]
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3E' , '-a:1:0x11', str(hex(xE11))]
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x10', str(hex(xF10))]
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3F' , '-a:1:0x11', str(hex(xF11))]
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)

def reset():
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3f' , '-a:1:0x7D', '12']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3f' , '-a:1:0x7D', '34']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3e' , '-a:1:0x7D', '12']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'w', '-s:0x3e' , '-a:1:0x7D', '34']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)


def get_temp():
    call_list = [os.path.join(CURRENT_DIR, "i2c.exe"), 'r', '-s:0x4F' , '-a:1:0x00', '-d:2']
    proc = subprocess.Popen(call_list, stdout=subprocess.PIPE, shell=False)
    read = proc.stdout.read()
    read = read.replace(" ","")
    numb = int(read,16)
    numb = numpy.int16(numb)
    numb = float(numb)
    numb = numb/(32*8)
    return numb
