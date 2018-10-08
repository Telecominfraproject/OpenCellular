/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== USBCDCD.c ========
 */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/gates/GateMutex.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <stdbool.h>
#include <stdint.h>

/* driverlib Header files */
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <inc/interfaces/usbcdcd.h>
#include <usblib/usb-ids.h>
#include <usblib/usblib.h>
#include <usblib/usbcdc.h>
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdcdc.h>

/* Example/Board Header files */

#if defined(TIVAWARE)
typedef uint32_t USBCDCDEventType;
#else
#define eUSBModeForceDevice USB_MODE_FORCE_DEVICE
typedef unsigned long USBCDCDEventType;
#endif

/* Defines */
#define USBBUFFERSIZE 256

/* Typedefs */
typedef volatile enum {
    USBCDCD_STATE_IDLE = 0,
    USBCDCD_STATE_INIT,
    USBCDCD_STATE_UNCONFIGURED
} USBCDCD_USBState;

/* Static variables and handles */
static volatile USBCDCD_USBState state;
static unsigned char receiveBuffer[USBBUFFERSIZE];
static unsigned char transmitBuffer[USBBUFFERSIZE];

static GateMutex_Handle gateTxSerial;
static GateMutex_Handle gateRxSerial;
static GateMutex_Handle gateUSBWait;
static Semaphore_Handle semTxSerial;
static Semaphore_Handle semRxSerial;
static Semaphore_Handle semUSBConnected;

/* Function prototypes */
static USBCDCDEventType cbRxHandler(void *cbData, USBCDCDEventType event,
                                    USBCDCDEventType eventMsg,
                                    void *eventMsgPtr);
static USBCDCDEventType cbSerialHandler(void *cbData, USBCDCDEventType event,
                                        USBCDCDEventType eventMsg,
                                        void *eventMsgPtr);
static USBCDCDEventType cbTxHandler(void *cbData, USBCDCDEventType event,
                                    USBCDCDEventType eventMsg,
                                    void *eventMsgPtr);
static Void USBCDCD_hwiHandler(UArg arg0);
static unsigned int rxData(unsigned char *pStr, unsigned int length,
                           unsigned int timeout);
static unsigned int txData(const unsigned char *pStr, int length,
                           unsigned int timeout);
void USBCDCD_init(void);
unsigned int USBCDCD_receiveData(unsigned char *pStr, unsigned int length,
                                 unsigned int timeout);
unsigned int USBCDCD_sendData(const unsigned char *pStr, unsigned int length,
                              unsigned int timeout);
bool USBCDCD_waitForConnect(unsigned int timeout);

/* The languages supported by this device. */
const unsigned char langDescriptor[] = { 4, USB_DTYPE_STRING,
                                         USBShort(USB_LANG_EN_US) };

/* The manufacturer string. */
const unsigned char manufacturerString[] = {
    (17 + 1) * 2, USB_DTYPE_STRING,
    'T',          0,
    'e',          0,
    'x',          0,
    'a',          0,
    's',          0,
    ' ',          0,
    'I',          0,
    'n',          0,
    's',          0,
    't',          0,
    'r',          0,
    'u',          0,
    'm',          0,
    'e',          0,
    'n',          0,
    't',          0,
    's',          0,
};

/* The product string. */
const unsigned char productString[] = { 2 + (16 * 2), USB_DTYPE_STRING,
                                        'V',          0,
                                        'i',          0,
                                        'r',          0,
                                        't',          0,
                                        'u',          0,
                                        'a',          0,
                                        'l',          0,
                                        ' ',          0,
                                        'C',          0,
                                        'O',          0,
                                        'M',          0,
                                        ' ',          0,
                                        'P',          0,
                                        'o',          0,
                                        'r',          0,
                                        't',          0 };

/* The serial number string. */
const unsigned char serialNumberString[] = { (8 + 1) * 2, USB_DTYPE_STRING,
                                             '1',         0,
                                             '2',         0,
                                             '3',         0,
                                             '4',         0,
                                             '5',         0,
                                             '6',         0,
                                             '7',         0,
                                             '8',         0 };

/* The interface description string. */
const unsigned char controlInterfaceString[] = { 2 + (21 * 2), USB_DTYPE_STRING,
                                                 'A',          0,
                                                 'C',          0,
                                                 'M',          0,
                                                 ' ',          0,
                                                 'C',          0,
                                                 'o',          0,
                                                 'n',          0,
                                                 't',          0,
                                                 'r',          0,
                                                 'o',          0,
                                                 'l',          0,
                                                 ' ',          0,
                                                 'I',          0,
                                                 'n',          0,
                                                 't',          0,
                                                 'e',          0,
                                                 'r',          0,
                                                 'f',          0,
                                                 'a',          0,
                                                 'c',          0,
                                                 'e',          0 };

/* The configuration description string. */
const unsigned char configString[] = { 2 + (26 * 2), USB_DTYPE_STRING,
                                       'S',          0,
                                       'e',          0,
                                       'l',          0,
                                       'f',          0,
                                       ' ',          0,
                                       'P',          0,
                                       'o',          0,
                                       'w',          0,
                                       'e',          0,
                                       'r',          0,
                                       'e',          0,
                                       'd',          0,
                                       ' ',          0,
                                       'C',          0,
                                       'o',          0,
                                       'n',          0,
                                       'f',          0,
                                       'i',          0,
                                       'g',          0,
                                       'u',          0,
                                       'r',          0,
                                       'a',          0,
                                       't',          0,
                                       'i',          0,
                                       'o',          0,
                                       'n',          0 };

/* The descriptor string table. */
const unsigned char *const stringDescriptors[] = {
    langDescriptor,     manufacturerString,     productString,
    serialNumberString, controlInterfaceString, configString
};

#define STRINGDESCRIPTORSCOUNT \
    (sizeof(stringDescriptors) / sizeof(unsigned char *))

#if defined(TIVAWARE)
tUSBBuffer txBuffer;
tUSBBuffer rxBuffer;
static tUSBDCDCDevice serialDevice;

tUSBBuffer rxBuffer = {
    false, /* This is a receive buffer. */
    cbRxHandler, /* pfnCallback */
    (void *)&serialDevice, /* Callback data is our device pointer. */
    USBDCDCPacketRead, /* pfnTransfer */
    USBDCDCRxPacketAvailable, /* pfnAvailable */
    (void *)&serialDevice, /* pvHandle */
    receiveBuffer, /* pcBuffer */
    USBBUFFERSIZE, /* ulBufferSize */
    { { 0, 0, 0, 0 }, 0, 0 } /* private data workspace */
};

tUSBBuffer txBuffer = {
    true, /* This is a transmit buffer. */
    cbTxHandler, /* pfnCallback */
    (void *)&serialDevice, /* Callback data is our device pointer. */
    USBDCDCPacketWrite, /* pfnTransfer */
    USBDCDCTxPacketAvailable, /* pfnAvailable */
    (void *)&serialDevice, /* pvHandle */
    transmitBuffer, /* pcBuffer */
    USBBUFFERSIZE, /* ulBufferSize */
    { { 0, 0, 0, 0 }, 0, 0 } /* private data workspace */
};

static tUSBDCDCDevice serialDevice = { USB_VID_TI_1CBE,
                                       USB_PID_SERIAL,
                                       0,
                                       USB_CONF_ATTR_SELF_PWR,

                                       cbSerialHandler,
                                       NULL,

                                       USBBufferEventCallback,
                                       (void *)&rxBuffer,

                                       USBBufferEventCallback,
                                       (void *)&txBuffer,

                                       stringDescriptors,
                                       STRINGDESCRIPTORSCOUNT };
#else
const tUSBBuffer rxBuffer;
const tUSBBuffer txBuffer;
static unsigned char receiveBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];
static unsigned char transmitBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];

static tCDCSerInstance serialInstance;
const tUSBDCDCDevice serialDevice;

const tUSBBuffer rxBuffer = {
    false, /* This is a receive buffer. */
    cbRxHandler, /* pfnCallback */
    (void *)&serialDevice, /* Callback data is our device pointer. */
    USBDCDCPacketRead, /* pfnTransfer */
    USBDCDCRxPacketAvailable, /* pfnAvailable */
    (void *)&serialDevice, /* pvHandle */
    receiveBuffer, /* pcBuffer */
    USBBUFFERSIZE, /* ulBufferSize */
    receiveBufferWorkspace /* pvWorkspace */
};

const tUSBBuffer txBuffer = {
    true, /* This is a transmit buffer. */
    cbTxHandler, /* pfnCallback */
    (void *)&serialDevice, /* Callback data is our device pointer. */
    USBDCDCPacketWrite, /* pfnTransfer */
    USBDCDCTxPacketAvailable, /* pfnAvailable */
    (void *)&serialDevice, /* pvHandle */
    transmitBuffer, /* pcBuffer */
    USBBUFFERSIZE, /* ulBufferSize */
    transmitBufferWorkspace /* pvWorkspace */
};

const tUSBDCDCDevice serialDevice = {
    USB_VID_TI,
    USB_PID_SERIAL,
    0,
    USB_CONF_ATTR_SELF_PWR,

    cbSerialHandler,
    NULL,

    USBBufferEventCallback,
    (void *)&rxBuffer,

    USBBufferEventCallback,
    (void *)&txBuffer,

    stringDescriptors,
    STRINGDESCRIPTORSCOUNT,

    &serialInstance /* Old usblib stores a pointer */
};
#endif

static tLineCoding g_sLineCoding = {
    115200, /* 115200 baud rate. */
    1, /* 1 Stop Bit. */
    0, /* No Parity. */
    8 /* 8 Bits of data. */
};

/*
 *  ======== cbRxHandler ========
 *  Callback handler for the USB stack.
 *
 *  Callback handler call by the USB stack to notify us on what has happened in
 *  regards to the keyboard.
 *
 *  @param(cbData)          A callback pointer provided by the client.
 *
 *  @param(event)           Identifies the event that occurred in regards to
 *                          this device.
 *
 *  @param(eventMsgData)    A data value associated with a particular event.
 *
 *  @param(eventMsgPtr)     A data pointer associated with a particular event.
 *
 */
static USBCDCDEventType cbRxHandler(void *cbData, USBCDCDEventType event,
                                    USBCDCDEventType eventMsg,
                                    void *eventMsgPtr)
{
    switch (event) {
        case USB_EVENT_RX_AVAILABLE:
            Semaphore_post(semRxSerial);
            break;

        case USB_EVENT_DATA_REMAINING:
            break;

        case USB_EVENT_REQUEST_BUFFER:
            break;

        default:
            break;
    }

    return (0);
}

/*
 *  ======== cbSerialHandler ========
 *  Callback handler for the USB stack.
 *
 *  Callback handler call by the USB stack to notify us on what has happened in
 *  regards to the keyboard.
 *
 *  @param(cbData)          A callback pointer provided by the client.
 *
 *  @param(event)           Identifies the event that occurred in regards to
 *                          this device.
 *
 *  @param(eventMsgData)    A data value associated with a particular event.
 *
 *  @param(eventMsgPtr)     A data pointer associated with a particular event.
 *
 */
static USBCDCDEventType cbSerialHandler(void *cbData, USBCDCDEventType event,
                                        USBCDCDEventType eventMsg,
                                        void *eventMsgPtr)
{
    tLineCoding *psLineCoding;

    /* Determine what event has happened */
    switch (event) {
        case USB_EVENT_CONNECTED:
            state = USBCDCD_STATE_INIT;
            Semaphore_post(semUSBConnected);
            break;

        case USB_EVENT_DISCONNECTED:
            state = USBCDCD_STATE_UNCONFIGURED;
            break;

        case USBD_CDC_EVENT_GET_LINE_CODING:
            /* Create a pointer to the line coding information. */
            psLineCoding = (tLineCoding *)eventMsgPtr;

            /* Copy the current line coding information into the structure. */
            *(psLineCoding) = g_sLineCoding;
            break;

        case USBD_CDC_EVENT_SET_LINE_CODING:
            /* Create a pointer to the line coding information. */
            psLineCoding = (tLineCoding *)eventMsgPtr;

            /*
             * Copy the line coding information into the current line coding
             * structure.
             */
            g_sLineCoding = *(psLineCoding);
            break;

        case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
            break;

        case USBD_CDC_EVENT_SEND_BREAK:
            break;

        case USBD_CDC_EVENT_CLEAR_BREAK:
            break;

        case USB_EVENT_SUSPEND:
            break;

        case USB_EVENT_RESUME:
            break;

        default:
            break;
    }

    return (0);
}

/*
 *  ======== cbTxHandler ========
 *  Callback handler for the USB stack.
 *
 *  Callback handler call by the USB stack to notify us on what has happened in
 *  regards to the keyboard.
 *
 *  @param(cbData)          A callback pointer provided by the client.
 *
 *  @param(event)           Identifies the event that occurred in regards to
 *                          this device.
 *
 *  @param(eventMsgData)    A data value associated with a particular event.
 *
 *  @param(eventMsgPtr)     A data pointer associated with a particular event.
 *
 */
static USBCDCDEventType cbTxHandler(void *cbData, USBCDCDEventType event,
                                    USBCDCDEventType eventMsg,
                                    void *eventMsgPtr)
{
    switch (event) {
        case USB_EVENT_TX_COMPLETE:
            /*
             * Data was sent, so there should be some space available on the
             * buffer
             */
            Semaphore_post(semTxSerial);
            break;

        default:
            break;
    }

    return (0);
}

/*
 *  ======== USBCDCD_hwiHandler ========
 *  This function calls the USB library's device interrupt handler.
 */
static Void USBCDCD_hwiHandler(UArg arg0)
{
    USB0DeviceIntHandler();
}

/*
 *  ======== rxData ========
 */
static unsigned int rxData(unsigned char *pStr, unsigned int length,
                           unsigned int timeout)
{
    unsigned int read = 0;

    if (USBBufferDataAvailable(&rxBuffer) ||
        Semaphore_pend(semRxSerial, timeout)) {
        read = USBBufferRead(&rxBuffer, pStr, length);
    }

    return (read);
}

/*
 *  ======== txData ========
 */
static unsigned int txData(const unsigned char *pStr, int length,
                           unsigned int timeout)
{
    unsigned int buffAvailSize;
    unsigned int bufferedCount = 0;
    unsigned int sendCount = 0;
    unsigned char *sendPtr;

    while (bufferedCount != length) {
        /* Determine the buffer size available */
        buffAvailSize = USBBufferSpaceAvailable(&txBuffer);

        /* Determine how much needs to be sent */
        if ((length - bufferedCount) > buffAvailSize) {
            sendCount = buffAvailSize;
        } else {
            sendCount = length - bufferedCount;
        }

        /* Adjust the pointer to the data */
        sendPtr = (unsigned char *)pStr + bufferedCount;

        /* Place the contents into the USB BUffer */
        bufferedCount += USBBufferWrite(&txBuffer, sendPtr, sendCount);

        /* Pend until some data was sent through the USB*/
        if (!Semaphore_pend(semTxSerial, timeout)) {
            break;
        }
    }

    return (bufferedCount);
}

/*
 *  ======== USBCDCD_init ========
 */
void USBCDCD_init(void)
{
    Hwi_Handle hwi;
    Error_Block eb;
    Semaphore_Params semParams;

    Error_init(&eb);

    /* Install interrupt handler */
    hwi = Hwi_create(INT_USB0, USBCDCD_hwiHandler, NULL, &eb);
    if (hwi == NULL) {
        System_abort("Can't create USB Hwi");
    }

    /* RTOS primitives */
    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    semTxSerial = Semaphore_create(0, &semParams, &eb);
    if (semTxSerial == NULL) {
        System_abort("Can't create TX semaphore");
    }

    semRxSerial = Semaphore_create(0, &semParams, &eb);
    if (semRxSerial == NULL) {
        System_abort("Can't create RX semaphore");
    }

    semUSBConnected = Semaphore_create(0, &semParams, &eb);
    if (semUSBConnected == NULL) {
        System_abort("Can't create USB semaphore");
    }

    gateTxSerial = GateMutex_create(NULL, &eb);
    if (gateTxSerial == NULL) {
        System_abort("Can't create gate");
    }

    gateRxSerial = GateMutex_create(NULL, &eb);
    if (gateRxSerial == NULL) {
        System_abort("Can't create gate");
    }

    gateUSBWait = GateMutex_create(NULL, &eb);
    if (gateUSBWait == NULL) {
        System_abort("Could not create USB Wait gate");
    }

    /* State specific variables */
    state = USBCDCD_STATE_UNCONFIGURED;

    /* Set the USB stack mode to Device mode with VBUS monitoring */
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    /*
     * Pass our device information to the USB HID device class driver,
     * initialize the USB controller and connect the device to the bus.
     */
    if (!USBDCDCInit(0, &serialDevice)) {
        System_abort("Error initializing the serial device");
    }
}

/*
 *  ======== USBCDCD_receiveData ========
 */
unsigned int USBCDCD_receiveData(unsigned char *pStr, unsigned int length,
                                 unsigned int timeout)
{
    unsigned int retValue = 0;
    unsigned int key;

    switch (state) {
        case USBCDCD_STATE_UNCONFIGURED:
            USBCDCD_waitForConnect(timeout);
            break;

        case USBCDCD_STATE_INIT:
            /* Acquire lock */
            key = GateMutex_enter(gateRxSerial);

            USBBufferInit(&txBuffer);
            USBBufferInit(&rxBuffer);

            state = USBCDCD_STATE_IDLE;

            retValue = rxData(pStr, length, timeout);

            /* Release lock */
            GateMutex_leave(gateRxSerial, key);
            break;

        case USBCDCD_STATE_IDLE:
            /* Acquire lock */
            key = GateMutex_enter(gateRxSerial);

            retValue = rxData(pStr, length, timeout);

            /* Release lock */
            GateMutex_leave(gateRxSerial, key);
            break;

        default:
            break;
    }

    return (retValue);
}

/*
 *  ======== USBCDCD_sendData ========
 */
unsigned int USBCDCD_sendData(const unsigned char *pStr, unsigned int length,
                              unsigned int timeout)
{
    unsigned int retValue = 0;
    unsigned int key;

    switch (state) {
        case USBCDCD_STATE_UNCONFIGURED:
            USBCDCD_waitForConnect(timeout);
            break;

        case USBCDCD_STATE_INIT:
            /* Acquire lock */
            key = GateMutex_enter(gateTxSerial);

            USBBufferInit(&txBuffer);
            USBBufferInit(&rxBuffer);

            state = USBCDCD_STATE_IDLE;

            retValue = txData(pStr, length, timeout);

            /* Release lock */
            GateMutex_leave(gateTxSerial, key);
            break;

        case USBCDCD_STATE_IDLE:
            /* Acquire lock */
            key = GateMutex_enter(gateTxSerial);

            retValue = txData(pStr, length, timeout);

            /* Release lock */
            GateMutex_leave(gateTxSerial, key);
            break;

        default:
            break;
    }

    return (retValue);
}

/*
 *  ======== USBCDCD_waitForConnect ========
 */
bool USBCDCD_waitForConnect(unsigned int timeout)
{
    bool ret = true;
    unsigned int key;

    /* Need exclusive access to prevent a race condition */
    key = GateMutex_enter(gateUSBWait);

    if (state == USBCDCD_STATE_UNCONFIGURED) {
        if (!Semaphore_pend(semUSBConnected, timeout)) {
            ret = false;
        }
    }

    GateMutex_leave(gateUSBWait, key);

    return (ret);
}
