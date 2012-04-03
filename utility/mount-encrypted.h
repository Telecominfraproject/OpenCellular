/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Private header file for mount-encrypted helper tool.
 */
#ifndef _MOUNT_ENCRYPTED_H_
#define _MOUNT_ENCRYPTED_H_

/* TODO(keescook): Disable debugging in production. */
#define DEBUG_ENABLED 1

#include <openssl/err.h>
#include <openssl/sha.h>

#define DIGEST_LENGTH SHA256_DIGEST_LENGTH

#define _ERROR(f, a...)	do { \
	fprintf(stderr, "ERROR %s (%s, %d): ", \
			__func__, __FILE__, __LINE__); \
	fprintf(stderr, f, ## a); \
} while (0)
#define ERROR(f, a...)	do { \
	_ERROR(f, ## a); \
	fprintf(stderr, "\n"); \
} while (0)
#define PERROR(f, a...)	do { \
	_ERROR(f, ## a); \
	fprintf(stderr, ": %s\n", strerror(errno)); \
} while (0)

#define SSL_ERROR(f, a...)	do { \
	ERR_load_crypto_strings(); \
	_ERROR(f, ## a); \
	fprintf(stderr, "%s\n", ERR_error_string(ERR_get_error(), NULL)); \
} while (0)

#if DEBUG_ENABLED
static struct timeval tick;
# define TICK_INIT() gettimeofday(&tick, NULL)
# ifdef DEBUG_TIME_DELTA
#  define TICK_REPORT() do { \
	struct timeval now, diff; \
	gettimeofday(&now, NULL); \
	diff.tv_sec = now.tv_sec - tick.tv_sec; \
	if (tick.tv_usec > now.tv_usec) { \
		diff.tv_sec -= 1; \
		diff.tv_usec = 1000000 - tick.tv_usec + now.tv_usec; \
	} else { \
		diff.tv_usec = now.tv_usec - tick.tv_usec; \
	} \
	tick = now; \
	printf("\tTook: [%2d.%06d]\n", (int)diff.tv_sec, (int)diff.tv_usec); \
} while (0)
# else
#  define TICK_REPORT() do { \
	gettimeofday(&tick, NULL); \
	printf("[%d:%2d.%06d] ", getpid(), (int)tick.tv_sec, (int)tick.tv_usec); \
} while (0)
# endif
#else
# define TICK_INIT() do { } while (0)
# define TICK_REPORT() do { } while (0)
#endif

#define INFO(f, a...) do { \
	TICK_REPORT(); \
	printf(f, ## a); \
	printf("\n"); \
} while (0)
#define INFO_INIT(f, a...) do { \
	TICK_INIT(); \
	INFO(f, ## a); \
} while (0)
#if DEBUG_ENABLED
# define DEBUG(f, a...) do { \
	TICK_REPORT(); \
	printf(f, ## a); \
	printf("\n"); \
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
