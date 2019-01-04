
import lte_calibtools
from opentest.integration import rfmeasure, calib_utils
from nute import utils
import time
import os
import csv

##region Calibration Validation

def write_calib(context):
    cal_folder = context.CALIBRATION_POSTPROCESS_FOLDER
    remote_folder = context.CALIBRATION_REMOTE_CALIB_FOLDER
    remote_validation_folder = context.CALIBRATION_REMOTE_CALIB_VALIDATION_FOLDER
    file_name = context.CALIBRATION_FILE_NAME

    create_cal_tgz_to_remote(context, remote_folder)
    context.server.ssh.execute_command('cd %s; tar -xvf %s' % (remote_folder, file_name))
    #context.server.ssh.execute_command('rm %s%s' % (remote_folder, file_name)) # Remove .tgz for read validation
    #print ("remote folder = %s" % remote_folder)
    write_cal_folder_to_eeprom(context, remote_folder)
    context.server.ssh.execute_command('mkdir %s' % remote_validation_folder, assertexitcode=None)
    read_eeprom_cal_to_folder(context, remote_validation_folder)
    compare_write_read_eeprom_folders(context, remote_folder, remote_validation_folder)

def create_cal_tgz_to_remote(context, remote_folder):
    cal_folder = context.CALIBRATION_POSTPROCESS_FOLDER
    file_name = context.CALIBRATION_FILE_NAME
    os.chdir(os.path.join(cal_folder, '..'))
    full_path = os.getcwd()
    #context.logger.info('calfolder %s filename %s joined %s ' % (cal_folder,file_name,os.path.join(cal_folder, file_name)))
    #create_cal_tgz_from_folder(cal_folder, os.path.join(cal_folder, file_name))
    #print ('full path = %s' % full_path)
    create_cal_tgz_from_folder(cal_folder, os.path.join(full_path, file_name))
    context.server.ssh.execute_command('mkdir %s' % remote_folder, assertexitcode=None)
    #os.chdir(cal_folder)
    #context.server.scp.putfile(file=file_name, localpath='.', remotepath=remote_folder)
    #full_path = os.getcwd()
    context.server.scp.putfile(file=file_name, localpath=full_path, remotepath=remote_folder)
    #context.server.scp.putfile(file=file_name, localpath=cal_folder, remotepath=remote_folder)
    context.server.ssh.execute_command('cd %s; tar -xvf %s' % (remote_folder, file_name))

def compare_write_read_eeprom_folders(context, write_folder, read_folder):
    context.logger.info('Comparing written and read calibration tables')
    diff_output = context.server.ssh.execute_command('diff --brief -r %s %s' % (os.path.dirname(write_folder), os.path.dirname(read_folder)))
    context.logger.debug(diff_output)

    # First assert validates that all files that were written were read, but authorizes files that were read but not written (e.g. if a certain calibration was not done)
    assert 'Only in %s:' % write_folder not in diff_output, 'Some files were not written'
    # Second assert validates that all files which were both read and written are equal
    assert 'differ' not in diff_output, 'Some files with the same name do not contain the same data'
    assert 'No such file or directory' not in diff_output, 'DEV ERROR. Folders to compare do not exist'

    context.logger.info('EEPROM write validation successful')

def write_cal_folder_to_eeprom(context, folder):
    context.logger.info('Writing calib to EEPROM. This might take up to 2 minutes.')
    output = lte_calibtools.execute_command_from_calib_dir(context.server.ssh, 'caleepromtool -E /sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0051/eeprom -p %s -l DEBUG -w' % folder, timeout=100)
    context.logger.debug(output)

def read_eeprom_cal_to_folder(context, folder):
    context.logger.info('Reading calib from EEPROM.')
    output = lte_calibtools.execute_command_from_calib_dir(context.server.ssh, 'caleepromtool -E /sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0051/eeprom -p %s -l DEBUG -r' % folder, timeout=100)
    context.logger.debug(output)

def create_cal_tgz_from_folder(cal_folder, tgz_file_name):
    import tarfile

    # with tarfile.open(tgz_file_name, 'w:gz') as tar:
    #     for root, dirs, files in os.walk(cal_folder):
    #         for f in files:
    #             f_name, ext = os.path.splitext(f)
    #             if ext == '.cal':
    #                 tar.add(os.path.join(cal_folder, f), arcname=f)
    #print("calfolder = %s, tgz = %s" % (cal_folder,tgz_file_name))
    with tarfile.open(tgz_file_name, "w:gz") as tar:
        tar.add(cal_folder, arcname=os.path.basename(cal_folder))

def generate_postprocess_tables(context, bw):
    pre_folder = context.CALIBRATION_PREPROCESS_FOLDER
    post_folder = context.CALIBRATION_POSTPROCESS_FOLDER

    for tx in range(2):
        bb_table = calib_utils.LTEBBCalibTable()
        tx_table = calib_utils.LTETXCalibTable()
        fb_table = calib_utils.LTEFBCalibTable()
        #context.logger.info('Reading %s' % pre_folder)
        with utils.stack_chdir(pre_folder):
            currentdir = os.getcwd()
            #context.logger.info('current is %s' % currentdir)
            unified_table = utils.load_obj(lte_unified_calib_table_file_name(bw=bw, tx=tx))

        unified_table.generate_interpolation_function(kind='linear')
        bb_attens, tx_attens, fb_attens = unified_table.interpolate_step(1, raw_values=True)
        start, stop = unified_table.freq_start_stop()

        bb_table.set_values(range(start, stop+1), temp=unified_table.temp, attens=bb_attens)
        tx_table.set_values(range(start, stop+1), temp=unified_table.temp, pwr_range=range(30,31), attens=tx_attens)
        fb_table.set_values(range(start, stop+1), temp=unified_table.temp, pwr_range=range(30,31), attens=fb_attens)

        post_path = post_folder
        try:
            os.makedirs(post_path)
        except:
            pass
        with utils.stack_chdir(post_path):
            bb_table.save(bw, tx)
            tx_table.save(bw, tx)
            fb_table.save(bw, tx)


def generate_postprocess_tables_OLD(context, pre_folder, post_folder, bw):
    for tx in range(2):
        bb_table = calib_utils.LTEBBCalibTable()
        tx_table = calib_utils.LTETXCalibTable()
        fb_table = calib_utils.LTEFBCalibTable()

        with utils.stack_chdir(pre_folder):
            bb_table.load(bw, tx)
            tx_table.load(bw, tx)
            fb_table.load(bw, tx)

        bb_attens = bb_table.interpolate_table()
        tx_to_bb_ratio = - (context.CALIBRATION_TX_ATTN_RESOLUTION_DIVIDER/context.CALIBRATION_BB_ATTN_RESOLUTION_DIVIDER)
        tx_attens = tx_table.interpolate_table(affecting_attens=bb_attens, this_to_affecting_ratio=tx_to_bb_ratio)
        fb_attens = fb_table.interpolate_table(affecting_attens=tx_attens, this_to_affecting_ratio=-1)

        with utils.stack_chdir(post_folder):
            bb_table.save(bw, tx)
            tx_table.save(bw, tx)
            fb_table.save(bw, tx)

def validate_calibration_etm3_2(context, folder, bw):
    attens_tx0, attens_tx1 = attens_from_folder(context, folder, bw)
    start_etm_all(context, '3.2')
    name = "FREQ"
    context.logger.info(name)
    validate_calibration_attens_at_freq(context, attens_tx0, attens_tx1, bw, 1845, default_values_at_end=False)
    lte_calibtools.set_tx_enable(context, 0, 0)
    context.server.spectrum.read_evm()
    context.wait_tester_feedback("Press enter to continue")

def validate_calibration_folder_both_tx_random_freq_range(context, folder, bw, min_freq, max_freq):
    validate_calibration_folder_both_tx_at_freq(context, folder, bw, freq)

def validate_eeprom_calibration_both_tx_at_BMT_2_rand(context, bw, cal_tgz_path=None, invert_tx_validation=False):

    def check_cal(arg1, arg2):
        if (arg1 != arg2):
            print ('arg1 %s, != arg2 %s' % (arg1, arg2))
    #FIXME removing the with for now, everything here was indented under with
    #with context.criteria.evaluate_block() as evaluate_block:

    def get_tx_done_callback(name):
        def tx_done_callback(context, tx, freq):
            real_tx = lte_calibtools.real_tx(tx)
            power_name = 'POWER_OUT_dBm'
            current_name = 'PA_CURRENT'
            aclr_name = 'ACLR'
            freq_name = 'FREQ'
            freq_err_name = 'FREQ_ERROR'
            evm_name = 'EVM'
            time.sleep(2)
            aclr = context.server.spectrum.find_stable_aclr()
            pwr = read_output_power(context, context.server.spectrum)
            print_power_readings(context, tx)
            context.server.spectrum.read_evm()
            freq_error = context.server.spectrum.fetch_freq_error()
            evm = context.server.spectrum.fetch_evm()

			#FIXME need to do better calibration check using calibration_criteria in pyscripts\script\system\system_tx.py
            # evaluate_block.evaluate(power_name, pwr)
            # evaluate_block.evaluate(current_name, context.server.sensor.current_tx(tx))
            # evaluate_block.evaluate(aclr_name, aclr)
            # evaluate_block.evaluate(freq_name, freq)
            # evaluate_block.evaluate(freq_err_name, abs(freq_error))
            # evaluate_block.evaluate(evm_name, evm)
            check_cal(power_name, pwr)
            check_cal(current_name, context.server.sensor.current_tx(tx))
            check_cal(aclr_name, aclr)
            check_cal(freq_name, freq)
            check_cal(freq_err_name, abs(freq_error))
            check_cal(evm_name, evm)


        return tx_done_callback

    context.server.spectrum.setup_lte_dl_mode(lte_bw=bw, cont='ON')
    #etm is already running don't think I need to run again..?
    #start_etm_all(context, '1.1')

    lowest_freq = getattr(context, 'CALIBRATION_BW%d_FREQS' % bw)[0]
    highest_freq = getattr(context, 'CALIBRATION_BW%d_FREQS' % bw)[-1]
    middle_freq = context.RF_DL_MIDDLE_FREQ
    freq_low = context.VALIDATION_BOTTOM_RANDOM
    freq_high = context.VALIDATION_TOP_RANDOM
    tx_list = [1,0] if invert_tx_validation else [0,1]
    for tx in tx_list:
        for freq, name in [(lowest_freq, "BOTTOM"), (middle_freq, "MIDDLE"), (highest_freq, "TOP"), (freq_low, "RANDOM_BOTTOM"), (freq_high, "RANDOM_TOP")]:
            context.logger.info("%s (%d MHz)" % (name, freq))
            validate_calibration_point_from_eeprom(context, tx, freq, cal_tgz_path=cal_tgz_path)
            callback = get_tx_done_callback(name)
            callback(context, tx, freq)

def validate_eeprom_calibration_single_tx_at_BMT_2_rand(context, bw, cal_tgz_path=None, tx=0):

    def check_cal(arg1, arg2):
        if (arg1 != arg2):
            print ('arg1 %s, != arg2 %s' % (arg1, arg2))

    #removing the with for now, everything here was indented under with
    #with context.criteria.evaluate_block() as evaluate_block:
    def get_tx_done_callback(name):
        def tx_done_callback(context, tx, freq):
            real_tx = lte_calibtools.real_tx(tx)
            power_name = 'POWER_OUT_dBm'
            current_name = 'PA_CURRENT'
            aclr_name = 'ACLR'
            freq_name = 'FREQ'
            freq_err_name = 'FREQ_ERROR'
            evm_name = 'EVM'
            time.sleep(2)
            aclr = context.server.spectrum.find_stable_aclr()
            pwr = read_output_power(context, context.server.spectrum)
            print_power_readings(context, tx)
            context.server.spectrum.read_evm()
            freq_error = context.server.spectrum.fetch_freq_error()
            evm = context.server.spectrum.fetch_evm()

			#FIXME need to do better calibration check using calibration_criteria in pyscripts\script\system\system_tx.py
            # evaluate_block.evaluate(power_name, pwr)
            # evaluate_block.evaluate(current_name, context.server.sensor.current_tx(tx))
            # evaluate_block.evaluate(aclr_name, aclr)
            # evaluate_block.evaluate(freq_name, freq)
            # evaluate_block.evaluate(freq_err_name, abs(freq_error))
            # evaluate_block.evaluate(evm_name, evm)
            check_cal(power_name, pwr)
            check_cal(current_name, context.server.sensor.current_tx(tx))
            check_cal(aclr_name, aclr)
            check_cal(freq_name, freq)
            check_cal(freq_err_name, abs(freq_error))
            check_cal(evm_name, evm)


        return tx_done_callback

    context.server.spectrum.setup_lte_dl_mode(lte_bw=bw, cont='ON')
    #etm is already running don't think I need to run again..?
    #start_etm_all(context, '1.1')

    lowest_freq = getattr(context, 'CALIBRATION_BW%d_FREQS' % bw)[0]
    highest_freq = getattr(context, 'CALIBRATION_BW%d_FREQS' % bw)[-1]
    middle_freq = context.RF_DL_MIDDLE_FREQ
    freq_low = context.VALIDATION_BOTTOM_RANDOM
    freq_high = context.VALIDATION_TOP_RANDOM
    tx_list = [tx]
    for tx in tx_list:
        for freq, name in [(lowest_freq, "BOTTOM"), (middle_freq, "MIDDLE"), (highest_freq, "TOP"), (freq_low, "RANDOM_BOTTOM"), (freq_high, "RANDOM_TOP")]:
            context.logger.info("%s (%d MHz)" % (name, freq))
            validate_calibration_point_from_eeprom(context, tx, freq, cal_tgz_path=cal_tgz_path)
            callback = get_tx_done_callback(name)
            callback(context, tx, freq)

def validate_calibration_folder_both_tx_at_all_freqs(context, folder, bw):

    attens_tx0, attens_tx1 = attens_from_folder(context, folder, bw)

    for freq, bb_atten in attens_tx0[0].iteritems():
        validate_calibration_attens_at_freq(context, attens_tx0, attens_tx1, bw, freq)

def empty_callback(context, tx, freq):
    pass

def validate_calibration_folder_both_tx_at_freq(context, folder, bw, freq, done_tx_callback=empty_callback):

    attens_tx0, attens_tx1 = attens_from_folder(context, folder, bw)

    validate_calibration_attens_at_freq(context, attens_tx0, attens_tx1, bw, freq)

def attens_from_folder(context, folder, bw):
    folder_path = os.path.join(context.LOGS_PATH, folder)
    attens_tx0 = get_attenuations_from_folder(folder_path, 1, bw)
    attens_tx1 = get_attenuations_from_folder(folder_path, 2, bw)
    return attens_tx0, attens_tx1


def get_attenuations_from_folder(folder_name, tx, bw):
    bb_attens = lte_calibtools.quick_calib_file_read(os.path.join(folder_name, bb_atten_file_name(bw, tx)), 1)
    tx_attens = lte_calibtools.quick_calib_file_read(os.path.join(folder_name, tx_atten_file_name(bw, tx)), 3)
    fb_attens = lte_calibtools.quick_calib_file_read(os.path.join(folder_name, fb_atten_file_name(bw, tx)), 2)

    return bb_attens, tx_attens, fb_attens



def sweep_attens_folder(context, tx, bw, freq_range, cal_tgz_path=None):
    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    context.logger.info("Calibrating TX %d w/ BW of %d MHz" % (lte_calibtools.real_tx(tx), bw))
    spectrum = context.server.spectrum
    ssh = context.server.ssh

    actual_bw = int(context.server.env_var.get_value('bw'))

    if bw != actual_bw:
        context.logger.info("Changing bandwidth; requires reboot.")
        lte_calibtools.reset_and_change_lte_bw(context, bw)

    spectrum.setup_lte_dl_mode(lte_bw=bw, cont='ON')

    lte_calibtools.set_tx_enable(context, tx, 1)

    #default values for tx and fb attn
    lte_calibtools.set_bb_atten(context, ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN)
    lte_calibtools.set_tx_atten(context, ssh, tx, context.CALIBRATION_TX_ATTN_INIT_ATTEN)
    lte_calibtools.set_fb_atten(context, ssh, tx, context.CALIBRATION_FB_ATTN_INIT_ATTEN)

    with utils.stack_chdir(context.CALIBRATION_POSTPROCESS_FOLDER):
        with open('pwr_sweep_bw%s_ant%d.csv' % (bw, tx+1), 'w') as f:
            f.write('freq, pwr, aclr, pa current, evm\n')
            for freq in freq_range:
                validate_calibration_point_from_eeprom(context, tx, freq, cal_tgz_path=cal_tgz_path)
                time.sleep(3)
                print_power_readings(context, tx)
                aclr = context.server.spectrum.find_stable_aclr()
                current = context.server.sensor.current_tx(tx)
                pwr = read_output_power(context, context.server.spectrum)
                context.server.spectrum.read_evm()
                freq_error = context.server.spectrum.fetch_freq_error()
                evm = context.server.spectrum.fetch_evm()
                context.logger.debug('%d, %.2f, %.2f, %.3f, %.3f' % (freq, pwr, aclr, current, evm))
                f.write('%d, %.2f, %.2f, %.3f, %.3f\n' % (freq, pwr, aclr, current, evm))

    lte_calibtools.set_tx_enable(context, tx, 0)

def validate_calibration_attens_at_freq(context, attens_tx0, attens_tx1, bw, freq, done_tx_callback=empty_callback, default_values_at_end=True):

    validate_calibration_attens_at_freq_tx(context, attens_tx0, bw, freq, 0)
    done_tx_callback(context, 0, freq)

    validate_calibration_attens_at_freq_tx(context, attens_tx1, bw, freq, 1)
    done_tx_callback(context, 1, freq)

    if default_values_at_end:
        set_default_atten_values(context, 0)
        set_default_atten_values(context, 1)

def validate_calibration_attens_at_freq_tx(context, attens, bw, freq, tx):
    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    freq = validate_calibration_point_from_attenuations(context, tx, bw, freq, attens)


def set_default_atten_values(context, tx):
    lte_calibtools.set_bb_atten(context, context.server.ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN)
    try:
        lte_calibtools.set_tx_atten(context, context.server.ssh, tx, context.CALIBRATION_TX_ATTN_INIT_ATTEN)
        lte_calibtools.set_fb_atten(context, context.server.ssh, tx, context.CALIBRATION_FB_ATTN_INIT_ATTEN)
    except AssertionError:
        context.logger.debug('Unable to set default values for FE attenuators.')
        pass

def get_atten_from_attens(context, freq, attens, attn_index):
    freq = find_closest_freq(context, freq, attens[attn_index])
    return attens[attn_index][freq]

def validate_calibration_from_eeprom_at_freq(context, freq, cal_tgz_path=None, done_tx_callback=empty_callback, tx=None):
    do_both = tx is None
    if do_both or tx == 0:
        validate_calibration_point_from_eeprom(context, 0, freq, cal_tgz_path=cal_tgz_path)
        done_tx_callback(context, 0, freq)

    if do_both or tx == 1:
        validate_calibration_point_from_eeprom(context, 1, freq, cal_tgz_path=cal_tgz_path)
        done_tx_callback(context, 1, freq)


def validate_calibration_point_from_eeprom(context, tx, freq, cal_tgz_path=None):
    context.logger.info("TX %d Validating EEPROM calib frequency %s MHz" % (lte_calibtools.real_tx(tx), freq))
    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    #context.server.spectrum.setup_measurement(center_freq_mhz=freq, span_mhz=50)
    setup_etm_measurement(context, freq)
    use_cal_tgz = ''
    if cal_tgz_path:
        use_cal_tgz = '-c %s' % cal_tgz_path
    lte_calibtools.execute_command_from_calib_dir(context.server.ssh, 'pltd-enablefe -t %d %s' % (freq, use_cal_tgz), timeout=10)



def validate_calibration_point_from_attenuations(context, tx, bw, freq, attens):

    bb_freq = find_closest_freq(context, freq, attens[0])
    tx_freq = find_closest_freq(context, freq, attens[1])
    fb_freq = find_closest_freq(context, freq, attens[2])

    assert bb_freq == tx_freq == fb_freq, "Mismatch between calibrated frequencies"

    context.logger.info("TX %d Validating frequency %s MHz(closest to %s)" % (lte_calibtools.real_tx(tx), bb_freq, freq))
    freq=bb_freq
    bb_att = attens[0][bb_freq]
    tx_att = attens[1][tx_freq]
    fb_att = attens[2][fb_freq]

    validate_calibration_point(context, tx, bw, freq, bb_att, tx_att, fb_att)
    return freq

def find_closest_freq(context, wanted_freq, attens):
    closest = None
    closest_distance = 999
    for freq, atten in attens.iteritems():
        dist = abs(wanted_freq-freq)
        if dist < closest_distance:
            closest = freq
            closest_distance = dist

    return closest

def validate_calibration_point(context, tx, bw, freq, bb_atten, tx_atten, fb_atten):
    spectrum = context.server.spectrum
    ssh = context.server.ssh

    setup_etm_measurement(context, freq)

    lte_calibtools.set_tx_atten_raw(context, ssh, tx, tx_atten)
    lte_calibtools.set_fb_atten_raw(context, ssh, tx, fb_atten)
    lte_calibtools.set_bb_atten_raw(context, ssh, tx, bb_atten)
    time.sleep(0.5)
    context.logger.info("Point: bb_atten=%.2f, tx_atten=%.2f, fb_atten=%.2f" % (bb_atten, tx_atten, fb_atten))


def print_power_readings(context, tx):
    rf_in = lte_calibtools.read_rfpal_in_dbm(context, context.server.ssh, tx)
    fb_in = lte_calibtools.read_rfpal_fb_dbm(context, context.server.ssh, tx)
    acquire_acp(context)
    pwr_out = read_output_power(context, context.server.spectrum)
    bb_att = lte_calibtools.get_bb_atten_raw(context, tx)
    tx_att = lte_calibtools.get_tx_atten_raw(context, tx)
    fb_att = lte_calibtools.get_fb_atten_raw(context, tx)
    context.logger.info("Readings: bb_att=%d, tx_att=%d, fb_att=%d, rf_in=%.2f, fb_in=%.2f, pwr_out=%.2f" % (bb_att, tx_att, fb_att, rf_in, fb_in, pwr_out))

def validate_bb_rssi(context, tx, path_gain=0, with_feedback=True):
    from opentest.script import testrunner

    context.server.generator.output_enable(False)
    time.sleep(2)
    rssi_no_signal = lte_calibtools.read_bb_rssi(context, tx)
    while with_feedback:
        try :
            rssi_no_signal = lte_calibtools.read_bb_rssi(context, tx)
            context.logger.info("RSSI w/o signal= %d" % rssi_no_signal)
            context.wait_tester_feedback("Read RSSI",fb_type= testrunner.FeedbackType.PASS_FAIL_RETRY)
            break
        except testrunner.TesterRetry:
            pass

    context.server.generator.setup_waveform_in_gen("10MHZ_FRCA13_RO_0")
    context.server.generator.execute_command(':RAD:ARB:SCL:RATE 15.36MHz')
    lte_calibtools.set_rx_freq(context, context.RF_UL_MIDDLE_FREQ)

    context.logger.debug("RSSI w/o signal= %d" % rssi_no_signal)

    with context.criteria.evaluate_block() as block:
        for dbm in [-30, -40, -50]:
            context.server.generator.setup_output(center_freq_mhz=context.RF_UL_MIDDLE_FREQ, power_dbm= dbm - path_gain)
            context.server.generator.output_enable(True, mod_enable=True)
            time.sleep(context.RF_STABILISATION_TIME*2)
            rssi = lte_calibtools.read_bb_rssi(context, tx)
            rssi_pow = estimate_power_from_rssi(context, rssi)
            context.logger.debug("RSSI %d dBm signal= %d -> %.2f dBm" % (dbm, rssi, rssi_pow))

            block.evaluate('BB_TSC%d51_RX_RSSI_WITH_SIGNAL_%d_dBm' % (lte_calibtools.real_tx(tx), -dbm), rssi_pow)

        block.evaluate('BB_TSC%d51_RX_RSSI_WITHOUT_SIGNAL' % lte_calibtools.real_tx(tx), rssi_no_signal)
        context.server.generator.output_enable(False, mod_enable=False)

def validate_bb_sensitivity(context, tx):
    context.server.generator.setup_waveform_in_gen("10MHZ_FRCA13_RO_0")#TODO
    context.server.generator.execute_command(':RAD:ARB:SCL:RATE 15.36MHz')
    lte_calibtools.set_rx_freq(context, context.RF_UL_MIDDLE_FREQ)
    time.sleep(context.RF_STABILISATION_TIME*2)

    bb_gain = context.server.env_var.get_value('gain%d' % lte_calibtools.real_tx(tx))
    bb_gain = int(bb_gain)
    context.logger.debug('BB Gain = %d dB' % bb_gain)
    dbm = -105 + bb_gain
    rssi = lte_calibtools.read_bb_rssi(context, tx)
    rssi_pow = estimate_power_from_rssi(context, rssi) - bb_gain
    context.logger.debug('RSSI = %d' % rssi)
    context.server.generator.output_enable(True, mod_enable=True)
    while abs(int(rssi_pow) - dbm + bb_gain) > 0.25:
        context.server.generator.setup_output(center_freq_mhz=context.RF_UL_MIDDLE_FREQ, power_dbm=dbm)
        rssi = lte_calibtools.read_bb_rssi(context, tx)
        rssi_pow = estimate_power_from_rssi(context, rssi) - bb_gain
        context.logger.debug('Input = %d dBm, RSSI = %.1f dBm' % (dbm - bb_gain, rssi_pow))
        dbm = dbm + 1
    dbm = dbm - bb_gain
    context.criteria.evaluate('BB_TSC%d51_RX_SENSITIVITY' % lte_calibtools.real_tx(tx), dbm)
    context.server.generator.output_enable(False, mod_enable=False)

def estimate_power_from_rssi(context, rssi):
    return (context.CALIBRATION_RSSI_SLOPE*rssi) + context.CALIBRATION_RSSI_INTERCEPT


##endregion

##region ETM functions

def start_etm_tx(context, tx, mode):
    lte_calibtools.enable_fe_all_on(context)
    lte_calibtools.start_etm(context, mode)
    lte_calibtools.set_all_tx_off(context)
    lte_calibtools.set_tx_enable(context, tx, 1)

def start_etm_all(context, mode):
    lte_calibtools.enable_fe_all_on(context)
    lte_calibtools.start_etm(context, mode)

def setup_etm_measurement(context, freq):
    context.server.spectrum.setup_measurement(center_freq_mhz=freq, span_mhz=50)
    lte_calibtools.set_etm_freq(context, freq)

def enable_fe(context, tx):
    lte_calibtools.enable_fe(context, tx)

##endregion

##region Calibration

def calibrate_ocxo(context, tx, freq, force_calib=False):
    # # from spectrum_example
    # server = serverinterface.LocalLaunchedServerInterface(port=5050)
    #
    # server.spectrum = server.get_service_client_from_name('KeysightEXASpectrumClient', url='spectrum')
    # server.spectrum.create_service("TCPIP0::K-N9030B-41982.local::inst0::INSTR")
    #
    # server.spectrum.instrument_reset()
    # server.spectrum.setup_lte_dl_mode(lte_bw=5, mode='ACP', cont='OFF')
    # server.spectrum.read_evm()
    #
    # # from rfmeasure
    # def set_spectrum_path(context, path, tx):
    #     tx_str = '%s%d' % (path, tx)
    #     ret_val = context.server.rfswitch.set_switch_path('SPECTRUM', tx_str)
    #     assert ret_val, "RFSwitch must be of type LossRFSwitch[...] for LTE"
    #     loss = context.server.rfswitch.loss.get_loss_key(ret_val)
    #     context.logger.debug("Spectrum linked with %s. %.2f dB loss" % (tx_str, loss))
    #     context.server.spectrum.set_loss(loss)
    #
    ## things to add


    context.server.spectrum.instrument_reset()


    ## where the code originally started

    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    spectrum = context.server.spectrum
    ssh = context.server.ssh

    lte_calibtools.set_tx_enable(context, tx, 1)
    bw = int(context.server.env_var.get_value('bw'))

    spectrum.setup_lte_dl_mode(lte_bw=bw, cont = 'ON')

    # lte_calibtools.enable_fe(context, tx)
    # lte_calibtools.start_etm(context, '1.1')

    lte_calibtools.set_tx_enable(context, tx, 1)
    setup_etm_measurement(context, freq)

    calibrate_ocxo_no_setup(context, force_calib=force_calib)

def calibrate_ocxo_no_setup(context, force_calib=False):

    context.server.spectrum.read_evm()
    freq_error = context.server.spectrum.fetch_freq_error()

    if abs(freq_error) > context.CALIBRATION_OCXO_MAX_FREQ_ERROR/2 or force_calib:
        context.logger.info("Frequency error too high (%d Hz), calibrating OCXO" % freq_error)

        lte_calibtools.ocxo_set_pwm_low(context, context.CALIBRATION_OCXO_PWM_LOW)
        pwm = lte_calibtools.ocxo_calibrate(context, iterations=5, init_pwm=context.CALIBRATION_OCXO_PWM_HIGH)
        freq_error = context.server.spectrum.fetch_freq_error()

        context.CALIBRATION_OCXO_PWM_HIGH = pwm

        context.server.env_var.set_value('pwmreglow', str(context.CALIBRATION_OCXO_PWM_LOW))
        context.server.env_var.set_value('pwmreghigh', str(context.CALIBRATION_OCXO_PWM_HIGH))
    else:
        context.logger.info("Skipped OCXO Calibration, frequency error = %d Hz" % freq_error)

    # context.criteria.evaluate('BB_TSC020_OCXO_FREQ_ERROR', abs(freq_error))

def calibrate_rfpal(context, tx, freq):
    spectrum = context.server.spectrum
    ssh = context.server.ssh


    calibrate_single_freq(context, tx, freq)

    lte_calibtools.clearcal_rfpal(context, tx)

    lte_calibtools.reset_rfpal(context)
    aclr = context.server.spectrum.find_stable_aclr()

    lte_calibtools.setcal_rfpal(context, tx)

    # context.criteria.evaluate("FE_TSC%d20_RFPAL_POST_CALIBRATION_ACLR" % lte_calibtools.real_tx(tx), aclr)
    lte_calibtools.set_tx_enable(context, tx, 0)

def calibrate_single_freq(context, tx, freq):
    #rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    spectrum = context.server.spectrum
    ssh = context.server.ssh

    lte_calibtools.set_tx_enable(context, tx, 1)
    bw = int(context.server.env_var.get_value('bw'))

    spectrum.setup_lte_dl_mode(lte_bw=bw, cont = 'ON')
    lte_calibtools.reset_rfpal(context)
    #
    # lte_calibtools.enable_fe(context, tx)
    # lte_calibtools.start_etm(context, '1.1')

    # Make sure rfpal in power is in the right range
    calibrate_tx_freq(context, tx, freq, init_values=None)

def inspect_rolloff(context, tx, bw, freq_range):
    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    context.logger.info("Calibrating TX %d w/ BW of %d MHz" % (lte_calibtools.real_tx(tx), bw))
    spectrum = context.server.spectrum
    ssh = context.server.ssh

    actual_bw = int(context.server.env_var.get_value('bw'))

    if bw != actual_bw:
        context.logger.info("Changing bandwidth; requires reboot.")
        lte_calibtools.reset_and_change_lte_bw(context, bw)

    spectrum.setup_lte_dl_mode(lte_bw=bw, cont='ON')

    lte_calibtools.set_tx_enable(context, tx, 1)

    #default values for tx and fb attn
    lte_calibtools.set_bb_atten(context, ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN)
    lte_calibtools.set_tx_atten(context, ssh, tx, context.CALIBRATION_TX_ATTN_INIT_ATTEN)
    lte_calibtools.set_fb_atten(context, ssh, tx, context.CALIBRATION_FB_ATTN_INIT_ATTEN)

    with utils.stack_chdir(context.CALIBRATION_POSTPROCESS_FOLDER):
        with open('rolloff%s_ant%d.csv' % (bw, tx), 'w') as f:
            for freq in freq_range:
                setup_etm_measurement(context, freq)
                time.sleep(1)
                f.write('%.2f\n' % read_output_power(context, context.server.spectrum))

    lte_calibtools.set_tx_enable(context, tx, 0)


class LTEUnifiedCalibTable(object):
    def __init__(self, context=None):
        self.bb_table = calib_utils.Generic1DCalibTable()
        self.bb_tx_table = calib_utils.Generic1DCalibTable()
        self.fb_table = calib_utils.Generic1DCalibTable()
        self.temp = 0

        if context is not None:
            self.bb_resolution = context.CALIBRATION_BB_ATTN_RESOLUTION_DIVIDER
            self.tx_resolution = context.CALIBRATION_TX_ATTN_RESOLUTION_DIVIDER
            self.fb_resolution = context.CALIBRATION_FB_ATTN_RESOLUTION_DIVIDER

    def set_temp(self, value):
        self.temp = round(value)

    def set_resolutions(self, bb, tx, fb):
        self.bb_resolution = bb
        self.tx_resolution = tx
        self.fb_resolution = fb

    def add_measure(self, freq, bb_attn, tx_attn, fb_attn, rf_in_err, pwr_out_err, fb_in_err):
        desired_bb_attn = bb_attn + rf_in_err
        desired_bb_tx_attn = bb_attn + tx_attn + pwr_out_err
        desired_fb_attn = fb_attn + pwr_out_err + fb_in_err

        self.bb_table.add_measure(freq, desired_bb_attn)
        self.bb_tx_table.add_measure(freq, desired_bb_tx_attn)
        self.fb_table.add_measure(freq, desired_fb_attn)

    def generate_interpolation_function(self, **kwargs):
        self.bb_table.generate_interpolation_function(**kwargs)
        self.bb_tx_table.generate_interpolation_function(**kwargs)
        self.fb_table.generate_interpolation_function(**kwargs)

    def interpolate_step(self, step, raw_values=False):
        desired_bb_attns = self.bb_table.interpolate_step(step)
        desired_bb_tx_attns = self.bb_tx_table.interpolate_step(step)
        desired_fb_attns = self.fb_table.interpolate_step(step)

        real_bb_attns = []
        real_tx_attns = []
        real_fb_attns = []

        for index in range(len(desired_bb_attns)):
            ##BB is simple: simply round to the nearest real attenuation
            real_bb = calib_utils.round_with_divider_resolution(desired_bb_attns[index], self.bb_resolution)
            rf_in_err = desired_bb_attns[index] - real_bb

            ##Actual tx attn needs to remove the real bb attn we just calculated
            desired_tx_attn = desired_bb_tx_attns[index] - real_bb
            real_tx = calib_utils.round_with_divider_resolution(desired_tx_attn, self.tx_resolution)
            pwr_out_err = desired_tx_attn - real_tx

            real_fb = calib_utils.round_with_divider_resolution(desired_fb_attns[index] - pwr_out_err, self.fb_resolution)

            real_bb_attns.append(real_bb)
            real_tx_attns.append(real_tx)
            real_fb_attns.append(real_fb)

        if raw_values:
            real_bb_attns = [int(bb_attn*self.bb_resolution) for bb_attn in real_bb_attns]
            real_tx_attns = [int(tx_attn*self.tx_resolution) for tx_attn in real_tx_attns]
            real_fb_attns = [int(fb_attn*self.fb_resolution) for fb_attn in real_fb_attns]


        return real_bb_attns, real_tx_attns, real_fb_attns

    def freq_start_stop(self):
        start = self.bb_table.freq_start()
        stop = self.bb_table.freq_stop()
        return start, stop

def lte_unified_calib_table_file_name(bw, tx):
    return 'caltable_%dMHz_ant%d' % (bw, tx)

def calibrate_tx(context, tx, bw):
    # rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    context.logger.info("Calibrating TX %d w/ BW of %d MHz" % (lte_calibtools.real_tx(tx), bw))
    spectrum = context.server.spectrum
    ssh = context.server.ssh

    actual_bw = int(context.server.env_var.get_value('bw'))

    if bw != actual_bw:
        context.logger.info("Changing bandwidth; requires reboot.")
        lte_calibtools.reset_and_change_lte_bw(context, bw)

    spectrum.setup_lte_dl_mode(lte_bw=bw, cont='ON')

    lte_calibtools.set_tx_enable(context, tx, 1)

    #default values for tx and fb attn
    lte_calibtools.set_bb_atten(context, ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN)
    lte_calibtools.set_tx_atten(context, ssh, tx, context.CALIBRATION_TX_ATTN_INIT_ATTEN)
    lte_calibtools.set_fb_atten(context, ssh, tx, context.CALIBRATION_FB_ATTN_INIT_ATTEN)

    freq_range = getattr(context, 'CALIBRATION_BW%d_FREQS' % bw)
    calibrate_by_freq(context, ssh, spectrum, tx, bw, freq_range)

    lte_calibtools.set_tx_enable(context, tx, 0)

def calibrate_tx_b28(context, tx, bw):
    # rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    context.logger.info("Calibrating TX %d w/ BW of %d MHz" % (lte_calibtools.real_tx(tx), bw))
    spectrum = context.server.spectrum
    ssh = context.server.ssh

    actual_bw = int(context.server.env_var.get_value('bw'))

    if bw != actual_bw:
        context.logger.info("Changing bandwidth; requires reboot.")
        lte_calibtools.reset_and_change_lte_bw(context, bw)

    spectrum.setup_lte_dl_mode(lte_bw=bw, cont='ON')

    #lte_calibtools.set_tx_enable(context, tx, 1)

    #default values for tx and fb attn
    # lte_calibtools.set_bb_atten(context, ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN)
    # lte_calibtools.set_tx_atten(context, ssh, tx, context.CALIBRATION_TX_ATTN_INIT_ATTEN)
    # lte_calibtools.set_fb_atten(context, ssh, tx, context.CALIBRATION_FB_ATTN_INIT_ATTEN)

    freq_range = getattr(context, 'CALIBRATION_BW%d_FREQS' % bw)
    print ("freq_range = %s" % freq_range)
    calibrate_by_freq_b28(context, ssh, spectrum, tx, bw, freq_range)

    lte_calibtools.set_tx_enable(context, tx, 0)

def calibrate_by_freq_b28(context, ssh, spectrum, tx, bw, freq_range):
    bb_atten = []
    tx_atten = []
    fb_atten = []

    last_values = None
    unified_table = LTEUnifiedCalibTable(context)

    #temp_before = context.server.sensor.temp_tx(tx)
    with utils.stack_chdir(context.CALIBRATION_PREPROCESS_FOLDER):
        with open('results_%dMHz_TX%d.csv' % (bw, tx), 'wb') as csv_file:
            csv_writer = csv.writer(csv_file)
            for freq in freq_range:
                context.logger.info("B28 Calibrating Attn values for TX %d @ %d MHz" % (lte_calibtools.real_tx(tx), freq))
                #lte_calibtools.reset_rfpal(context)
                last_values = calibrate_tx_freq_b5(context, tx, freq, init_values=last_values, debug_csv_writer=csv_writer, lte_unified_cable=unified_table)
                #for ant2
                # last_values = calibrate_tx_freq_b5(context, 1, freq, init_values=last_values,
                #                                   debug_csv_writer=csv_writer, lte_unified_cable=unified_table)

                bb_atten = bb_atten + [last_values[0]]
                tx_atten = tx_atten + [last_values[1]]
                fb_atten = fb_atten + [last_values[2]]

                last_values = (last_values[0]+1, last_values[1]+1, last_values[2])
                #last_values[0] = last_values[0] + 1
                #last_values[1] = last_values[1] + 4

    #temp_after = context.server.sensor.temp_tx(tx)

    #temp_avg = (temp_after + temp_before)/2
    temp_avg = 30
    unified_table.set_temp(temp_avg)
    pwr_range = range(30, 31)
    print("going to save tx %s, but should go here %s" % (tx, lte_calibtools.real_tx(tx)))
    with utils.stack_chdir(context.CALIBRATION_PREPROCESS_FOLDER):
        utils.save_obj(unified_table, lte_unified_calib_table_file_name(bw=bw, tx=tx))
        #utils.save_obj(unified_table, lte_unified_calib_table_file_name(bw=bw, tx=lte_calibtools.real_tx(tx)))

    return unified_table

def calibrate_tx_b5(context, tx, bw):
    # rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    context.logger.info("Calibrating TX %d w/ BW of %d MHz" % (lte_calibtools.real_tx(tx), bw))
    spectrum = context.server.spectrum
    ssh = context.server.ssh

    actual_bw = int(context.server.env_var.get_value('bw'))

    if bw != actual_bw:
        context.logger.info("Changing bandwidth; requires reboot.")
        lte_calibtools.reset_and_change_lte_bw(context, bw)

    spectrum.setup_lte_dl_mode(lte_bw=bw, cont='ON')

    #lte_calibtools.set_tx_enable(context, tx, 1)

    #default values for tx and fb attn
    # lte_calibtools.set_bb_atten(context, ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN)
    # lte_calibtools.set_tx_atten(context, ssh, tx, context.CALIBRATION_TX_ATTN_INIT_ATTEN)
    # lte_calibtools.set_fb_atten(context, ssh, tx, context.CALIBRATION_FB_ATTN_INIT_ATTEN)

    freq_range = getattr(context, 'CALIBRATION_BW%d_FREQS' % bw)
    calibrate_by_freq_b5(context, ssh, spectrum, tx, bw, freq_range)

    lte_calibtools.set_tx_enable(context, tx, 0)

def calibrate_by_freq_b5(context, ssh, spectrum, tx, bw, freq_range):
    bb_atten = []
    tx_atten = []
    fb_atten = []

    last_values = None
    unified_table = LTEUnifiedCalibTable(context)

    #temp_before = context.server.sensor.temp_tx(tx)
    with utils.stack_chdir(context.CALIBRATION_PREPROCESS_FOLDER):
        with open('results_%dMHz_TX%d.csv' % (bw, tx), 'wb') as csv_file:
            csv_writer = csv.writer(csv_file)
            for freq in freq_range:
                context.logger.info("Calibrating Attn values for TX %d @ %d MHz" % (lte_calibtools.real_tx(tx), freq))
                #lte_calibtools.reset_rfpal(context)
                last_values = calibrate_tx_freq_b5(context, tx, freq, init_values=last_values, debug_csv_writer=csv_writer, lte_unified_cable=unified_table)

                bb_atten = bb_atten + [last_values[0]]
                tx_atten = tx_atten + [last_values[1]]
                fb_atten = fb_atten + [last_values[2]]

                last_values = (last_values[0]+1, last_values[1]+1, last_values[2])
                #last_values[0] = last_values[0] + 1
                #last_values[1] = last_values[1] + 4

    #temp_after = context.server.sensor.temp_tx(tx)

    #temp_avg = (temp_after + temp_before)/2
    temp_avg = 30
    unified_table.set_temp(temp_avg)
    pwr_range = range(30, 31)
    with utils.stack_chdir(context.CALIBRATION_PREPROCESS_FOLDER):
        utils.save_obj(unified_table, lte_unified_calib_table_file_name(bw=bw, tx=tx))

    return unified_table

def calibrate_by_freq(context, ssh, spectrum, tx, bw, freq_range):
    bb_atten = []
    tx_atten = []
    fb_atten = []

    last_values = None
    unified_table = LTEUnifiedCalibTable(context)

    temp_before = context.server.sensor.temp_tx(tx)
    with utils.stack_chdir(context.CALIBRATION_PREPROCESS_FOLDER):
        with open('results_%dMHz_TX%d.csv' % (bw, tx), 'wb') as csv_file:
            csv_writer = csv.writer(csv_file)
            for freq in freq_range:
                context.logger.info("Calibrating Attn values for TX %d @ %d MHz" % (lte_calibtools.real_tx(tx), freq))
                #lte_calibtools.reset_rfpal(context)
                last_values = calibrate_tx_freq(context, tx, freq, init_values=last_values, debug_csv_writer=csv_writer, lte_unified_cable=unified_table)

                bb_atten = bb_atten + [last_values[0]]
                tx_atten = tx_atten + [last_values[1]]
                fb_atten = fb_atten + [last_values[2]]

                last_values = (last_values[0]+1, last_values[1]+1, last_values[2])
                #last_values[0] = last_values[0] + 1
                #last_values[1] = last_values[1] + 4

    temp_after = context.server.sensor.temp_tx(tx)

    temp_avg = (temp_after + temp_before)/2
    unified_table.set_temp(temp_avg)
    pwr_range = range(30, 31)
    with utils.stack_chdir(context.CALIBRATION_PREPROCESS_FOLDER):
        utils.save_obj(unified_table, lte_unified_calib_table_file_name(bw=bw, tx=tx))

    return unified_table


def bb_atten_file_name(bw, tx):
    return 'bbtxatten_b28_%d_ant%d.cal' % (bw, tx)

def tx_atten_file_name(bw, tx):
    return 'fetxatten_b3_%d_ant%d.cal' % (bw, tx)

def fb_atten_file_name(bw, tx):
    return 'fefbatten_b3_%d_ant%d.cal' % (bw, tx)

def calibrate_tx_freq(context, tx, freq, init_values=None, debug_csv_writer=None, lte_unified_cable=None):
    if not init_values:
        init_values = (None,None,None)

    setup_etm_measurement(context, freq)
    bb_attn, rf_in_error = BBAttenuatorCalibrator(context, tx).calibrate_with_diff(freq, init_value=init_values[0])
    tx_attn, pwr_error = TXAttenuatorCalibrator(context, tx).calibrate_with_diff(freq, init_value=init_values[1])
    fb_attn, fb_error = FBAttenuatorCalibrator(context, tx).calibrate_with_diff(freq, init_value=init_values[2])

    if lte_unified_cable is not None:
        lte_unified_cable.add_measure(freq, bb_attn, tx_attn, fb_attn, rf_in_error, pwr_error, fb_error)

    if debug_csv_writer is not None:
        debug_csv_writer.writerow([freq, bb_attn, tx_attn, fb_attn, rf_in_error, pwr_error, fb_error])

    return (bb_attn, tx_attn, fb_attn)

def calibrate_tx_freq_b5(context, tx, freq, init_values=None, debug_csv_writer=None, lte_unified_cable=None):
    if not init_values:
        init_values = (None,None,None)

    setup_etm_measurement(context, freq)
    bb_attn, rf_in_error = BBAttenuatorCalibrator(context, tx).calibrate_with_diff_spectrum(freq, init_value=init_values[0])
    #tx_attn, pwr_error = TXAttenuatorCalibrator(context, tx).calibrate_with_diff(freq, init_value=init_values[1])
    #fb_attn, fb_error = FBAttenuatorCalibrator(context, tx).calibrate_with_diff(freq, init_value=init_values[2])
    tx_attn = 0
    pwr_error = 0
    fb_attn = 0
    fb_error = 0

    if lte_unified_cable is not None:
        lte_unified_cable.add_measure(freq, bb_attn, tx_attn, fb_attn, rf_in_error, pwr_error, fb_error)

    if debug_csv_writer is not None:
        debug_csv_writer.writerow([freq, bb_attn, tx_attn, fb_attn, rf_in_error, pwr_error, fb_error])

    return (bb_attn, tx_attn, fb_attn)

from pid_controller.pid import PID
class AttenuatorCalibrator(object):
    def __init__(self, context, tx):
        self.context = context
        self.tx = tx
        self.pid = PID(p=0.95, i=0.01)
        self.pid.target = 0

    def calibrate_with_diff(self, freq, init_value=None):
        if init_value is None:
            #Start with known value
            init_value = self.get_init_value()

        current_atten = self.set_atten(init_value)
        self.setup_signal(freq)

        self.standard_wait()
        diff, diff_required = self.get_diff_required()
        max_iter = 10
        loose_acceptance_count = 0
        while diff_required != 0 and max_iter > 0:
            current_atten = self.set_atten(current_atten + diff_required)
            #self.standard_wait()
            diff, diff_required = self.get_diff_required()
            if abs(diff_required) <= self.loose_acceptance():
                loose_acceptance_count = loose_acceptance_count + 1

            if loose_acceptance_count >= 3:
                diff_required = 0

            max_iter = max_iter - 1
        self.done_callback(freq, current_atten)
        return current_atten, diff

    def calibrate_with_diff_spectrum(self, freq, init_value=None):
        if init_value is None:
            #Start with known value
            init_value = self.get_init_value()

        current_atten = self.set_atten(init_value)
        self.setup_signal(freq)

        self.standard_wait()
        diff, diff_required = self.get_diff_required_spectrum()
        max_iter = 10
        loose_acceptance_count = 0
        while diff_required != 0 and max_iter > 0:
            current_atten = self.set_atten(current_atten + diff_required)
            #self.standard_wait()
            diff, diff_required = self.get_diff_required_spectrum()
            print("in loop: diff=%s, diff_required=%s" %(diff,diff_required))
            if abs(diff_required) <= self.loose_acceptance():
                loose_acceptance_count = loose_acceptance_count + 1

            if loose_acceptance_count >= 3:
                diff_required = 0

            max_iter = max_iter - 1
        self.done_callback_spectrum(freq, current_atten)
        return current_atten, diff

    def calibrate(self, freq, init_value=None):
        return self.calibrate_with_diff(freq, init_value)[0]

    def to_db(self, value):
        return float(value)/float(self.get_resolution_divider())

    def to_raw(self, value):
        return int(value*float(self.get_resolution_divider()))

    def tight_acceptance(self):
        return 0.5/float(self.get_resolution_divider())

    def loose_acceptance(self):
        return 1/float(self.get_resolution_divider())

    def get_diff_required(self):

        stable_diff = self.tight_acceptance()

        # if self.is_power_close_to_target(value):
            #Stabilize output when close
            # self.context.logger.debug("Value = %.2f, stabilizing" % value)
        value = self.read_power()#utils.stabilize_output(_get_value, stable_diff=stable_diff, average_count=1, poll_rate=0.01)

        #value = -18, target = -16.5
        diff = value - self.get_target()
        self.iteration_callback(value)
        diff = self.pid(diff)
        #print("value=%.2f, target=%.2f, diff=%.2f" % (value, self.get_target(), diff))
        round_diff = self.round_atten(diff)
        return diff, round_diff

    def get_diff_required_spectrum(self):

        stable_diff = self.tight_acceptance()

        # if self.is_power_close_to_target(value):
            #Stabilize output when close
            # self.context.logger.debug("Value = %.2f, stabilizing" % value)
        value = self.read_power_spectrum()#utils.stabilize_output(_get_value, stable_diff=stable_diff, average_count=1, poll_rate=0.01)

        #value = -18, target = -16.5
        diff = value - self.get_target()
        print("subtraction value=%.2f, target=%.2f, diff=%.2f" % (value, self.get_target(), diff))
        self.iteration_callback(value)
        diff = self.pid(diff)
        print("value=%.2f, target=%.2f, diff=%.2f" % (value, self.get_target(), diff))
        round_diff = self.round_atten(diff)
        return diff, round_diff

    def round_atten(self, atten):
        return calib_utils.round_with_divider_resolution(atten, self.get_resolution_divider())

    def is_power_close_to_target(self, pwr):
        target = self.get_target()
        resolution = self.get_resolution_divider()
        return abs(target-pwr) < (0.5)

    def setup_signal(self, freq):
        raise NotImplementedError

    def get_init_value(self):
        raise NotImplementedError

    def get_resolution_divider(self):
        raise NotImplementedError

    def get_target(self):
        raise NotImplementedError

    def read_power(self):
        raise NotImplementedError

    def set_atten(self, value):
        raise NotImplementedError

    def standard_wait(self):
        raise NotImplementedError

    def iteration_callback(self, attn_db):
        pass

    def done_callback(self, freq, attn_db):
        pass

class BBGeneratedSignalCalibrator(AttenuatorCalibrator):
    def setup_signal(self, freq):
        pass

##region BB Calibration

class BBAttenuatorCalibrator(BBGeneratedSignalCalibrator):

    def get_init_value(self):
        return self.context.CALIBRATION_BB_ATTN_INIT_ATTEN

    def get_resolution_divider(self):
        return self.context.CALIBRATION_BB_ATTN_RESOLUTION_DIVIDER

    def get_target(self):
        return self.context.CALIBRATION_RFPAL_IN_TARGET_dBm

    def set_atten(self, value):
        return lte_calibtools.set_bb_atten(self.context, self.context.server.ssh, self.tx, value)

    def read_power(self):
        if not hasattr(self, 'previous'):
            self.previous = None
        value = lte_calibtools.read_rfpal_in_dbm(self.context, self.context.server.ssh, self.tx)
        if not self.previous:
            self.previous = value
        while self.previous == value:
            value = lte_calibtools.read_rfpal_in_dbm(self.context, self.context.server.ssh, self.tx)
        self.previous = value
        return value

    def read_power_spectrum(self):
        #rfmeasure.set_spectrum_path(self.context, 'UUT_ANT', tx)
        self.context.server.spectrum.set_loss(31)
        if not hasattr(self, 'previous'):
            self.previous = None
        value = read_output_power(self.context, self.context.server.spectrum)
        print ("First spectrum pwr val = %s" % value)
        #value = lte_calibtools.read_rfpal_in_dbm(self.context, self.context.server.ssh, self.tx)
        if not self.previous:
            self.previous = value
        while self.previous == value:
            value = read_output_power(self.context, self.context.server.spectrum)
            print ("Second spectrum pwr val = %s" % value)
            #value = lte_calibtools.read_rfpal_in_dbm(self.context, self.context.server.ssh, self.tx)
        self.previous = value
        return value

    def standard_wait(self):
        time.sleep(self.context.CALIBRATION_STD_SLEEP)

    def iteration_callback(self, attn_db):
        self.context.logger.debug("RFPAL IN=%.2f, target=%.2f" % (attn_db, self.get_target()))

    def done_callback_spectrum(self, freq, attn_db):
        self.context.logger.info("[BB] f=%s, \tbb_attn=%s, \tin_pwr=%.2f" % (str(freq), str(attn_db), self.read_power_spectrum()))

    def done_callback(self, freq, attn_db):
        self.context.logger.info("[BB] f=%s, \tbb_attn=%s, \tin_pwr=%.2f, \tfb=%.2f" % (str(freq), str(attn_db), lte_calibtools.read_rfpal_in_dbm(self.context, self.context.server.ssh, self.tx), lte_calibtools.read_rfpal_fb_dbm(self.context, self.context.server.ssh, self.tx)))

def calibrate_bb_att(context, ssh, spectrum, tx, freq_range):
    context.logger.info("Calibrating BB Attn values for TX %d" % lte_calibtools.real_tx(tx))
    atten = []
    last= None
    prev_freq = None
    for freq in freq_range:
        setup_etm_measurement(context, freq)
        new_last = calibrate_bb_att_freq(context, ssh, spectrum, tx, freq, init_value=last)
        if not prev_freq:
            atten = atten + [new_last]
        else:
            atten = atten + interpolate_atten((prev_freq, last), (freq, new_last)) + [new_last]
        prev_freq = freq
        last = new_last

    return atten

def calibrate_bb_att_freq(context, ssh, spectrum, tx, freq, init_value=None):
    calibrator = BBAttenuatorCalibrator(context, tx)
    return calibrator.calibrate(freq, init_value)

##endregion

##region TX Calibration

class TXAttenuatorCalibrator(BBGeneratedSignalCalibrator):

    def get_init_value(self):
        return self.context.CALIBRATION_TX_ATTN_INIT_ATTEN

    def get_resolution_divider(self):
        return self.context.CALIBRATION_TX_ATTN_RESOLUTION_DIVIDER

    def get_target(self):
        return self.context.CALIBRATION_OUTPUT_POWER_TARGET_dBm

    def set_atten(self, value):
        return lte_calibtools.set_tx_atten(self.context, self.context.server.ssh, self.tx, value)

    def read_power(self):
        acquire_acp(self.context)
        return read_output_power(self.context, self.context.server.spectrum)

    def standard_wait(self):
        time.sleep(self.context.CALIBRATION_STD_SLEEP)

    def iteration_callback(self, attn_db):
        self.context.logger.debug("Power out=%.2f, target=%.2f" % (attn_db, self.context.CALIBRATION_OUTPUT_POWER_TARGET_dBm))

    def done_callback(self, freq, attn_db):
        self.context.logger.info("[TX] f=%s, \ttx_attn=%s, \tin_pwr=%.2f, \tout_pwr=%.2f, \tfb=%.2f" % (str(freq), str(attn_db), lte_calibtools.read_rfpal_in_dbm(self.context, self.context.server.ssh, self.tx), read_output_power(self.context, self.context.server.spectrum), lte_calibtools.read_rfpal_fb_dbm(self.context, self.context.server.ssh, self.tx)))


def calibrate_tx_att(context, ssh, spectrum, tx, freq_range, bb_atten_values):
    context.logger.info("Calibrating TX Attn values for TX %d" % lte_calibtools.real_tx(tx))
    atten = []
    last = None
    prev_freq = None
    prev_atten_index = 0
    for freq in freq_range:
        atten_index = 0 if not prev_freq else prev_atten_index + freq-prev_freq
        bb_atten = bb_atten_values[atten_index]

        lte_calibtools.set_bb_atten(context, ssh, tx, bb_aten)
        new_last = calibrate_tx_att_freq(context, ssh, spectrum, tx, freq, init_value=last)
        if not prev_freq:
            atten = atten + [new_last]
        else:
            atten = atten + interpolate_tx_atten_based_on_bb((prev_freq, last), (freq, new_last), bb_atten_values[prev_atten_index:atten_index+1], context.CALIBRATION_TX_ATTN_RESOLUTION_DIVIDER) + [new_last]
        prev_freq = freq
        prev_atten_index = atten_index
        last = new_last

    return atten

def calibrate_tx_att_freq(context, ssh, spectrum, tx, freq, init_value=None):
    calibrator = TXAttenuatorCalibrator(context, tx)
    return calibrator.calibrate(freq, init_value)

##endregion

##region FB Calibration

class FBAttenuatorCalibrator(BBGeneratedSignalCalibrator):
    def get_init_value(self):
        return self.context.CALIBRATION_FB_ATTN_INIT_ATTEN

    def get_resolution_divider(self):
        return self.context.CALIBRATION_FB_ATTN_RESOLUTION_DIVIDER

    def get_target(self):
        return self.context.CALIBRATION_RFPAL_FB_TARGET_dBm

    def set_atten(self, value):
        return lte_calibtools.set_fb_atten(self.context, self.context.server.ssh, self.tx, value)

    def read_power(self):
        if not hasattr(self, 'previous'):
            self.previous = None
        value = lte_calibtools.read_rfpal_fb_dbm(self.context, self.context.server.ssh, self.tx)
        if not self.previous:
            self.previous = value
        while self.previous == value:
            value = lte_calibtools.read_rfpal_fb_dbm(self.context, self.context.server.ssh, self.tx)
        self.previous = value
        return value

    def standard_wait(self):
        time.sleep(self.context.CALIBRATION_STD_SLEEP*2)

    def iteration_callback(self, attn_db):
        self.context.logger.debug("RFPAL FB=%.2f, target=%.2f" % (attn_db, self.context.CALIBRATION_RFPAL_FB_TARGET_dBm))

    def done_callback(self, freq, attn_db):
        self.context.logger.info("[FB] f=%s, \tfb_attn=%s, \tin_pwr=%.2f, \tout_pwr=%.2f, \tfb=%.2f" % (str(freq), str(attn_db),
                                lte_calibtools.read_rfpal_in_dbm(self.context, self.context.server.ssh, self.tx),
                                read_output_power(self.context, self.context.server.spectrum),
                                lte_calibtools.read_rfpal_fb_dbm(self.context, self.context.server.ssh, self.tx)))


def calibrate_fb_att(context, ssh, spectrum, tx, freq_range, bb_atten_values, tx_atten_values):
    context.logger.info("Calibrating FB Attn values for TX %d" % lte_calibtools.real_tx(tx))
    atten = []
    last = None
    for freq, bb_atten, tx_atten in zip(freq_range, bb_atten_values, tx_atten_values):
        context.logger.debug("bb=%.2f, tx=%.2f" % (bb_atten, tx_atten))
        tx_atten = float(tx_atten)/context.CALIBRATION_TX_ATTN_RESOLUTION_DIVIDER
        bb_atten = float(bb_atten)/context.CALIBRATION_BB_ATTN_RESOLUTION_DIVIDER
        lte_calibtools.set_bb_atten(context, ssh, tx, bb_atten)
        lte_calibtools.set_tx_atten(context, ssh, tx, tx_atten)
        last = calibrate_fb_att_freq(context, ssh, spectrum, tx, freq, init_value=last)
        atten = atten + [last]

    return atten

def calibrate_fb_att_freq(context, ssh, spectrum, tx, freq, init_value=None):
    calibrator = FBAttenuatorCalibrator(context, tx)
    return calibrator.calibrate(freq, init_value)

##endregion

##region RX CALIBRATION

class RXAttenuatorCalibrator(AttenuatorCalibrator):

    def get_init_value(self):
        return self.context.CALIBRATION_FB_ATTN_INIT_ATTEN

    def get_resolution_divider(self):
        return self.context.CALIBRATION_FB_ATTN_RESOLUTION_DIVIDER

    def get_target(self):
        return self.context.CALIBRATION_RFPAL_FB_TARGET_dBm

    def set_atten(self, value):
        return lte_calibtools.set_fb_atten(self.context, self.context.server.ssh, self.tx, value)

    def read_power(self):
        raise NotImplementedError
        return lte_calibtools.read_rfpal_fb_dbm(context, ssh, tx)

    def standard_wait(self):
        time.sleep(self.context.CALIBRATION_STD_SLEEP)

    def iteration_callback(self, attn_db):
        self.context.logger.debug("RFPAL FB=%.2f, target=%.2f" % (attn_db, self.context.CALIBRATION_RFPAL_FB_TARGET_dBm))

    def done_callback(self, freq, attn_db):
        self.context.logger.info("[FB] f=%s, \tfb_attn=%s, \tin_pwr=%.2f, \tout_pwr=%.2f, \tfb=%.2f" % (str(freq), str(current_atten),
                                lte_calibtools.read_rfpal_in_dbm(self.context, self.context.server.ssh, self.tx),
                                read_output_power(self.context, self.context.server.spectrum),
                                lte_calibtools.read_rfpal_fb_dbm(self.context, self.context.server.ssh, self.tx)))


##endregion

def interpolate_all_attens(bb_atten, tx_atten, fb_atten):
    new_bb_atten = []
    new_tx_atten = []
    new_fb_atten = []

def interpolate_tx_atten_based_on_bb(tx_point_a, tx_point_b, bb_attens_including_ends, tx_to_bb_ratio):
    return calib_utils.interpolate_atten_with_affecting_atten(tx_point_a, tx_point_b, bb_attens_including_ends, tx_to_bb_ratio)

def acquire_acp(context):
    context.server.spectrum.read_aclr()

def read_output_power(context, spectrum):
    output = spectrum.fetch_channel_power()
    return output

##endregion
