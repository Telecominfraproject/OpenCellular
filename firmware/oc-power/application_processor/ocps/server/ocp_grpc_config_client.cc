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

#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <grpcpp/grpcpp.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/server_credentials.h>


#include "ocp_services.h"
#include "configuration.grpc.pb.h"
#include "control.grpc.pb.h"
#include "telemetry.grpc.pb.h"

#define BOOL_VAL(x)    ((x)?true:false)

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using ocp::Configuration;
using ocp::IdRequest;
using ocp::IdResponse;


using ocp::TelemetryData;
using ocp::BatteryTelemetry;
using ocp::PowerPortStatus;
using ocp::TelemetryRequest;
using ocp::TelemetryResponse;
using ocp::TelemetryService;

using ocp::ActionDescription;
using ocp::ActionResult;
using ocp::Control;

typedef struct TransportChannelParam {
    int sock;
    std::shared_ptr<grpc::Channel> channel;
}TransChannelParam;

void *ocp_control_thread(void *ptr);


class ConfigurationServiceClient {
    public:
        ConfigurationServiceClient(std::shared_ptr<Channel> channel)
            : stub_ (Configuration::NewStub(channel)) {

            }

        void GetId() {
            IdRequest treq;
            IdResponse tresp;

            ClientContext context;

            treq.set_extended(false);

            std::cout << "Looking for Serial id with "<< treq.extended()<<" extended request."                                << std::endl;
            Status status = stub_->GetId(&context, treq, &tresp);

            if (status.ok()) {
                std::cout<<"Reading configuration data." <<std::endl; 
                if(tresp.has_id()) {
                    std::cout<<"Serial id read is "<<tresp.id()<<std::endl;
                }
            }
        }
    private:
        std::unique_ptr<Configuration::Stub> stub_;
};

int TcpServer()
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char hello[] = "Hello from server";

    // Creating socket file descriptor
    while ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( 8095 );

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, 
                sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                    (socklen_t*)&addrlen))<0)
    {
        perror("accept");
    }

    //valread = read( new_socket , buffer, 1024);
    //std::cout<<buffer<<std::endl;
    //send(new_socket , hello , strlen(hello) , 0 );
    return 	new_socket;
}

int GetTelemetry(int serverSocket, std::shared_ptr<grpc::Channel> channel, uint32_t startTime, uint32_t endTime)
{
    /* Send a gRPC request.*/
    std::unique_ptr<ocp::TelemetryService::Stub> stub_(
            ocp::TelemetryService::NewStub(
                channel
                )
            ); 
    TelemetryRequest treq;
    TelemetryResponse tresp;
#if 0
    uint32_t startTime = 0x0000;
    uint32_t endTime = 0x0000;
    std::cout<<"Enter the epoch time stamp for the start of Telemetry Logs:";
    std::cin>>startTime;
    std::cout<<std::endl;
    std::cout<<"Enter the epoch time stamp for the end of Telemetry Logs:";
    std::cin>>endTime;
    std::cout<<std::endl;
#endif
    ClientContext scontext;
    treq.set_from_ts(startTime);
    treq.set_to_ts(endTime);

    std::cout << "Looking data for timestamp between " << treq.from_ts()<<" to "<<treq.to_ts()
        << std::endl;
    Status status1 = stub_->GetTelemetry(&scontext, treq, &tresp);

    if (status1.ok()) {
        for(int i=0; i<tresp.data_size(); i++)
        {
            std::cout<<"Reading Telemtry data at index " <<i<< " is below :" <<std::endl; 
            const TelemetryData& data = tresp.data(i);
            if (data.has_timestamp())
                std::cout<< "TimeStamp : "<<data.timestamp()<< std::endl;
            if(data.has_timeinterval())
                std::cout<< "TimeInterval : "<<data.timeinterval() <<std::endl;
            if(data.has_p1v())
                std::cout<< "P1V : "<<(float)(data.p1v())/100 <<std::endl;
            if(data.has_p1c())
                std::cout<< "P1C : "<<(float)(data.p1c())/100 <<std::endl;
            if(data.has_adp1v())
                std::cout<< "ADP1V : "<<(float)(data.adp1v())/100 <<std::endl;
            if(data.has_adp1c())
                std::cout<< "ADP1C : "<<(float)(data.adp1c())/100 <<std::endl;
            if(data.has_l1v())
                std::cout<< "L1V : "<<(float)(data.l1v())/100 <<std::endl;
            if(data.has_l1c())
                std::cout<< "l1c : "<<(float)(data.l1c())/100 <<std::endl;
            if(data.has_l2v())
                std::cout<< "L2V : "<<(float)(data.l2v())/100 <<std::endl;
            if(data.has_l2c())
                std::cout<< "l2c : "<<(float)(data.l2c())/100 <<std::endl;
            if(data.has_l3v())
                std::cout<< "L3V : "<<(float)(data.l3v())/100 <<std::endl;
            if(data.has_l3c())
                std::cout<< "l3c : "<<(float)(data.l3c())/100 <<std::endl;
            if(data.has_l4v())
                std::cout<< "L4V : "<<(float)(data.l4v())/100 <<std::endl;
            if(data.has_l4c())
                std::cout<< "l4c : "<<(float)(data.l4c())/1000 <<std::endl;
            if(data.has_l5v())
                std::cout<< "L5V : "<<(float)(data.l5v())/100 <<std::endl;
            if(data.has_l5c())
                std::cout<< "l5c : "<<(float)(data.l5c())/1000 <<std::endl;
            if(data.has_power()) {
                std::cout<< "Power Status : " <<std::endl;
                if (data.power().has_bv())
                    std::cout<< "PowerBattery : "<<data.power().bv()<< std::endl;
                if (data.power().has_p1())
                    std::cout<< "PowerSolar : "<<data.power().p1()<< std::endl; 
                if (data.power().has_adp1())
                    std::cout<< "PowerADP : "<<data.power().adp1()<< std::endl;
                if (data.power().has_l1())
                    std::cout<< "PowerL1  : "<<data.power().l1()<< std::endl;
                if (data.power().has_l2())
                    std::cout<< "PowerL2 : "<<data.power().l2()<< std::endl;
                if (data.power().has_l3())
                    std::cout<< "PowerL3 : "<<data.power().l3()<< std::endl;
                if (data.power().has_l4())
                    std::cout<< "PowerL4 : "<<data.power().l4()<< std::endl;
                if (data.power().has_l5())
                    std::cout<< "PowerL5 : "<<data.power().l5()<< std::endl;

            }
            if(data.has_battery()) {
                std::cout<< "Battery Status : " <<std::endl;
                if(data.battery().has_bv()) 
                    std::cout<< "BatteryVoltage : "<<(float)(data.battery().bv())/100<< std::endl;
                if(data.battery().has_bc())
                    std::cout<< "BatteryCurrent : "<<(float)(data.battery().bc())/100<< std::endl;
            }
            
            for (int tempId=0;tempId<data.temp_size();tempId++) {
                std::cout<<"Temperature-"<<tempId<<" is : "<<data.temp(tempId)<<std::endl;
            }

            for (int alertID=0;alertID<data.alerts_size();alertID++) {
                std::cout<<"Alerts: at index "<<alertID<<"alert "<<data.alerts(alertID) 
                    <<std::endl;     
            }

            for (int ca=0; ca<data.customactions_size(); ca++) {
                std::cout<<"CustomActions: at index "<<ca<<"ca "<<data.customactions(ca)
                    <<std::endl; 
            } 
            std::cout<<" End for Telemetry data at index " << i << std::endl;
            
            sleep(2);

        }
    } else {
        std::cout << status1.error_code() << ": " << status1.error_message()
            << std::endl;
        std::cout << "RPC failed" <<std::endl;
        return -1;
    }
    
    return 0;
}


void GetConfig(int serverSocket, std::shared_ptr<grpc::Channel> channel)
{
    /* Send a gRPC request.*/
#if 0
        std::unique_ptr<ocp::Configuration::Stub> stub(
            ocp::Configuration::NewStub(
                grpc::CreateInsecureChannelFromFd("192.168.1.10", serverSocket)
                )
            );
#endif
    
    std::unique_ptr<ocp::Configuration::Stub> stub(
            ocp::Configuration::NewStub(
                channel
                )
            );
    IdRequest creq;
    IdResponse cresp;

    ClientContext context;

    creq.set_extended(false);

    std::cout << "Looking for Serial id with "<< creq.extended()<<" extended request."<< std::endl;
    Status status = stub->GetId(&context, creq, &cresp);

    if (status.ok()) {
        std::cout<<"Reading configuration data." <<std::endl; 
        if(cresp.has_id()){
            std::cout<<"Serial id read is "<<cresp.id()<<std::endl;
        }
    }
    sleep(2);
#if 0
    ClientContext context0;
    std::cout << "Looking for Serial id with "<< creq.extended()<<" extended request."<< std::endl;
    status = stub->GetId(&context0, creq, &cresp);

    if (status.ok()) {
        std::cout<<"Reading configuration data." <<std::endl; 
        if(cresp.has_id()){
            std::cout<<"Serial id read is "<<cresp.id()<<std::endl;
        }
    }
    sleep(2);
    ClientContext context1;
    std::cout << "Looking for Serial id with "<< creq.extended()<<" extended request."<< std::endl;
    status = stub->GetId(&context1, creq, &cresp);

    if (status.ok()) {
        std::cout<<"Reading configuration data." <<std::endl; 
        if(cresp.has_id()){
            std::cout<<"Serial id read is "<<cresp.id()<<std::endl;
        }
    }
    sleep(2);
    ClientContext context2;
    std::cout << "Looking for Serial id with "<< creq.extended()<<" extended request."<< std::endl;
    status = stub->GetId(&context2, creq, &cresp);

    if (status.ok()) {
        std::cout<<"Reading configuration data." <<std::endl; 
        if(cresp.has_id()){
            std::cout<<"Serial id read is "<<cresp.id()<<std::endl;
        }
    }
#endif
}

bool ocp_ControlInfo(ActionDescription& req)
{
    int action  = 0;
    int portNumber = 0;
    int timeSec = 0;
    bool rc = true;
    std::cout<< "Control service options for the OCPS: "<<std::endl;
    std::cout<< "1: Disable all Ports."<<std::endl;
    std::cout<< "2: Disable Port."<<std::endl;
    std::cout<< "3: Disable Port with timer."<<std::endl;
    std::cout<< "4: Enable Port."<<std::endl;
    std::cout<< "5: Enable Port with timer. "<<std::endl;
    std::cout<< "6: Enable all ports."<<std::endl;
    std::cout<< "7: Reboot."<<std::endl;

    std::cout<< "Enter your choice : "<<std::endl;
    std::cin>>action;

    switch(action) {
        case ocp::ActionDescription_Action_DISABLE_ALL_PORTS:
            {
                std::cout<<" Disabling all output ports."<<std::endl;
                req.set_action(ocp::ActionDescription_Action_DISABLE_ALL_PORTS);
            }
            break;
        case ocp::ActionDescription_Action_DISABLE_PORT: 
            {
                std::cout<<" Enter the port number to disable port: "<<std::endl;
                std::cin>>portNumber;
                req.set_action(ocp::ActionDescription_Action_DISABLE_PORT);
                req.set_value(portNumber);
            }
            break;
         case ocp::ActionDescription_Action_DISABLE_PORT_WITH_TIMER:
            {
                std::cout<<" Enter the port number you would like to disable: "<<std::endl;
                std::cin>>portNumber;
                std::cout<<" Enter the time in seconds you want to disable it for :"<<std::endl;
                std::cin>>timeSec;
                req.set_action(ocp::ActionDescription_Action_DISABLE_PORT_WITH_TIMER);
                req.set_value(portNumber);
                req.set_time(timeSec);    
            }
            break;
         case ocp::ActionDescription_Action_ENABLE_PORT:
            {
                std::cout<<" Enter the port number to enable port: "<<std::endl;
                std::cin>>portNumber;
                req.set_action(ocp::ActionDescription_Action_ENABLE_PORT);
                req.set_value(portNumber);
            }
            break;
          case ocp::ActionDescription_Action_ENABLE_PORT_WITH_TIMER:
            {
                std::cout<<" Enter the port number you would like to enable: "<<std::endl;
                std::cin>>portNumber;
                std::cout<<" Enter the time in seconds you want to enable it for :"<<std::endl;
                std::cin>>timeSec;
                req.set_action(ocp::ActionDescription_Action_ENABLE_PORT_WITH_TIMER);
                req.set_value(portNumber);
                req.set_time(timeSec);    
            }
            break;
          case ocp::ActionDescription_Action_ENABLE_ALL_PORTS:
            {
                std::cout<<" Enabling all output ports."<<std::endl;
                req.set_action(ocp::ActionDescription_Action_ENABLE_ALL_PORTS);
                req.set_value(portNumber);
            }
            break;
          case ocp::ActionDescription_Action_LOG:
            {
                std::cout<<" Log request."<<std::endl;
                req.set_action(ocp::ActionDescription_Action_LOG);
            }
            break;
          case ocp::ActionDescription_Action_REBOOT:
            {
                std::cout<<" Rebooting the remote system."<<std::endl;
                req.set_action(ocp::ActionDescription_Action_REBOOT);

            }
            break;
          default:
             {
                 std::cout<<" Not a valid option."<<std::endl;
                 rc =false;
             }
         }
         return rc;
}

void setControls(int serverSocket, std::shared_ptr<grpc::Channel> channel)
{
    /* Send a gRPC request.*/
    std::unique_ptr<ocp::Control::Stub> stub(
            ocp::Control::NewStub(
                channel
                )
            ); 
    ActionDescription actDesc;
    ActionResult actResult;
    ClientContext context;
    int rc = 0;
    //actDescription.set_action(1);
    bool ret = ocp_ControlInfo(actDesc);
    if (ret) {
        std::cout << "Sending request Message."<< std::endl;
        Status status = stub->PerformAction(&context, actDesc, &actResult);

        if (status.ok()) {
            std::cout<<"Control request response is received." <<std::endl; 
            if(actResult.has_result()){
                rc = actResult.result();
                std::cout<<"Control Status response is "<<rc<<std::endl;
            }
        }
    }
    sleep(2);
}

void *ocp_control_thread(void *args)
{
    TransChannelParam *tChannelParam = (TransChannelParam*)args;
    char input;
    while (true) {
        std::cout<<"Please press C to enter into command mode."<<std::endl;
        std::cin>>input;
        if(input == 'C') {
            std::cout<<"Selected command mode."<<std::endl;
            setControls(tChannelParam->sock, tChannelParam->channel);
        } else {
            sleep(5);
        }
    }    
    std::cout<<"Terminating thread."<<std::endl;
}

int main(int argc, char** argv)
{
    /*Get socket FD to talk to gRPC Server*/ 
    std::cout<<"Starting TCP server."<<std::endl;
    int serverSocket = TcpServer();
    struct timeval tv;
    pthread_t ocpCtrlThread;
    int rc = 0;
    TransChannelParam tChannelParam;
#ifndef ETH
    std::shared_ptr<grpc::Channel> channel = grpc::CreateInsecureChannelFromFd("192.168.7.1", serverSocket); 
#else 
    std::shared_ptr<grpc::Channel> channel = grpc::CreateInsecureChannelFromFd("10.252.1.105", serverSocket); 
#endif
    tChannelParam.sock = serverSocket;
    tChannelParam.channel = channel;

   // rc = pthread_create(&ocpCtrlThread,NULL,ocp_control_thread,(void*)&tChannelParam);
   // if(rc) {
   //     std::cout<<"pthread_create() return code : "<<rc<<std::endl;
   //     return 0;
   // }

    sleep(5);
    std::cout << "--------------Starting Requesting Data--------------" << std::endl;

    GetConfig(serverSocket, channel);
    
    uint32_t nextRead = 0;
    do {

        sleep(5);
        if (GetTelemetry(serverSocket, channel, nextRead , 0) == 0){
        gettimeofday(&tv, NULL);
        nextRead = (uint32_t)tv.tv_sec;
        std::cout<<"(Next Read) Seconds since Jan. 1, 1970: "<<nextRead<<std::endl;
        sleep(300);
        } else {
     //       status = pthread_kill(ocpCtrlThread, SIGUSR1);
     //       if (status < 0 ) {
     //           perror("pthread_kill failed.");
     //       }
          std::cout<<" Aborting client application."<<std::endl;
          abort();
        }
    } while(1);
    std::cout <<"---------------End of the Client---------"<<std::endl;
    return 0;

}

