from opentest.script import testrunner, utils
from opentest.integration import rfmeasure
from opentest.integration.calib_utils import PathLosses

class RFSwitchLosses(testrunner.TestSuite):

    @testrunner.testcase("Launch Service Server", critical=True)
    def RF_PRP000(self, context):
        import opentest.script.server_scripts as server_scripts
        server_scripts.launch(context)
        context.server.open_logs_request()

    @testrunner.testcase("Launch ComponentTester", critical=True)
    def RF_PRP001(self, context):
        context.server.component_tester = rfmeasure.ComponentTester(spec=context.server.spectrum, gen=context.server.generator)
        rfmeasure.create_generator(context)
        context.server.generator.loss = 0
        rfmeasure.create_spectrum(context)
        context.server.spectrum.loss = 0

    @testrunner.testcase("Launch RFSwitch Service", critical=True)
    def RF_PRP002(self, context):
        rfmeasure.create_rfswitch(context)

    @testrunner.testcase("Prepare Instruments", critical=True)
    def RF_PRP003(self, context):
        context.server.component_tester.setup_tone()
        pause_generator_output(context)
        context.losses = PathLosses()

    @testrunner.testcase("RX1 to SPEC", critical=True)
    def RF_TSC001(self, context):
        measure_path(context, 'SPECTRUM', 'UUT_RX0', "Connect RX1 to the Generator", context.RFSWITCH_MEASURE_OTHER_LOSS_POWER, context.RFSWITCH_ufl_ADDITIONNAL_LOSS)

    @testrunner.testcase("RX2 to SPEC", critical=True)
    def RF_TSC002(self, context):
        measure_path(context, 'SPECTRUM', 'UUT_RX1', "Connect RX2 to the Generator", context.RFSWITCH_MEASURE_OTHER_LOSS_POWER, context.RFSWITCH_ufl_ADDITIONNAL_LOSS)

    @testrunner.testcase("ANT1 to SPEC", critical=True)
    def RF_TSC003(self, context):
        measure_path(context, 'SPECTRUM', 'UUT_ANT0', "Connect ANT1 to the Generator", context.RFSWITCH_MEASURE_ANT_LOSS_POWER)

    @testrunner.testcase("ANT2 to SPEC", critical=True)
    def RF_TSC004(self, context):
        measure_path(context, 'SPECTRUM', 'UUT_ANT1', "Connect ANT2 to the Generator", context.RFSWITCH_MEASURE_ANT_LOSS_POWER)

    @testrunner.testcase("TX1 to GEN", critical=True)
    def RF_TSC005(self, context):
        measure_path(context, 'GENERATOR', 'UUT_TX0', "Connect TX1 to the Spectrum", context.RFSWITCH_MEASURE_OTHER_LOSS_POWER, context.RFSWITCH_ufl_ADDITIONNAL_LOSS)

    @testrunner.testcase("TX2 to GEN", critical=True)
    def RF_TSC006(self, context):
        measure_path(context, 'GENERATOR', 'UUT_TX1', "Connect TX2 to the Spectrum", context.RFSWITCH_MEASURE_OTHER_LOSS_POWER, context.RFSWITCH_ufl_ADDITIONNAL_LOSS)

    @testrunner.testcase("ANT1 to GEN", critical=True)
    def RF_TSC007(self, context):
        measure_path(context, 'GENERATOR', 'UUT_ANT0', "Connect ANT1 to the Spectrum", context.RFSWITCH_MEASURE_ANT_LOSS_POWER)

    @testrunner.testcase("ANT2 to GEN", critical=True)
    def RF_TSC008(self, context):
        measure_path(context, 'GENERATOR', 'UUT_ANT1', "Connect ANT2 to the Spectrum", context.RFSWITCH_MEASURE_ANT_LOSS_POWER)

    @testrunner.testcase("Write file", critical=True)
    def RF_CLN001(self, context):
        with context.chdir_logs_path():
            context.losses.save_to_file('path_losses.ini')

def measure_path(context, patha, pathb, message, power, add_loss):
    context.server.component_tester.setup_tone_measure(center_freq_mhz=context.RF_DL_MIDDLE_FREQ, generator_power_dbm=power)
    context.logger.info("Path: %s to %s, input power= %.2f" % (patha, pathb, power))
    context.server.rfswitch.set_switch_path(patha, pathb)
    context.wait_tester_feedback(message)
    start_generator_output(context)
    loss = measure_loss(context, power)
    context.logger.info("Loss = %.2f (+ %.2f)" % (loss, add_loss))
    context.losses.set_loss(patha, pathb, loss + add_loss)
    pause_generator_output(context)

def pause_generator_output(context):
    context.logger.debug("Generator off")
    context.server.generator.output_enable(False, mod_enable=False)

def start_generator_output(context):
    context.logger.debug("Generator on")
    context.server.generator.output_enable(True, mod_enable=False)

def measure_loss(context, input_power):
    peak = context.server.spectrum.find_peak_stable_amp(stable_diff=0.5)
    return input_power - peak[1]
