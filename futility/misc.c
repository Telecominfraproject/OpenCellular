/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <linux/fs.h>		/* For BLKGETSIZE64 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "futility.h"
#include "gbb_header.h"

int debugging_enabled;
void Debug(const char *format, ...)
{
	if (!debugging_enabled)
		return;

	va_list ap;
	va_start(ap, format);
	fprintf(stderr, "DEBUG: ");
	vfprintf(stderr, format, ap);
	va_end(ap);
}

static int is_null_terminated(const char *s, int len)
{
	len--;
	s += len;
	while (len-- >= 0)
		if (!*s--)
			return 1;
	return 0;
}

static inline uint32_t max(uint32_t a, uint32_t b)
{
	return a > b ? a : b;
}

int futil_looks_like_gbb(GoogleBinaryBlockHeader *gbb, uint32_t len)
{
	if (memcmp(gbb->signature, GBB_SIGNATURE, GBB_SIGNATURE_SIZE))
		return 0;
	if (gbb->major_version > GBB_MAJOR_VER)
		return 0;
	if (sizeof(GoogleBinaryBlockHeader) > len)
		return 0;

	/* close enough */
	return 1;
}

int futil_valid_gbb_header(GoogleBinaryBlockHeader *gbb, uint32_t len,
			   uint32_t *maxlen_ptr)
{
	if (memcmp(gbb->signature, GBB_SIGNATURE, GBB_SIGNATURE_SIZE))
		return 0;
	if (gbb->major_version != GBB_MAJOR_VER)
		return 0;

	/* Check limits first, to help identify problems */
	if (maxlen_ptr) {
		uint32_t maxlen = gbb->header_size;
		maxlen = max(maxlen,
			     gbb->hwid_offset + gbb->hwid_size);
		maxlen = max(maxlen,
			     gbb->rootkey_offset + gbb->rootkey_size);
		maxlen = max(maxlen,
			     gbb->bmpfv_offset + gbb->bmpfv_size);
		maxlen = max(maxlen,
			     gbb->recovery_key_offset + gbb->recovery_key_size);
		*maxlen_ptr = maxlen;
	}

	if (gbb->header_size != GBB_HEADER_SIZE || gbb->header_size > len)
		return 0;
	if (gbb->hwid_offset < GBB_HEADER_SIZE)
		return 0;
	if (gbb->hwid_offset + gbb->hwid_size > len)
		return 0;
	if (gbb->hwid_size) {
		const char *s = (const char *)
			((uint8_t *)gbb + gbb->hwid_offset);
		if (!is_null_terminated(s, gbb->hwid_size))
			return 0;
	}
	if (gbb->rootkey_offset < GBB_HEADER_SIZE)
		return 0;
	if (gbb->rootkey_offset + gbb->rootkey_size > len)
		return 0;

	if (gbb->bmpfv_offset < GBB_HEADER_SIZE)
		return 0;
	if (gbb->bmpfv_offset + gbb->bmpfv_size > len)
		return 0;
	if (gbb->recovery_key_offset < GBB_HEADER_SIZE)
		return 0;
	if (gbb->recovery_key_offset + gbb->recovery_key_size > len)
		return 0;

	/* Seems legit... */
	return 1;
}


/*
 * TODO: All sorts of race conditions likely here, and everywhere this is used.
 * Do we care? If so, fix it.
 */
void futil_copy_file_or_die(const char *infile, const char *outfile)
{
	pid_t pid;
	int status;

	pid = fork();

	if (pid < 0) {
		fprintf(stderr, "Couldn't fork /bin/cp process: %s\n",
			strerror(errno));
		exit(1);
	}

	/* child */
	if (!pid) {
		execl("/bin/cp", "/bin/cp", infile, outfile, NULL);
		fprintf(stderr, "Child couldn't exec /bin/cp: %s\n",
			strerror(errno));
		exit(1);
	}

	/* parent - wait for child to finish */
	if (wait(&status) == -1) {
		fprintf(stderr,
			"Couldn't wait for /bin/cp process to exit: %s\n",
			strerror(errno));
		exit(1);
	}

	if (WIFEXITED(status)) {
		status = WEXITSTATUS(status);
		/* zero is normal exit */
		if (!status)
			return;
		fprintf(stderr, "/bin/cp exited with status %d\n", status);
		exit(1);
	}

	if (WIFSIGNALED(status)) {
		status = WTERMSIG(status);
		fprintf(stderr, "/bin/cp was killed with signal %d\n", status);
		exit(1);
	}

	fprintf(stderr, "I have no idea what just happened\n");
	exit(1);
}


int futil_map_file(int fd, int writeable, uint8_t **buf, uint32_t *len)
{
	struct stat sb;
	void *mmap_ptr;
	uint32_t reasonable_len;

	if (0 != fstat(fd, &sb)) {
		fprintf(stderr, "Can't stat input file: %s\n",
			strerror(errno));
		return 1;
	}

	if (S_ISBLK(sb.st_mode))
		ioctl(fd, BLKGETSIZE64, &sb.st_size);

	/* If the image is larger than 2^32 bytes, it's wrong. */
	if (sb.st_size < 0 || sb.st_size > UINT32_MAX) {
		fprintf(stderr, "Image size is unreasonable\n");
		return 1;
	}
	reasonable_len = (uint32_t)sb.st_size;

	if (writeable)
		mmap_ptr = mmap(0, sb.st_size,
				PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	else
		mmap_ptr = mmap(0, sb.st_size,
				PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);

	if (mmap_ptr == (void *)-1) {
		fprintf(stderr, "Can't mmap %s file: %s\n",
			writeable ? "output" : "input",
			strerror(errno));
		return 1;
	}

	*buf = (uint8_t *)mmap_ptr;
	*len = reasonable_len;
	return 0;
}

int futil_unmap_file(int fd, int writeable, uint8_t *buf, uint32_t len)
{
	void *mmap_ptr = buf;
	int errorcnt = 0;

	if (writeable &&
	    (0 != msync(mmap_ptr, len, MS_SYNC|MS_INVALIDATE))) {
		fprintf(stderr, "msync failed: %s\n", strerror(errno));
		errorcnt++;
	}

	if (0 != munmap(mmap_ptr, len)) {
		fprintf(stderr, "Can't munmap pointer: %s\n",
			strerror(errno));
		errorcnt++;
	}

	return errorcnt;
}
