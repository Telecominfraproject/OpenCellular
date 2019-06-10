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

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/server_credentials.h>

#include "ocp_services.h"
#include "telemetry.grpc.pb.h"

#define BOOL_VAL(x)    ((x)?true:false)

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using ocp::TelemetryData;
using ocp::BatteryTelemetry;
using ocp::PowerPortStatus;
using ocp::TelemetryRequest;
using ocp::TelemetryResponse;
using ocp::TelemetryService;

class TelemetryServiceClient {
    public:
        TelemetryServiceClient(std::shared_ptr<Channel> channel)
            : stub_ (TelemetryService::NewStub(channel)) {
            
        }

        void GetTelemetry() {
            TelemetryRequest treq;
            TelemetryResponse tresp;
            
            ClientContext context;

            treq.set_from_ts(111111);
            treq.set_to_ts(222222);

            std::cout << "Looking data for timestamp between " << treq.from_ts()<<" to "<<treq.to_ts()
                      << std::endl;
            Status status = stub_->GetTelemetry(&context, treq, &tresp);
            
            if (status.ok()) {
                for(int i=0; i<tresp.data_size(); i++)
                {
                   std::cout<<"Reading Telemtry data at index " <<i<< " is below :" <<std::endl; 
                   const TelemetryData& data = tresp.data(i);
                   if (data.has_timestamp())
                       std::cout<< "Time stamp : "<<data.timestamp()<< std::endl;
                   if(data.has_timeinterval())
                       std::cout<< "Time interval : "<<data.timeinterval() <<std::endl;
                   if(data.has_p1v())
                       std::cout<< "P1V: "<<data.p1v() <<std::endl;
                   if(data.has_p1c())
                       std::cout<< "P1C : "<<data.p1c() <<std::endl;
                   if(data.has_adp1v())
                       std::cout<< "ADP1V : "<<data.adp1v() <<std::endl;
                   if(data.has_adp1c())
                       std::cout<< "ADP1C : "<<data.adp1c() <<std::endl;
                   if(data.has_l1v())
                       std::cout<< "L1V : "<<data.l1v() <<std::endl;
                   if(data.has_l1c())
                       std::cout<< "l1c : "<<data.l1c() <<std::endl;
                   if(data.has_l2v())
                       std::cout<< "L2V : "<<data.l2v() <<std::endl;
                   if(data.has_l2c())
                       std::cout<< "l2c : "<<data.l2c() <<std::endl;
                   if(data.has_l3v())
                       std::cout<< "L3V : "<<data.l3v() <<std::endl;
                   if(data.has_l3c())
                       std::cout<< "l3c : "<<data.l3c() <<std::endl;
                   if(data.has_l4v())
                       std::cout<< "L4V : "<<data.l4v() <<std::endl;
                   if(data.has_l4c())
                       std::cout<< "l4c : "<<data.l4c() <<std::endl;
                   if(data.has_l5v())
                       std::cout<< "L5V : "<<data.l5v() <<std::endl;
                   if(data.has_l5c())
                       std::cout<< "l5c : "<<data.l5c() <<std::endl;
                   if(data.has_power()) {
                       std::cout<< "Power Status : " <<std::endl;
                       if (data.power().has_bv())
                           std::cout<< "Power Battery : "<<data.power().bv()<< std::endl;
                       if (data.power().has_p1())
                           std::cout<< " Power Solar : "<<data.power().p1()<< std::endl; 
                       if (data.power().has_adp1())
                           std::cout<< " Power ADP : "<<data.power().adp1()<< std::endl;
                       if (data.power().has_l1())
                           std::cout<< " Power L1  : "<<data.power().l1()<< std::endl;
                       if (data.power().has_l2())
                           std::cout<< " Power L2 : "<<data.power().l2()<< std::endl;
                       if (data.power().has_l3())
                           std::cout<< " Power L3 : "<<data.power().l3()<< std::endl;
                       if (data.power().has_l4())
                           std::cout<< " Power L4 : "<<data.power().l4()<< std::endl;
                       if (data.power().has_l5())
                           std::cout<< " Power L5 : "<<data.power().l5()<< std::endl;
                        
                   }
                   if(data.has_battery()) {
                       std::cout<< "Battery Status : " <<std::endl;
                       if(data.battery().has_bv()) 
                           std::cout<< "Battery voltage : "<<data.battery().bv()<< std::endl;
                       if(data.battery().has_bc())
                           std::cout<< "Battery current : "<<data.battery().bc()<< std::endl;
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

                }
            } else {
                std::cout << status.error_code() << ": " << status.error_message()
                          << std::endl;
                std::cout << "RPC failed" <<std::endl;
            }
        }
    private:
        std::unique_ptr<TelemetryService::Stub> stub_;

};

int main(int argc, char** argv) {
  TelemetryServiceClient tsc(
      grpc::CreateChannel("localhost:50051",
                          grpc::InsecureChannelCredentials()));

  std::cout << "-------------- GetFeature --------------" << std::endl;
  tsc.GetTelemetry();
  std::cout <<"---------------End of the Client---------"<<std::endl;
  return 0;

}

