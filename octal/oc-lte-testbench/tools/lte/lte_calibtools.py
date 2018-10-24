
import time
REMOTE_CALIBRATION_PATH = '/tmp/rffe/bin'

def real_tx(tx):
    assert 0 <= tx <= 1, "Internal TX %d not valid" % tx
    return tx+1

def invert_real_tx(tx):
    tx=real_tx(tx)
    if tx==1:
        return 2
    else:
        return 1

def unreal_tx(tx):
    assert 1 <= tx <= 2, "TX %d not valid" % tx
    return tx-1

def execute_command_from_calib_dir(ssh, cmd, *args, **kwargs):
    return ssh.execute_command('%s' % (cmd), *args, **kwargs)
    #return ssh.execute_command('cd %s; %s' % (REMOTE_CALIBRATION_PATH, cmd), *args, **kwargs)

##region Calib Table files

def quick_calib_file_read(file_path, atten_line_index):
    result = {}
    with open(file_path, 'r') as f:
        lines = f.readlines()
        freq_range_line = lines[0].split(' ')
        atten_line = lines[atten_line_index].split(' ')
        freq_start = int(freq_range_line[0])
        freq_step = int(freq_range_line[1])

        freq = freq_start
        for i in range(len(atten_line)):
            try:
                result[freq] = float(atten_line[i])
            except ValueError:
                pass
            freq = freq + freq_step
    return result

##endregion

##region File Setup

def login_from_com(context):
    context.logger.info("Login in progress")
    login_result = bool(context.server.com.login(context.SSH_USERNAME, context.SSH_PASSWORD, context.SSH_USERTARGET, context.SSH_ROOTTARGET, context.SSH_FIRSTTOUT, context.SSH_SECONDTOUT))
    assert login_result, "Unable to login"

def stop_in_uboot(context):
    context.logger.info("U-boot access in progress")
    enteruboot_result = bool(context.server.com.enteruboot(context.UBOOT_WAITSTRING, context.UBOOT_PASSWORD, context.UBOOT_PROMPT))
    assert enteruboot_result, "Unable to access u-boot"

def reset_from_uboot(context):
    context.logger.info("Reset from U-boot in progress")
    resetfromuboot_result = bool(context.server.com.resetfromuboot(context.UBOOT_RESETSTRING))
    assert resetfromuboot_result, "Unable to reset from u-boot"

def reset_and_change_lte_bw(context, lte_bw):
    context.server.env_var.set_value('bw', lte_bw)
    context.server.ssh.reboot()

    context.server.ssh.wait_for_initializing(context)
    launchscp(context)
    updatesetup_transfer_files(context)

    enable_fe_all_on(context)
    start_etm(context, '1.1')
    set_all_tx_off(context)

def updatesetup_transfer_files(context):
    import os
    from opentest.interface import filexfer
    context.logger.info("Updating setup file list...")
    configfile = os.path.join(context.TEST_PACKAGE_PATH, context.UPDATESETUP_FILE)
    if context.UPDATESETUP_FILE_LOCAL_TEST_PATH:
        localprepath = os.path.join(context.TEST_PACKAGE_PATH, "")
    else:
        localprepath = os.path.join(context.UPDATESETUP_FILE_LOCAL_PATH, "")
    remoteprepath = os.path.join(context.UPDATESETUP_FILE_REMOTE_PATH, "")
    filexfer.filexferwithfile(context, configfile, localprepath, remoteprepath)
    context.server.ssh.execute_command('tar -xvf /mnt/app/rffe.tgz -C /')
    context.logger.info("Updating setup file list... Done")

def launchssh(context):
    context.server.ssh.create_service(context.UUT_IP_ADDR, context.SSH_USERNAME, context.SSH_PASSWORD)

def launchscp(context):
    context.server.scp.create_service(context.SSH_IP_ADDR, context.SSH_USERNAME, context.SSH_PASSWORD, context.SCP_TIMEOUT)

##endregion

##region Sensors

def validate_both_pa_current(context):
    for tx in range(0,2):
        validate_pa_current(context, tx)

def validate_pa_current(context, tx):
    try:
        pa_current = context.server.sensor.current_tx(tx)
        pa_volt = context.server.sensor.voltage_tx(tx)
        info = 'PA %d = (%s V, %s A)' % (real_tx(tx), str(pa_volt), str(pa_current))
        context.logger.debug(info)
        setattr(context.exports, 'PA%d_INFO' % tx, info)
        assert pa_current < context.SAFETY_MAX_PA_CURRENT_A, "PA %d current over limit [%.2f A]" % (tx, pa_current)
        assert pa_volt > context.SAFETY_MIN_PA_VOLTAGE_V, "PA %d voltage under limit [%.2f V]" % (tx, pa_volt)
    except ValueError:
        context.logger.debug('Unable to read PA current/voltage')

def read_particular_sensor(context, name, value_index, index):
    value_string = context.server.ssh.execute_command("sensors | grep '%s' | awk '{print $%d}' | sed -n %dp" % (name, value_index, index))
    return float(value_string)

##endregion

##region Enables

def set_all_tx_off(context):
    set_tx_enable(context, 0, 0)
    set_tx_enable(context, 1, 0)

def set_other_tx_enable(context, tx, value):
    other_tx = 0 if tx == 1 else 1
    set_tx_enable(context, other_tx, value)

def set_tx_enable(context, tx, value):
    set_all_enables(context, tx, value, value, value)

def set_all_enables(context, tx, pred1, pred2, pa):
    set_pred1(context, tx, pred1)
    set_pred2(context, tx, pred2)
    set_pa(context, tx, pa)

def set_pred1(context, tx, value):
    context.logger.debug("TX %d Predriver1 set to %d" % (real_tx(tx), value))
    set_enable_file_path(context, "/sys/devices/soc.0/0.tip/tx%d_predriver1_en" % tx, value)

def set_pred2(context, tx, value):
    context.logger.debug("TX %d Predriver2 set to %d" % (real_tx(tx), value))
    set_enable_file_path(context, "/sys/devices/soc.0/0.tip/tx%d_predriver2_en" % tx, value)

def set_pa(context, tx, value):
    context.logger.debug("TX %d PA set to %d" % (real_tx(tx), value))
    set_enable_file_path(context, "/sys/devices/soc.0/0.pa%d/state" % tx, value)

def set_rx_enable(context, tx, value):
    #active low
    #value = 0 if value != 0 else 1

    context.logger.debug("RX %d LNA set to %d" % (real_tx(tx), value))
    set_enable_file_path(context, "/sys/devices/soc.0/0.tip/rx%d_lna_en" % tx, value)

def set_rx1_switches(context):
    if context.FE_REV == 'E':
        context.server.ssh.execute_command("fe-setsniffercircuit --off")
    else:
        context.server.ssh.execute_command("echo 1 > /sys/devices/soc.0/0.tip/ant1_or_sniffer_to_rx1_switch")

def set_sniffer_active(context, active=True):
    arg = '--on' if active else '--off'
    context.server.ssh.execute_command("fe-setsniffercircuit %s" % arg)

def set_enable_file_path(context, path, value):
    context.server.ssh.write_file(path, value)


##endregion

##region ETM fn

def start_etm(context, mode):
    context.logger.info("Start ETM%s" % mode)
    value = execute_command_from_calib_dir(context.server.ssh, 'etm-start-tmp %s' % mode)
    assert value.strip() == '', 'Start ETM output should be empty: %s' % value

def set_etm_freq(context, freq):
    execute_command_from_calib_dir(context.server.ssh, 'etm-settxfreq ' + str(freq))

def enable_fe(context, tx):
    enable_fe_all_on(context)
    set_other_tx_enable(context, tx, 0)

def enable_fe_all_on(context):
    context.logger.debug(execute_command_from_calib_dir(context.server.ssh, 'fe-enable').strip())

def enable_fe_28V(context, value):
    configure_enable_io_expander(context)
    enable_i2c_set(context, '0x10', value)

def enable_fe_5V_3V3(context, value):
    configure_enable_io_expander(context)
    enable_i2c_set(context, '0x20', value)

def enable_fe_1V8(context, value):
    configure_enable_io_expander(context)
    enable_i2c_set(context, '0x40', value)

def enable_i2c_set(context, mask, value):
    value = '0xff' if value else '0x00'
    context.server.ssh.execute_command('i2cset -y -m %s 1 0x21 3 %s' % (mask, value))

def configure_enable_io_expander(context):
    context.server.ssh.execute_command('i2cset -y -m 0x70 1 0x21 7 0x00')


##endregion

##region OCXO

def ocxo_set_pwm_high(context, value):
    context.logger.debug("Set PWM High to %d" % value)
    cn_rfdriver_command_list(context, 'pwm h %d' % value)
    #context.server.ssh.execute_command('fsetenv pwmreghigh %d' % value)

def ocxo_set_pwm_low(context, value):
    context.logger.debug("Set PWM Low to %d" % value)
    cn_rfdriver_command_list(context, 'pwm l %d' % value)
    #context.server.ssh.execute_command('fsetenv pwmreglow %d' % value)

def ocxo_calibrate(context, iterations, init_pwm):
    from pid_controller.pid import PID
    ocxo_set_pwm_high(context, init_pwm)
    freq_error = 10000
    pid = PID(p=-2, i=-0.5)
    pid.target = 0
    pwm = init_pwm
    with context.stack_logger('ocxo'):
        context.logger.info("Calibrating OCXO Freq")
        while iterations > 0:
            context.server.spectrum.read_evm()
            freq_error = context.server.spectrum.fetch_freq_error()
            context.logger.debug("Freq error = %d" % freq_error)
            pwm = int(pwm + pid(freq_error))
            ocxo_set_pwm_high(context, pwm)
            iterations = iterations - 1
    return pwm

##endregion

##region BB RSSI In

def read_bb_rssi(context, tx):
    import re
    regex_tx = [r'The PRI Antenna REG value=(\w+),', r'The Sec Antenna REG value=(\w+),']
    string = cn_rfdriver_command_list(context, 's')
    return int(find_in_string_tx(tx, string, regex_tx), 16)

def find_in_string_tx(tx, string, regex_array):
    import re
    regex = regex_array[tx]
    match = re.findall(regex, string)
    try:
        _m = match[0]
    except IndexError:
        _m = ""
    return _m

def set_rx_freq(context, freq):
    cn_rfdriver_command_list(context, 'u %d' % freq)
##endregion

##region RFPAL

def reset_rfpal(context):
    context.logger.debug("Reset RFPAL")
    execute_command_from_calib_dir(context.server.ssh, 'fe-resetrfpal')
    assert_rfpal_responsive(context)

def clearcal_rfpal(context, tx):
    context.logger.debug("TX%d Clear RFPAL Calibration" % real_tx(tx))
    execute_command_from_calib_dir(context.server.ssh, 'fe-clearrfpalcal -c %d' % real_tx(tx))
    assert_rfpal_responsive(context)

def setcal_rfpal(context, tx):
    context.logger.debug("TX%d Set RFPAL Calibration" % real_tx(tx))
    execute_command_from_calib_dir(context.server.ssh, 'fe-setrfpalcal -c %d' % real_tx(tx))
    assert_rfpal_responsive(context)

def assert_rfpal_responsive(context, timeout=5):
    STEPS = 10
    init_timeout = timeout
    interval = float(timeout)/float(STEPS)
    value = -1
    while timeout > 0:
        timeout = timeout - interval
        try:
            value = read_rfpal_in_dbm(context, context.server.ssh, 0)
        except AssertionError:
            value = -1
        if  value < -0.01 and value != -1:
            break
        time.sleep(interval)
    else:
        if value == -1:
            #value is -1
            raise AssertionError("RFPAL did not respond within %.2f s, SPI comm error" % init_timeout)
        else:
            #value is 0
            raise AssertionError("RFPAL did not respond within %.2f s, SPI comm OK" % init_timeout)

def rfpal_default_config(context):
    rfpaltoolset(context, '0xfc 0x00 0x05 2 0x0e 0x10')
    rfpaltoolset(context, '0xfc 0x02 0x05 2 0x0e 0xd8')
    rfpaltoolset(context, '0xfc 0x14 0x05 2 0x0e 0x10')
    rfpaltoolset(context, '0xfc 0x16 0x05 2 0x0e 0xd8')

def rfpaltoolset(context, cmd):
    execute_command_from_calib_dir(context.server.ssh, 'rfpaltoolset %s' % cmd)

def rfpal_write(context, addr_args, write_args, expect):
    context.server.ssh.execute_command('rfpaleepromtoolset %s %s' % (addr_args, write_args))
    read_back = context.server.ssh.execute_command('rfpaleepromtoolget %s' % addr_args)
    assert expect.upper() in read_back.upper(), 'RFPAL program freq range failed. Read back [%s] but expected [%s]' % (read_back, expect)


def rfpal_program_freq_range(context, band):
    if band == 3:
        rfpal_write(context, '0xfc 0x14 4', '0x0e 0x10 0x0e 0xd8', '0XE 0X10 0XE 0XD8')
        rfpal_write(context, '0xfc 0x00 4', '0x0e 0x10 0x0e 0xd8', '0XE 0X10 0XE 0XD8')
    else:
        raise Exception('Band %d not implemented' % band)

##endregion

##region RFPAL FB Power

def read_rfpal_fb_dbm(context, ssh, tx):
    tx = real_tx(tx)
    output = get_rfpal_fb_pwr_in(context, ssh, tx)
    dbn = int(output)
    assert_rfpal_dbn(dbn)
    return dbn_to_dbm(dbn)

def assert_rfpal_dbn(dbn):
    assert dbn != -1, "RFPAL SPI Error"

def get_rfpal_fb_pwr_in(context, ssh, tx):
    if tx == 1:
        cmd = "rfpaltooldump 2>&1 | grep 'PMU RF FB RMS Value' | awk '{print $6}' | egrep -o '^[^\.dBM]*' | sed -n 2p"
    else:
        cmd = "rfpaltooldump 2>&1 | grep 'PMU RF FB RMS Value' | awk '{print $6}' | egrep -o '^[^\.dBM]*' | sed -n 1p"
    value = execute_command_from_calib_dir(ssh, cmd)
    return value

def dbn_to_dbm(dbn):
    return (dbn*3.01)/1024.0
##endregion

##region RFPAL IN Power

def read_rfpal_in_dbm(context, ssh, tx):
    tx = real_tx(tx)
    output = _get_rfpal_pwr_in(context, ssh, tx)
    try:
        dbn = int(output)
    except ValueError:
        context.logger.debug('RFPAL output: %s' % execute_command_from_calib_dir(context.server.ssh, 'rfpaltooldump'))
        raise
    assert_rfpal_dbn(dbn)
    return dbn_to_dbm(dbn)

def _get_rfpal_pwr_in(context, ssh, tx):
    if tx == 1:
        cmd = "rfpaltooldump 2>&1 | grep 'PMU RF IN RMS Value' | awk '{print $6}' | egrep -o '^[^\.dBM]*' | sed -n 2p"
    else:
        cmd = "rfpaltooldump 2>&1 | grep 'PMU RF IN RMS Value' | awk '{print $6}' | egrep -o '^[^\.dBM]*' | sed -n 1p"
    value = execute_command_from_calib_dir(ssh, cmd)
    return value

##endregion

##region Attenuator Validation

def default_atten_values(context, tx):
    set_tx_atten(context, context.server.ssh, tx, context.CALIBRATION_TX_ATTN_INIT_ATTEN)
    set_bb_atten(context, context.server.ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN)
    set_fb_atten(context, context.server.ssh, tx, context.CALIBRATION_FB_ATTN_INIT_ATTEN)

def generic_valid_attn(context, set_attn_fn, power_fn, criteria_prefix, retries=3):

    init_value = 0b1111111
    # (attenuator setting, expected gain from previous, CRITERIA)
    test_values = [
    (0b0011111, 'B6_B5_GAIN'), # +22db
    (0b0100000, 'B5_INVERT_GAIN'), # -0.25db
    (0b0100001, 'B0_GAIN'), # -0.25db
    (0b0100011, 'B1_GAIN'), # -0.5
    (0b0100111, 'B2_GAIN'), # -1
    (0b0101111, 'B3_GAIN'), # -2
    (0b0111111, 'B4_GAIN'), # -4
    ]


    _generic_valid_attn_evaluate_block(context, set_attn_fn, power_fn, criteria_prefix, init_value, test_values, retries=retries)


def _generic_valid_attn_evaluate_block(context, set_attn_fn, power_fn, criteria_prefix, init_value, test_values, retries=3):
    context.logger.info("Set attenuator to initial value %d" % init_value)
    set_attn_fn(init_value)
    previous = init_value

    with context.criteria.evaluate_block() as evaluate_block:

        for attn, criteria_suffix in test_values:
            criteria = criteria_prefix + criteria_suffix
            attn_retries = retries
            while True:
                pwr_before = power_fn()
                set_attn_fn(attn)
                time.sleep(0.5)
                pwr_after = power_fn()
                diff = pwr_after - pwr_before
                try:
                    context.criteria.evaluate(criteria, diff)
                    break
                except AssertionError:
                    attn_retries = attn_retries - 1
                    if attn_retries <= 0:
                        evaluate_block.evaluate(criteria, diff)
                        break
                    context.logger.info('Attn bit validation failed, retrying. (%d retries left)' % attn_retries)
                    set_attn_fn(previous)
                    time.sleep(0.5)

            previous = attn



##endregion

##region TX Attenuator

def get_tx_atten_raw(context, tx):
    return int(context.server.ssh.execute_command('cat /sys/devices/soc.0/0.tip/tx%d_pa_att' % tx))

def set_tx_atten_raw(context, ssh, tx, value):
    return set_tx_atten(context, ssh, tx, float(value)/float(context.CALIBRATION_TX_ATTN_RESOLUTION_DIVIDER))

def set_tx_atten(context, ssh, tx, value):
    tx = real_tx(tx)
    value = clamp_warning(context, 'TX Attn', context.CALIBRATION_TX_ATTN_MIN, value, context.CALIBRATION_TX_ATTN_MAX)
    quarter_dbs = int(value*context.CALIBRATION_TX_ATTN_RESOLUTION_DIVIDER)
    execute_command_from_calib_dir(ssh, 'fe-settxatt %d %d' % (tx, quarter_dbs))
    context.logger.debug("TX Attn %d set to %.2f dB" % (tx, value))
    validate_pa_current(context, unreal_tx(tx))
    return value
##endregion

##region BB Attenuator

def get_bb_atten_raw(context, tx):
    import re
    regex_tx = [r'attn1 ([\d.]+) dB', r'attn2 ([\d.]+) dB']
    string = cn_rfdriver_command_list(context, 'rag')
    return float(find_in_string_tx(tx, string, regex_tx))

def set_bb_atten_raw(context, ssh, tx, value):
    return set_bb_atten(context, ssh, tx, value)

def set_bb_atten(context, ssh, tx, value):
    tx = real_tx(tx)
    value = clamp_warning(context, 'BB Attn', context.CALIBRATION_BB_ATTN_MIN, value, context.CALIBRATION_BB_ATTN_MAX)
    execute_command_from_calib_dir(ssh, 'bb-settxatt %d %d' % (tx, value))
    context.logger.debug("BB Attn %d set to %.2f dB" % (tx, value))
    validate_pa_current(context, unreal_tx(tx))
    return value
##endregion

##region FB Attenuator

def get_fb_atten_raw(context, tx):
    return int(context.server.ssh.execute_command('cat /sys/devices/soc.0/0.tip/tx%d_rfpal_att' % tx))

def set_fb_atten_raw(context, ssh, tx, value):
    return set_fb_atten(context, ssh, tx, float(value)/float(context.CALIBRATION_FB_ATTN_RESOLUTION_DIVIDER))

def set_fb_atten(context, ssh, tx, value):
    tx = real_tx(tx)
    value = clamp_warning(context, 'FB Attn', context.CALIBRATION_FB_ATTN_MIN, value, context.CALIBRATION_FB_ATTN_MAX)
    quarter_dbs = int(value*context.CALIBRATION_FB_ATTN_RESOLUTION_DIVIDER)
    #Inverted (tx 1)
    if tx == 1:
        path = "/sys/devices/soc.0/0.tip/tx0_rfpal_att"
    else:
        path = "/sys/devices/soc.0/0.tip/tx1_rfpal_att"

    execute_command_from_calib_dir(ssh, "echo %d > %s" % (quarter_dbs, path))
    context.logger.debug("FB Attn %d set to %.2f dB" % (tx, value))
    return value

##endregion

##region RX Attenuator

def set_rx_atten_raw(context, ssh, tx, value):
    return set_rx_atten(context, ssh, tx, float(value)/float(context.CALIBRATION_FB_ATTN_RESOLUTION_DIVIDER))

def set_rx_atten(context, ssh, tx, value):
    value = clamp_warning(context, 'RX Attn', context.CALIBRATION_RX_ATTN_MIN, value, context.CALIBRATION_RX_ATTN_MAX)
    quarter_dbs = int(value*context.CALIBRATION_FB_ATTN_RESOLUTION_DIVIDER)

    path = "/sys/devices/soc.0/0.tip/rx%d_att" % tx
    execute_command_from_calib_dir(ssh, "echo %d > %s" % (quarter_dbs, path))
    context.logger.debug("RX Attn %d set to %.2f dB" % (real_tx(tx), value))
    return value

##endregion

def cn_rfdriver_command_list(context, command):
    command = command + ' q eof'
    return context.server.ssh.execute_command('oncpu 0 cn_rfdriver', stdin = [command], timeout=0.4, assertexitcode=None, get_pty=True)

def clamp_warning(context, value_name, minn, old_value, maxn):
    value = clamp(minn, old_value, maxn)
    if old_value != value: context.logger.warning("%s value clamped ! Desired value=%.2f" % (value_name, old_value))
    return value

def clamp(minn, n, maxn):
    return max(min(maxn, n), minn)

def find_y_for_x(intercept, step, x):
    return (float(x)*float(step)) + float(intercept)

def get_rms_errors_from_prediction(context, tx, power):
    fwd = context.server.ssh_rms.read_rms('trx%d_fwd' % tx)
    rev = context.server.ssh_rms.read_rms('trx%d_rev' % tx)

    context.logger.info('FWD value = %d, REV value = %d, Power out = %.2f' % (fwd, rev, power))

    expected_fwd = find_y_for_x(context.CALIBRATION_RMS_FWD_INTERCEPT, context.CALIBRATION_RMS_FWD_STEP, power)
    expected_rev = find_y_for_x(context.CALIBRATION_RMS_REV_INTERCEPT, context.CALIBRATION_RMS_REV_STEP, power)

    return (expected_fwd-fwd, expected_rev-rev)
