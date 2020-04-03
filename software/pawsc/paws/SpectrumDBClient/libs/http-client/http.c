/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

// Own header
#include "http.h"

// Project headers
#include "string-buffer/string_buffer.h"

// Platform headers
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Standard headers
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static __thread char g_error_buf[2048] = { 0 };
#define handle_error(fmt, ...) snprintf(g_error_buf, sizeof(g_error_buf) - 1, fmt "\n", ##__VA_ARGS__);


const char *http_get_last_error()
{
    return g_error_buf;
}


void http_clear_last_error()
{
    g_error_buf[0] = '\0';
}


int http_init(const char *hostname, const char *port)
{
    struct addrinfo hints = { 0 };
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res;
    int return_code = getaddrinfo(hostname, port, &hints, &res);
    if (return_code != 0) {
        handle_error("getaddrinfo failed");
        return -1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        handle_error("socket failed");
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 15;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) != 0 ||
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval)) != 0) {
        handle_error("setsockopt failed");
        return -1;
    }

    int status = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (status != 0) {
        freeaddrinfo(res);
        handle_error("connect failed");
        return -1;
    }

    freeaddrinfo(res);

    return sockfd;
}


int http_send_get(int sockfd, const char *hostname, const char *request_path)
{
    StrBuf *buf = strbuf_alloc();
    if (!buf) {
        handle_error("strbuf_alloc failed");
        return -1;
    }

    strbuf_append_cstr(buf, "GET ");
    strbuf_append_cstr(buf, request_path);
    strbuf_append_cstr(buf, " HTTP/1.0\r\nHost: ");
    strbuf_append_cstr(buf, hostname);
    strbuf_append_cstr(buf, "\r\nConnection: close\r\n\r\n");

    ssize_t bytes_sent = send(sockfd, buf->data, buf->len, 0);
    ssize_t buf_len = buf->len;
    strbuf_free(buf);

    if (bytes_sent != buf_len) {
        handle_error("Couldn't send get request");
        return -1;
    }

    return 1;
}


int http_send_post(int sockfd, const char *hostname, const char *request_path, const char *data, int data_len)
{
    StrBuf *buf = strbuf_alloc();
    if (!buf) {
        handle_error("strbuf_alloc failed");
        return -1;
    }

    char data_len_as_string[] = "123123";
    snprintf(data_len_as_string, 6, "%d", data_len);

    strbuf_append_cstr(buf, "POST ");
    strbuf_append_cstr(buf, request_path);
    strbuf_append_cstr(buf, " HTTP/1.0\r\nHost: ");
    strbuf_append_cstr(buf, hostname);
    strbuf_append_cstr(buf, "\r\nConnection: close\r\nContent-Length: ");
    strbuf_append_cstr(buf, data_len_as_string);
    strbuf_append_cstr(buf, "\r\n\r\n");
    strbuf_append_cstr(buf, data);

    ssize_t bytes_sent = send(sockfd, buf->data, buf->len, 0);
    int buf_len = buf->len;
    strbuf_free(buf);

    if (bytes_sent != buf_len) {
        handle_error("Couldn't send post request");
        return -1;
    }

    return 1;
}


int http_get_message_body_offset(const char *message)
{
    // RFC2616, section 4.1 says all we have to do is search for
    // \r\n\r\n
    const char *header_body_seperator = strstr(message, "\r\n\r\n");
    if (!header_body_seperator)
        return -1;
    return header_body_seperator - message + 4;
}


char *http_fetch_response(int sockfd, int *body_offset)
{
    StrBuf *buf = strbuf_alloc();
    if (!buf) {
        handle_error("strbuf_alloc failed");
        return NULL;
    }

    while (1) {
        const unsigned RECV_SIZE = 1550;
        char data[RECV_SIZE];
        ssize_t bytes_received = recv(sockfd, data, RECV_SIZE, 0);

        if (bytes_received == -1) {
            handle_error("recv failed");
            goto error;
        } else if (bytes_received == 0) {
            break;
        }

        if (bytes_received > 0) {
            if (strbuf_append(buf, data, bytes_received) == -1) {
                handle_error("strbuf_append failed");
                goto error;
            }
        }
    }

    char *rv = strbuf_to_cstr(buf);
    if (!rv)
        handle_error("strbuf_to_cstr failed");
    strbuf_free(buf);

    *body_offset = http_get_message_body_offset(rv);
    return rv;

error:
    strbuf_free(buf);
    return NULL;
}


static int string_starts_with(const char *str_to_look_in, const char *str_to_look_for)
{
    int search_len = strlen(str_to_look_for);
    return strncmp(str_to_look_in, str_to_look_for, search_len) == 0;
}


int http_get_status_code(const char *message)
{
    if (!string_starts_with(message, "HTTP"))
        return 0;
    
    const char *code_str = strchr(message, ' ');
    if (!code_str)
        return 0;

    while (isspace(*code_str))
        code_str++;

    return strtol(code_str, NULL, 10);
}


void http_close(int sockfd)
{
    if (sockfd >= 0)
        close(sockfd);
}




