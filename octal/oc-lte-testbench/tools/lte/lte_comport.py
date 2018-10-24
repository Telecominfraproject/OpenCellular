
from opentest.server.service.iointerface_service import COMServiceClient, COMException

class LTECOMServiceClient(COMServiceClient):
    def get_ip(self, eth):
        commandstr = ('ifconfig ' + str(eth) + ' | grep "Bcast" | cut -d " " -f 12\n')
        returned_str = self.com_target(commandstr,"addr:",1)
        cleanstr = self._get_second_to_last_line(returned_str).replace("addr:", "")
        return cleanstr

    def get_mac(self, eth):
        commandstr = ("ifconfig " + str(eth) + " | grep 'HWaddr ' | cut -b 39-\n")
        returned_str = self.com_target(commandstr,"\n",1)
        cleanstr = self._get_second_to_last_line(returned_str)
        return cleanstr

    def detect(self, target, timeout=300, **kwargs):
        targetstr_response = self.com_target("",target, timeout, **kwargs)
        success = self.is_target_in_response(target, targetstr_response)
        return success

    def login(self, username="root", password="nuranwireless", user_target="gsm@", root_target="root@", firsttout=30, secondtout=30, thirdtout=12, login_target="login:", password_target="Password:", linux_target="Linux version ", sustr="su -"):

        targetstr_response = self.com_target("",linux_target, firsttout)

        all_target = login_target + "|" + password_target + "|" + root_target + "|" + user_target
        usernamecmd = username + "\n"
        passwordcmd = password + "\n"
        sucmd = sustr + "\n"

        targetstr_response = self.com_target("\n",all_target, secondtout)

        success = 0
        if targetstr_response is "":
            print "Failure to login"
            return 0

        if password_target in targetstr_response:
            #We dont know the entered login username, restart with empty password.
            targetstr_response = self.com_target("\n", all_target, thirdtout)

        if login_target in targetstr_response:
            #Enter the username
            targetstr_response = self.com_target(usernamecmd, all_target, thirdtout)

        if password_target in targetstr_response:
            #Now enter the password
            targetstr_response = self.com_target(passwordcmd, all_target, thirdtout)

        if root_target in targetstr_response:
            #Success !
            success = 1
        else:
            if user_target in targetstr_response:
                #Now enter su privilege
                targetstr_response = self.com_target(sucmd, all_target, thirdtout)

            if password_target in targetstr_response:
                #Now enter the su privilege password
                targetstr_response = self.com_target(passwordcmd, all_target, thirdtout)

            if root_target in targetstr_response:
                #Success !
                success = 1

        return success

    # From powerup, enter u-boot, or if already in u-boot, also working...
    def enteruboot(self, waitstr="MMC:   ", password="c", ubootprompt="Octeon zen=>"):

        all_target = waitstr + "|" + ubootprompt

        success = 0
        targetstr_response = self.com_target("",all_target, 20)

        if ubootprompt in targetstr_response:
            #Success !
            success = 1
        else:
            if waitstr in targetstr_response:
                #Ok, we are ready to provide the password
                targetstr_response = self.com_target(password, all_target, 15)
                if ubootprompt in targetstr_response:
                    #Success !
                    success = 1
            else:
                if targetstr_response is "":
                                        #Ok, are we already in u-boot?
                    targetstr_response = self.com_target("#\n", all_target, 3)
                    if ubootprompt in targetstr_response:
                        #Success !
                        success = 1

        return success

    # WARNING: this function works only if already in u-boot
    # setup u-boot environment variables from provided envdict dictionnary, save it if save flag is True
    def setupuboot(self, envdict={}, ubootprompt="Octeon zen=>", save=False):

        all_target = ubootprompt
        for key, value in envdict.iteritems():
            if value is "":
                myenvvar = "setenv " + str(key) + "\n"
                printmyvar = "printenv " + str(key) + "\n"
                myvarresult = "\"" + str(key) + "\" not defined"
            else:
                myenvvar = "setenv " + str(key) + " '" + str(value) + "'\n"
                printmyvar = "printenv " + str(key) + "\n"
                myvarresult = str(key) + "=" + str(value)
            targetstr_response = self.com_target(myenvvar, all_target, 3, poll_rate=0.01)
            if targetstr_response is "":
                raise COMException('Failure to setupuboot variable: {0},{1}'.format(key, value))

            if not ubootprompt in targetstr_response:
                raise COMException('Error no prompt in setting a variable in setupuboot: {0},{1}'.format(key, value))

            targetstr_response = self.com_target(printmyvar, all_target, 3, poll_rate=0.01)
            if targetstr_response is "":
                raise COMException('Failure to readback variable: {0},{1}'.format(key, value))

            if not ubootprompt in targetstr_response:
                raise COMException('Error no prompt in reading a variable in setupuboot: {0},{1}'.format(key, value))

            if not myvarresult in targetstr_response:
                raise COMException('Error incorrect variable value readback in setupuboot: {0},{1}'.format(key, value))

        if save:
            self.execubootcmd("saveenv", ubootprompt, 30)

        success = 1
        return success

    # Reset system if already in u-boot
    def resetfromuboot(self, resetstring="Jumping to start of image at address"):

        all_target = resetstring

        targetstr_response = self.com_target("reset\n",all_target, 2)

        success = 0
        if targetstr_response is "":
            print "Failure to resetfromuboot"
            return 0

        if all_target in targetstr_response:
            #Success !
            success = 1

        return success

    # WARNING: this function works only if u-boot is compiled to support hush command parser and already in u-boot
    # setup u-boot with built-in default environment variables, save it if save flag is True
    def setdefaultubootvar(self, ubootprompt="Octeon zen=>", save=False, ignore_list=[]):

        ok_ans1 = "^C"
        ok_ans2 = "OK^!"
        ok_ans_nr = "^C\r\nOK^!"
        ok_ans_n = "^C\nOK^!"
        cmd_ans = " && echo " + ok_ans1 + " && echo " + ok_ans2 + "\n"
        all_target = ubootprompt

        # older u-boot command format
        targetstr_response = self.com_target("env default -f" + cmd_ans, all_target, 3, poll_rate=0.01)
        if targetstr_response is "":
            raise COMException('Failure to set default environment -f in setdefaultubootvar')

        if not ubootprompt in targetstr_response:
            raise COMException('Error no prompt restoring default environment -f in setdefaultubootvar')

        if ((not ok_ans1 in targetstr_response) or (not ok_ans2 in targetstr_response)):
            # newer u-boot command format
            targetstr_response = self.com_target("env default -a" + cmd_ans, all_target, 3, poll_rate=0.01)
            if targetstr_response is "":
                raise COMException('Failure to set default environment -a in setdefaultubootvar')
            if not ubootprompt in targetstr_response:
                raise COMException('Error no prompt restoring default environment -a in setdefaultubootvar')
            if ((not ok_ans1 in targetstr_response) or (not ok_ans2 in targetstr_response)):
                raise COMException('Error in restoring default environment -a in setdefaultubootvar')

        if save:
            self.execubootcmd("saveenv", ubootprompt, 30)

        success = 1
        return success

    # WARNING: this function works only if already in u-boot
    # setup u-boot environment variables from provided text file, save it if save flag is True
    def setupubootwithfile(self, envfile="", ubootprompt="Octeon zen=>", save=False, additionnal_vars={}):

        all_target = ubootprompt
        env_var = parse_env_file(envfile)

        def set_var(myvar, myvalue):
            if myvalue is "":
                myenvvar = "setenv " + myvar + "\n"
                printmyvar = "printenv " + myvar + "\n"
                myvarresult = "\"" + myvar + "\" not defined"
            else:
                myenvvar = "setenv " + myvar + " '" + myvalue + "'\n"
                printmyvar = "printenv " + myvar + "\n"
                myvarresult = myvar + "=" + myvalue
            targetstr_response = self.com_target(myenvvar, all_target, 3, poll_rate=0.01)
            if targetstr_response is "":
                raise COMException('Failure to set uboot variable: {0},{1}'.format(myvar, myvalue))

            if not ubootprompt in targetstr_response:
                raise COMException('Error no prompt in setting a variable in setupuboot: {0},{1}'.format(myvar, myvalue))

            targetstr_response = self.com_target(printmyvar, all_target, 3, poll_rate=0.01)
            if targetstr_response is "":
                raise COMException('Failure to readback variable: {0},{1}'.format(myvar, myvalue))

            if not ubootprompt in targetstr_response:
                raise COMException('Error no prompt in reading a variable in setupuboot: {0},{1}'.format(myvar, myvalue))

            if not myvarresult in targetstr_response:
                raise COMException('Error incorrect variable value readback in setupuboot: {0},{1}'.format(myvar, myvalue))

        for myvar, myvalue in env_var.iteritems():
            set_var(myvar, myvalue)

        for myvar, myvalue in additionnal_vars.iteritems():
            set_var(myvar, myvalue)

        if save:
            self.execubootcmd("saveenv", ubootprompt, 30)

        success = 1
        return success

    # WARNING: this function works only if already in u-boot
    # if usedhcp is False, an expected u-boot static ip config must be already there and working before using this
    def updateuboot(self, updatefile="", ubootprompt="Octeon zen=>", usedhcp=True):

        success = 0
        if usedhcp:
            dhcp_res = self.execubootcmd("dhcp", ubootprompt, 30)
        else:
            dhcp_res = 1

        if dhcp_res is 1:
            mydn = "tftp " + "0x2000000 " + updatefile
            dn_res = self.execubootcmd(mydn, ubootprompt, 30)

            if dn_res is 1:
                myupdate = "protect off 0x17cc0000 +160000 && erase 0x17cc0000 +160000 && cp.b 0x2000000 0x17cc0000 ${filesize} && protect on 0x17cc0000 +160000"
                self.execubootcmd(myupdate, ubootprompt, 30)

        success = 1
        return success

    # WARNING: this function works only if u-boot is compiled to support hush command parser and already in u-boot
    # execute a u-boot command and validates if it executed without error
    def execubootcmd(self, ubootcmd="echo .", ubootprompt="Octeon zen=>", cmdtout=3):

        ok_ans1 = "^C"
        ok_ans2 = "OK^!"
        ok_ans_nr = "^C\r\nOK^!"
        ok_ans_n = "^C\nOK^!"
        cmd_ans = " && echo " + ok_ans1 + " && echo " + ok_ans2 + "\n"
        all_target = ubootprompt
        targetstr_response = self.com_target(ubootcmd + cmd_ans, all_target, cmdtout)
        self.logger.debug(targetstr_response)
        if targetstr_response is "":
            raise COMException('Failure to execute u-boot cmd in execubootcmd')

        if ((not ok_ans1 in targetstr_response) or (not ok_ans2 in targetstr_response)):
            raise COMException('Error in execute u-boot cmd in execubootcmd')

        if not ubootprompt in targetstr_response:
            raise COMException('Error no prompt in execute u-boot cmd in execubootcmd')

        success = 1
        return success

    def extract_coreclock_from_boot(self):
        TARGET = 'Core clock:'
        targetstr_response = self.com_target('', TARGET, 15, poll_rate=0.01)
        if self.is_target_in_response(TARGET, targetstr_response):
            import re
            regex = '(?:%s (\d+ \wHz))' % TARGET
            return re.search(regex, targetstr_response).group(1)
        else:
            raise COMException('Unable to extract core clock from boot')


def parse_env_file(file_name):
    env_var = {}
    with open(file_name) as f: lines = [line.strip() for line in f]
    for myitem in lines:
        sindex = myitem.find('=')
        # is "variable=value" line format?
        if sindex is not -1:
            myvar = myitem[0:sindex]
            myvalue = myitem[sindex+1:len(myitem)]
            env_var[myvar] = myvalue
        else:
            # check if invalid line is not commented
            if len(myitem) is not 0 and myitem[0] is not ';':
                raise COMException('Error invalid environment string format in setupubootwithfile: {0}'.format(myitem))

    return env_var
