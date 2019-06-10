#include <iostream>
#include <memory>
#include <cstring>
#include <unistd.h>
#include "ocp_helper_ip.h"

#define ETH_IFACE

#ifdef ETH_IFACE
#define IFACE "eth0"
#else
#define IFACE "usb0"
#endif

#define GRPC_CLIENT "gitlab.limbo.cbtx9.com"

int valid_digit(char *ip_str)
{
    while (*ip_str) {
        if (*ip_str >= '0' && *ip_str <= '9')
            ++ip_str;
        else
            return 0;
    }
    return 1;
}

/* return 1 if IP string is valid, else return 0 */
int is_valid_ip(char *ip_str)
{
    int i, num, dots = 0;
    char *ptr;
    if (ip_str == NULL) {
        return 0;
    }
    ptr = strtok(ip_str, DELIM);
    if (ptr == NULL) {
        return 0;
    }
    while (ptr) {
        /* after parsing string, it must contain only digits */
        if (!valid_digit(ptr)){
            return 0;
        }
        num = atoi(ptr);
        /* check for valid IP */
        if (num >= 0 && num <= 255) {
            /* parse remaining string */
            ptr = strtok(NULL, DELIM);
            if (ptr != NULL)
                ++dots;
        } else {
            return 0;
        }
    }
    /* valid IP string must contain 3 dots */
    if (dots != 3){
        return 0;
    }
    return 1;
}

char* getIPAddress()
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    char *iphost = (char*)malloc(sizeof(char)*NI_MAXHOST);
    if(!host) {
        return NULL;
    }
    if (getifaddrs(&ifaddr) == -1) { 
        perror("getifaddrs");
        return NULL; 
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL){
            continue;  
        }
        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if((strcmp(ifa->ifa_name,IFACE)==0)&&(ifa->ifa_addr->sa_family==AF_INET)) {
            if (s != 0) {
                std::cout<<"getnameinfo() failed:" << gai_strerror(s)<<std::endl;
                return NULL;
            }
            std::cout<<"Interface :"<<ifa->ifa_name<<std::endl;
            std::cout<<"Address :"<< host<<std::endl; 
            memcpy(iphost,host,NI_MAXHOST);
            break;
        }
    }
    freeifaddrs(ifaddr);
    memcpy(host,iphost,NI_MAXHOST); 
    if(is_valid_ip(host)) {
        return iphost;
    } else {
        return NULL;
    } 
}


char* getgRPCClientIp()
{
    char hostname[] = GRPC_CLIENT;// "gitlab.limbo.cbtx9.com";
    char hostip[NI_MAXHOST]={0};
    char* ip = (char*)malloc(sizeof(char)*NI_MAXHOST);
    if(ip) {
        int sockfd;  
        struct addrinfo hints, *servinfo, *p;
        struct sockaddr_in *h;
        int rv;
        memset(ip,0,(sizeof(char)*NI_MAXHOST));
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
        hints.ai_socktype = SOCK_STREAM;

        if ( (rv = getaddrinfo( hostname , "http" , &hints , &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return NULL;
        }

        // loop through all the results and connect to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            h = (struct sockaddr_in *) p->ai_addr;
            strcpy(hostip , inet_ntoa( h->sin_addr ) );
        }

        freeaddrinfo(servinfo); 
        memcpy(ip,hostip,NI_MAXHOST);
        if(is_valid_ip(hostip)) {
            std::cout<<"Valid Ip is : "<<ip<<" ."<<std::endl;
            return ip;
        } else {
            std::cout<<"Invalid IP: "<<ip<<" ."<<std::endl; 
            ip = NULL;
        } 
    }
    return ip;   
}
