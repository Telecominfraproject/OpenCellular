/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../http-client/http.h"
#include "https.h"


#define fatal_error(fmt, ...) { fprintf(stderr, "FATAL ERROR: " fmt "\n", ##__VA_ARGS__); exit(-1); }


int main()
{
    {
		if (!(https_init()))
			fatal_error("Problem in https_init");

        char const *hostname = "tvws-databases.ofcom.org.uk";
        char const *port = HTTPS_PORT;
		SSL* conn = NULL;
		
		int sockfd=-1;

		sockfd = http_init(hostname, port);
		if (sockfd < 0)
			fatal_error("Could not make connection to '%s' on port '%s'", hostname, port);

		// make an ssl connection
		conn = https_connect(sockfd);
		if (!conn)
			fatal_error("Failure making ssl connection to %s", hostname);

        int status = https_send_get(conn, hostname, "/weblist.json");
        if (status <= 0)
            fatal_error("Sending request failed");

        int body_offset;
        char *response = https_fetch_response(conn, &body_offset);
        if (!response)
            fatal_error("Fetching response failed");

        printf("Status code was %d\n", http_get_status_code(response));
        printf("Message body offset = %d\n", body_offset);
        puts(response);

		https_disconnect(conn);
        http_close(sockfd);
        free(response);
		https_free();
    }

	return 0;
}
