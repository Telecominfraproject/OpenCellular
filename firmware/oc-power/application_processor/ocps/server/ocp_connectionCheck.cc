#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>

#include "ocp_connectionCheck.h"
#define CONN_MONITOR_INTERVAL 1800

static bool activeMonitoring = false;
static void serviceTerm_cbFunc();
static sem_t serviceTerm_sem;
static pthread_t serviceTermination_thread;

static void signalHandler(int signal)
{
    sem_post(&serviceTerm_sem);
}

static void * serviceTerminationThread (void *_)
{
    while(activeMonitoring) {
        int rc = sem_wait(&serviceTerm_sem);
        if (rc== -1 && errno == EINTR)
            continue;
        if (rc == -1) {
            std::cout<<"OCP_SERVER::sem_wait failed for connection monitoring."<<std::endl;
            exit(-1);
        }
        if(activeMonitoring) {
            std::cout<<"OCP_SERVER:: Stopping OCP server."<<std::endl;
            exit(0);    
        }
    }
}

void stopConnectionMonitoring()
{
    activeMonitoring = false;
    pthread_join(serviceTermination_thread, 0);
}

void setConnectionMonitoringInterval()
{
    long seconds = CONN_MONITOR_INTERVAL;
    struct itimerval tval = {
        .it_interval = { 
                           .tv_sec = seconds,
                           .tv_usec = 0
                       },
        .it_value = {
                        .tv_sec = seconds,
                        .tv_usec = 0
                    }
    };
    std::cout<<"OCP_SERVER::Reseting CM time interval for OCP Server."<<std::endl;
    setitimer(ITIMER_REAL, &tval, (struct itimerval*)0);
}

void startConnectionMonitoring()
{
    sem_init(&serviceTerm_sem, 0 , 0);
    pthread_create(&serviceTermination_thread,(pthread_attr_t*)0, serviceTerminationThread, (void*)0);
    signal(SIGALRM,signalHandler);
    activeMonitoring = true;
    setConnectionMonitoringInterval();
}


