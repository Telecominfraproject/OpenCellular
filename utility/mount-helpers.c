/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This is a collection of helper utilities for use with the "mount-encrypted"
 * utility.
 *
 */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <linux/fs.h>
#include <linux/loop.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <openssl/evp.h>

#include "mount-encrypted.h"
#include "mount-helpers.h"

static const gchar * const kRootDir = "/";
static const gchar * const kLoopTemplate = "/dev/loop%d";
static const int kLoopMajor = 7;
static const int kLoopMax = 8;
static const unsigned int kResizeStepSeconds = 2;
static const size_t kResizeBlocks = 32768 * 10;
static const gchar * const kExt4ExtendedOptions = "discard,lazy_itable_init";

int remove_tree(const char *tree)
{
	const gchar *rm[] = {
		"/bin/rm", "-rf", tree,
		NULL
	};

	return runcmd(rm, NULL);
}

size_t get_sectors(const char *device)
{
	size_t sectors;
	int fd;
	if ((fd = open(device, O_RDONLY | O_NOFOLLOW)) < 0) {
		PERROR("open(%s)", device);
		return 0;
	}
	if (ioctl(fd, BLKGETSIZE, &sectors)) {
		PERROR("ioctl(%s, BLKGETSIZE)", device);
		return 0;
	}
	close(fd);
	return sectors;
}

int runcmd(const gchar *argv[], gchar **output)
{
	gint rc;
	gchar *out = NULL, *errout = NULL;
	GError *err = NULL;

	g_spawn_sync(kRootDir, (gchar **)argv, NULL, 0, NULL, NULL,
		     &out, &errout, &rc, &err);
	if (err) {
		ERROR("%s: %s", argv[0], err->message);
		g_error_free(err);
		return -1;
	}

	if (rc)
		ERROR("%s failed (%d)\n%s\n%s", argv[0], rc, out, errout);

	if (output)
		*output = out;
	else
		g_free(out);
	g_free(errout);

	return rc;
}

int same_vfs(const char *mnt_a, const char *mnt_b)
{
	struct statvfs stat_a, stat_b;

	if (statvfs(mnt_a, &stat_a)) {
		PERROR("statvfs(%s)", mnt_a);
		exit(1);
	}
	if (statvfs(mnt_b, &stat_b)) {
		PERROR("statvfs(%s)", mnt_b);
		exit(1);
	}
	return (stat_a.f_fsid == stat_b.f_fsid);
}

/* Returns allocated string that holds [length]*2 + 1 characters. */
char *stringify_hex(uint8_t *binary, size_t length)
{
	char *string;
	size_t i;

	string = malloc(length * 2 + 1);
	if (!string) {
		PERROR("malloc");
		return NULL;
	}
	for (i = 0; i < length; ++i)
		sprintf(string + (i * 2), "%02x", binary[i]);
	string[length * 2] = '\0';

	return string;
}

/* Returns allocated byte array that holds strlen([string])/2 bytes. */
uint8_t *hexify_string(char *string, uint8_t *binary, size_t length)
{
	size_t bytes, i;

	bytes = strlen(string) / 2;
	if (bytes > length) {
		ERROR("Hex string too long (%zu) for byte array (%zu)",
			bytes, length);
		return NULL;
	}

	for (i = 0; i < bytes; ++i) {
		if (sscanf(&string[i * 2], "%2hhx", &binary[i]) != 1) {
			ERROR("Invalid hex code at byte %zu.", i);
			return NULL;
		}
        }

	return binary;
}

static int is_loop_device(int fd)
{
	struct stat info;

	return (fstat(fd, &info) == 0 && S_ISBLK(info.st_mode) &&
		major(info.st_rdev) == kLoopMajor);
}

static int loop_is_attached(int fd, struct loop_info64 *info)
{
	struct loop_info64 local_info;

	return ioctl(fd, LOOP_GET_STATUS64, info ? info : &local_info) == 0;
}

/* Returns either the matching loopback name, or next available, if NULL. */
static int loop_locate(gchar **loopback, const char *name)
{
	int i, fd, namelen = 0;

	if (name) {
		namelen = strlen(name);
		if (namelen >= LO_NAME_SIZE) {
			ERROR("'%s' too long (>= %d)", name, LO_NAME_SIZE);
			return -1;
		}
	}

	*loopback = NULL;
	for (i = 0; i < kLoopMax; ++i) {
		struct loop_info64 info;
		int attached;

		g_free(*loopback);
		*loopback = g_strdup_printf(kLoopTemplate, i);
		if (!*loopback) {
			PERROR("g_strdup_printf");
			return -1;
		}

		fd = open(*loopback, O_RDONLY | O_NOFOLLOW);
		if (fd < 0) {
			PERROR("open(%s)", *loopback);
			goto failed;
		}
		if (!is_loop_device(fd)) {
			close(fd);
			continue;
		}

		memset(&info, 0, sizeof(info));
		attached = loop_is_attached(fd, &info);
		close(fd);

		if (attached)
			DEBUG("Saw %s on %s", info.lo_file_name, *loopback);

		if ((attached && name &&
		     strncmp((char *)info.lo_file_name, name, namelen) == 0) ||
		    (!attached && !name)) {
			DEBUG("Using %s", *loopback);
			/* Reopen for working on it. */
			fd = open(*loopback, O_RDWR | O_NOFOLLOW);
			if (is_loop_device(fd) &&
			    loop_is_attached(fd, NULL) == attached)
				return fd;
		}
	}
	ERROR("Ran out of loopback devices");

failed:
	g_free(*loopback);
	*loopback = NULL;
	return -1;
}

static int loop_detach_fd(int fd)
{
	if (ioctl(fd, LOOP_CLR_FD, 0)) {
		PERROR("LOOP_CLR_FD");
		return 0;
	}
	return 1;
}

int loop_detach(const gchar *loopback)
{
	int fd, rc = 1;

	fd = open(loopback, O_RDONLY | O_NOFOLLOW);
	if (fd < 0) {
		PERROR("open(%s)", loopback);
		return 0;
	}
	if (!is_loop_device(fd) || !loop_is_attached(fd, NULL) ||
	    !loop_detach_fd(fd))
		rc = 0;

	close (fd);
	return rc;
}

int loop_detach_name(const char *name)
{
	gchar *loopback = NULL;
	int loopfd, rc;

	loopfd = loop_locate(&loopback, name);
	if (loopfd < 0)
		return 0;
	rc = loop_detach_fd(loopfd);

	close(loopfd);
	g_free(loopback);
	return rc;
}

/* Closes fd, returns name of loopback device pathname. */
gchar *loop_attach(int fd, const char *name)
{
	gchar *loopback = NULL;
	int loopfd;
	struct loop_info64 info;

	loopfd = loop_locate(&loopback, NULL);
	if (loopfd < 0)
		return NULL;
	if (ioctl(loopfd, LOOP_SET_FD, fd) < 0) {
		PERROR("LOOP_SET_FD");
		goto failed;
	}

	memset(&info, 0, sizeof(info));
	strncpy((char*)info.lo_file_name, name, LO_NAME_SIZE);
	if (ioctl(loopfd, LOOP_SET_STATUS64, &info)) {
		PERROR("LOOP_SET_STATUS64");
		goto failed;
	}

	close(loopfd);
	close(fd);
	return loopback;
failed:
	close(loopfd);
	close(fd);
	g_free(loopback);
	return 0;
}

int dm_setup(size_t sectors, const gchar *encryption_key, const char *name,
		const gchar *device, const char *path, int discard)
{
	/* Mount loopback device with dm-crypt using the encryption key. */
	gchar *table = g_strdup_printf("0 %zu crypt " \
				       "aes-cbc-essiv:sha256 %s " \
				       "0 %s 0%s",
				       sectors,
				       encryption_key,
				       device,
				       discard ? " 1 allow_discards" : "");
	if (!table) {
		PERROR("g_strdup_printf");
		return 0;
	}

	const gchar *argv[] = {
		"/sbin/dmsetup",
		"create", name,
		"--noudevrules", "--noudevsync",
		"--table", table,
		NULL
	};

	/* TODO(keescook): replace with call to libdevmapper. */
	if (runcmd(argv, NULL) != 0) {
		g_free(table);
		return 0;
	}
	g_free(table);

	/* Make sure the dm-crypt device showed up. */
	if (access(path, R_OK)) {
		ERROR("%s does not exist", path);
		return 0;
	}

	return 1;
}

int dm_teardown(const gchar *device)
{
	const char *argv[] = {
		"/sbin/dmsetup",
		"remove", device,
		"--noudevrules", "--noudevsync",
		NULL
	};
	/* TODO(keescook): replace with call to libdevmapper. */
	if (runcmd(argv, NULL) != 0)
		return 0;
	return 1;
}

char *dm_get_key(const gchar *device)
{
	gchar *output = NULL;
	char *key;
	int i;
	const char *argv[] = {
		"/sbin/dmsetup",
		"table", "--showkeys",
		device,
		NULL
	};
	/* TODO(keescook): replace with call to libdevmapper. */
	if (runcmd(argv, &output) != 0)
		return NULL;

	/* Key is 4th field in the output. */
	for (i = 0, key = strtok(output, " ");
	     i < 4 && key;
	     ++i, key = strtok(NULL, " ")) { }

	/* Create a copy of the key and free the output buffer. */
	if (key) {
		key = strdup(key);
		g_free(output);
	}

	return key;
}

int sparse_create(const char *path, size_t size)
{
	int sparsefd;

	sparsefd = open(path, O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW,
			S_IRUSR | S_IWUSR);
	if (sparsefd < 0)
		goto out;

	if (ftruncate(sparsefd, size)) {
		int saved_errno = errno;

		close(sparsefd);
		unlink(path);
		errno = saved_errno;

		sparsefd = -1;
	}

out:
	return sparsefd;
}

int filesystem_build(const char *device, size_t block_bytes, size_t blocks_min,
			size_t blocks_max)
{
	int rc = 0;

	gchar *blocksize = g_strdup_printf("%zu", block_bytes);
	if (!blocksize) {
		PERROR("g_strdup_printf");
		goto out;
	}

	gchar *blocks_str;
	blocks_str = g_strdup_printf("%zu", blocks_min);
	if (!blocks_str) {
		PERROR("g_strdup_printf");
		goto free_blocksize;
	}

	gchar *extended;
	if (blocks_min < blocks_max) {
		extended = g_strdup_printf("%s,resize=%zu",
			kExt4ExtendedOptions, blocks_max);
	} else {
		extended = g_strdup_printf("%s", kExt4ExtendedOptions);
	}
	if (!extended) {
		PERROR("g_strdup_printf");
		goto free_blocks_str;
	}

	const gchar *mkfs[] = {
		"/sbin/mkfs.ext4",
		"-T", "default",
		"-b", blocksize,
		"-m", "0",
		"-O", "^huge_file,^flex_bg",
		"-E", extended,
		device,
		blocks_str,
		NULL
	};

	rc = (runcmd(mkfs, NULL) == 0);
	if (!rc)
		goto free_extended;

	const gchar *tune2fs[] = {
		"/sbin/tune2fs",
		"-c", "0",
		"-i", "0",
		device,
		NULL
	};
	rc = (runcmd(tune2fs, NULL) == 0);

free_extended:
	g_free(extended);
free_blocks_str:
	g_free(blocks_str);
free_blocksize:
	g_free(blocksize);
out:
	return rc;
}

/* Spawns a filesystem resizing process. */
int filesystem_resize(const char *device, size_t blocks, size_t blocks_max)
{
	/* Ignore resizing if we know the filesystem was built to max size. */
	if (blocks >= blocks_max) {
		INFO("Resizing aborted. blocks:%zu >= blocks_max:%zu",
		     blocks, blocks_max);
		return 1;
	}

	/* TODO(keescook): Read superblock to find out the current size of
	 * the filesystem (since statvfs does not report the correct value).
	 * For now, instead of doing multi-step resizing, just resize to the
	 * full size of the block device in one step.
	 */
	blocks = blocks_max;

	INFO("Resizing started in %d second steps.", kResizeStepSeconds);

	do {
		gchar *blocks_str;

		sleep(kResizeStepSeconds);

		blocks += kResizeBlocks;
		if (blocks > blocks_max)
			blocks = blocks_max;

		blocks_str = g_strdup_printf("%zu", blocks);
		if (!blocks_str) {
			PERROR("g_strdup_printf");
			return 0;
		}

		const gchar *resize[] = {
			"/sbin/resize2fs",
			"-f",
			device,
			blocks_str,
			NULL
		};

		INFO("Resizing filesystem on %s to %zu.", device, blocks);
		if (runcmd(resize, NULL)) {
			ERROR("resize2fs failed");
			return 0;
		}
		g_free(blocks_str);
	} while (blocks < blocks_max);

	INFO("Resizing finished.");
	return 1;
}

char *keyfile_read(const char *keyfile, uint8_t *system_key)
{
	char *key = NULL;
	unsigned char *cipher = NULL;
	gsize length;
	uint8_t *plain = NULL;
	int plain_length, final_len;
	GError *error = NULL;
	EVP_CIPHER_CTX ctx;
	const EVP_CIPHER *algo = EVP_aes_256_cbc();

	DEBUG("Reading keyfile %s", keyfile);
	if (EVP_CIPHER_key_length(algo) != DIGEST_LENGTH) {
		ERROR("cipher key size mismatch (got %d, want %d)",
			EVP_CIPHER_key_length(algo), DIGEST_LENGTH);
		goto out;
	}

	if (access(keyfile, R_OK)) {
		/* This file being missing is handled in caller, so
		 * do not emit error message.
		 */
		INFO("%s does not exist.", keyfile);
		goto out;
	}

	if (!g_file_get_contents(keyfile, (gchar **)&cipher, &length,
				 &error)) {
		ERROR("Unable to read %s: %s", keyfile, error->message);
		g_error_free(error);
		goto out;
	}
	plain = malloc(length + EVP_CIPHER_block_size(algo));
	if (!plain) {
		PERROR("malloc");
		goto free_cipher;
	}

	DEBUG("Decrypting keyfile %s", keyfile);
	/* Use the default IV. */
	if (!EVP_DecryptInit(&ctx, algo, system_key, NULL)) {
		SSL_ERROR("EVP_DecryptInit");
		goto free_plain;
	}
	if (!EVP_DecryptUpdate(&ctx, plain, &plain_length, cipher, length)) {
		SSL_ERROR("EVP_DecryptUpdate");
		goto free_ctx;
	}
	if (!EVP_DecryptFinal(&ctx, plain+plain_length, &final_len)) {
		SSL_ERROR("EVP_DecryptFinal");
		goto free_ctx;
	}
	plain_length += final_len;

	if (plain_length != DIGEST_LENGTH) {
		ERROR("Decrypted encryption key length (%d) is not %d.",
		      plain_length, DIGEST_LENGTH);
		goto free_ctx;
	}

	debug_dump_hex("encryption key", plain, DIGEST_LENGTH);

	key = stringify_hex(plain, DIGEST_LENGTH);

free_ctx:
	EVP_CIPHER_CTX_cleanup(&ctx);
free_plain:
	free(plain);
free_cipher:
	g_free(cipher);
out:
	DEBUG("key:%p", key);
	return key;
}

int keyfile_write(const char *keyfile, uint8_t *system_key, char *string)
{
	int rc = 0;
	size_t length;
	uint8_t plain[DIGEST_LENGTH];
	uint8_t *cipher = NULL;
	int cipher_length, final_len;
	GError *error = NULL;
	EVP_CIPHER_CTX ctx;
	const EVP_CIPHER *algo = EVP_aes_256_cbc();
	mode_t mask;

	DEBUG("Staring to process keyfile %s", keyfile);
	/* Have key file be read/write only by root user. */
	mask = umask(0077);

	if (EVP_CIPHER_key_length(algo) != DIGEST_LENGTH) {
		ERROR("cipher key size mismatch (got %d, want %d)",
			EVP_CIPHER_key_length(algo), DIGEST_LENGTH);
		goto out;
	}

	if (access(keyfile, R_OK) == 0) {
		ERROR("%s already exists.", keyfile);
		goto out;
	}

	length = strlen(string);
	if (length != sizeof(plain) * 2) {
		ERROR("Encryption key string length (%zu) is not %zu.",
		      length, sizeof(plain) * 2);
		goto out;
	}

	length = sizeof(plain);
	if (!hexify_string(string, plain, length)) {
		ERROR("Failed to convert encryption key to byte array");
		goto out;
	}

	debug_dump_hex("encryption key", plain, DIGEST_LENGTH);

	cipher = malloc(length + EVP_CIPHER_block_size(algo));
	if (!cipher) {
		PERROR("malloc");
		goto out;
	}

	DEBUG("Encrypting keyfile %s", keyfile);
	/* Use the default IV. */
	if (!EVP_EncryptInit(&ctx, algo, system_key, NULL)) {
		SSL_ERROR("EVP_EncryptInit");
		goto free_cipher;
	}
	if (!EVP_EncryptUpdate(&ctx, cipher, &cipher_length,
			       (unsigned char *)plain, length)) {
		SSL_ERROR("EVP_EncryptUpdate");
		goto free_ctx;
	}
	if (!EVP_EncryptFinal(&ctx, cipher+cipher_length, &final_len)) {
		SSL_ERROR("EVP_EncryptFinal");
		goto free_ctx;
	}
	length = cipher_length + final_len;

	DEBUG("Writing %zu bytes to %s", length, keyfile);
	if (!g_file_set_contents(keyfile, (gchar *)cipher, length, &error)) {
		ERROR("Unable to write %s: %s", keyfile, error->message);
		g_error_free(error);
		goto free_ctx;
	}

	rc = 1;

free_ctx:
	EVP_CIPHER_CTX_cleanup(&ctx);
free_cipher:
	free(cipher);
out:
	umask(mask);
	DEBUG("keyfile write rc:%d", rc);
	return rc;
}
