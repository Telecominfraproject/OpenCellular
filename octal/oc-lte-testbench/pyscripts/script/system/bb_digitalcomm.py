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
