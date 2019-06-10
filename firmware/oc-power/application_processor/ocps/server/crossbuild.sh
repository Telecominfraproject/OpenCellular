#!/bin/bash
CXX=/home/oc_lab/tools/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++
LD=/home/oc_lab/tools/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-ld
CXXFLAGS=-std=c++11  
LDFLAGS=-L../libs/
LDFLAGPROTOC=-L../libs/protobuf
LDBSP=-L/home/oc/OCPS/oc-power/ocpio
LDBSP=-L../../ocpio

echo "Cleaning files"
rm -f *.o *.pb.cc *.pb.h *.bin 

echo "Building proto files." 
protoc -I ../protos --grpc_out=. --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) telemetry.proto alert_enum.proto configuration.proto control.proto action_description.proto
protoc -I ../protos --cpp_out=. telemetry.proto alert_enum.proto configuration.proto control.proto action_description.proto

echo "Building *.cc files."
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o alert_enum.grpc.pb.o alert_enum.grpc.pb.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o alert_enum.pb.o alert_enum.pb.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o ocp_grpc_client.o ocp_grpc_client.cc 
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o ocp_grpc_server.o ocp_grpc_server.cc 
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o ocp_services.o ocp_services.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o telemetry.pb.o  telemetry.pb.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o telemetry.grpc.pb.o telemetry.grpc.pb.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o configuration.grpc.pb.o configuration.grpc.pb.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o configuration.pb.o configuration.pb.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o ocp_grpc_config_client.o ocp_grpc_config_client.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o ocp_helper_ip.o ocp_helper_ip.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o ocp_connectionCheck.o ocp_connectionCheck.cc 
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o control.grpc.pb.o control.grpc.pb.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o control.pb.o control.pb.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o action_description.grpc.pb.o action_description.grpc.pb.cc
$CXX $CXXFLAGS `pkg-config --cflags protobuf grpc` -g -c -o action_description.pb.o action_description.pb.cc

echo "Building Server bin."
$CXX alert_enum.grpc.pb.o alert_enum.pb.o ocp_grpc_server.o ocp_services.o ocp_helper_ip.o telemetry.pb.o telemetry.grpc.pb.o configuration.grpc.pb.o configuration.pb.o ocp_connectionCheck.o action_description.pb.o action_description.grpc.pb.o control.pb.o control.grpc.pb.o $LDFLAGS $LDFLAGPROTOC $LDBSP -locpbsp -lgrpc++ -lgrpc -lgrpc++_reflection -lprotobuf -lpthread -ldl -o ocp_grpc_server.bin

#echo "Building Telemetry Client bin"
#$CXX alert_enum.grpc.pb.o alert_enum.pb.o ocp_grpc_client.o ocp_telemetry.pb.o ocp_telemetry.grpc.pb.o $LDFLAGS $LDFLAGPROTOC -lgrpc++ -lgrpc -lgrpc++_reflection -lprotobuf -lpthread -ldl -o ocp_grpc_telemetry_client.bin

#echo "Building Configuration Client bin"
#$CXX ocp_grpc_config_client.o configuration.grpc.pb.o configuration.pb.o $LDFLAGS $LDFLAGPROTOC -lgrpc++ -lgrpc -lgrpc++_reflection -lprotobuf -lpthread -ldl -o ocp_grpc_config_client.bin

