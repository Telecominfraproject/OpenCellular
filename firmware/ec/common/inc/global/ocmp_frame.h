/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OCMP_FRAME_H_
#define OCMP_FRAME_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include <stdint.h>

/*****************************************************************************
 *                            MACRO DEFINITIONS
 *****************************************************************************/
/* Start Of Frame & Message Lengths */
#define OCMP_MSG_SOF 0x55
#define OCMP_FRAME_TOTAL_LENGTH 64
#define OCMP_FRAME_HEADER_LENGTH 17
#define OCMP_FRAME_MSG_LENGTH \
    (OCMP_FRAME_TOTAL_LENGTH - OCMP_FRAME_HEADER_LENGTH)

/*****************************************************************************
 *                          STRUCT/ENUM DEFINITIONS
 *****************************************************************************/

typedef enum {
    OC_SS_BB = -1, //Hack around the fact that IPC reuses OCMP to allow us
    //  to split BB (internal) and SYS (CLI) message handling
    OC_SS_SYS = 0,
    OC_SS_PWR,
    OC_SS_BMS,
    OC_SS_HCI,
    OC_SS_ETH_SWT,
    OC_SS_OBC,
    OC_SS_GPP,
    OC_SS_SDR,
    OC_SS_RF,
    OC_SS_SYNC,
    OC_SS_TEST_MODULE,
    OC_SS_DEBUG,
    OC_SS_MAX_LIMIT, //TODO:REV C Change
    OC_SS_WD
    //OC_SS_ALERT_MNGR,
    //OC_SS_MAX_LIMIT
} OCMPSubsystem;

typedef enum {
    OCMP_COMM_IFACE_UART = 1, // Uart             - 1
    OCMP_COMM_IFACE_ETHERNET, // Ethernet         - 2
    OCMP_COMM_IFACE_SBD, // SBD(Satellite)   - 3
    OCMP_COMM_IFACE_USB // Usb              - 4
} OCMPInterface;

/*
 * OCMPMsgType - msg type specifies what is the communication all about.
 * It can be Configuration, Status, Alert, Command, Watchdog, Debug
 * OCMPMsgType 1 byte message.
 * |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
 * ||     7     ||        6     ||        5     ||        4     ||        3     ||        2     ||        1     ||        0     ||
 * |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
   ||                                                       Message Type                                                         ||
 * |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
 */
typedef enum {
    OCMP_MSG_TYPE_CONFIG = 1,
    OCMP_MSG_TYPE_STATUS,
    OCMP_MSG_TYPE_ALERT,
    OCMP_MSG_TYPE_COMMAND,
    OCMP_MSG_TYPE_WATCHDOG,
    OCMP_MSG_TYPE_DEBUG,
    OCMP_MSG_TYPE_EVENTINFO,
    OCMP_MSG_TYPE_ITCMSG,
    OCMP_MSG_TYPE_POST,
} OCMPMsgType;

/*
 * OCMPActionType - It is about setting something or getting or Reply back.
 */
typedef enum {
    OCMP_AXN_TYPE_GET = 1,
    OCMP_AXN_TYPE_SET,
    OCMP_AXN_TYPE_REPLY,
    OCMP_AXN_TYPE_ACTIVE,
    OCMP_AXN_TYPE_CLEAR,
    OCMP_AXN_TYPE_RESET,
    OCMP_AXN_TYPE_ENABLE,
    OCMP_AXN_TYPE_DISABLE,
    OCMP_AXN_REG_READ,
    OCMP_AXN_REG_WRITE,
    OCMP_AXN_TYPE_ECHO,

    /* TODO: Really shouldn't be generic commands, but keeping here for now
     * because refactoring would be too painful just to test the CLI
     */
    OCMP_AXN_DIS_NETWORK,
    OCMP_AXN_CONN_NETWORK,
    OCMP_AXN_SEND_SMS,
    OCMP_AXN_DIAL_NUMBER,
    OCMP_AXN_ANSWER,
    OCMP_AXN_HANGUP,
    OCMP_NUM_ACTIONS
} OCMPActionType;

typedef enum {
    OCMP_SENSOR_EVENT = 1,
    OCMP_TEMP_EVENT,
    OCMP_CURRENT_EVENT,
    OCMP_REV_POWER_EVENT,
    OCMP_BATT_EVENT,
    OCMP_PSE_EVENT,
    OCMP_PD_EVENT,
    OCMP_ETH_EVENT,
    OCMP_GPS_EVENT
} OCMPEvents;

/*
 * eOCMPDebugOperation - This forms the complete debug message frame for
 * communication with EC from External entity (e.g. AP over UART, Ethernet
 * or SBD)
 */
typedef enum { OCMP_DEBUG_READ = 1, OCMP_DEBUG_WRITE } eOCMPDebugOperation;

/* TODO::This OCWARE_HOST has to be removed with OCMP cleanUp*/
#ifndef OCWARE_HOST
#define OC_SS OCMPSubsystem
#define OC_MSG_TYP OCMPMsgType
#define OC_AXN_TYP OCMPActionType
#else
#define OC_SS uint8_t
#define OC_MSG_TYP uint8_t
#define OC_AXN_TYP uint8_t
#define OC_IFACE_TYP uint8_t
#endif
/*
 * Header is the field which will be containing SOF, Framelen,
 * Source Interface, Sequence number, and timestamp.
 */
typedef struct __attribute__((packed, aligned(1))) {
    uint8_t ocmpSof; // SOF - It must be 0x55
    uint8_t ocmpFrameLen; // Framelen - tells about the configuration size ONLY.
    OCMPInterface ocmpInterface; // Interface - UART/Ethernet/SBD
    uint32_t ocmpSeqNumber; // SeqNo - Don't know!!!
    uint32_t ocmpTimestamp; // Timestamp - When AP sent the command?
} OCMPHeader;

/*
 * This is the Message structure for Subsystem level information
 */
typedef struct __attribute__((packed, aligned(1))) {
    OC_SS subsystem; // RF/GPP/BMS/Watchdog etc..
    uint8_t componentID; // Compononent ID. Different for different subsystem.
    OCMPMsgType
            msgtype; // Msg type is Config/Status/Alert/Command/Watchdog/Debug
    uint8_t action; // Action is - Get/Set/Reply.
    uint16_t parameters; // List of Parameters to be set or get.
#ifndef OCWARE_HOST
    uint8_t ocmp_data[]; // The data payload.
#else
    int8_t *info;
#endif
} OCMPMessage;

/*
 * OCMPMessageFrame - This forms the complete message frame for communication
 * with EC from External entity (e.g. AP over UART, Ethernet or SBD)
 */
typedef struct __attribute__((packed, aligned(1))) {
    OCMPHeader header;
    OCMPMessage message;
} OCMPMessageFrame;

#endif /* OCMP_FRAME_H_ */
