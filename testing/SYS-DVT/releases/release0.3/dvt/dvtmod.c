#include<stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<poll.h>

struct pollfd pfd;
int pollRet = 0;
FILE *fp_gpio;

int main()
{
    int fd = open("/dev/dvt",O_RDONLY);
    if(fd < 0)
    {
	    printf("Failed to open the device file\n");
	    return -1;
    }
    pfd.fd = fd;
    pfd.events = POLLIN;
    pollRet = poll(&pfd,1,-1);
    if (pollRet > 0) 
    {
        if (pfd.revents & POLLIN) 
        {
                printf("intr");
        }
    }/* end of while loop */

   return 0;
}   

