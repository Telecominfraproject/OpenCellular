/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/server_posix.h>
#include <grpcpp/security/server_credentials.h>

#include "ocp_connectionCheck.h"
#include "ocp_services.h"
#include "ocp_helper_ip.h"
#include "configuration.grpc.pb.h"
#include "control.grpc.pb.h"
#include "telemetry.grpc.pb.h"

#define BOOL_VAL(x)    ((x)?true:false)
//#define MIN(x,y)       ( ((x)<(y))?(x):(y) )
#define CONVERT_TO_SIGNEDINT32(x) ((int32_t)((int16_t)(x)))
//Providing SSL functionality.
#define SOCAT_USED

#ifdef SOCAT_USED
#define SOCAT_PORT 50055
#else
#define GRPC_CLIENT_PORT 8085
#endif

#define MAX_NUMBER_TEMPSENSOR 4

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using ocp::TelemetryData;
using ocp::BatteryTelemetry;
using ocp::PowerPortStatus;
using ocp::TelemetryRequest;
using ocp::TelemetryResponse;
using ocp::TelemetryService;

using ocp::IdRequest;
using ocp::IdResponse;
using ocp::Configuration;

using ocp::ActionDescription;
using ocp::ActionResult;
using ocp::Control;

/* Decoding telemetry request data.*/
OCPTelemetryRequest* ocp_telemetry_req_decode(const TelemetryRequest* telmReq)
{
    OCPTelemetryRequest* req = (OCPTelemetryRequest*)malloc(sizeof(OCPTelemetryRequest));
    if(req) {
        if(telmReq->has_from_ts()) {
            req->from_ts = (uint32_t)telmReq->from_ts();
        }

        if(telmReq->has_to_ts()) {
            req->to_ts = (uint32_t)telmReq->to_ts();
        } else {
            //TODO: Add current linux timestamp or send zero.
            req->to_ts = 0;
        }
        std::cout<<"OCP_GRPC_SERVER::TR:Start time stamp: "<<req->from_ts<< " End time stamp: "<<req->to_ts <<std::endl;
    }
    return req;
}


/* Encoding telemetry request data */
void ocp_telemetry_resp_encode(OCPTelemetryResponse* resp,
        TelemetryResponse* telmResp)
{
    /* Start Encoding */
    std::cout<<"OCP_GRPC_SERVER::Starting Encoding for TelemetryResponse with "<<resp->numberOfEntries<<" entries."<<std::endl;
    for(int i = 0; i<resp->numberOfEntries; i++) {
        std::cout<<"OCP_GRPC_SERVER::Telemetry Data at: "<<i<<std::endl;
        TelemetryData *data = telmResp->add_data();
        /* Updating the voltage and current parmaters */
        data->set_timestamp(resp->telemetryData[i].timestamp);
        data->set_timeinterval(resp->telemetryData[i].timeinterval);
        data->set_p1v(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].p1v));
        data->set_p1c(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].p1c));
        data->set_adp1v(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].adp1v));
        data->set_adp1c(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].adp1c));
        data->set_l1v(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l1v));
        data->set_l1c(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l1c));
        data->set_l2v(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l2v));
        data->set_l2c(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l2c));
        data->set_l3v(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l3v));
        data->set_l3c(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l3c));
        data->set_l4v(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l4v));
        data->set_l4c(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l4c));
        data->set_l5v(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l5v));
        data->set_l5c(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].l5c));
        std::cout<<"OCP_GRPC_SERVER::Timestamp : "<<resp->telemetryData[i].timestamp<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::TimeInterval : "<<resp->telemetryData[i].timeinterval<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::P1V : "<<resp->telemetryData[i].p1v<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::P1C : "<<resp->telemetryData[i].p1c<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::ADP1V : "<<resp->telemetryData[i].adp1v<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::ADP1C : "<<resp->telemetryData[i].adp1c<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L1V : "<<resp->telemetryData[i].l1v<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L1C : "<<resp->telemetryData[i].l1c<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L2V : "<<resp->telemetryData[i].l2v<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L2C : "<<resp->telemetryData[i].l2c<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L3V : "<<resp->telemetryData[i].l3v<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L3C : "<<resp->telemetryData[i].l3c<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L4V : "<<resp->telemetryData[i].l4v<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L4C : "<<resp->telemetryData[i].l4c<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L5V : "<<resp->telemetryData[i].l5v<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::L5C : "<<resp->telemetryData[i].l5c<<std::endl;

        /* Updating port status */
        /*Update power structure*/
        PowerPortStatus power;
        power.set_bv(BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.batt));
        power.set_p1(BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.pv));
        power.set_adp1(BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.adp));
        power.set_l1(BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l1));
        power.set_l2(BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l2));
        power.set_l3(BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l3));
        power.set_l4(BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l4));
        power.set_l5(BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l5));
        std::cout<<"OCP_GRPC_SERVER::PPS Battery: "<<BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.batt)<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::PPS PV: "<<BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.pv)<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::PPS ADP: "<<BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.adp)<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::PPS L1: "<<BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l1)<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::PPS L2: "<<BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l2)<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::PPS L3: "<<BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l3)<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::PPS L4: "<<BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l4)<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::PPS L5: "<<BOOL_VAL(resp->telemetryData[i].powerPortStatus.bits.l5)<<std::endl;

        /* Copy Power telemetry to Response now. */
        data->mutable_power()->CopyFrom(power);

        /* Updating battery status */
        BatteryTelemetry battery;
        battery.set_bv(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].batteryTelemetry.bv));
        battery.set_bc(CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].batteryTelemetry.bc));
        /* Copy Battery telemetry to Response now */
        data->mutable_battery()->CopyFrom(battery);

        //std::cout<<"OCP_GRPC_SERVER::Battery BV: "<<resp->telemetryData[i].batteryTelemetry.bv<<std::endl;
        //std::cout<<"OCP_GRPC_SERVER::Battery BC: "<<resp->telemetryData[i].batteryTelemetry.bc<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::Battery BV: "<<CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].batteryTelemetry.bv)<<std::endl;
        std::cout<<"OCP_GRPC_SERVER::Battery BC: "<<CONVERT_TO_SIGNEDINT32(resp->telemetryData[i].batteryTelemetry.bc)<<std::endl;
        for(int count =0; count<MAX_NUMBER_TEMPSENSOR;count++) {
            data->add_temp(resp->telemetryData[i].ocpTempData.temp[count]);
            std::cout<<"OCP_GRPC_SERVER:: Temperature["<< count <<"] is "<<resp->telemetryData[i].ocpTempData.temp[count]<<std::endl; 
      }
        /* Updating alarm status */
        for(int32_t index=0; index<MAX_NUMBER_ALERTS; index++) {
            if( (resp->telemetryData[i].alarmStatus[index/8].alarmWord) & (1<<(index%8)) ) {
                data->add_alerts((ocp::Alert)index);
                std::cout<<"OCP_GRPC_SERVER::Alert ID: "<< index <<" added."<<std::endl;
            }
        }

        /* Updating custom actions */
        for(int32_t action=0; action<MAX_CUSTOM_ACTION; action++) {
            if(resp->telemetryData[i].customActions[action].customAction) {
                /*TODO::Dirty hack for flash failed reads till we have some data for custom actions.*/
                //resp->telemetryData[i].customActions[action].customAction = 0;
                //data->add_customactions(resp->telemetryData[i].customActions[action].customAction);
                std::cout<<"OCP_GRPC_SERVER::Added custom action "<<resp->telemetryData[i].customActions[action].customAction<<std::endl;
            }
        }
    }
}

/* Starting request service procedures. */
bool ocp_telemetry_req(const TelemetryRequest* telmReq,
        TelemetryResponse* telmResp)
{
    /* Reset Connection Monotoring Interval*/
    setConnectionMonitoringInterval();
    
    /* Decode telemetry request */
    OCPTelemetryRequest*  ocpTelmReq;
    OCPTelemetryResponse* ocpTelmResp;
    std::cout<<"OCP_GRPC_SERVER::Starting decoding of the Telemetry request."<<std::endl;
    ocpTelmReq = ocp_telemetry_req_decode(telmReq);
    if(ocpTelmReq) {
        /* Request data from OC-Power */
        std::cout<<"OCP_GRPC_SERVER::Sending Telemetry request to OCP Board."<<std::endl;
        ocpTelmResp = ::ocp_telemetry_data_req(ocpTelmReq);
        if(ocpTelmResp) {
            std::cout<<"OCP_GRPC_SERVER::Received Telemetry response from OCP Board."<<std::endl;
            /* Encode the Telemetry response */
            ocp_telemetry_resp_encode(ocpTelmResp,telmResp);
            if (ocpTelmResp->telemetryData) {
                /* Free the Telemetery data first*/
                free(ocpTelmResp->telemetryData);
            }
            /* Free the telemetry response*/
            if(ocpTelmResp) {
                free(ocpTelmResp);
                ocpTelmResp = NULL;
            }
        }
        if(ocpTelmReq) {
            free(ocpTelmReq);
            ocpTelmReq = NULL;
        }
        std::cout<<"OCP_GRPC_SERVER::Telemetry request served."<<std::endl;
    }
}


class TelemetryServiceImpl final : public TelemetryService::Service {
    public:
        explicit TelemetryServiceImpl () {
        }

        Status GetTelemetry(ServerContext* context,
                const TelemetryRequest* telmReq,
                TelemetryResponse* telmResp) {

            /* Requesting telemetry records.*/
            std::cout <<" Received a Telemetry Request. "<<std::endl;
            ocp_telemetry_req(telmReq, telmResp);
            return Status::OK;
        }
};

/* Decoding Config request data.*/
OCPConfigRequest* ocp_config_req_decode(const IdRequest* confReq)
{
    OCPConfigRequest* req = (OCPConfigRequest*)malloc(sizeof(OCPConfigRequest));
    if(req) {
        if(confReq->has_extended()) {
            req->extendedReq = confReq->extended();
        } else {
            req->extendedReq = false;
        }
        std::cout<<"OCP_GRPC_SERVER::Config Service is with "<<req->extendedReq << "Extended request."<<std::endl;
    }
    return req;
}

/* Encoding telemetry request data */
void ocp_config_resp_encode(OCPConfigResponse* resp,
        IdResponse* IdResp)
{
    /* Start Encoding */
    uint8_t stlen = strlen(resp->serialId);
    IdResp->set_id(resp->serialId, MIN(SERIAL_ID_LENGTH,stlen));
    std::cout<<"OCP_GRPC_SERVER::SerialId coded is "<<resp->serialId<<std::endl;
}

/* Starting request service procedures for configuration. */
bool ocp_config_req(const IdRequest* confReq,
        IdResponse* confResp)
{
    /* Reset Connection Monotoring Interval*/
    setConnectionMonitoringInterval();
    
    /* Decode telemetry request */
    OCPConfigRequest*  ocpConfReq;
    OCPConfigResponse* ocpConfResp;
    std::cout<<"OCP_GRPC_SERVER::Starting decoding of the Config request."<<std::endl;
    ocpConfReq = ocp_config_req_decode(confReq);
    if(ocpConfReq) {
        /* Request data from OC-Power */
        std::cout<<"OCP_GRPC_SERVER::Sending Config request to OCP Board."<<std::endl;
        ocpConfResp = ::ocp_config_data_req(ocpConfReq);
        if(ocpConfResp) {
            std::cout<<"OCP_GRPC_SERVER::Received Config repsonse from OCP Board."<<std::endl;
            /* Encode the Config response */
            ocp_config_resp_encode(ocpConfResp,confResp);
            /* Free the Config response*/
            std::cout<<"OCP_GRPC_SERVER::Free OCPS response."<<std::endl;
            free(ocpConfResp);
            ocpConfResp = NULL;
        }
        if (ocpConfReq) {
            std::cout<<"OCP_GRPC_SERVER::Free gRPC request." <<std::endl;;
            free(ocpConfReq);
            ocpConfReq = NULL;
        }
        std::cout<<"OCP_GRPC_SERVER::Config request served."<<std::endl;
    }
}

class ConfigurationServiceImpl final : public Configuration::Service {
    public:
        explicit ConfigurationServiceImpl () {
        }

        Status GetId(ServerContext* context,
                const IdRequest* confReq,
                IdResponse* confResp) {
            /* Requesting telemetry records.*/
            std::cout <<"OCP_GRPC_SERVER:: Received a Serial Id Request. "<<std::endl;
            ocp_config_req(confReq, confResp);
            return Status::OK;
        }
};

/* Decoding Control request data.*/
OCPControlRequest* ocp_control_req_decode(const ActionDescription* ctrlReq)
{
    OCPControlRequest* req = (OCPControlRequest*)malloc(sizeof(OCPControlRequest));
    if(req) {
        if(ctrlReq->has_action()) {
            req->action = ctrlReq->action();
        } else {
            req = NULL;
        }
        
        if(ctrlReq->has_value()) {
            req->port = ctrlReq->value();
        } else {
            req->port = 0x00;
        }

        if(ctrlReq->has_time()) {
            req->timer = ctrlReq->time();
        } else {
            req->timer = 0x00;
        }

        std::cout<<"OCP_GRPC_SERVER::Control Service "<<req->action << " for Port "<<req->port<<" time "<<req->timer <<std::endl;
    }
    return req;
}

/* Encoding control request data */
void ocp_control_resp_encode(OCPControlResponse* resp,
        ActionResult* ctrlResp)
{
    /* Start Encoding */

    ctrlResp->set_result((ocp::ActionResult_Result)resp->result);
    std::cout<<"OCP_GRPC_SERVER::Control response is "<<resp->result<<std::endl;
}

/* Starting request service procedures for ocp control. */
bool ocp_control_req(const ActionDescription* ctrlReq,
        ActionResult* ctrlResp)
{
    /* Reset Connection Monotoring Interval*/
    setConnectionMonitoringInterval();
    
    /* Decode telemetry request */
    OCPControlRequest*  ocpCtrlReq;
    OCPControlResponse* ocpCtrlResp;
    std::cout<<"OCP_GRPC_SERVER::Starting decoding of the Control request."<<std::endl;
    ocpCtrlReq = ocp_control_req_decode(ctrlReq);
    if(ocpCtrlReq) {
        /* Request data from OC-Power */
        std::cout<<"OCP_GRPC_SERVER::Sending Control request to OCP Board."<<std::endl;
        ocpCtrlResp = ::ocp_control_req(ocpCtrlReq);
        if(ocpCtrlResp) {
            std::cout<<"OCP_GRPC_SERVER::Received Control repsonse from OCP Board."<<std::endl;
            /* Encode the Control response */
            ocp_control_resp_encode(ocpCtrlResp,ctrlResp);
            /* Free the Control response*/
            std::cout<<"OCP_GRPC_SERVER::Free OCPS response."<<std::endl;
            free(ocpCtrlResp);
            ocpCtrlResp = NULL;
        }
        if (ocpCtrlReq) {
            std::cout<<"OCP_GRPC_SERVER::Free gRPC request." <<std::endl;;
            free(ocpCtrlReq);
            ocpCtrlReq = NULL;
        }
        std::cout<<"OCP_GRPC_SERVER::Control request served."<<std::endl;
    }
}

class ControlServiceImpl final : public Control::Service {
    public:
        explicit ControlServiceImpl () {
        }

        Status PerformAction(ServerContext* context,
                const ActionDescription* ctrlReq,
                ActionResult* ctrlResp) {

            /* Requesting telemetry records.*/
            std::cout <<" Received a Control Request. "<<std::endl;
            ocp_control_req(ctrlReq, ctrlResp);
            return Status::OK;
        }
};


int TcpClient(char* serverAdd, int strLen) {
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;

    char hello[] = "Hello from TCP client (GRPC SERVER)";
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK , 0)) < 0)
    {
        std::cout<<"Socket creation error."<<std::endl;
        return -1 ;
    }

    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        std::cout<<"OCP_GRPC_SERVER:: setsockopt(SO_REUSEADDR) failed"<<std::endl;
    }
    std::cout<<"OCP_GRPC_SERVER:: FD used for TCP client:"<<sock<<std::endl;  

    memset(&serv_addr, '0', sizeof(struct sockaddr_in));
    memset(&client_addr, '0', sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(50001);

    if (bind(sock, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in)) == 0){
        std::cout<<"OCP_GRPC_SERVER:: Binded Correctly"<<std::endl;
    }
    else {
        std::cout<<"Unable to bind"<<std::endl;
    }

#ifdef SOCAT_USED
    /*if socat is used than GRPC will be sending data to local port which will forward it to gRPC client with ssl encryption.*/
    char* clientIp = (char*)malloc(sizeof(char)*(strLen+1));
    if(clientIp){
        memset(clientIp, '\0', (strLen+1));
        memcpy(clientIp, serverAdd,strLen);
    } else {
        std::cout<<"OCP_GRPC_SERVER:: Failed to get client by name."<<std::endl;
        return -1;
    }
#else
    char *clientIp = getgRPCClientIp();
    if(!clientIp) {
        char cip[] = "192.168.1.1";
        clientIp = (char*)malloc(sizeof(char)*strlen(cip)+1);
        if(clientIp){
            memset(clientIp, '\0', strlen(cip)+1);
            memcpy(clientIp, cip, strlen(cip));
        } else {
            std::cout<<"OCP_GRPC_SERVER:: Failed to get client by name."<<std::endl;
            return -1;
        }
    }
#endif

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;

#ifdef SOCAT_USED
    client_addr.sin_port = htons(SOCAT_PORT);
    std::cout<<"OCP_GRPC_SERVER::gRPC Client (SOCAT) IP is "<<clientIp<<" Port is "
        <<SOCAT_PORT<<std::endl;

#else
    client_addr.sin_port = htons(GRPC_CLIENT_PORT);
    std::cout<<"OCP_GRPC_SERVER::gRPC Client IP is "<<clientIp<<" Port is "
        <<GRPC_CLIENT_PORT<<std::endl;
#endif

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, clientIp, &client_addr.sin_addr)<=0) 
    {
        std::cout<<"OCP_GRPC_SERVER::Invalid address/ Address not supported."<<std::endl;
        free(clientIp);
        return -1;
    }

    while (connect(sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
    {
        std::cout<<"OCP_GRPC_SERVER:: Connection Failed."<<std::endl;
        sleep(1);
    }

    //send(sock , hello , strlen(hello) , 0 );
    //std::cout<<"OCP_GRPC_SERVER::Hello message sent to grpc client."<<std::endl;
    //valread = read( sock , buffer, 1024);
    //std::cout<<"OCP_GRPC_SERVER::"<<buffer<<std::endl;
    std::cout<<"OCP_GRPC_SERVER::Starting server now."<<std::endl;
    if(clientIp){
        free(clientIp);
    }
    return sock;
}

void RunServer() {
    /* Getting IP for the gRPC server*/	
    char port[] = "50051";
    char *serverAdd;
    char* ip = getIPAddress();
    static int serverResetCount = 0;
    uint8_t ipLen = 0;
    if(ip) {
        ipLen = strlen(ip);
        std::cout<<"OCP_GRPC_SERVER:: IP Address is: "<<ip<<std::endl;
        std::cout<<"Length: "<<(ipLen+strlen(port)+1)<<std::endl;
        serverAdd = (char*)malloc(sizeof(char)*(ipLen+strlen(port)+2));
        if(serverAdd) {
            memset(serverAdd,'\0',(sizeof(char)*(ipLen+strlen(port)+2)));
            memcpy(&serverAdd[0],ip,ipLen);
            memcpy(&serverAdd[ipLen],":",1);
            memcpy(&serverAdd[ipLen+1],port,strlen(port));
        }
        else {
            std::cout<<"OCP_GRPC_SERVER::Failed while setting IP for the grpc server."<<std::endl;
            return ;
        }

        if(ip){
            free(ip);
        }
    } else {
        std::cout<<"OCP_GRPC_SERVER::No available IP found for gRPC server."<<std::endl;
        sleep(30);
        serverResetCount++;
        if(serverResetCount > MAX_TRY) {
            //Reboot;
            //TODO:: What should we do here.
        } else {
            RunServer();
        }  
    }   
    std::cout<<"OCP_GRPC_SERVER::Address for gRPC server:"<<serverAdd<<std::endl;

    /* Starting TCP client */
    int tcpClientSock = TcpClient(serverAdd, ipLen);
    if (tcpClientSock<0) {
        std::cout<<"OCP_GRPC_SERVER::Failed while createing a socket."<<std::endl;
        sleep(30);
        tcpClientSock = TcpClient(serverAdd, ipLen);
        if (tcpClientSock<0) {
            std::cout<<"OCP_GRPC_SERVER::Failed again while createing a socket."<<std::endl; 
            return;
        }
    } 
    /* Starting the gRPC server */
    std::string server_address(serverAdd);
    //std::string server_address("192.168.7.2:8085");
    TelemetryServiceImpl service;
    ConfigurationServiceImpl configService;
    ControlServiceImpl controlService;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    builder.RegisterService(&configService);
    builder.RegisterService(&controlService);
    
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout<<"OCP_GRPC_SERVER::FD used for gRPC server :"<<tcpClientSock<<std::endl;  
    grpc::AddInsecureChannelFromFd( server.get(), tcpClientSock );
    std::cout << "OCP_GRPC_SERVER::Server listening on " << server_address << std::endl;
    
    /* Start Connection Monitoring*/
    startConnectionMonitoring();
    
    server->Wait();

    if(serverAdd){
        free(serverAdd);
    }
}

int main(int argc, char** argv) {
    /*Initialize the bsp lib for ocp srever*/
    ocp_init_req();

    RunServer();

    return 0;
}
