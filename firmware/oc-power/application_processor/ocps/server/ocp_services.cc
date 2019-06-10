#include <iostream>
#include <memory>
#include <string>
#include <sys/time.h>

#include "bspMsgHandler.h"
#include "ocp_serviceHeader.h"
#include "ocp_services.h"
#include "telmHeader.h"

#define STATUS_OK 0x00

/*handling Alarms from OC-Power board*/
void async_msg_from_ocp_board(uint8_t* rxMsg)
{
    //Add handling part. 
    std::cout<<"OCP_SERVICE:: Adding Async msg handler."<<std::endl;

}

/* Intializing callback.*/
void ocp_init_req() 
{
    struct timeval tv;
    std::cout<<"OCP_SERVICE:: Starting ocp intialization."<<std::endl;
    ocp_msg_handler_init(async_msg_from_ocp_board);
    sleep(5);
    /*Update time on the OCP-Controller*/
    gettimeofday(&tv, NULL);
    std::cout<<"Seconds since Jan. 1, 1970: "<<tv.tv_sec<<std::endl;
    bool rc = ocp_config_rtc_time_req((uint32_t)tv.tv_sec);
    if(rc){
        std::cout<<"OCP_SERVICE::OC-Power adjusted the rtc time."<<std::endl;
     }
}

/*Creating frame fro sending the request to OC-Power board.*/
uint8_t* ocp_frame_msg(OCP_Service service, uint8_t functionId, uint8_t payLoadSize,uint8_t noOfEntries, void* payload)
{
    std::cout<<"OCP_SERVICE::Requested size for request frame is " <<+(sizeof(char)*((noOfEntries*payLoadSize)+OCP_HEADER_SIZE))<<"."<<std::endl;
    uint8_t* ocp_msg = (uint8_t*) malloc((sizeof(char)*((noOfEntries*payLoadSize)+OCP_HEADER_SIZE)));
    if(ocp_msg) {
        ocp_msg[OCP_MSG_SRVC_INDEX] = service;
        ocp_msg[OCP_MSG_FUNC_INDEX] = functionId;
        ocp_msg[OCP_MSG_FSIZE_INDEX] = payLoadSize;
        ocp_msg[OCP_MSG_FILDS_INDEX] = noOfEntries;
        ocp_msg[OCP_MSG_STATS_INDEX] = 0x00;
        memcpy(&ocp_msg[OCP_MSG_PAYLD_INDEX],payload,(payLoadSize*noOfEntries));
    }	
    return ocp_msg;	
}


/* Handling telemetry Get request. */
OCPTelemetryResponse* ocp_telemetry_data_req(OCPTelemetryRequest* ocpTelmReq)
{
    /*Serialize Request message for OC-Power.*/
    OCPTelemetryResponse* rc;
    uint8_t* ocp_req_msg = ocp_frame_msg(SERVICE_TELEMETRY,OCP_TELEMETRY_GET,sizeof(OCPTelemetryRequest),1,ocpTelmReq);
    if(ocp_req_msg) {
        std::cout<<"OCP_SERVICE::Sending OCP telemetry request message to OCP Board."<<std::endl;
        uint8_t* syncRespMsg = ocp_sync_msg_hanlder(ocp_req_msg, (sizeof(OCPTelemetryRequest)+OCP_HEADER_SIZE));
        if (syncRespMsg) {
            std::cout<<"OCP_SERVICE::Received telemetry response message from OCP Board."<<std::endl;
            if (syncRespMsg[OCP_MSG_STATS_INDEX] == STATUS_OK) {
                std::cout<<"OCP_SERVICE::No of entries: "<<+syncRespMsg[OCP_MSG_FILDS_INDEX] <<" with size "<<+syncRespMsg[OCP_MSG_FSIZE_INDEX]<<"."<<std::endl;
                OCPTelemetryResponse* resp = (OCPTelemetryResponse*)malloc(sizeof(OCPTelemetryResponse));
                if(resp) {
                    std::cout<<"OCP_SERVICE::Processing received telemetry data."<<std::endl;
                    resp->numberOfEntries = syncRespMsg[OCP_MSG_FILDS_INDEX];
                    resp->telemetryData = (OCPTelemetryData*)malloc((syncRespMsg[OCP_MSG_FILDS_INDEX]*syncRespMsg[OCP_MSG_FSIZE_INDEX]));
                    if (resp->telemetryData) {
                        memcpy(resp->telemetryData,&syncRespMsg[OCP_MSG_PAYLD_INDEX],
                                (syncRespMsg[OCP_MSG_FILDS_INDEX]*syncRespMsg[OCP_MSG_FSIZE_INDEX]));
                    }
                }
                rc = resp;          
            }		
            std::cout<<"Free syncResponse."<<std::endl;
            free(syncRespMsg);
            syncRespMsg = NULL;
        }
        std::cout<<"Free ocp request message."<<std::endl;
        free(ocp_req_msg);
        ocp_req_msg = NULL;
    }
    std::cout<<"OCP_SERVICE::Sending board response to server for serailization."<<std::endl;
    return rc;    	    
}

OCPConfigResponse* ocp_config_data_req(OCPConfigRequest* ocpConfReq)
{
    /*Serialize Config Request message for OC-Power.*/
    uint8_t* syncRespMsg;
    OCPConfigResponse* resp;
    uint8_t* ocp_req_msg = ocp_frame_msg(SERVICE_CONFIGURATION,OCP_CONFIG_ID,sizeof(OCPConfigRequest),1,ocpConfReq);
    if(ocp_req_msg) {
        std::cout<<"OCP_SERVICE::Sending OCP Config request message to OCP Board."<<std::endl;
        syncRespMsg = ocp_sync_msg_hanlder(ocp_req_msg, sizeof(OCPConfigRequest)+OCP_HEADER_SIZE);
        if (syncRespMsg) {
            std::cout<<"OCP_SERVICE::SyncResponse is "<<+syncRespMsg<<std::endl;
            if (syncRespMsg[OCP_MSG_STATS_INDEX] == STATUS_OK) {
                std::cout<<"OCP_SERVICE::No of entries: "<<+syncRespMsg[OCP_MSG_FILDS_INDEX] <<" with size "<<+syncRespMsg[OCP_MSG_FSIZE_INDEX]<<"."<<std::endl;
                resp = (OCPConfigResponse*)malloc(sizeof(OCPConfigResponse));
                if(resp) {
                    memset(resp,'\0',sizeof(OCPConfigResponse));
                    std::cout<<"OCP_SERVICE::Processing received Config data."<<std::endl;
                    memcpy(&resp->serialId[0],&syncRespMsg[OCP_MSG_PAYLD_INDEX],
                            (syncRespMsg[OCP_MSG_FILDS_INDEX]*syncRespMsg[OCP_MSG_FSIZE_INDEX]));
                }
            }
            free(syncRespMsg);
            syncRespMsg = NULL;
        }
        free(ocp_req_msg);     
        ocp_req_msg = NULL;
    }
    return resp;
}

bool ocp_config_rtc_time_req(uint32_t time)
{
    /*Serialize Config Request message for OC-Power.*/
    uint8_t* syncRespMsg;
    bool rc = false;
    uint8_t* ocp_req_msg = ocp_frame_msg(SERVICE_CONFIGURATION,OCP_CONFIG_RTC,sizeof(uint32_t),1,&time);
    if(ocp_req_msg) {
        std::cout<<"OCP_SERVICE::Sending OCP Config request message to OCP Board."<<std::endl;
        syncRespMsg = ocp_sync_msg_hanlder(ocp_req_msg, sizeof(uint32_t)+OCP_HEADER_SIZE);
        if (syncRespMsg) {
            std::cout<<"OCP_SERVICE::SyncResponse is "<<+syncRespMsg<<std::endl;
            if (syncRespMsg[OCP_MSG_STATS_INDEX] == STATUS_OK) {
                std::cout<<"OCP_SERVICE::No of entries: "<<+syncRespMsg[OCP_MSG_FILDS_INDEX] <<" with size "<<+syncRespMsg[OCP_MSG_FSIZE_INDEX]<<"."<<std::endl;
                rc = true;
            } else {
               std::cout<<"OCP_SERVICE::RTC time setting failed."<<std::endl;
            }
            free(syncRespMsg);
            syncRespMsg = NULL;
        }
        free(ocp_req_msg);
        ocp_req_msg = NULL;
    }
    return rc;
}

/* Handling Control request. */
OCPControlResponse* ocp_control_req(OCPControlRequest* ocpCtrlReq)
{
    /*Serialize Request message Control Service for OC-Power.*/
    uint8_t* syncRespMsg;
    OCPControlResponse* resp;
    uint8_t* ocp_req_msg =
    ocp_frame_msg(SERVICE_CONTROL,(OCPCtrlFuncMap)ocpCtrlReq->action,(sizeof(OCPControlRequest)-sizeof(ocpCtrlReq->action)),1,&(ocpCtrlReq->port));
    if(ocp_req_msg) {
        uint8_t* syncRespMsg = ocp_sync_msg_hanlder(ocp_req_msg, sizeof(OCPControlRequest)+OCP_HEADER_SIZE);
        if(syncRespMsg) {
            std::cout<<"OCP_SERVICE::SyncResponse is "<<+syncRespMsg<<std::endl;
            if(syncRespMsg[OCP_MSG_STATS_INDEX] == STATUS_OK) {
                std::cout<<"OCP_SERVICE::No of entries: "<<+syncRespMsg[OCP_MSG_FILDS_INDEX] <<" with size "<<+syncRespMsg[OCP_MSG_FSIZE_INDEX]<<"."<<std::endl;
                resp = (OCPControlResponse*)malloc(sizeof(OCPControlResponse));
                if(resp) {
                    memset(resp,'\0',sizeof(OCPControlResponse));
                    std::cout<<"OCP_SERVICE::Processing received control response."<<std::endl;
                    memcpy(&resp->result,&syncRespMsg[OCP_MSG_PAYLD_INDEX],
                    (syncRespMsg[OCP_MSG_FILDS_INDEX]*syncRespMsg[OCP_MSG_FSIZE_INDEX]));
                }
            }  
            free(syncRespMsg);
            syncRespMsg = NULL;
        }
        free(ocp_req_msg);			
        ocp_req_msg = NULL;       
    }
    return resp;
}

#ifdef HOST 
//Only As a stub
uint8_t* ocp_sync_msg_hanlder(uint8_t* ocp_req_msg, int length) 
{
    std::cout<< "OCP_SERVICE_STUB:Request is received with length "<<length<<"."<<std::endl;   
    uint8_t*  msg;
    if(ocp_req_msg[0]!=SERVICE_CONFIGURATION) {
        msg =(uint8_t*) malloc(sizeof(char)*66);
        {
            if(msg){
                msg[0] = 3;
                msg[1] = 1;
                msg[2] = 61;
                msg[3] = 1;
                msg[4] = 0;
                memset(&msg[5],'\0',61);
                memcpy(&msg[5],"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa0000000000000aa",61);
            }
        }
    }else {
        msg =(uint8_t*) malloc(sizeof(char)*37);
        if(msg){
            msg[0] = 1;
            msg[1] = 1;
            msg[2] = 32;
            msg[3] = 1;
            msg[4] = 0;
            memset(&msg[5],'\0',32);
            memcpy(&msg[5],"012345678901234567890123456789012",32);
        }
    }
    return msg;
}

void ocp_msg_handler_init(OCPCallbackHandlers cb)
{

}

/* Handling Control request. */
bool ocp_control_req(uint8_t ocpTelmReq)
{
    /*Serialize Request message for OC-Power.*/
    bool rc = false;
    uint8_t* ocp_req_msg = ocp_frame_msg(SERVICE_CONTROL,0x00,sizeof(OCPTelemetryRequest),1,&ocpTelmReq);
    if(ocp_req_msg) {
        uint8_t* syncRespMsg = ocp_sync_msg_hanlder(ocp_req_msg, sizeof(OCPTelemetryRequest)+OCP_HEADER_SIZE);
        if (syncRespMsg) {
            if (syncRespMsg[OCP_MSG_STATS_INDEX] == STATUS_OK) {
                rc= true;	  	
            }
            free(syncRespMsg);	  
            syncRespMsg = NULL;
        }		
        free(ocp_req_msg);			
        ocp_req_msg = NULL;       
    }
    return rc;      	    
}
#endif
