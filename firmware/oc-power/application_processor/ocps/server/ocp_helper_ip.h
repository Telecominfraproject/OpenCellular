#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

#define DELIM "."
#define MAX_TRY 10

int valid_digit(char *ip_str);
int is_valid_ip(char *ip_str);
char* getIPAddress();
char* getgRPCClientIp();
