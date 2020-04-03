/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/


// Platform headers
#ifdef _MSC_VER
#include <winsock2.h>
typedef unsigned socklen_t;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "json-parser/json_utils.h"

#include "utils/types.h"

#include "logger_common.h"


//#######################################################################################
int logger_get_datetimeUTC_ms(char* dst, int dstlen)
{
	int millisec;
	struct tm* tm_info;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	millisec = lrint(tv.tv_usec / 1000.0); // Round to nearest millisec
	if (millisec >= 1000) { // Allow for rounding up to nearest second
		millisec -= 1000;
		tv.tv_sec++;
	}

	tm_info = localtime(&tv.tv_sec);

	int len, len2;
	if ((len = strftime(dst, dstlen, "%Y/%m/%dT%H:%M:%S", tm_info)))
	{
		if ((len2 = snprintf(dst+len, (dstlen - len), ".%03dZ", millisec)))
			return len + len2;
	}
	dst[0] = 0;
	return 0;
}