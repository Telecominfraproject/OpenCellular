#include "common/inc/global/OC_CONNECT1.h"
#include "common/inc/global/Framework.h"
#include "drivers/GpioPCA9557.h"
#include "fake/fake_I2C.h"
#include "fake/fake_GPIO.h"
#include "inc/subsystem/rffe/rffe.h"
#include "inc/subsystem/rffe/rffe_ctrl.h"
#include "inc/subsystem/rffe/rffe_powermonitor.h"
#include "inc/subsystem/sdr/sdr.h"
#include <string.h>
#include "src/registry/SSRegistry.h"
#include "platform/oc-sdr/schema.h"
#include "unity.h"
/* ======================== Constants & variables =========================== */
#define SDR_FX3_IOEXP_ADDRESS                       0x1E
#define OC_CONNECT1_I2C3                            0x05

const Component sys_schema[OC_SS_MAX_LIMIT];

static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static Sdr_gpioCfg sdr_gpioCfg = (Sdr_gpioCfg) {
    /* EC_TRXFECONN_GPIO2/SDR_REG_LDO_PGOOD */
    .pin_sdr_reg_ldo_pgood  = { &s_fake_io_port, OC_EC_SDR_PWR_GD },
    /* TRXFE_12V_ONOFF */
    .pin_trxfe_12v_onoff    = { &s_fake_io_port, OC_EC_SDR_PWR_CNTRL },
    /* EC_FE_RESET_OUT/RF_FE_IO_RESET */
    .pin_rf_fe_io_reset     = { &s_fake_io_port, OC_EC_SDR_FE_IO_RESET_CTRL },
    /* EC_TRXFECONN_GPIO1/SDR_RESET_IN */
    .pin_sdr_reset_in       = { &s_fake_io_port, OC_EC_SDR_DEVICE_CONTROL },
    /* EC_TRXFE_RESET */
    .pin_ec_trxfe_reset     = { &s_fake_io_port, OC_EC_RFFE_RESET },
    /* FX3_RESET */
};

static uint8_t PCA9557_regs_ch1_TX[] = {
    [0x00] = 0x00, /* Input values */
    [0x01] = 0x00, /* Output values */
    [0x02] = 0x00, /* Polarity */
    [0x03] = 0x00, /* Dir Config */
};

static uint8_t PCA9557_regs_ch2_TX[] = {
    [0x00] = 0x00, /* Input values */
    [0x01] = 0x00, /* Output values */
    [0x02] = 0x00, /* Polarity */
    [0x03] = 0x00, /* Dir Config */
};

static uint8_t PCA9557_regs_ch1_RX[] = {
    [0x00] = 0x00, /* Input values */
    [0x01] = 0x00, /* Output values */
    [0x02] = 0x00, /* Polarity */
    [0x03] = 0x00, /* Dir Config */
};

static uint8_t PCA9557_regs_ch2_RX[] = {
    [0x00] = 0x00, /* Input values */
    [0x01] = 0x00, /* Output values */
    [0x02] = 0x00, /* Polarity */
    [0x03] = 0x00, /* Dir Config */
};

static uint8_t PCA9557_regs_revpower[] = {
    [0x00] = 0x00, /* Input values */
    [0x01] = 0x00, /* Output values */
    [0x02] = 0x00, /* Polarity */
    [0x03] = 0x00, /* Dir Config */
};

static uint8_t ch1_ads7830[] = {
    [0x00] = 0x00, /* Input values */
    [0xA4] = 0x00, /* Dir Config */
};

static uint8_t ch2_ads7830[] = {
    [0x00] = 0x00, /* Input values */
    [0xA4] = 0x00, /* Dir Config */
};
static bool rffe_GpioPins[] = {
    [1] = 0x1, /* Pin = 1 */
    [OC_EC_GPIOCOUNT] = 0x1,
};

static uint32_t rffe_GpioConfig[] = {
    [1] = OCGPIO_CFG_INPUT,
    [OC_EC_GPIOCOUNT] = OCGPIO_CFG_INPUT,
};


static Fe_gpioCfg fe_gpiocfg = {
     /* EC_TRXFECONN_GPIO3/RF_PGOOD_LDO */
     .pin_rf_pgood_ldo         = { &s_fake_io_port, OC_EC_FE_PWR_GD },
     /* FE_12V_CTRL */
     .pin_fe_12v_ctrl          = { &s_fake_io_port, OC_EC_FE_CONTROL },
     .pin_trxfe_conn_reset = { &s_fake_io_port, OC_EC_FE_TRXFE_CONN_RESET },
};

static OcGpio_Port fe_watchdog_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_IO_REVPOWER_ALERT_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

static Fe_Watchdog_Cfg fe_watchdog_cfg = {
    /* AOSEL_FPGA */
    .pin_aosel_fpga     = { &fe_watchdog_io, 0 },
    /* CH2_RF_PWR_OFF */
    .pin_ch2_rf_pwr_off = { &fe_watchdog_io, 1 },
    /* CO6_WD */
    .pin_co6_wd         = { &fe_watchdog_io, 2 },
    /* CO5_WD */
    .pin_co5_wd         = { &fe_watchdog_io, 3 },
    /* CO4_WD */
    .pin_co4_wd         = { &fe_watchdog_io, 4 },
    /* CO3_WD */
    .pin_co3_wd         = { &fe_watchdog_io, 5 },
    /* CO2_WD */
    .pin_co2_wd         = { &fe_watchdog_io, 6 },
    /* COPOL_FPGA */
    .pin_copol_fpga     = { &fe_watchdog_io, 7 },
};

/* FE CH watch dog */
static RfWatchdog_Cfg fe_ch1_watchdog = {
    .pin_alert_lb = &fe_watchdog_cfg.pin_co6_wd,
    .pin_alert_hb = &fe_watchdog_cfg.pin_co5_wd,
    .pin_interrupt = &fe_gpiocfg.pin_trxfe_conn_reset,
};

/* FE CH watch dog */
static RfWatchdog_Cfg fe_ch2_watchdog = {
    .pin_alert_lb = &fe_watchdog_cfg.pin_co3_wd,
    .pin_alert_hb = &fe_watchdog_cfg.pin_co4_wd,
    .pin_interrupt = &fe_gpiocfg.pin_trxfe_conn_reset,
};

static OcGpio_Port fe_ch1_gain_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_CHANNEL1_IO_TX_ATTEN_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

static OcGpio_Port fe_ch2_gain_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_CHANNEL2_IO_TX_ATTEN_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

static OcGpio_Port fe_ch1_lna_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_CHANNEL1_IO_RX_ATTEN_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

static OcGpio_Port fe_ch2_lna_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_CHANNEL2_IO_RX_ATTEN_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

static Fe_Gain_Cfg fe_ch1_gain = {
    /* CH1_TX_ATTN_16DB */
    .pin_tx_attn_16db = { &fe_ch1_gain_io, 1 },
    /* CH1_TX_ATTN_P5DB */
    .pin_tx_attn_p5db = { &fe_ch1_gain_io, 2 },
    /* CH1_TX_ATTN_1DB */
    .pin_tx_attn_1db  = { &fe_ch1_gain_io, 3 },
    /* CH1_TX_ATTN_2DB */ 
    .pin_tx_attn_2db  = { &fe_ch1_gain_io, 4 },
    /* CH1_TX_ATTN_4DB */
    .pin_tx_attn_4db  = { &fe_ch1_gain_io, 5 }, 
    /* CH1_TX_ATTN_8DB */
    .pin_tx_attn_8db  = { &fe_ch1_gain_io, 6 },
    /* CH1_TX_ATTN_ENB */
    .pin_tx_attn_enb  = { &fe_ch1_gain_io, 7 },
};

static Fe_Gain_Cfg fe_ch2_gain = {
       /* CH2_TX_ATTN_16DB */
       .pin_tx_attn_16db = { &fe_ch2_gain_io, 1 },
       /* CH2_TX_ATTN_P5DB */
       .pin_tx_attn_p5db = { &fe_ch2_gain_io, 2 },
       /* CH2_TX_ATTN_1DB */
       .pin_tx_attn_1db  = { &fe_ch2_gain_io, 3 },
       /* CH2_TX_ATTN_2DB */
       .pin_tx_attn_2db  = { &fe_ch2_gain_io, 4 },
       /* CH2_TX_ATTN_4DB */
       .pin_tx_attn_4db  = { &fe_ch2_gain_io, 5 },
       /* CH2_TX_ATTN_8DB */
       .pin_tx_attn_8db  = { &fe_ch2_gain_io, 6 },
       /* CH2_TX_ATTN_ENB */
       .pin_tx_attn_enb  = { &fe_ch2_gain_io, 7 },
};

static Fe_Lna_Cfg fe_ch1_lna = {
    /* CH1_RX_ATTN_P5DB */
    .pin_rx_attn_p5db = { &fe_ch1_lna_io, 2 },
    /* CH1_RX_ATTN_1DB */
    .pin_rx_attn_1db  = { &fe_ch1_lna_io, 3 },
    /* CH1_RX_ATTN_2DB */
    .pin_rx_attn_2db  = { &fe_ch1_lna_io, 4 },
    /* CH1_RX_ATTN_4DB */
    .pin_rx_attn_4db  = { &fe_ch1_lna_io, 5 },
    /* CH1_RX_ATTN_8DB */
    .pin_rx_attn_8db  = { &fe_ch1_lna_io, 6 },
    /* CH1_RX_ATTN_ENB */
    .pin_rx_attn_enb  = { &fe_ch1_lna_io, 7 },
};

static Fe_Lna_Cfg fe_ch2_lna = {
    /* CH2_RX_ATTN_P5DB */
    .pin_rx_attn_p5db = { &fe_ch2_lna_io, 2 },
    /* CH2_RX_ATTN_1DB */
    .pin_rx_attn_1db  = { &fe_ch2_lna_io, 3 },
    /* CH2_RX_ATTN_2DB */
    .pin_rx_attn_2db  = { &fe_ch2_lna_io, 4 },
    /* CH2_RX_ATTN_4DB */
    .pin_rx_attn_4db  = { &fe_ch2_lna_io, 5 },
    /* CH2_RX_ATTN_8DB */
    .pin_rx_attn_8db  = { &fe_ch2_lna_io, 6 },
    /* CH2_RX_ATTN_ENB */
    .pin_rx_attn_enb  = { &fe_ch2_lna_io, 7 },
};

//FE Ch1 TX Gain control
static Fe_Ch1_Gain_Cfg fe_ch1_tx_gain_cfg = (Fe_Ch1_Gain_Cfg) {
    .fe_gain_cfg = &fe_ch1_gain,
};

//FE Ch2 TX Gain control
static Fe_Ch2_Gain_Cfg fe_ch2_tx_gain_cfg = (Fe_Ch2_Gain_Cfg) {
    /* CH1_2G_LB_BAND_SEL_L */
    .pin_ch1_2g_lb_band_sel_l = { &fe_ch2_gain_io, 0 },
    .fe_gain_cfg = &fe_ch2_gain,
};

//FE Ch1 LNA config
static Fe_Ch1_Lna_Cfg fe_ch1_rx_gain_cfg = (Fe_Ch1_Lna_Cfg) {
    .fe_lna_cfg = &fe_ch1_lna,
};

//FE Ch2 LNA config
static Fe_Ch2_Lna_Cfg fe_ch2_rx_gain_cfg = (Fe_Ch2_Lna_Cfg) {
    /* CH1_RF_PWR_OFF */
    .pin_ch1_rf_pwr_off = { &fe_ch2_lna_io, 1 },
    .fe_lna_cfg = &fe_ch2_lna,
};