/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#pragma once

#define HTTP_PORT	  "80"

// Returns a pointer to a string describing the last error. Will be "" if there
// has been no error. The string returned is a static thread local string buffer.
// This makes this function thread safe - eg if you call an API function that sets
// an error string and before you get chance to call http_get_last_error, another
// thread sets another error string, you are still guaranteed to get your error,
// and not theirs.
// 
// The string returned is owned by this library - do NOT free it!
const char *http_get_last_error();

void http_clear_last_error();

// Returns sockfd on success, -1 on failure.
int http_init(const char *hostname, const char *port);

// Returns 0 on success, -1 on failure.
// Params:
//   hostname - Something like "foo.com"
//   request_path - Somthing like "anything/really". Could also include query parameters like "?token=XXX
int http_send_get(int sockfd, const char *hostname, const char *request_path);

// Returns 0 on success, -1 on failure.
// Params similar to http_send_get().
int http_send_post(int sockfd, const char *hostname, const char *request_path, const char *data, int data_len);

// Returns position oy payload. -1 on failure
int http_get_message_body_offset(const char *message);

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
char *http_fetch_response(int sockfd, int *body_offset);

// Pass in an HTTP response, typically as returned by http_fetch_response. Get
// back an HTTP status code like 200 (OK) or 404 (Not found). Returns 0 if
// message cannot be parsed.
int http_get_status_code(const char *message);

void http_close(int sockfd);