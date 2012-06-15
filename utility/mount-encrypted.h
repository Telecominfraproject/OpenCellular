/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Private header file for mount-encrypted helper tool.
 */
#ifndef _MOUNT_ENCRYPTED_H_
#define _MOUNT_ENCRYPTED_H_

/* #define DEBUG_ENABLED 1 */
#define DEBUG_TIME_DELTA 1

#include <openssl/err.h>
#include <openssl/sha.h>

#define DIGEST_LENGTH SHA256_DIGEST_LENGTH

#define _ERROR(f, a...)	do { \
	fprintf(stderr, "ERROR[pid:%d] %s (%s, %d): ", \
			getpid(), __func__, __FILE__, __LINE__); \
	fprintf(stderr, f, ## a); \
} while (0)
#define ERROR(f, a...)	do { \
	_ERROR(f, ## a); \
	fprintf(stderr, "\n"); \
	fflush(stderr); \
} while (0)
#define PERROR(f, a...)	do { \
	_ERROR(f, ## a); \
	fprintf(stderr, ": %s\n", strerror(errno)); \
	fflush(stderr); \
} while (0)

#define SSL_ERROR(f, a...)	do { \
	ERR_load_crypto_strings(); \
	_ERROR(f, ## a); \
	fprintf(stderr, "%s\n", ERR_error_string(ERR_get_error(), NULL)); \
	fflush(stderr); \
} while (0)

#if DEBUG_ENABLED
extern struct timeval tick;
extern struct timeval tick_start;
# define TICK_INIT() do { \
	gettimeofday(&tick, NULL); \
	tick_start = tick; \
} while (0)
# ifdef DEBUG_TIME_DELTA
/* This timeval helper copied from glibc manual. */
static inline int timeval_subtract(struct timeval *result,
				   struct timeval *x,
				   struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 * tv_usec is certainly positive.
	 */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}
#  define TICK_REPORT() do { \
	struct timeval now, diff; \
	gettimeofday(&now, NULL); \
	timeval_subtract(&diff, &now, &tick); \
	printf("\tTook: [pid:%d, %2lu.%06lus]\n", getpid(), \
		(unsigned long)diff.tv_sec, (unsigned long)diff.tv_usec); \
	tick = now; \
} while (0)
# else
#  define TICK_REPORT() do { \
	gettimeofday(&tick, NULL); \
	printf("[%2d.%06d] ", (int)tick.tv_sec, (int)tick.tv_usec); \
} while (0)
# endif
# define TICK_DONE() do { \
	struct timeval tick_done; \
	TICK_REPORT(); \
	timeval_subtract(&tick_done, &tick, &tick_start); \
	printf("Process Lifetime: [pid:%d, %2d.%06ds]\n", getpid(), \
		(int)tick_done.tv_sec, (int)tick_done.tv_usec); \
} while (0)
#else
# define TICK_INIT() do { } while (0)
# define TICK_REPORT() do { } while (0)
# define TICK_DONE() do { } while (0)
#endif

#define _INFO(f, a...) do { \
	printf("[pid:%d] ", getpid()); \
	printf(f, ## a); \
	printf("\n"); \
	fflush(stdout); \
} while (0)
#define INFO(f, a...) do { \
	TICK_REPORT(); \
	_INFO(f, ## a); \
} while (0)
#define INFO_INIT(f, a...) do { \
	TICK_INIT(); \
	INFO(f, ## a); \
} while (0)
#define INFO_DONE(f, a...) do { \
	TICK_DONE(); \
	INFO(f, ## a); \
} while (0)
#if DEBUG_ENABLED
# define DEBUG(f, a...) do { \
	TICK_REPORT(); \
	_INFO(f, ## a); \
} while (0)
#else
# define DEBUG(f, a...) do { } while (0)
#endif

#if DEBUG_ENABLED
static inline void debug_dump_hex(const char *name, uint8_t *data,
				  uint32_t size)
{
	int i;
	printf("%s: ", name);
	for (i = 0; i < size; i++) {
		printf("%02x ", data[i]);
	}
	printf("\n");
}
#else
# define debug_dump_hex(n, d, s) do { } while (0)
#endif

#endif /* _MOUNT_ENCRYPTED_H_ */
