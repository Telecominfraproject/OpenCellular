#!/bin/bash

echo "Cleaning files"
rm -f *.o *.pb.cc *.pb.h *.bin 

echo "Building proto files." 
protoc -I ../protos --grpc_out=. --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) telemetry.proto alert_enum.proto configuration.proto control.proto action_description.proto
protoc -I ../protos --cpp_out=. telemetry.proto alert_enum.proto configuration.proto control.proto action_description.proto

echo "Building *.cc files."
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o alert_enum.grpc.pb.o alert_enum.grpc.pb.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o alert_enum.pb.o alert_enum.pb.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o ocp_grpc_client.o ocp_grpc_client.cc 
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o ocp_grpc_server.o ocp_grpc_server.cc 
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc` -DHOST  -c -o ocp_services.o ocp_services.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o telemetry.pb.o  telemetry.pb.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o telemetry.grpc.pb.o telemetry.grpc.pb.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o configuration.grpc.pb.o configuration.grpc.pb.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o configuration.pb.o configuration.pb.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc`  -c -o ocp_grpc_config_client.o ocp_grpc_config_client.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc` -g -c -o ocp_helper_ip.o ocp_helper_ip.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc` -g -c -o ocp_connectionCheck.o ocp_connectionCheck.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc` -g -c -o control.grpc.pb.o control.grpc.pb.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc` -g -c -o control.pb.o control.pb.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc` -g -c -o action_description.grpc.pb.o action_description.grpc.pb.cc
g++ -g -std=c++11 `pkg-config --cflags protobuf grpc` -g -c -o action_description.pb.o action_description.pb.cc
echo "Building Server bin."
g++ alert_enum.grpc.pb.o alert_enum.pb.o ocp_grpc_server.o ocp_services.o telemetry.pb.o telemetry.grpc.pb.o configuration.grpc.pb.o configuration.pb.o ocp_helper_ip.o ocp_connectionCheck.o action_description.pb.o action_description.grpc.pb.o control.pb.o control.grpc.pb.o -L/usr/local/lib `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o ocp_grpc_server.host
#echo "Building Telemetry Client bin"
#g++ alert_enum.grpc.pb.o alert_enum.pb.o ocp_grpc_client.o ocp_services.o telemetry.pb.o telemetry.grpc.pb.o -L/usr/local/lib `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o ocp_grpc_telemetry_client.bin

echo "Building Configuration Client bin"
g++ ocp_grpc_config_client.o configuration.grpc.pb.o configuration.pb.o alert_enum.grpc.pb.o alert_enum.pb.o ocp_services.o telemetry.pb.o telemetry.grpc.pb.o action_description.pb.o action_description.grpc.pb.o control.pb.o control.grpc.pb.o -L/usr/local/lib `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o ocp_grpc_config_client.host

