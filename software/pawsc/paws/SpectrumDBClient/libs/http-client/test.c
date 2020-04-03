/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "http.h"


#define fatal_error(fmt, ...) { fprintf(stderr, "FATAL ERROR: " fmt "\n", ##__VA_ARGS__); exit(-1); }


int main()
{
    {
        char const *hostname = "<a non-secure server>";
        char const *port = "80";
        int sockfd = http_init(hostname, port);
        if (sockfd <= 0)
            fatal_error("Could not make connection to '%s' on port '%s'", hostname, port);

        int status = http_send_get(sockfd, hostname, "/<a file to get>");
        if (status <= 0)
            fatal_error("Sending request failed");

        int body_offset;
        char *response = http_fetch_response(sockfd, &body_offset);
        if (!response)
            fatal_error("Fetching response failed");

        printf("Status code was %d\n", http_get_status_code(response));
        printf("Message body offset = %d\n", body_offset);
        puts(response);

        http_close(sockfd);
        free(response);
    }
 
    // For this test to work, you will need to run an http server on localhost port 80.
    {
        char const *hostname = "localhost";
        char const *port = "80";
        int sockfd = http_init(hostname, port);
        if (sockfd <= 0)
            fatal_error("Could not make connection to '%s' on port '%s'", hostname, port);

        static char const *data =
            "\n{"
            "\n \"params\": {"
            "\n  \"version\": \"1.0\","
            "\n  \"type\": \"INIT_REQ\","
            "\n  \"deviceDesc\": {"
            "\n   \"etsiEnDeviceType\": \"A\","
            "\n   \"serialNumber\": \"fap-jenkins-01\","
            "\n   \"etsiEnDeviceEmissionsClass\": 5,"
            "\n   \"etsiEnTechnologyId\": \"AngularJS\","
            "\n   \"etsiEnDeviceCategory\": \"master\","
            "\n   \"rulesetIds\": ["
            "\n    \"ETSI-EN-301-598-1.1.1\""
            "\n   ],"
            "\n   \"manufacturerId\": \"IPAccess\","
            "\n   \"modelId\": \"E40\""
            "\n  },"
            "\n  \"location\": {"
            "\n   \"confidence\": 95,"
            "\n   \"point\": {"
            "\n    \"orientation\": 0,"
            "\n    \"semiMinorAxis\": 0,"
            "\n    \"center\": {"
            "\n     \"latitude\": 51.4998,"
            "\n     \"longitude\": 0.0084"
            "\n    },"
            "\n    \"semiMajorAxis\": 0"
            "\n   }"
            "\n  }"
            "\n },"
            "\n \"jsonrpc\": \"2.0\","
            "\n \"method\": \"spectrum.paws.init\","
            "\n \"timestamp\": \"15/12/2016 @ 12:07:19\","
            "\n \"id\": 0"
            "\n}    ";
        int const data_len = strlen(data);

        int status = http_send_post(sockfd, hostname, "?token=u-e15c5ee7-8f28-4a9e-92e9-7797c162851e", data, data_len);
        if (status <= 0) fatal_error("Sending request failed");

        int body_offset = 0;
        char *response = http_fetch_response(sockfd, &body_offset);
        if (!response) fatal_error("Fetching response failed");

        puts(response);

        http_close(sockfd);
        free(response);
    }

    return 0;
}
