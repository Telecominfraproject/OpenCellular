from nute.service.base_service import RemoteServerError

from tools.lte import lte_calibtools, lte_ssh_client
import opentest.script.testrunner as testrunner

#Adresses I2C to test
I2C_REV = {
    'D':{
    'i2c0': ("0x24", "0x25", "0x26", "0x27", "0x28", "0x40", "0x4a", "0x51"),
    'i2c1': ("0x25", "0x26", "0x27", "0x28", "0x40", "0x4a"),
    'i2c4': ("0x20", "0x49", "0x50", "0x4f", "0x3e", "0x3f")
    },
    'E':{
    'i2c0': ("0x24", "0x25", "0x26", "0x27", "0x28", "0x4a", "0x51"),
    'i2c1': ("0x25", "0x26", "0x27", "0x28", "0x40", "0x41", "0x4a"),
    'i2c4': ("0x20", "0x49", "0x50", "0x4f", "0x3e", "0x3f")
    }
}

#Path
i2c0_path = "/sys/devices/soc.0/1180000001000.i2c/i2c-0"
i2c1_path = "/sys/devices/soc.0/1180000001200.i2c/i2c-1"

def get_rev(context):
    rev = 'D'
    try:
        rev = getattr(context, 'FE_REV')
        if rev != 'E':
            rev = 'D'
    except AttributeError:
        pass
    return rev

class DigitalComm(testrunner.TestSuite):

    @testrunner.testcase("I2C0 Scan", critical=False)
    def FE_TSC010(self, context):
        i2c0_addr = I2C_REV[get_rev(context)]['i2c0']
        result = context.server.i2c.detect(i2c0_path, i2c0_addr).strip()
        assert result == lte_ssh_client.TEST_SUCCESS_STRING, result

    @testrunner.testcase("I2C1 Scan", critical=False)
    def FE_TSC011(self, context):
        i2c1_addr = I2C_REV[get_rev(context)]['i2c1']
        result = context.server.i2c.detect(i2c1_path, i2c1_addr).strip()
        assert result == lte_ssh_client.TEST_SUCCESS_STRING, result

    @testrunner.testcase("I2C4 Scan", critical=False)
    def FE_TSC012(self, context):
        i2c4_addr = I2C_REV[get_rev(context)]['i2c4']
        result = context.server.i2c.detect(i2c1_path, i2c4_addr).strip()#Since I2C4 only exist with the jumper, we will connect with I2C1
        assert result == lte_ssh_client.TEST_SUCCESS_STRING, result

    @testrunner.testcase("SPI RFPAL", critical=True)
    def FE_TSC013(self, context):
        lte_calibtools.reset_rfpal(context)
        lte_calibtools.rfpal_default_config(context)

    #@testrunner.critical
    @testrunner.testcase("Validate FE Rev")
    def FE_TSC016(self, context):
        try:
            rev = context.server.ssh.read_board_rev().strip()
            if len(rev) != 1:
                raise Exception('Invalid rev: %s' % rev)
        except:
            rev = 'D'
        assert rev == context.FE_REV, "Scanned FE Rev [%s] does not match with on-board Rev [%s]" % (context.FE_REV, rev)

    @testrunner.not_implemented
    @testrunner.testcase("Temp tx0 Alert", critical=False)
    def FE_TSC014_0(self, context):
        context.logger.info("NOT IMPLEMENTED")
    """
        result = context.server.sensor.alarm_alert_bool("tx0_temp")
        context.logger.info(result)
        assert result,"no reaction"
    """
    @testrunner.not_implemented
    @testrunner.testcase("Temp tx1 Alert", critical=False)
    def FE_TSC014_1(self, context):
        context.logger.info("NOT IMPLEMENTED")
    """
        result = context.server.sensor.tempalert_bool("tx1_temp")
        assert result,"no reaction"
    """

    @testrunner.not_implemented
    @testrunner.testcase("Temp fe Alert", critical=False)
    def FE_TSC014_2(self, context):
        context.logger.info("NOT IMPLEMENTED")
        """result = context.server.sensor.alarm_alert_bool("fe_temp")
        context.logger.info(result)
        assert result,"no reaction"
        """

    @testrunner.not_implemented
    @testrunner.testcase("Current Tx0 Alerts", critical=False)
    def FE_TSC015_0(self, context):
        context.logger.info("NOT IMPLEMENTED")
    """
        result = context.server.sensor.alarm_alert_bool("tx0_current")
        assert result,"no reaction"
    """
    @testrunner.not_implemented
    @testrunner.testcase("Current Tx1 Alerts", critical=False)
    def FE_TSC015_1(self, context):
        context.logger.info("NOT IMPLEMENTED")
    """
        result = context.server.sensor.alarm_alert_bool("tx1_current")
        assert result,"no reaction"
    """
