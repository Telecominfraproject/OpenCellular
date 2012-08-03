/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This tool will attempt to mount or create the encrypted stateful partition,
 * and the various bind mountable subdirectories.
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
#include <grp.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <linux/fs.h>

#include <glib.h>

#include <openssl/rand.h>

#define CHROMEOS_ENVIRONMENT
#include "tlcl.h"
#include "crossystem.h"

#include "mount-encrypted.h"
#include "mount-helpers.h"

#define STATEFUL_MNT "mnt/stateful_partition"
#define ENCRYPTED_MNT STATEFUL_MNT "/encrypted"
#define BUF_SIZE 1024
#define PROP_SIZE 64

static const gchar * const kKernelCmdline = "/proc/cmdline";
static const gchar * const kKernelCmdlineOption = " encrypted-stateful-key=";
static const gchar * const kEncryptedFSType = "ext4";
static const gchar * const kCryptDevName = "encstateful";
static const gchar * const kTpmDev = "/dev/tpm0";
static const gchar * const kNullDev = "/dev/null";
static const float kSizePercent = 0.3;
static const float kMigrationSizeMultiplier = 1.1;
static const uint32_t kLockboxIndex = 0x20000004;
static const uint32_t kLockboxSizeV1 = 0x2c;
static const uint32_t kLockboxSizeV2 = 0x45;
static const uint32_t kLockboxSaltOffset = 0x5;
static const size_t kSectorSize = 512;
static const size_t kExt4BlockSize = 4096;
static const size_t kExt4MinBytes = 16 * 1024 * 1024;
static const char * const kStaticKeyDefault = "default unsafe static key";
static const char * const kStaticKeyFactory = "factory unsafe static key";
static const char * const kStaticKeyFinalizationNeeded = "needs finalization";
static const int kModeProduction = 0;
static const int kModeFactory = 1;
static const int kCryptAllowDiscard = 1;

enum migration_method {
	MIGRATE_TEST_ONLY,
	MIGRATE_FOR_REAL,
};

enum bind_dir {
	BIND_SOURCE,
	BIND_DEST,
};

static struct bind_mount {
	char * src;		/* Location of bind source. */
	char * dst;		/* Destination of bind. */
	char * previous;	/* Migratable prior bind source. */
	char * pending;		/* Location for pending deletion. */
	char * owner;
	char * group;
	mode_t mode;
	int submount;		/* Submount is bound already. */
} bind_mounts_default[] = {
	{ ENCRYPTED_MNT "/var", "var",
	  STATEFUL_MNT "/var", STATEFUL_MNT "/.var",
	  "root", "root",
	  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH, 0 },
	{ ENCRYPTED_MNT "/chronos", "home/chronos",
	  STATEFUL_MNT "/home/chronos", STATEFUL_MNT "/home/.chronos",
	  "chronos", "chronos",
	  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH, 1 },
	{ },
};

#if DEBUG_ENABLED
struct timeval tick = { };
struct timeval tick_start = { };
#endif

static struct bind_mount *bind_mounts = NULL;
static gchar *rootdir = NULL;
static gchar *stateful_mount = NULL;
static gchar *key_path = NULL;
static gchar *needs_finalization_path = NULL;
static gchar *block_path = NULL;
static gchar *encrypted_mount = NULL;
static gchar *dmcrypt_name = NULL;
static gchar *dmcrypt_dev = NULL;
static int has_tpm = 0;

static void tpm_init(void)
{
	int tpm;

	DEBUG("Opening TPM");
	tpm = open(kTpmDev, O_RDWR);
	if (tpm >= 0) {
		has_tpm = 1;
		close(tpm);
	}
	else {
		/* TlclLibInit does not fail, it exits, so instead,
		 * have it open /dev/null if the TPM is not available.
		 */
		setenv("TPM_DEVICE_PATH", kNullDev, 1);
	}
	TlclLibInit();
	DEBUG("TPM %s", has_tpm ? "Ready" : "not available");
}

/* Returns TPM result status code, and on TPM_SUCCESS, stores ownership
 * flag to "owned".
 */
static uint32_t tpm_owned(uint8_t *owned)
{
	uint32_t result;

	DEBUG("Reading TPM Ownership Flag");
	result = TlclGetOwnership(owned);
	DEBUG("TPM Ownership Flag returned: %s", result ? "FAIL" : "ok");

	return result;
}

static void tpm_close(void)
{
	TlclLibClose();
}

static void sha256(char *string, uint8_t *digest)
{
	SHA256((unsigned char *)string, strlen(string), digest);
}

/* Extract the desired system key from the kernel's boot command line. */
static int get_key_from_cmdline(uint8_t *digest)
{
	int result = 0;
	gchar *buffer;
	gsize length;
	char *cmdline, *option_end;
	/* Option name without the leading space. */
	const gchar *option = kKernelCmdlineOption + 1;

	if (!g_file_get_contents(kKernelCmdline, &buffer, &length, NULL)) {
		PERROR(kKernelCmdline);
		return 0;
	}

	/* Find a string match either at start of string or following
	 * a space.
	 */
	cmdline = buffer;
	if (strncmp(cmdline, option, strlen(option)) == 0 ||
	    (cmdline = strstr(cmdline, kKernelCmdlineOption))) {
		/* The "=" exists because it is in kKernelCmdlineOption. */
		cmdline = strstr(cmdline, "=");
		/* strchrnul() cannot return NULL. */
		option_end = strchrnul(cmdline, ' ');
		*option_end = '\0';
		sha256(cmdline, digest);
		debug_dump_hex("system key", digest, DIGEST_LENGTH);
		result = 1;
	}

	g_free(buffer);
	return result;
}

static int get_system_property(const char *prop, char *buf, size_t length)
{
	const char *rc;

	DEBUG("Fetching System Property '%s'", prop);
	rc = VbGetSystemPropertyString(prop, buf, length);
	DEBUG("Got System Property 'mainfw_type': %s", rc ? buf : "FAIL");

	return rc != NULL;
}

static int has_chromefw(void)
{
	static int state = -1;
	char fw[PROP_SIZE];

	/* Cache the state so we don't have to perform the query again. */
	if (state != -1)
		return state;

	if (!get_system_property("mainfw_type", fw, sizeof(fw)))
		state = 0;
	else
		state = strcmp(fw, "nonchrome") != 0;
	return state;
}

static int is_cr48(void)
{
	static int state = -1;
	char hwid[PROP_SIZE];

	/* Cache the state so we don't have to perform the query again. */
	if (state != -1)
		return state;

	if (!get_system_property("hwid", hwid, sizeof(hwid)))
		state = 0;
	else
		state = strstr(hwid, "MARIO") != NULL;
	return state;
}

static uint32_t
_read_nvram(uint8_t *buffer, size_t len, uint32_t index, uint32_t size)
{
	uint32_t result;

	if (size > len) {
		ERROR("NVRAM size (0x%x > 0x%zx) is too big", size, len);
		return 0;
	}

	DEBUG("Reading NVRAM area 0x%x (size %u)", index, size);
	result = TlclRead(index, buffer, size);
	DEBUG("NVRAM read returned: %s", result == TPM_SUCCESS ? "ok"
							       : "FAIL");

	return result;
}

/*
 * TPM ownership cases:
 *  - unowned (OOBE):
 *    - expect modern lockbox (no migration allowed).
 *  - owned: depends on NVRAM area (below).
 *
 * NVRAM area cases:
 *  - no NVRAM area at all:
 *    - interrupted install (cryptohome has the TPM password)
 *    - ancient device (cr48, cryptohome has thrown away TPM password)
 *    - broken device (cryptohome has thrown away/never had TPM password)
 *      - must expect worst-case: no lockbox ever, and migration allowed.
 *  - defined NVRAM area, but not written to ("Finalized"); interrupted OOBE:
 *    - if legacy size, allow migration.
 *    - if not, disallow migration.
 *  - written ("Finalized") NVRAM area:
 *    - if legacy size, allow migration.
 *    - if not, disallow migration.
 *
 * When returning 1: (NVRAM area found and used)
 *  - *digest populated with NVRAM area entropy.
 *  - *migrate is 1 for NVRAM v1, 0 for NVRAM v2.
 * When returning 0: (NVRAM missing or error)
 *  - *digest untouched.
 *  - *migrate always 1
 */
static int get_nvram_key(uint8_t *digest, int *migrate)
{
	uint8_t owned = 0;
	uint8_t value[kLockboxSizeV2], bytes_anded, bytes_ored;
	uint32_t size, result, i;
	uint8_t *rand_bytes;
	uint32_t rand_size;

	/* Default to allowing migration (disallow when owned with NVRAMv2). */
	*migrate = 1;

	/* Ignore unowned TPM's NVRAM area. */
	result = tpm_owned(&owned);
	if (result != TPM_SUCCESS) {
		INFO("Could not read TPM Permanent Flags.");
		return 0;
	}
	if (!owned) {
		INFO("TPM not Owned, ignoring NVRAM area.");
		return 0;
	}

	/* Reading the NVRAM takes 40ms. Instead of querying the NVRAM area
	 * for its size (which takes time), just read the expected size. If
	 * it fails, then fall back to the older size. This means cleared
	 * devices take 80ms (2 failed reads), legacy devices take 80ms
	 * (1 failed read, 1 good read), and populated devices take 40ms,
	 * which is the minimum possible time (instead of 40ms + time to
	 * query NVRAM size).
	 */
	size = kLockboxSizeV2;
	result = _read_nvram(value, sizeof(value), kLockboxIndex, size);
	if (result != TPM_SUCCESS) {
		size = kLockboxSizeV1;
		result = _read_nvram(value, sizeof(value), kLockboxIndex, size);
		if (result != TPM_SUCCESS) {
			/* No NVRAM area at all. */
			INFO("No NVRAM area defined.");
			return 0;
		}
		/* Legacy NVRAM area. */
		INFO("Version 1 NVRAM area found.");
	} else {
		*migrate = 0;
		INFO("Version 2 NVRAM area found.");
	}

	debug_dump_hex("nvram", value, size);

	/* Ignore defined but unwritten NVRAM area. */
	bytes_ored = 0x0;
	bytes_anded = 0xff;
	for (i = 0; i < size; ++i) {
		bytes_ored |= value[i];
		bytes_anded &= value[i];
	}
	if (bytes_ored == 0x0 || bytes_anded == 0xff) {
		INFO("NVRAM area has been defined but not written.");
		return 0;
	}

	/* Choose random bytes to use based on NVRAM version. */
	if (*migrate) {
		rand_bytes = value;
		rand_size = size;
	} else {
		rand_bytes = value + kLockboxSaltOffset;
		if (kLockboxSaltOffset + DIGEST_LENGTH > size) {
			INFO("Impossibly small NVRAM area size (%d).", size);
			return 0;
		}
		rand_size = DIGEST_LENGTH;
	}
	if (rand_size < DIGEST_LENGTH) {
		INFO("Impossibly small rand_size (%d).", rand_size);
		return 0;
	}
	debug_dump_hex("rand_bytes", rand_bytes, rand_size);

	SHA256(rand_bytes, rand_size, digest);
	debug_dump_hex("system key", digest, DIGEST_LENGTH);

	return 1;
}

/* Find the system key used for decrypting the stored encryption key.
 * ChromeOS devices are required to use the NVRAM area, all the rest will
 * fallback through various places (kernel command line, BIOS UUID, and
 * finally a static value) for a system key.
 */
static int find_system_key(int mode, uint8_t *digest, int *migration_allowed)
{
	gchar *key;
	gsize length;

	/* By default, do not allow migration. */
	*migration_allowed = 0;

	/* Factory mode uses a static system key. */
	if (mode == kModeFactory) {
		INFO("Using factory insecure system key.");
		sha256((char *)kStaticKeyFactory, digest);
		debug_dump_hex("system key", digest, DIGEST_LENGTH);
		return 1;
	}

	/* Force ChromeOS devices into requiring the system key come from
	 * NVRAM.
	 */
	if (has_chromefw()) {
		int rc;
		rc = get_nvram_key(digest, migration_allowed);

		if (rc) {
			INFO("Using NVRAM as system key; already populated%s.",
				*migration_allowed ? " (legacy)" : "");
		} else {
			INFO("Using NVRAM as system key; finalization needed.");
		}
		return rc;
	}

	if (get_key_from_cmdline(digest)) {
		INFO("Using kernel command line argument as system key.");
		return 1;
	}
	if (g_file_get_contents("/sys/class/dmi/id/product_uuid",
				&key, &length, NULL)) {
		sha256(key, digest);
		debug_dump_hex("system key", digest, DIGEST_LENGTH);
		g_free(key);
		INFO("Using UUID as system key.");
		return 1;
	}

	INFO("Using default insecure system key.");
	sha256((char *)kStaticKeyDefault, digest);
	debug_dump_hex("system key", digest, DIGEST_LENGTH);
	return 1;
}

/* Returns 1 on success, 0 on failure. */
static int get_random_bytes_tpm(unsigned char *buffer, int wanted)
{
	uint32_t remaining = wanted;

	/* Read random bytes from TPM, which can return short reads. */
	while (remaining) {
		uint32_t result, size;

		result = TlclGetRandom(buffer + (wanted - remaining),
				       remaining, &size);
		if (result != TPM_SUCCESS || size > remaining) {
			ERROR("TPM GetRandom failed.");
			return 0;
		}
		remaining -= size;
	}

	return 1;
}

/* Returns 1 on success, 0 on failure. */
static int get_random_bytes(unsigned char *buffer, int wanted)
{
	if (has_tpm)
		return get_random_bytes_tpm(buffer, wanted);
	else
		return RAND_bytes(buffer, wanted);
}

static char *choose_encryption_key(void)
{
	unsigned char rand_bytes[DIGEST_LENGTH];
	unsigned char digest[DIGEST_LENGTH];

	get_random_bytes(rand_bytes, sizeof(rand_bytes));

	SHA256(rand_bytes, DIGEST_LENGTH, digest);
	debug_dump_hex("encryption key", digest, DIGEST_LENGTH);

	return stringify_hex(digest, DIGEST_LENGTH);
}

static int check_bind(struct bind_mount *bind, enum bind_dir dir)
{
	struct passwd *user;
	struct group *group;
	const gchar *target;

	if (dir == BIND_SOURCE)
		target = bind->src;
	else
		target = bind->dst;

	if (access(target, R_OK) && mkdir(target, bind->mode)) {
		PERROR("mkdir(%s)", target);
		return -1;
	}

	/* Destination may be on read-only filesystem, so skip tweaks. */
	if (dir == BIND_DEST)
		return 0;

	if (!(user = getpwnam(bind->owner))) {
		PERROR("getpwnam(%s)", bind->owner);
		return -1;
	}
	if (!(group = getgrnam(bind->group))) {
		PERROR("getgrnam(%s)", bind->group);
		return -1;
	}

	/* Must do explicit chmod since mkdir()'s mode respects umask. */
	if (chmod(target, bind->mode)) {
		PERROR("chmod(%s)", target);
		return -1;
	}
	if (chown(target, user->pw_uid, group->gr_gid)) {
		PERROR("chown(%s)", target);
		return -1;
	}

	return 0;
}

static int migrate_contents(struct bind_mount *bind,
			    enum migration_method method)
{
	const gchar *previous = NULL;
	const gchar *pending = NULL;
	gchar *dotdir;

	/* Skip migration if the previous bind sources are missing. */
	if (bind->pending && access(bind->pending, R_OK) == 0)
		pending = bind->pending;
	if (bind->previous && access(bind->previous, R_OK) == 0)
		previous = bind->previous;
	if (!pending && !previous)
		return 0;

	/* Pretend migration happened. */
	if (method == MIGRATE_TEST_ONLY)
		return 1;

	check_bind(bind, BIND_SOURCE);

	/* Prefer the pending-delete location when doing migration. */
	if (!(dotdir = g_strdup_printf("%s/.", pending ? pending : previous))) {
		PERROR("g_strdup_printf");
		goto mark_for_removal;
	}

	INFO("Migrating bind mount contents %s to %s.", dotdir, bind->src);
	const gchar *cp[] = {
		"/bin/cp", "-a",
		dotdir,
		bind->src,
		NULL
	};

	if (runcmd(cp, NULL) != 0) {
		/* If the copy failed, it may have partially populated the
		 * new source, so we need to remove the new source and
		 * rebuild it. Regardless, the previous source must be removed
		 * as well.
		 */
		INFO("Failed to migrate %s to %s!", dotdir, bind->src);
		remove_tree(bind->src);
		check_bind(bind, BIND_SOURCE);
	}

mark_for_removal:
	g_free(dotdir);

	/* The removal of the previous directory needs to happen at finalize
	 * time, otherwise /var state gets lost on a migration if the
	 * system is powered off before the encryption key is saved. Instead,
	 * relocate the directory so it can be removed (or re-migrated).
	 */

	if (previous) {
		/* If both pending and previous directory exists, we must
		 * remove previous entirely now so it stops taking up disk
		 * space. The pending area will stay pending to be deleted
		 * later.
		 */
		if (pending)
			remove_tree(pending);
		if (rename(previous, bind->pending)) {
			PERROR("rename(%s,%s)", previous, bind->pending);
		}
	}

	/* As noted above, failures are unrecoverable, so getting here means
	 * "we're done" more than "it worked".
	 */
	return 1;
}

static void finalized(void)
{
	/* TODO(keescook): once ext4 supports secure delete, just unlink. */
	if (access(needs_finalization_path, R_OK) == 0) {
		/* This is nearly useless on SSDs. */
		shred(needs_finalization_path);
		unlink(needs_finalization_path);
	}
}

static void finalize(uint8_t *system_key, char *encryption_key)
{
	struct bind_mount *bind;

	INFO("Writing keyfile %s.", key_path);
	if (!keyfile_write(key_path, system_key, encryption_key)) {
		ERROR("Failed to write %s -- aborting.", key_path);
		return;
	}

	finalized();

	for (bind = bind_mounts; bind->src; ++ bind) {
		if (!bind->pending || access(bind->pending, R_OK))
			continue;
		INFO("Removing %s.", bind->pending);
#if DEBUG_ENABLED
		continue;
#endif
		remove_tree(bind->pending);
	}
}

static void needs_finalization(char *encryption_key)
{
	uint8_t useless_key[DIGEST_LENGTH];
	sha256((char *)kStaticKeyFinalizationNeeded, useless_key);

	INFO("Writing finalization intent %s.", needs_finalization_path);
	if (!keyfile_write(needs_finalization_path, useless_key,
			   encryption_key)) {
		ERROR("Failed to write %s -- aborting.",
		      needs_finalization_path);
		return;
	}
}

/* This triggers the live encryption key to be written to disk, encrypted
 * by the system key. It is intended to be called by Cryptohome once the
 * TPM is done being set up. If the system key is passed as an argument,
 * use it, otherwise attempt to query the TPM again.
 */
static int finalize_from_cmdline(char *key)
{
	uint8_t system_key[DIGEST_LENGTH];
	char *encryption_key;
	int migrate;

	/* Early sanity-check to see if the encrypted device exists,
	 * instead of failing at the end of this function.
	 */
	if (access(dmcrypt_dev, R_OK)) {
		ERROR("'%s' does not exist, giving up.", dmcrypt_dev);
		return EXIT_FAILURE;
	}

	if (key) {
		if (strlen(key) != 2 * DIGEST_LENGTH) {
			ERROR("Invalid key length.");
			return EXIT_FAILURE;
		}

		if (!hexify_string(key, system_key, DIGEST_LENGTH)) {
			ERROR("Failed to convert hex string to byte array");
			return EXIT_FAILURE;
		}
	} else {
		/* Factory mode will never call finalize from the command
		 * line, so force Production mode here.
		 */
		if (!find_system_key(kModeProduction, system_key, &migrate)) {
			ERROR("Could not locate system key.");
			return EXIT_FAILURE;
		}
	}

	encryption_key = dm_get_key(dmcrypt_dev);
	if (!encryption_key) {
		ERROR("Could not locate encryption key for %s.", dmcrypt_dev);
		return EXIT_FAILURE;
	}

	finalize(system_key, encryption_key);

	return EXIT_SUCCESS;
}

static void spawn_resizer(const char *device, size_t blocks,
			  size_t blocks_max)
{
	pid_t pid;

	/* Skip resize before forking, if it's not going to happen. */
	if (blocks >= blocks_max) {
		INFO("Resizing skipped. blocks:%zu >= blocks_max:%zu",
		     blocks, blocks_max);
		return;
	}

	fflush(NULL);
	pid = fork();
	if (pid < 0) {
		PERROR("fork");
		return;
	}
	if (pid != 0) {
		INFO("Started filesystem resizing process %d.", pid);
		return;
	}

	/* Child */
	tpm_close();
	INFO_INIT("Resizer spawned.");

	if (daemon(0, 1)) {
		PERROR("daemon");
		goto out;
	}

	filesystem_resize(device, blocks, blocks_max);

out:
	INFO_DONE("Done.");
	exit(0);
}

/* Do all the work needed to actually set up the encrypted partition.
 * Takes "mode" argument to help determine where the system key should
 * come from.
 */
static int setup_encrypted(int mode)
{
	int has_system_key;
	uint8_t system_key[DIGEST_LENGTH];
	char *encryption_key = NULL;
	int migrate_allowed = 0, migrate_needed = 0, rebuild = 0;
	gchar *lodev = NULL;
	size_t sectors;
	struct bind_mount *bind;
	int sparsefd;
	struct statvfs stateful_statbuf;
	size_t blocks_min, blocks_max;

	/* Use the "system key" to decrypt the "encryption key" stored in
	 * the stateful partition.
	 */
	has_system_key = find_system_key(mode, system_key, &migrate_allowed);
	if (has_system_key) {
		encryption_key = keyfile_read(key_path, system_key);
	} else {
		INFO("No usable system key found.");
	}

	if (encryption_key) {
		/* If we found a stored encryption key, we've already
		 * finished a complete login and Cryptohome Finalize
		 * so migration is finished.
		 */
		migrate_allowed = 0;
	} else {
		uint8_t useless_key[DIGEST_LENGTH];
		sha256((char *)kStaticKeyFinalizationNeeded, useless_key);
		encryption_key = keyfile_read(needs_finalization_path,
					      useless_key);
		if (!encryption_key) {
			/* This is a brand new system with no keys. */
			INFO("Generating new encryption key.");
			encryption_key = choose_encryption_key();
			if (!encryption_key)
				return 0;
			rebuild = 1;
		} else {
			ERROR("Finalization unfinished! " \
			      "Encryption key still on disk!");
		}
	}

	if (rebuild) {
		off_t fs_bytes_max;

		/* Wipe out the old files, and ignore errors. */
		unlink(key_path);
		unlink(block_path);

		/* Calculate the desired size of the new partition. */
		if (statvfs(stateful_mount, &stateful_statbuf)) {
			PERROR(stateful_mount);
			return 0;
		}
		fs_bytes_max = stateful_statbuf.f_blocks;
		fs_bytes_max *= kSizePercent;
		fs_bytes_max *= stateful_statbuf.f_frsize;

		INFO("Creating sparse backing file with size %llu.",
		     (unsigned long long)fs_bytes_max);

		/* Create the sparse file. */
		sparsefd = sparse_create(block_path, fs_bytes_max);
		if (sparsefd < 0) {
			PERROR(block_path);
			return 0;
		}
	} else {
		sparsefd = open(block_path, O_RDWR | O_NOFOLLOW);
		if (sparsefd < 0) {
			PERROR(block_path);
			return 0;
		}
	}

	/* Set up loopback device. */
	INFO("Loopback attaching %s (named %s).", block_path, dmcrypt_name);
	lodev = loop_attach(sparsefd, dmcrypt_name);
	if (!lodev || strlen(lodev) == 0) {
		ERROR("loop_attach failed");
		goto failed;
	}

	/* Get size as seen by block device. */
	sectors = get_sectors(lodev);
	if (!sectors) {
		ERROR("Failed to read device size");
		goto lo_cleanup;
	}

	/* Mount loopback device with dm-crypt using the encryption key. */
	INFO("Setting up dm-crypt %s as %s.", lodev, dmcrypt_dev);
	if (!dm_setup(sectors, encryption_key, dmcrypt_name, lodev,
		      dmcrypt_dev, kCryptAllowDiscard)) {
		/* If dm_setup() fails, it could be due to lacking
		 * "allow_discard" support, so try again with discard
		 * disabled. There doesn't seem to be a way to query
		 * the kernel for this feature short of a fallible
		 * version test or just trying to set up the dm table
		 * again, so do the latter.
		 */
		if (!dm_setup(sectors, encryption_key, dmcrypt_name, lodev,
			      dmcrypt_dev, !kCryptAllowDiscard)) {
			ERROR("dm_setup failed");
			goto lo_cleanup;
		}
		INFO("%s: dm-crypt does not support discard; disabling.",
		     dmcrypt_dev);
	}

	/* Decide now if any migration will happen. If so, we will not
	 * grow the new filesystem in the background, since we need to
	 * copy the contents over before /var is valid again.
	 */
	if (!rebuild)
		migrate_allowed = 0;
	if (migrate_allowed) {
		for (bind = bind_mounts; bind->src; ++ bind) {
			if (migrate_contents(bind, MIGRATE_TEST_ONLY))
				migrate_needed = 1;
		}
	}

	/* Calculate filesystem min/max size. */
	blocks_max = sectors / (kExt4BlockSize / kSectorSize);
	blocks_min = kExt4MinBytes / kExt4BlockSize;
	if (migrate_needed && migrate_allowed) {
		off_t fs_bytes_min;
		size_t calc_blocks_min;
		/* When doing a migration, the new filesystem must be
		 * large enough to hold what we're going to migrate.
		 * Instead of walking the bind mount sources, which would
		 * be IO and time expensive, just read the bytes-used
		 * value from statvfs (plus 10% for overhead). It will
		 * be too large, since it includes the eCryptFS data, so
		 * we must cap at the max filesystem size just in case.
		 */

		/* Bytes used in stateful partition plus 10%. */
		fs_bytes_min = stateful_statbuf.f_blocks -
			       stateful_statbuf.f_bfree;
		fs_bytes_min *= stateful_statbuf.f_frsize;
		DEBUG("Stateful bytes used: %llu",
			(unsigned long long)fs_bytes_min);
		fs_bytes_min *= kMigrationSizeMultiplier;

		/* Minimum blocks needed for that many bytes. */
		calc_blocks_min = fs_bytes_min / kExt4BlockSize;
		/* Do not use more than blocks_max. */
		if (calc_blocks_min > blocks_max)
			calc_blocks_min = blocks_max;
		/* Do not use less than blocks_min. */
		else if (calc_blocks_min < blocks_min)
			calc_blocks_min = blocks_min;

		DEBUG("Maximum fs blocks: %zu", blocks_max);
		DEBUG("Minimum fs blocks: %zu", blocks_min);
		DEBUG("Migration blocks chosen: %zu", calc_blocks_min);
		blocks_min = calc_blocks_min;
	}

	if (rebuild) {
		INFO("Building filesystem on %s "
			"(blocksize:%zu, min:%zu, max:%zu).",
			dmcrypt_dev, kExt4BlockSize, blocks_min, blocks_max);
		if (!filesystem_build(dmcrypt_dev, kExt4BlockSize,
					blocks_min, blocks_max))
			goto dm_cleanup;
	}

	/* Mount the dm-crypt partition finally. */
	INFO("Mounting %s onto %s.", dmcrypt_dev, encrypted_mount);
	if (access(encrypted_mount, R_OK) &&
	    mkdir(encrypted_mount, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
		PERROR(dmcrypt_dev);
		goto dm_cleanup;
	}
	if (mount(dmcrypt_dev, encrypted_mount, kEncryptedFSType,
		  MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_RELATIME,
		  "discard")) {
		PERROR("mount(%s,%s)", dmcrypt_dev, encrypted_mount);
		goto dm_cleanup;
	}

	/* Always spawn filesystem resizer, in case growth was interrupted. */
	/* TODO(keescook): if already full size, don't resize. */
	spawn_resizer(dmcrypt_dev, blocks_min, blocks_max);

	/* If the legacy lockbox NVRAM area exists, we've rebuilt the
	 * filesystem, and there are old bind sources on disk, attempt
	 * migration.
	 */
	if (migrate_needed && migrate_allowed) {
		/* Migration needs to happen before bind mounting because
		 * some partitions were not already on the stateful partition,
		 * and would be over-mounted by the new bind mount.
		 */
		for (bind = bind_mounts; bind->src; ++ bind)
			migrate_contents(bind, MIGRATE_FOR_REAL);
	}

	/* Perform bind mounts. */
	for (bind = bind_mounts; bind->src; ++ bind) {
		INFO("Bind mounting %s onto %s.", bind->src, bind->dst);
		if (check_bind(bind, BIND_SOURCE) ||
		    check_bind(bind, BIND_DEST))
			goto unbind;
		if (mount(bind->src, bind->dst, "none", MS_BIND, NULL)) {
			PERROR("mount(%s,%s)", bind->src, bind->dst);
			goto unbind;
		}
	}

	/* When we are creating the encrypted mount for the first time,
	 * either finalize immediately, or write the encryption key to
	 * disk (*sigh*) to handle the seemingly endless broken or
	 * wedged TPM states.
	 */
	if (rebuild) {
		/* Devices that already have the NVRAM area populated and
		 * are being rebuilt don't need to wait for Cryptohome
		 * because the NVRAM area isn't going to change.
		 *
		 * Devices that do not have the NVRAM area populated
		 * may potentially never have the NVRAM area populated,
		 * which means we have to write the encryption key to
		 * disk until we finalize. Once secure deletion is
		 * supported on ext4, this won't be as horrible.
		 */
		if (has_system_key)
			finalize(system_key, encryption_key);
		else
			needs_finalization(encryption_key);
	} else {
		/* If we're not rebuilding and we have a sane system
		 * key, then we must have finalized. Force any required
		 * clean up.
		 */
		if (has_system_key)
			finalized();
	}

	free(lodev);
	return 1;

unbind:
	for (bind = bind_mounts; bind->src; ++ bind) {
		INFO("Unmounting %s.", bind->dst);
		umount(bind->dst);
	}

	INFO("Unmounting %s.", encrypted_mount);
	umount(encrypted_mount);

dm_cleanup:
	INFO("Removing %s.", dmcrypt_dev);
	/* TODO(keescook): something holds this open briefly on mkfs failure
	 * and I haven't been able to catch it yet. Adding an "fuser" call
	 * here is sufficient to lose the race. Instead, just sleep during
	 * the error path.
	 */
	sleep(1);
	dm_teardown(dmcrypt_dev);

lo_cleanup:
	INFO("Unlooping %s.", lodev);
	loop_detach(lodev);

failed:
	free(lodev);

	return 0;
}

/* Clean up all bind mounts, mounts, attaches, etc. Only the final
 * action informs the return value. This makes it so that failures
 * can be cleaned up from, and continue the shutdown process on a
 * second call. If the loopback cannot be found, claim success.
 */
static int shutdown(void)
{
	struct bind_mount *bind;

	for (bind = bind_mounts; bind->src; ++ bind) {
		INFO("Unmounting %s.", bind->dst);
		errno = 0;
		/* Allow either success or a "not mounted" failure. */
		if (umount(bind->dst)) {
			if (errno != EINVAL) {
				PERROR("umount(%s)", bind->dst);
				return EXIT_FAILURE;
			}
		}
	}

	INFO("Unmounting %s.", encrypted_mount);
	errno = 0;
	/* Allow either success or a "not mounted" failure. */
	if (umount(encrypted_mount)) {
		if (errno != EINVAL) {
			PERROR("umount(%s)", encrypted_mount);
			return EXIT_FAILURE;
		}
	}

	/* Optionally run fsck on the device after umount. */
	if (getenv("MOUNT_ENCRYPTED_FSCK")) {
		char *cmd;

		if (asprintf(&cmd, "fsck -a %s", dmcrypt_dev) == -1)
			PERROR("asprintf");
		else {
			int rc;

			rc = system(cmd);
			if (rc != 0)
				ERROR("'%s' failed: %d", cmd, rc);
		}
	}

	INFO("Removing %s.", dmcrypt_dev);
	if (!dm_teardown(dmcrypt_dev))
		ERROR("dm_teardown(%s)", dmcrypt_dev);

	INFO("Unlooping %s (named %s).", block_path, dmcrypt_name);
	if (!loop_detach_name(dmcrypt_name)) {
		ERROR("loop_detach_name(%s)", dmcrypt_name);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static void check_mount_states(void)
{
	struct bind_mount *bind;

	/* Verify stateful partition exists. */
	if (access(stateful_mount, R_OK)) {
		INFO("%s does not exist.", stateful_mount);
		exit(1);
	}
	/* Verify stateful is either a separate mount, or that the
	 * root directory is writable (i.e. a factory install, dev mode
	 * where root remounted rw, etc).
	 */
	if (same_vfs(stateful_mount, rootdir) && access(rootdir, W_OK)) {
		INFO("%s is not mounted.", stateful_mount);
		exit(1);
	}

	/* Verify encrypted partition is missing or not already mounted. */
	if (access(encrypted_mount, R_OK) == 0 &&
	    !same_vfs(encrypted_mount, stateful_mount)) {
		INFO("%s already appears to be mounted.", encrypted_mount);
		exit(0);
	}

	/* Verify that bind mount targets exist. */
	for (bind = bind_mounts; bind->src; ++ bind) {
		if (access(bind->dst, R_OK)) {
			PERROR("%s mount point is missing.", bind->dst);
			exit(1);
		}
	}

	/* Verify that old bind mounts on stateful haven't happened yet. */
	for (bind = bind_mounts; bind->src; ++ bind) {
		if (bind->submount)
			continue;

		if (same_vfs(bind->dst, stateful_mount)) {
			INFO("%s already bind mounted.", bind->dst);
			exit(1);
		}
	}

	INFO("VFS mount state sanity check ok.");
}

static int report_info(void)
{
	uint8_t system_key[DIGEST_LENGTH];
	uint8_t owned = 0;
	struct bind_mount *mnt;
	int migrate = -1;

	printf("TPM: %s\n", has_tpm ? "yes" : "no");
	if (has_tpm) {
		printf("TPM Owned: %s\n", tpm_owned(&owned) != TPM_SUCCESS ?
			"fail" : (owned ? "yes" : "no"));
	}
	printf("ChromeOS: %s\n", has_chromefw() ? "yes" : "no");
	printf("CR48: %s\n", is_cr48() ? "yes" : "no");
	if (has_chromefw()) {
		int rc;
		rc = get_nvram_key(system_key, &migrate);
		if (!rc)
			printf("NVRAM: missing.\n");
		else {
			printf("NVRAM: %s, %s.\n",
				migrate ? "legacy" : "modern",
				rc ? "available" : "ignored");
		}
	}
	else {
		printf("NVRAM: not present\n");
	}

	printf("rootdir: %s\n", rootdir);
	printf("stateful_mount: %s\n", stateful_mount);
	printf("key_path: %s\n", key_path);
	printf("block_path: %s\n", block_path);
	printf("encrypted_mount: %s\n", encrypted_mount);
	printf("dmcrypt_name: %s\n", dmcrypt_name);
	printf("dmcrypt_dev: %s\n", dmcrypt_dev);
	printf("bind mounts:\n");
	for (mnt = bind_mounts; mnt->src; ++mnt) {
		printf("\tsrc:%s\n", mnt->src);
		printf("\tdst:%s\n", mnt->dst);
		printf("\tprevious:%s\n", mnt->previous);
		printf("\tpending:%s\n", mnt->pending);
		printf("\towner:%s\n", mnt->owner);
		printf("\tmode:%o\n", mnt->mode);
		printf("\tsubmount:%d\n", mnt->submount);
		printf("\n");
	}

	return EXIT_SUCCESS;
}

/* This expects "mnt" to be allocated and initialized to NULL bytes. */
static int dup_bind_mount(struct bind_mount *mnt, struct bind_mount *old,
			  char *dir)
{
	if (old->src && asprintf(&mnt->src, "%s%s", dir, old->src) == -1)
		goto fail;
	if (old->dst && asprintf(&mnt->dst, "%s%s", dir, old->dst) == -1)
		goto fail;
	if (old->previous && asprintf(&mnt->previous, "%s%s", dir,
				      old->previous) == -1)
		goto fail;
	if (old->pending && asprintf(&mnt->pending, "%s%s", dir,
				     old->pending) == -1)
		goto fail;
	if (!(mnt->owner = strdup(old->owner)))
		goto fail;
	if (!(mnt->group = strdup(old->group)))
		goto fail;
	mnt->mode = old->mode;
	mnt->submount = old->submount;

	return 0;

fail:
	perror(__FUNCTION__);
	return 1;
}

static void prepare_paths(void)
{
	char *dir = NULL;
	struct bind_mount *old;
	struct bind_mount *mnt;

	mnt = bind_mounts = calloc(sizeof(bind_mounts_default) /
					sizeof(*bind_mounts_default),
				   sizeof(*bind_mounts_default));
	if (!mnt) {
		perror("calloc");
		exit(1);
	}

	if ((dir = getenv("MOUNT_ENCRYPTED_ROOT")) != NULL) {
		unsigned char digest[DIGEST_LENGTH];
		gchar *hex;

		if (asprintf(&rootdir, "%s/", dir) == -1)
			goto fail;

		/* Generate a shortened hash for non-default cryptnames,
		 * which will get re-used in the loopback name, which
		 * must be less than 64 (LO_NAME_SIZE) bytes. */
		sha256(dir, digest);
		hex = stringify_hex(digest, sizeof(digest));
		hex[17] = '\0';
		if (asprintf(&dmcrypt_name, "%s_%s", kCryptDevName,
				hex) == -1)
			goto fail;
		g_free(hex);
	} else {
		rootdir = "/";
		if (!(dmcrypt_name = strdup(kCryptDevName)))
			goto fail;
	}

	if (asprintf(&stateful_mount, "%s%s", rootdir, STATEFUL_MNT) == -1)
		goto fail;
	if (asprintf(&key_path, "%s%s", rootdir,
		     STATEFUL_MNT "/encrypted.key") == -1)
		goto fail;
	if (asprintf(&needs_finalization_path, "%s%s", rootdir,
		     STATEFUL_MNT "/encrypted.needs-finalization") == -1)
		goto fail;
	if (asprintf(&block_path, "%s%s", rootdir,
		     STATEFUL_MNT "/encrypted.block") == -1)
		goto fail;
	if (asprintf(&encrypted_mount, "%s%s", rootdir, ENCRYPTED_MNT) == -1)
		goto fail;
	if (asprintf(&dmcrypt_dev, "/dev/mapper/%s", dmcrypt_name) == -1)
		goto fail;

	for (old = bind_mounts_default; old->src; ++old) {
		if (dup_bind_mount(mnt++, old, rootdir))
			exit(1);
	}

	return;

fail:
	perror("asprintf");
	exit(1);
}

int main(int argc, char *argv[])
{
	int okay;
	int mode = kModeProduction;

	INFO_INIT("Starting.");
	prepare_paths();
	tpm_init();

	if (argc > 1) {
		if (!strcmp(argv[1], "umount"))
			return shutdown();
		else if (!strcmp(argv[1], "info"))
			return report_info();
		else if (!strcmp(argv[1], "finalize"))
			return finalize_from_cmdline(argc > 2 ? argv[2] : NULL);
		else if (!strcmp(argv[1], "factory"))
			mode = kModeFactory;
		else {
			fprintf(stderr,
				"Usage: %s [info|finalize|umount|factory]\n",
				argv[0]);
			return 1;
		}
	}

	check_mount_states();

	okay = setup_encrypted(mode);
	/* If we fail, let chromeos_startup handle the stateful wipe. */

	INFO_DONE("Done.");

	/* Continue boot. */
	return okay ? EXIT_SUCCESS : EXIT_FAILURE;
}
