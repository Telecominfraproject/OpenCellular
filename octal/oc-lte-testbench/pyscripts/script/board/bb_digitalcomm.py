import opentest.script.testrunner as testrunner

#Adresses I2C to test
i2c0_addr = "0x48", "0x50", "0x71"
i2c1_addr = "0x21", "0x4e"

#Path
i2c0_path = "/sys/devices/soc.0/1180000001000.i2c/i2c-0"
i2c1_path = "/sys/devices/soc.0/1180000001200.i2c/i2c-1"

#SD Card
block_device = "/dev/mmcblk0p1"
mount_path = "/tmp"

class DigitalComm(testrunner.TestSuite):

    @testrunner.not_implemented
    @testrunner.testcase("I2C0 Scan", critical=False)
    def BB_TSC010(self, context):
        context.logger.info("NOT IMPLEMENTED WITH REV A")

        # result = context.server.i2c.detect(i2c0_path, i2c0_addr).strip() #Since I2C3 only exist with the jumper, we will connect with I2C0
        # context.logger.info(result)
        # assert result == "test pass", "At least one device is missing"

    @testrunner.not_implemented
    @testrunner.testcase("I2C1 Scan", critical=False)
    def BB_TSC011(self, context):
        context.logger.info("NOT IMPLEMENTED WITH REV A")

        # result = context.server.i2c.detect(i2c1_path, i2c1_addr).strip()
        # context.logger.info(result)
        # assert result == "test pass", "At least one device is missing"


    #@testrunner.testcase("MMC Card", critical=False)
    def BB_TSC012(self, context):
        result = context.server.sdcard.test(block_device, mount_path)
        if not result:
            context.wait_tester_feedback("Check if MMC Card is present")
            # Second try
            result = context.server.sdcard.test(block_device, mount_path)
        context.criteria.evaluate('BB_TSC012_SD_CARD_COMMUNICATION', result)

    @testrunner.not_implemented
    @testrunner.testcase("PCIE", critical=False)
    def BB_TSC013(self, context):
        context.logger.info("NOT IMPLEMENTED")

    @testrunner.testcase("GPS", critical=False)
    def BB_TSC014(self, context):
        try:
            context.logger.info("This test can take up to 5 min")
            context.logger.info("GPS Lock in progress...")
            result = context.server.gps.lock(first_time=False).strip() #Currently The first gps lock is done in the boot-up
            context.logger.info(result)
            assert "GPS fix location time out" not in result, "timeout, no GPS found"
            if "GPS not present" in result:
                assert context.server.env_var.get_value("syncby") == "gps", "ERROR DEV syncby != gps"
            #Testing pulse
            context.logger.info("PULSE")
            result = context.server.gps.pps_bool()
            result = ""
        except AssertionError as e:
            result = str(e)
        context.criteria.evaluate('BB_TSC014_GPS_LOCK', result)
