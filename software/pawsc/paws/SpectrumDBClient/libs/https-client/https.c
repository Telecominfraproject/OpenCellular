/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

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

// SSL headers
#include <openssl/err.h>


// Project headers
#include "string-buffer/string_buffer.h"
#include "http-client/http.h"

// Own header
#include "https.h"



static __thread char g_error_buf[2048] = { 0 };
#define handle_error(fmt, ...) snprintf(g_error_buf, sizeof(g_error_buf) - 1, fmt "\n", ##__VA_ARGS__);

static SSL_CTX *g_ssl_ctx = NULL;

//#######################################################################################
// This should just be called once
SSL_CTX* https_init(void)
{
	// initialize OpenSSL - do this once and stash ssl_ctx in a global var	
	SSL_load_error_strings();
	SSL_library_init();
	g_ssl_ctx = SSL_CTX_new(SSLv23_client_method());

	return g_ssl_ctx;
}


//#######################################################################################
void https_free(void)
{
	if (g_ssl_ctx)
	{
		SSL_CTX_free(g_ssl_ctx);
		ERR_free_strings();
		EVP_cleanup();
		g_ssl_ctx = NULL;
	}
}


//#######################################################################################
SSL* https_connect(int sockfd)
{
	SSL* conn = NULL;
	
	if (g_ssl_ctx)
	{
		// create an SSL connection and attach it to the socket
		conn = SSL_new(g_ssl_ctx);
		if (!conn)
			return NULL;

		SSL_set_fd(conn, sockfd);

		// perform the SSL/TLS handshake with the server - when on the
		// server side, this would use SSL_accept()
		int res = SSL_connect(conn);
		if (res != 1)
		{
			SSL_free(conn);
			return NULL;
		}
	}
	return conn;
}


//#######################################################################################
void https_disconnect(SSL* conn)
{
	if (conn)
	{
		SSL_shutdown(conn);
		SSL_shutdown(conn);
		SSL_free(conn);
	}
}
 


//#######################################################################################
int https_send_get(SSL *ssl, const char *hostname, const char *request_path)
{
	StrBuf *buf = strbuf_alloc();
	if (!buf) {
		printf("strbuf_alloc failed");
		return -1;
	}

	strbuf_append_cstr(buf, "GET ");
	strbuf_append_cstr(buf, request_path);
	strbuf_append_cstr(buf, " HTTPS/1.0\r\nHost: ");
	strbuf_append_cstr(buf, hostname);
	strbuf_append_cstr(buf, "\r\nConnection: close\r\n\r\n");

	ssize_t bytes_sent = SSL_write(ssl, buf->data, buf->len);
	ssize_t buf_len = buf->len;
	strbuf_free(buf);

	if (bytes_sent != buf_len) {
		printf("Couldn't send get request");
		return -1;
	}

	return 1;
}


//#######################################################################################
char *https_fetch_response(SSL *ssl, int *body_offset)
{
	StrBuf *buf = strbuf_alloc();
	if (!buf) {
		printf("strbuf_alloc failed");
		return NULL;
	}

	while (1) {
		const unsigned RECV_SIZE = 1550;
		char data[RECV_SIZE];
		ssize_t bytes_received = SSL_read(ssl, data, RECV_SIZE);

		if (bytes_received == -1) {
			printf("recv failed");
			goto error;
		}
		else if (bytes_received == 0) {
			break;
		}

		if (bytes_received > 0) {
			if (strbuf_append(buf, data, bytes_received) == -1) {
				printf("strbuf_append failed");
				goto error;
			}
		}
	}

	char *rv = strbuf_to_cstr(buf);
	if (!rv)
		printf("strbuf_to_cstr failed");
	strbuf_free(buf);

	*body_offset = http_get_message_body_offset(rv);
	return rv;

error:
	strbuf_free(buf);
	return NULL;
}


//#######################################################################################
int https_send_post(SSL *ssl, const char *hostname, const char *request_path, const char *data, int data_len)
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

	ssize_t bytes_sent = SSL_write(ssl, buf->data, buf->len);
	int buf_len = buf->len;
	strbuf_free(buf);

	if (bytes_sent != buf_len) {
		handle_error("Couldn't send post request");
		return -1;
	}

	return 1;
}
