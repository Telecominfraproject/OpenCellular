/*
 * ocp_serviceHeader.h
 *
 *  Created on: Aug 3, 2018
 *      Author: vthakur
 */
#ifndef ocp_serviceHeader
#define ocp_serviceHeader

#define OCP_HEADER_SIZE    5 

#define OCP_MSG_SRVC_INDEX    0
#define OCP_MSG_FUNC_INDEX    1
#define OCP_MSG_FSIZE_INDEX	   2
#define OCP_MSG_FILDS_INDEX	   3
#define OCP_MSG_STATS_INDEX    4
#define OCP_MSG_PAYLD_INDEX    5

typedef struct {
    uint8_t service; 
    uint8_t function;
    uint8_t length; 
    uint8_t noOffields;
    uint8_t status;
    uint8_t* payload;
}__attribute__((packed, aligned(1))) OCP_MSG;

typedef enum {
    SERVICE_TEST = 0,   
    SERVICE_CONFIGURATION = 1,	 
    SERVICE_CONTROL = 2,
    SERVICE_TELEMETRY = 3,		
    SERVICE_ALERT = 4,
    SERVICE_MAX
} OCP_Service;



#endif
