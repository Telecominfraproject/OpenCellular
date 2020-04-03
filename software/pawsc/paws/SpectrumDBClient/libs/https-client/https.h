/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#pragma once

#include <openssl/ssl.h>

#define HTTPS_PORT	  "443"


// Returns address of new allocated ssl context, NULL on failure.
// This should just be called once.
// Context is stored globally inside library.  Use retrun value just to check for success/failure checks.
extern SSL_CTX* https_init(void);

// Free ssl context
extern void https_free(void);

// Makes and returns ssl connection, NULL on failure.
extern SSL* https_connect(int sockfd);

// Free ssl connection
void https_disconnect(SSL* conn);


// Frees the connection
extern void https_disconnect(SSL* conn);


// Returns 0 on success, -1 on failure.
// Params:
//   hostname - Something like "foo.com"
//   request_path - Somthing like "anything/really". Could also include query parameters like "?token=XXX
extern int https_send_get(SSL *ssl, const char *hostname, const char *request_path);

// Returns 0 on success, -1 on failure.
// Params similar to https_send_get().
extern int https_send_post(SSL *ssl, const char *hostname, const char *request_path, const char *data, int data_len);

// Returns a c string on success, NULL on failure. You are responsible for 
// freeing the returned c string.
//
// This function considers any HTTP response to be a success, even if the
// response describes an HTTP error, eg a "404 Not Found" status code. Once this
// function has succeeded, you will typically call http_get_status_code()
// on the returned string.
//
// Params:
//   body_offset - On success will be set to the position in the returned
//                 string of the start of the message body.
extern char *https_fetch_response(SSL *ssl, int *body_offset);
