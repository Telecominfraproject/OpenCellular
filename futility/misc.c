/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#ifndef HAVE_MACOS
#include <linux/fs.h>		/* For BLKGETSIZE64 */
#endif
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

#include "cgptlib_internal.h"
#include "file_type.h"
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

enum futil_file_type recognize_gbb(uint8_t *buf, uint32_t len)
{
	GoogleBinaryBlockHeader *gbb = (GoogleBinaryBlockHeader *)buf;

	if (memcmp(gbb->signature, GBB_SIGNATURE, GBB_SIGNATURE_SIZE))
		return FILE_TYPE_UNKNOWN;
	if (gbb->major_version > GBB_MAJOR_VER)
		return FILE_TYPE_UNKNOWN;
	if (sizeof(GoogleBinaryBlockHeader) > len)
		return FILE_TYPE_UNKNOWN;

	/* close enough */
	return FILE_TYPE_GBB;
}

int futil_valid_gbb_header(GoogleBinaryBlockHeader *gbb, uint32_t len,
			   uint32_t *maxlen_ptr)
{
	if (len < sizeof(GoogleBinaryBlockHeader))
		return 0;

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

/* For GBB v1.2 and later, print the stored digest of the HWID (and whether
 * it's correct). Return true if it is correct. */
int print_hwid_digest(GoogleBinaryBlockHeader *gbb,
		      const char *banner, const char *footer)
{
	printf("%s", banner);

	/* There isn't one for v1.1 and earlier, so assume it's good. */
	if (gbb->minor_version < 2) {
		printf("<none>%s", footer);
		return 1;
	}

	uint8_t *buf = (uint8_t *)gbb;
	char *hwid_str = (char *)(buf + gbb->hwid_offset);
	int is_valid = 0;
	uint8_t *digest = DigestBuf(buf + gbb->hwid_offset,
				    strlen(hwid_str),
				    SHA256_DIGEST_ALGORITHM);
	if (digest) {
		int i;
		is_valid = 1;
		/* print it, comparing as we go */
		for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
			printf("%02x", gbb->hwid_digest[i]);
			if (gbb->hwid_digest[i] != digest[i])
				is_valid = 0;
		}
		free(digest);
	}

	printf("   %s", is_valid ? "valid" : "<invalid>");
	printf("%s", footer);
	return is_valid;
}

/* For GBB v1.2 and later, update the hwid_digest field. */
void update_hwid_digest(GoogleBinaryBlockHeader *gbb)
{
	/* There isn't one for v1.1 and earlier */
	if (gbb->minor_version < 2)
		return;

	uint8_t *buf = (uint8_t *)gbb;
	char *hwid_str = (char *)(buf + gbb->hwid_offset);
	uint8_t *digest = DigestBuf(buf + gbb->hwid_offset,
				    strlen(hwid_str),
				    SHA256_DIGEST_ALGORITHM);
	memcpy(gbb->hwid_digest, digest, SHA256_DIGEST_SIZE);
	free(digest);
}

/*
 * TODO: All sorts of race conditions likely here, and everywhere this is used.
 * Do we care? If so, fix it.
 */
void futil_copy_file_or_die(const char *infile, const char *outfile)
{
	pid_t pid;
	int status;

	Debug("%s(%s, %s)\n", __func__, infile, outfile);

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


enum futil_file_err futil_map_file(int fd, int writeable,
				   uint8_t **buf, uint32_t *len)
{
	struct stat sb;
	void *mmap_ptr;
	uint32_t reasonable_len;

	if (0 != fstat(fd, &sb)) {
		fprintf(stderr, "Can't stat input file: %s\n",
			strerror(errno));
		return FILE_ERR_STAT;
	}

#ifndef HAVE_MACOS
	if (S_ISBLK(sb.st_mode))
		ioctl(fd, BLKGETSIZE64, &sb.st_size);
#endif

	/* If the image is larger than 2^32 bytes, it's wrong. */
	if (sb.st_size < 0 || sb.st_size > UINT32_MAX) {
		fprintf(stderr, "Image size is unreasonable\n");
		return FILE_ERR_SIZE;
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
		return FILE_ERR_MMAP;
	}

	*buf = (uint8_t *)mmap_ptr;
	*len = reasonable_len;
	return FILE_ERR_NONE;
}

enum futil_file_err futil_unmap_file(int fd, int writeable,
				     uint8_t *buf, uint32_t len)
{
	void *mmap_ptr = buf;
	enum futil_file_err err = FILE_ERR_NONE;

	if (writeable &&
	    (0 != msync(mmap_ptr, len, MS_SYNC|MS_INVALIDATE))) {
		fprintf(stderr, "msync failed: %s\n", strerror(errno));
		err = FILE_ERR_MSYNC;
	}

	if (0 != munmap(mmap_ptr, len)) {
		fprintf(stderr, "Can't munmap pointer: %s\n",
			strerror(errno));
		if (err == FILE_ERR_NONE)
			err = FILE_ERR_MUNMAP;
	}

	return err;
}


#define DISK_SECTOR_SIZE 512
enum futil_file_type recognize_gpt(uint8_t *buf, uint32_t len)
{
	GptHeader *h;

	/* GPT header starts at sector 1, is one sector long */
	if (len < 2 * DISK_SECTOR_SIZE)
		return FILE_TYPE_UNKNOWN;

	h = (GptHeader *)(buf + DISK_SECTOR_SIZE);

	if (memcmp(h->signature, GPT_HEADER_SIGNATURE,
		   GPT_HEADER_SIGNATURE_SIZE) &&
	    memcmp(h->signature, GPT_HEADER_SIGNATURE2,
		   GPT_HEADER_SIGNATURE_SIZE))
		return FILE_TYPE_UNKNOWN;
	if (h->revision != GPT_HEADER_REVISION)
		return FILE_TYPE_UNKNOWN;
	if (h->size < MIN_SIZE_OF_HEADER || h->size > MAX_SIZE_OF_HEADER)
		return FILE_TYPE_UNKNOWN;

	if (HeaderCrc(h) != h->header_crc32)
		return FILE_TYPE_UNKNOWN;

	return FILE_TYPE_CHROMIUMOS_DISK;
}
