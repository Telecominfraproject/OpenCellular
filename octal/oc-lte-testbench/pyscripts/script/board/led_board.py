import opentest.script.testrunner as testrunner

class LedBoard(testrunner.TestSuite):
    @testrunner.testcase("RED Leds test", critical=False)
    def LD_TSC000_0(self, context):

        context.server.led.control(color="red")
        context.wait_tester_feedback("All presents LEDs are red", fb_type=testrunner.FeedbackType.PASS_FAIL, image=r"support\image\led_red.jpg")

    @testrunner.testcase("Green Leds test", critical=False)
    def LD_TSC000_1(self, context):

        context.server.led.control(color="green")
        context.wait_tester_feedback("All presents LEDs are green", fb_type=testrunner.FeedbackType.PASS_FAIL, image=r"support\image\led_green.jpg")

    @testrunner.testcase("Orange Leds test", critical=False)
    def LD_TSC000_2(self, context):

        context.server.led.control(color="orange")
        context.wait_tester_feedback("All presents LEDs are orange", fb_type=testrunner.FeedbackType.PASS_FAIL, image=r"support\image\led_orange.jpg")

    @testrunner.testcase("Led off test", critical=False)
    def LD_TSC000_3(self, context):

        context.server.led.control(on_off="off")
        context.wait_tester_feedback("All presents LEDs are off", fb_type=testrunner.FeedbackType.PASS_FAIL, image=r"support\image\led_off.jpg")
