/***************************************************************************
 *
 *    ****                              I
 *   ******                            ***
 *   *******                           ****
 *   ********    ****  ****     **** *********    ******* ****    ***********
 *   *********   ****  ****     **** *********  **************  *************
 *   **** *****  ****  ****     ****   ****    *****    ****** *****     ****
 *   ****  ***** ****  ****     ****   ****   *****      ****  ****      ****
 *  ****    *********  ****     ****   ****   ****       ****  ****      ****
 *  ****     ********  ****    ****I  ****    *****     *****  ****      ****
 *  ****      ******   ***** ******   *****    ****** *******  ****** *******
 *  ****        ****   ************    ******   *************   *************
 *  ****         ***     ****  ****     ****      *****  ****     *****  ****
 *                                                                       ****
 *          I N N O V A T I O N  T O D A Y  F O R  T O M M O R O W       ****
 *                                                                        ***
 *
 ***************************************************************************
 *
 *  Project     : SuperFemto
 *  File        : eeprom.h
 *  Description	: EEPROM interface.
 *
 *                Copyright (c) Nutaq. 2012
 *
 ***************************************************************************
 *
 * "$Revision: 1.1 $"
 * "$Name: $"
 * "$Date: 2012/06/20 02:18:30 $"
 * "$Author: Yves.Godin $"
 *
 ***************************************************************************/
#ifndef EEPROM_H__
#define EEPROM_H__

#include <stdint.h>

/****************************************************************************
 *                             Public constants                             *
 ****************************************************************************/

/**
 * EEPROM error code
 */
typedef enum
{
    EEPROM_SUCCESS          =  0,        ///< Success
    EEPROM_ERR_DEVICE       = -1,        ///< Device access error
    EEPROM_ERR_PARITY       = -2,        ///< Parity error
    EEPROM_ERR_UNAVAILABLE  = -3,        ///< Information unavailable
    EEPROM_ERR_INVALID      = -4,        ///< Invalid format
    EEPROM_ERR_UNSUPPORTED  = -5,        ///< Unsupported format
} eeprom_Error_t;


/****************************************************************************
 * Struct : eeprom_SysInfo_t
 ************************************************************************//**
 *
 * SuperFemto system information.
 *
 ***************************************************************************/
typedef struct eeprom_SysInfo
{
    char    szSn[16];               ///< Serial number
    uint8_t u8Rev;                  ///< Board revision
    uint8_t u8Tcxo;                 ///< TCXO present       (0:absent, 1:present, X:unknown)
    uint8_t u8Ocxo;                 ///< OCXO present       (0:absent, 1:present, X:unknown)
    uint8_t u8GSM850;               ///< GSM-850 supported  (0:unsupported, 1:supported, X:unknown)
    uint8_t u8GSM900;               ///< GSM-900 supported  (0:unsupported, 1:supported, X:unknown)
    uint8_t u8DCS1800;              ///< GSM-1800 supported (0:unsupported, 1:supported, X:unknown)
    uint8_t u8PCS1900;              ///< GSM-1900 supported (0:unsupported, 1:supported, X:unknown)
} eeprom_SysInfo_t;

/****************************************************************************
 * Struct : eeprom_RfClockCal_t
 ************************************************************************//**
 *
 * SuperFemto RF clock calibration.
 *
 ***************************************************************************/
typedef struct eeprom_RfClockCal
{
    int     iClkCor;                ///< Clock correction value in PPB.
    uint8_t u8ClkSrc;               ///< Clock source (0:None, 1:OCXO, 2:TCXO, 3:External, 4:GPS PPS, 5:reserved, 6:RX, 7:Edge)
} eeprom_RfClockCal_t;

/****************************************************************************
 * Struct : eeprom_TxCal_t
 ************************************************************************//**
 *
 * SuperFemto transmit calibration table.
 *
 ***************************************************************************/
typedef struct eeprom_TxCal
{
	uint8_t u8DspMajVer;            ///< DSP firmware major version
	uint8_t u8DspMinVer;            ///< DSP firmware minor version
	uint8_t u8FpgaMajVer;           ///< FPGA firmware major version
	uint8_t u8FpgaMinVer;           ///< FPGA firmware minor version
    float fTxGainGmsk[80];          ///< Gain setting for GMSK output level from +50dBm to -29 dBm
    float fTx8PskCorr;              ///< Gain adjustment for 8 PSK (default to +3.25 dB)
    float fTxExtAttCorr[31];        ///< Gain adjustment for external attenuator (0:@1dB, 1:@2dB, ..., 31:@32dB)
    float fTxRollOffCorr[374];      /**< Gain correction for each ARFCN
                                                for GSM-850 : 0=128, 1:129, ..., 123:251, [124-373]:unused
                                                for GSM-900 : 0=955, 1:956, ...,  70:1, ..., 317:956, [318-373]:unused
                                                for DCS-1800: 0=512, 1:513, ..., 373:885
                                                for PCS-1900: 0=512, 1:513, ..., 298:810, [299-373]:unused */
} eeprom_TxCal_t;

/****************************************************************************
 * Struct : eeprom_RxCal_t
 ************************************************************************//**
 *
 * SuperFemto receive calibration table.
 *
 ***************************************************************************/
typedef struct eeprom_RxCal
{
	uint8_t u8DspMajVer;            ///< DSP firmware major version
	uint8_t u8DspMinVer;            ///< DSP firmware minor version
	uint8_t u8FpgaMajVer;           ///< FPGA firmware major version
	uint8_t u8FpgaMinVer;           ///< FPGA firmware minor version
    float fExtRxGain;               ///< External RX gain
    float fRxMixGainCorr;           ///< Mixer gain error compensation
    float fRxLnaGainCorr[3];        ///< LNA gain error compensation (1:@-12 dB, 2:@-24 dB, 3:@-36 dB)
    float fRxRollOffCorr[374];      /***< Frequency roll-off compensation
                                                for GSM-850 : 0=128, 1:129, ..., 123:251, [124-373]:unused
                                                for GSM-900 : 0=955, 1:956, ...,  70:1, ..., 317:956, [318-373]:unused
                                                for DCS-1800: 0=512, 1:513, ..., 373:885
                                                for PCS-1900: 0=512, 1:513, ..., 298:810, [299-373]:unused */
    uint8_t u8IqImbalMode;          ///< IQ imbalance mode (0:off, 1:on, 2:auto)
    uint16_t u16IqImbalCorr[4];     ///< IQ imbalance compensation
} eeprom_RxCal_t;


/****************************************************************************
 *                             Public functions                             *
 ****************************************************************************/

eeprom_Error_t eeprom_ReadEthAddr( uint8_t *ethaddr );

/****************************************************************************
 * Function : eeprom_ResetCfg
 ************************************************************************//**
 *
 * This function reset the content of the EEPROM config area.
 *
 * @return
 *    0 if or an error core.
 *
 ****************************************************************************/
eeprom_Error_t eeprom_ResetCfg( void );

/****************************************************************************
 * Function : eeprom_ReadSysInfo
 ************************************************************************//**
 *
 * This function reads the system information from the EEPROM.
 *
 * @param [inout] pTime
 *    Pointer to a system info structure.
 *
 * @param [inout] pSysInfo
 *    Pointer to a system info structure.
 *
 * @return
 *    0 if or an error core.
 *
 ****************************************************************************/
eeprom_Error_t eeprom_ReadSysInfo( eeprom_SysInfo_t *pSysInfo );

/****************************************************************************
 * Function : eeprom_WriteSysInfo
 ************************************************************************//**
 *
 * This function writes the system information to the EEPROM.
 *
 * @param [in] pSysInfo
 *    Pointer to the system info structure to be written.
 *
 * @return
 *    0 if or an error core.
 *
 ****************************************************************************/
eeprom_Error_t eeprom_WriteSysInfo( const eeprom_SysInfo_t *pSysInfo );

/****************************************************************************
 * Function : eeprom_ReadRfClockCal
 ************************************************************************//**
 *
 * This function reads the RF clock calibration data from the EEPROM.
 *
 * @param [inout] pRfClockCal
 *    Pointer to a RF clock calibration structure.
 *
 * @return
 *    0 if or an error core.
 *
 ****************************************************************************/
eeprom_Error_t eeprom_ReadRfClockCal( eeprom_RfClockCal_t *pRfClockCal );

/****************************************************************************
 * Function : eeprom_WriteRfClockCal
 ************************************************************************//**
 *
 * This function writes the RF clock calibration data to the EEPROM.
 *
 * @param [in] pSysInfo
 *    Pointer to the system info structure to be written.
 *
 * @return
 *    0 if or an error core.
 *
 ****************************************************************************/
eeprom_Error_t eeprom_WriteRfClockCal( const eeprom_RfClockCal_t *pRfClockCal );

/****************************************************************************
 * Function : eeprom_ReadTxCal
 ************************************************************************//**
 *
 * This function reads the TX calibration tables for the specified band from
 * the EEPROM.
 *
 * @param [in] iBand
 *    GSM band (0:GSM-850, 1:GSM-900, 2:DCS-1800, 3:PCS-1900).
 *
 * @param [inout] pTxCal
 *    Pointer to a TX calibration table structure.
 *
 * @return
 *    0 if or an error core.
 *
 ****************************************************************************/
eeprom_Error_t eeprom_ReadTxCal( int iBand, eeprom_TxCal_t *pTxCal );

/****************************************************************************
 * Function : eeprom_WriteTxCal
 ************************************************************************//**
 *
 * This function writes the TX calibration tables for the specified band to
 * the EEPROM.
 *
 * @param [in] iBand
 *    GSM band (0:GSM-850, 1:GSM-900, 2:DCS-1800, 3:PCS-1900).
 *
 * @param [in] pTxCal
 *    Pointer to a TX calibration table structure.
 *
 * @return
 *    0 if or an error core.
 *
 ****************************************************************************/
eeprom_Error_t eeprom_WriteTxCal( int iBand, const eeprom_TxCal_t *pTxCal );

/****************************************************************************
 * Function : eeprom_ReadRxCal
 ************************************************************************//**
 *
 * This function reads the RX calibration tables for the specified band from
 * the EEPROM.
 *
 * @param [in] iBand
 *    GSM band (0:GSM-850, 1:GSM-900, 2:DCS-1800, 3:PCS-1900).
 *
 * @param [in] iUplink
 *    Uplink flag (0:downlink, X:downlink).
 *
 * @param [inout] pRxCal
 *    Pointer to a RX calibration table structure.
 *
 * @return
 *    0 if or an error core.
 *
 ****************************************************************************/
eeprom_Error_t eeprom_ReadRxCal( int iBand, int iUplink, eeprom_RxCal_t *pRxCal );

/****************************************************************************
 * Function : eeprom_WriteRxCal
 ************************************************************************//**
 *
 * This function writes the RX calibration tables for the specified band to
 * the EEPROM.
 *
 * @param [in] iBand
 *    GSM band (0:GSM-850, 1:GSM-900, 2:DCS-1800, 3:PCS-1900).
 *
 * @param [in] iUplink
 *    Uplink flag (0:downlink, X:downlink).
 *
 * @param [in] pRxCal
 *    Pointer to a RX calibration table structure.
 *
 * @return
 *    0 if or an error core.
 *
 ****************************************************************************/
eeprom_Error_t eeprom_WriteRxCal( int iBand, int iUplink, const eeprom_RxCal_t *pRxCal );

void eeprom_free_resources(void);

#endif  // EEPROM_H__
