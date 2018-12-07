#ifndef _OCMW_STUB_H_
#define _OCMW_STUB_H_

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <logger.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>

#include <occli_common.h>
#include <ocmw_occli_comm.h>
#include <math.h>
#include <ocmp_frame.h>

#define OC_EC_MSG_SIZE (64)
#define MAX_PAYLOAD_COUNT (OC_EC_MSG_SIZE - sizeof(OCMPHeader))

/* Max parameters to be stored in database */
#define MAX_NUMBER_PARAM 400
#define MAX_POST_DEVICE 400
#define MAX_I2C_COMP_NBR 1
#define MAX_GPIO_COMP_NBR 1
#define MAX_MDIO_COMP_NBR 1

/* Default values in the database */
#define DEFAULT_INT8 0x11
#define DEFAULT_INT16 0x2222
#define DEFAULT_INT32 0x33333333
#define DEFAULT_INT64 0x4444444444444444
#define DEFAULT_ENUM 0x05
#define DEFAULT_STRING "stub"

/* default values in the debug subsystem */
#define I2C_SLAVE_ADDRESS 1
#define I2C_NUM_BYTES 1
#define I2C_REG_ADDRESS 1
#define I2C_WRITE_COUNT 1
#define I2C_REG_VALUE 1
#define GPIO_PIN_NBR 1
#define GPIO_VALUE 1

/* size of strings and enum dataypes in schema */
#define SIZE_OF_TYPE_REGISTRATION 1
#define SIZE_OF_NWOP_STRUCT 3
#define SIZE_OF_LAST_ERROR 3
#define SIZE_OF_TYPE_MFG 10
#define SIZE_OF_TYPE_GETMODEL 5
#define SIZE_OF_TYPE_MODEL 4
#define SIZE_OF_NWOP_STRUCT 3
#define SIZE_OF_LAST_ERROR 3
#define SIZE_OF_TYPE_OCSERIAL_INFO 18
#define SIZE_OF_TYPE_GBCBOARD_INFO 18
#define SIZE_OF_TYPE_MACADDR 13

/* Masking related defines */
#define MASK_MSB 0xFF00
#define MASK_LSB 0xFF
#define SHIFT_NIBBLE 8

typedef struct {
    uint8_t subsystemId;
    uint8_t componentId;
    uint8_t msgtype;
    uint16_t paramId;
    uint8_t paramSize;
    uint8_t paramPos;
    uint8_t datatype;
    void *data;
} OCWareStubDatabase;

typedef struct {
    uint8_t SubsystemId;
    uint8_t DeviceNumber;
    ePostCode Status;
} OCWareStubPostData;

typedef enum {
    OCSTUB_VALUE_ZERO,
    OCSTUB_VALUE_TYPE_MFG,
    OCSTUB_VALUE_TYPE_GETMODEL,
    OCSTUB_VALUE_TYPE_MODEL,
    OCSTUB_VALUE_TYPE_REGISTRATION,
    OCSTUB_VALUE_TYPE_NWOP_STRUCT,
    OCSTUB_VALUE_TYPE_LAST_ERROR,
    OCSTUB_VALUE_TYPE_OCSERIAL_INFO,
    OCSTUB_VALUE_TYPE_GBCBOARD_INFO,
    OCSTUB_VALUE_TYPE_I2C_DEBUG,
    OCSTUB_VALUE_TYPE_GPIO_DEBUG,
    OCSTUB_VALUE_TYPE_MDIO_DEBUG,
} OCWareStubsizeflag;

typedef struct {
    uint8_t slaveAddress;
    uint8_t writeCount;
    uint32_t regAddress;
    uint8_t numOfBytes;
    uint16_t regValue;
} OCWareDebugI2Cinfo;

typedef struct {
    uint16_t regAddress;
    uint16_t regValue;
} OCWareDebugMDIOinfo;

typedef struct {
    uint8_t pin_nbr;
    uint8_t value;
} OCWareDebugGPIOinfo;

typedef enum ocware_ret { STUB_FAILED = -1, STUB_SUCCESS = 0 } ocware_stub_ret;

extern int8_t debugGetCommand;
extern int8_t debugSetCommand;
extern int8_t PostResult;
extern int8_t PostEnable;

/******************************************************************************
 * Function Name    : ocware_stub_parse_post_get_message
 * Description      : Parse post messages from MW
 *
 * @param   buffer - output pointer to the message from MW
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_parse_post_get_message(char *buffer);

/******************************************************************************
 * Function Name    : ocware_stub_parse_command_message
 * Description      : Parse command messages from MW
 *
 * @param   buffer - output pointer to the message from MW
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_parse_command_message(char *buffer);

/******************************************************************************
 * Function Name    : ocware_stub_get_set_params
 * Description      : Function to check if GET/SET operation is to be performed
 *
 * @param   msgFrameData - Pointer to the message frame field of the message
 *                         from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_get_set_params(OCMPMessage *msgFrameData);
/******************************************************************************
 * Function Name    : ocware_stub_init_database
 * Description      : Parse the schema and add entries in the DB
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_init_database(void);
/******************************************************************************
 * Function Name    : ocware_stub_send_msgframe_middleware
 * Description      : send message to the MW
 *
 * @param bufferlen - length of the message (by value)
 * @param buffer  - pointer to the message to be sent to MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_send_msgframe_middleware(char **buffer,
                                                            int32_t bufferlen);
/******************************************************************************
 * Function Name    : ocware_stub_init_ethernet_comm
 * Description      : initialise the socket IPC
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_init_ethernet_comm(void);
/******************************************************************************
 * Function Name    : ocware_stub_deinit_ethernet_comm
 * Description      : close the IPC socket
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_deinit_ethernet_comm();
/******************************************************************************
 * Function Name    : ocware_stub_recv_msgfrom_middleware
 * Description      : Receive message from MW
 *
 * @param bufferlen - length of the message (by value)
 * @param buffer  - pointer to the location where the message from MW is to be
 *                  stored for further processing (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_recv_msgfrom_middleware(char **buffer,
                                                           int32_t bufferlen);
/******************************************************************************
 * Function Name    : ocware_stub_get_database
 * Description      : Function to retrieve data from the DB
 *
 * @param   msgFrameData - Pointer to the message frame field of the message
 *                         from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_get_database(OCMPMessage *msgFrameData);
/******************************************************************************
 * Function Name    : ocware_stub_set_database
 * Description      : Function to modify data in the DB
 *
 * @param   msgFrameData - Pointer to the message frame field of the message
 *                         from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_set_database(OCMPMessage *msgFrameData);
/******************************************************************************
 * Function Name    : ocware_stub_get_post_result_paramvalue_from_table
 * Description      : Fill payload with the post information
 *
 * @param   message - output pointer to the message from MW
 * @param   payload - output pointer to the payload field of the message from MW
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret
ocware_stub_get_post_result_paramvalue_from_table(OCMPMessage *msgFrameData,
                                                  int8_t *payload);
/******************************************************************************
 * Function Name    :  ocware_stub_parse_debug_actiontype
 * Description      :  Convert debug actiontype into the SET/GET
 *
 * @param msgFrameData  - output pointer to the OCMPheader field of the message
 *                     from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret
ocware_stub_parse_debug_actiontype(OCMPMessage *msgFrameData);
/******************************************************************************
 * Function Name    :  ocware_stub_get_post_database
 * Description      :  extract device number and status from the post database
 *                     depending on the subsytem id
 *
 * @param msgFrameData  - output pointer to the OCMPheader field of the message
 *                     from MW (by reference)
 * @param   payload - output pointer to the payload field of the message from MW
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
extern ocware_stub_ret ocware_stub_get_post_database(OCMPMessage *msgFrameData,
                                                     char *payload);

#endif /* __OCMW_STUB_H__ */
