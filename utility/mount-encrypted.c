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
static const uint32_t kLockboxIndex = 0x20000004;
static const uint32_t kLockboxSizeV1 = 0x2c;
static const uint32_t kLockboxSizeV2 = 0x45;
static const uint32_t kLockboxSaltOffset = 0x5;
static const size_t kSectorSize = 512;
static const size_t kExt4BlockSize = 4096;
static const size_t kExt4MinBytes = 64 * 1024 * 1024;

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

static struct bind_mount *bind_mounts = NULL;
static gchar *rootdir = NULL;
static gchar *stateful_mount = NULL;
static gchar *key_path = NULL;
static gchar *block_path = NULL;
static gchar *encrypted_mount = NULL;
static gchar *dmcrypt_name = NULL;
static gchar *dmcrypt_dev = NULL;
static int has_tpm = 0;

void tpm_init(void)
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

uint32_t tpm_flags(TPM_PERMANENT_FLAGS *pflags)
{
	uint32_t result;

	DEBUG("Reading TPM Permanent Flags");
	result = TlclGetPermanentFlags(pflags);
	DEBUG("TPM Permanent Flags returned: %s", result ? "FAIL" : "ok");

	return result;
}

void tpm_close(void)
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

static int
_read_nvram(uint8_t *buffer, size_t len, uint32_t index, uint32_t size)
{
	int rc;

	if (size > len) {
		ERROR("NVRAM size (0x%x > 0x%zx) is too big", size, len);
		return 0;
	}

	DEBUG("Reading NVRAM area 0x%x (size %u)", index, size);
	rc = TlclRead(index, buffer, size);
	DEBUG("NVRAM read returned: %s", rc ? "FAIL" : "ok");

	return rc;
}

/*
 * Cases:
 *  - no NVRAM area at all (OOBE)
 *  - defined NVRAM area, but TPM not Owned
 *  - defined NVRAM area, but not Finalized
 *  - legacy NVRAM area (migration needed)
 *  - modern NVRAM area (\o/)
 */
static int get_nvram_key(uint8_t *digest, int *old_lockbox)
{
	TPM_PERMANENT_FLAGS pflags;
	uint8_t value[kLockboxSizeV2], bytes_anded, bytes_ored;
	uint32_t size, result, i;
	uint8_t *rand_bytes;
	uint32_t rand_size;

	/* Reading the NVRAM takes 40ms. Instead of querying the NVRAM area
	 * for its size (which takes time), just read the expected size. If
	 * it fails, then fall back to the older size. This means cleared
	 * devices take 80ms (2 failed reads), legacy devices take 80ms
	 * (1 failed read, 1 good read), and populated devices take 40ms,
	 * which is the minimum possible time (instead of 40ms + time to
	 * query NVRAM size).
	 */
	*old_lockbox = 0;
	size = kLockboxSizeV2;
	result = _read_nvram(value, sizeof(value), kLockboxIndex, size);
	if (result) {
		size = kLockboxSizeV1;
		result = _read_nvram(value, sizeof(value), kLockboxIndex, size);
		if (result) {
			/* No NVRAM area at all. */
			INFO("No NVRAM area defined.");
			return 0;
		}
		/* Legacy NVRAM area. */
		INFO("Legacy NVRAM area found.");
		*old_lockbox = 1;
	} else {
		INFO("NVRAM area found.");
	}

	debug_dump_hex("nvram", value, size);

	/* Ignore defined but unowned NVRAM area. */
	/* TODO(keescook): remove this check (it adds 40ms) once the
	 * NVRAM area is bound to owner so that it will be wiped out
	 * across device mode changes.
	 */
	result = tpm_flags(&pflags);
	if (result) {
		INFO("Could not read TPM Permanent Flags.");
		return 0;
	}
	if (!pflags.ownership) {
		INFO("TPM not Owned, ignoring NVRAM area.");
		return 0;
	}

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
	if (*old_lockbox) {
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
 * ChromeOS devices are required to use the NVRAM area (excepting CR-48s),
 * all the rest will fallback through various places (kernel command line,
 * BIOS UUID, and finally a static value) for a system key.
 */
static int find_system_key(uint8_t *digest, int *migration_allowed)
{
	gchar *key;
	gsize length;

	/* By default, do not allow migration. */
	*migration_allowed = 0;
	if (has_chromefw()) {
		int rc;
		rc = get_nvram_key(digest, migration_allowed);

		/* Since the CR-48 did not ship with a lockbox area, they
		 * are allowed to fall back to non-NVRAM system keys.
		 */
		if (rc || !is_cr48()) {
			INFO("Using NVRAM as system key; %s.",
				rc ? "already populated"
				   : "needs population");
			return rc;
		}
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
	sha256("default unsafe static key", digest);
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
		if (result || size > remaining) {
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

static void finalize(uint8_t *system_key, char *encryption_key)
{
	struct bind_mount *bind;

	INFO("Writing keyfile %s.", key_path);
	if (!keyfile_write(key_path, system_key, encryption_key)) {
		ERROR("Failed to write %s -- aborting.", key_path);
		return;
	}

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
		if (!find_system_key(system_key, &migrate)) {
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

void spawn_resizer(const char *device, size_t blocks, size_t blocks_max)
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

static int setup_encrypted(void)
{
	int has_system_key;
	uint8_t system_key[DIGEST_LENGTH];
	char *encryption_key = NULL;
	int migrate_allowed = 0, migrate_needed = 0, rebuild = 0;
	gchar *lodev = NULL;
	size_t sectors;
	struct bind_mount *bind;
	int sparsefd;
	size_t blocks_min, blocks_max;

	/* Use the "system key" to decrypt the "encryption key" stored in
	 * the stateful partition.
	 */
	has_system_key = find_system_key(system_key, &migrate_allowed);
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
		INFO("Generating new encryption key.");
		encryption_key = choose_encryption_key();
		if (!encryption_key)
			return 0;
		rebuild = 1;
	}

	if (rebuild) {
		struct statvfs buf;
		off_t size;

		/* Wipe out the old files, and ignore errors. */
		unlink(key_path);
		unlink(block_path);

		/* Calculate the desired size of the new partition. */
		if (statvfs(stateful_mount, &buf)) {
			PERROR(stateful_mount);
			return 0;
		}
		size = buf.f_blocks;
		size *= kSizePercent;
		size *= buf.f_frsize;

		INFO("Creating sparse backing file with size %llu.",
		     (unsigned long long)size);

		/* Create the sparse file. */
		sparsefd = sparse_create(block_path, size);
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
		      dmcrypt_dev)) {
		ERROR("dm_setup failed");
		goto lo_cleanup;
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
	blocks_min = migrate_needed ? blocks_max :
			kExt4MinBytes / kExt4BlockSize;
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

	/* Devices that are not using NVRAM for their system key do not
	 * need to wait for the NVRAM area to be populated by Cryptohome
	 * and a call to "finalize". Devices that already have the NVRAM
	 * area populated and are being rebuilt don't need to wait for
	 * Cryptohome because the NVRAM area isn't going to change.
	 */
	if (rebuild && has_system_key)
		finalize(system_key, encryption_key);

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

	/* Verify stateful partition exists and is mounted. */
	if (access(stateful_mount, R_OK) ||
	    same_vfs(stateful_mount, rootdir)) {
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

int report_info(void)
{
	uint8_t system_key[DIGEST_LENGTH];
	TPM_PERMANENT_FLAGS pflags;
	struct bind_mount *mnt;
	int old_lockbox = -1;

	printf("TPM: %s\n", has_tpm ? "yes" : "no");
	if (has_tpm) {
		printf("TPM Owned: %s\n", tpm_flags(&pflags) ?
			"fail" : (pflags.ownership ? "yes" : "no"));
	}
	printf("ChromeOS: %s\n", has_chromefw() ? "yes" : "no");
	printf("CR48: %s\n", is_cr48() ? "yes" : "no");
	if (has_chromefw()) {
		int rc;
		rc = get_nvram_key(system_key, &old_lockbox);
		if (!rc)
			printf("NVRAM: missing\n");
		else {
			printf("NVRAM: %s, %s\n",
				old_lockbox ? "legacy" : "modern",
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

	INFO_INIT("Starting.");
	prepare_paths();
	tpm_init();

	if (argc > 1) {
		if (!strcmp(argv[1], "umount"))
			return shutdown();
		if (!strcmp(argv[1], "info"))
			return report_info();
		if (!strcmp(argv[1], "finalize"))
			return finalize_from_cmdline(argc > 2 ? argv[2] : NULL);

		fprintf(stderr, "Usage: %s [info|finalize|umount]\n",
			argv[0]);
		return 1;
	}

	check_mount_states();

	okay = setup_encrypted();
	if (!okay) {
		INFO("Setup failed -- clearing files and retrying.");
		unlink(key_path);
		unlink(block_path);
		okay = setup_encrypted();
	}

	INFO_DONE("Done.");

	/* Continue boot. */
	return !okay;
}
