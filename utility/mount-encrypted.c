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

#define STATEFUL_MNT "/mnt/stateful_partition"
#define ENCRYPTED_MNT STATEFUL_MNT "/encrypted"
#define DMCRYPT_DEV_NAME "encstateful"
#define BUF_SIZE 1024
#define PROP_SIZE 64

static const gchar * const kRootDir = "/";
static const gchar * const kKernelCmdline = "/proc/cmdline";
static const gchar * const kKernelCmdlineOption = " encrypted-stateful-key=";
static const gchar * const kStatefulMount = STATEFUL_MNT;
static const gchar * const kEncryptedKey = STATEFUL_MNT "/encrypted.key";
static const gchar * const kEncryptedBlock = STATEFUL_MNT "/encrypted.block";
static const gchar * const kEncryptedMount = ENCRYPTED_MNT;
static const gchar * const kEncryptedFSType = "ext4";
static const gchar * const kCryptName = DMCRYPT_DEV_NAME;
static const gchar * const kCryptDev = "/dev/mapper/" DMCRYPT_DEV_NAME;
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

static struct bind_mount {
	const char * const src;
	const char * const old;
	const char * const dst;
	const char * const owner;
	const char * const group;
	const mode_t mode;
	const int submount;		/* Submount is bound already. */
	const int optional;		/* Non-fatal if this bind fails. */
} bind_mounts[] = {
#if DEBUG_ENABLED == 2
# define DEBUG_DEST ".new"
#else
# define DEBUG_DEST ""
#endif
	{ ENCRYPTED_MNT "/var", STATEFUL_MNT "/var",
	  "/var" DEBUG_DEST, "root", "root",
	  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH, 0, 0 },
	{ ENCRYPTED_MNT "/chronos", STATEFUL_MNT "/home/chronos",
	  "/home/chronos" DEBUG_DEST, "chronos", "chronos",
	  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH, 1, 1 },
	{ },
};

int has_tpm = 0;

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

static int has_chromefw(void)
{
	static int state = -1;
	char fw[PROP_SIZE];

	/* Cache the state so we don't have to perform the query again. */
	if (state != -1)
		return state;

	if (!VbGetSystemPropertyString("mainfw_type", fw, sizeof(fw)))
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

	if (!VbGetSystemPropertyString("hwid", hwid, sizeof(hwid)))
		state = 0;
	else
		state = strstr(hwid, "MARIO") != NULL;
	return state;
}

static int
_read_nvram(uint8_t *buffer, size_t len, uint32_t index, uint32_t size)
{
	if (size > len) {
		ERROR("NVRAM size (0x%x > 0x%zx) is too big", size, len);
		return 0;
	}

	return TlclRead(index, buffer, size);
}

/*
 * Cases:
 *  - no NVRAM area at all (OOBE)
 *  - defined NVRAM area, but TPM not Owned
 *  - defined NVRAM area, but not Finalized
 *  - legacy NVRAM area (migration needed)
 *  - modern NVRAM area (\o/)
 */
// TODO(keescook): recovery code needs to wipe NVRAM area to new size?
static int get_nvram_key(uint8_t *digest, int *old_lockbox)
{
	TPM_PERMANENT_FLAGS pflags;
	uint8_t value[kLockboxSizeV2], bytes_anded, bytes_ored;
	uint32_t size, result, i;
	uint8_t *rand_bytes;
	uint32_t rand_size;

	/* Start by expecting modern NVRAM area. */
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
	result = TlclGetPermanentFlags(&pflags);
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

static int check_bind_src(struct bind_mount *bind)
{
	struct passwd *user;
	struct group *group;

	if (!(user = getpwnam(bind->owner))) {
		PERROR("getpwnam(%s)", bind->owner);
		return -1;
	}
	if (!(group = getgrnam(bind->group))) {
		PERROR("getgrnam(%s)", bind->group);
		return -1;
	}

	if (access(bind->src, R_OK) && mkdir(bind->src, bind->mode)) {
		PERROR("mkdir(%s)", bind->src);
		return -1;
	}
	/* Must do explicit chmod since mkdir()'s mode respects umask. */
	if (chmod(bind->src, bind->mode)) {
		PERROR("chmod(%s)", bind->src);
		return -1;
	}
	if (chown(bind->src, user->pw_uid, group->gr_gid)) {
		PERROR("chown(%s)", bind->src);
		return -1;
	}

	return 0;
}

static void migrate_contents(struct bind_mount *bind)
{
	gchar *old;

	/* Skip migration if the old bind src is missing. */
	if (!bind->old || access(bind->old, R_OK))
		return;

	INFO("Migrating bind mount contents %s to %s.", bind->old, bind->src);
	check_bind_src(bind);

	if (!(old = g_strdup_printf("%s/.", bind->old))) {
		PERROR("g_strdup_printf");
		goto remove;
	}

	const gchar *cp[] = {
		"/bin/cp", "-a",
		old,
		bind->src,
		NULL
	};

	if (runcmd(cp, NULL) != 0) {
		/* If the copy failed, it may have partially populated the
		 * new source, so we need to remove the new source and
		 * rebuild it. Regardless, the old source must be removed
		 * as well.
		 */
		INFO("Failed to migrate %s to %s!", bind->old, bind->src);
		remove_tree(bind->src);
		check_bind_src(bind);
	}

remove:
	g_free(old);

	/* The removal of the old directory needs to happen at finalize
	 * time, otherwise /var state gets lost on a migration if the
	 * system is powered off before the encryption key is saved.
	 */
	return;
}

static void finalize(uint8_t *system_key, char *encryption_key)
{
	struct bind_mount *bind;

	INFO("Writing keyfile %s.", kEncryptedKey);
	if (!keyfile_write(kEncryptedKey, system_key, encryption_key)) {
		ERROR("Failed to write %s -- aborting.", kEncryptedKey);
		return;
	}

	for (bind = bind_mounts; bind->src; ++ bind) {
		if (access(bind->old, R_OK))
			continue;
		INFO("Removing %s.", bind->old);
#if DEBUG_ENABLED
		continue;
#endif
		remove_tree(bind->old);
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

	encryption_key = dm_get_key(kCryptDev);
	if (!encryption_key) {
		ERROR("Could not locate encryption key for %s.", kCryptDev);
		return EXIT_FAILURE;
	}

	finalize(system_key, encryption_key);

	return EXIT_SUCCESS;
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
		encryption_key = keyfile_read(kEncryptedKey, system_key);
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
		unlink(kEncryptedKey);
		unlink(kEncryptedBlock);

		/* Calculate the desired size of the new partition. */
		if (statvfs(kStatefulMount, &buf)) {
			PERROR(kStatefulMount);
			return 0;
		}
		size = buf.f_blocks;
		size *= kSizePercent;
		size *= buf.f_frsize;

		INFO("Creating sparse backing file with size %llu.",
		     (unsigned long long)size);

		/* Create the sparse file. */
		sparsefd = sparse_create(kEncryptedBlock, size);
		if (sparsefd < 0) {
			PERROR(kEncryptedBlock);
			return 0;
		}
	} else {
		sparsefd = open(kEncryptedBlock, O_RDWR | O_NOFOLLOW);
		if (sparsefd < 0) {
			PERROR(kEncryptedBlock);
			return 0;
		}
	}

	/* Set up loopback device. */
	INFO("Loopback attaching %s.", kEncryptedBlock);
	lodev = loop_attach(sparsefd, kEncryptedBlock);
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
	INFO("Setting up dm-crypt %s as %s.", lodev, kCryptDev);
	if (!dm_setup(sectors, encryption_key, kCryptName, lodev,
		      kCryptDev)) {
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
			/* Skip mounts that have no prior location defined. */
			if (!bind->old)
				continue;
			/* Skip mounts that have no prior data on disk. */
			if (access(bind->old, R_OK) != 0)
				continue;

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
			kCryptDev, kExt4BlockSize, blocks_min, blocks_max);
		if (!filesystem_build(kCryptDev, kExt4BlockSize,
					blocks_min, blocks_max))
			goto dm_cleanup;
	}

	/* Mount the dm-crypt partition finally. */
	INFO("Mounting %s onto %s.", kCryptDev, kEncryptedMount);
	if (access(kEncryptedMount, R_OK) &&
	    mkdir(kEncryptedMount, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
		PERROR(kCryptDev);
		goto dm_cleanup;
	}
	if (mount(kCryptDev, kEncryptedMount, kEncryptedFSType,
		  MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_RELATIME,
		  "discard")) {
		PERROR("mount(%s,%s)", kCryptDev, kEncryptedMount);
		goto dm_cleanup;
	}

	/* Always spawn filesystem resizer, in case growth was interrupted. */
	/* TODO(keescook): if already full size, don't resize. */
	filesystem_resizer(kCryptDev, blocks_min, blocks_max);

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
			migrate_contents(bind);
	}

	/* Perform bind mounts. */
	for (bind = bind_mounts; bind->src; ++ bind) {
		INFO("Bind mounting %s onto %s.", bind->src, bind->dst);
		if (check_bind_src(bind) ||
		    mount(bind->src, bind->dst, "none", MS_BIND, NULL)) {
			PERROR("mount(%s,%s)", bind->src, bind->dst);
			if (bind->optional)
				continue;
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

	INFO("Unmounting %s.", kEncryptedMount);
	umount(kEncryptedMount);

dm_cleanup:
	INFO("Removing %s.", kCryptDev);
	/* TODO(keescook): something holds this open briefly on mkfs failure
	 * and I haven't been able to catch it yet. Adding an "fuser" call
	 * here is sufficient to lose the race. Instead, just sleep during
	 * the error path.
	 */
	sleep(1);
	dm_teardown(kCryptDev);

lo_cleanup:
	INFO("Unlooping %s.", lodev);
	loop_detach(lodev);

failed:
	free(lodev);

	return 0;
}


static void check_mount_states(void)
{
	struct bind_mount *bind;

	/* Verify stateful partition exists and is mounted. */
	if (access(kStatefulMount, R_OK) ||
	    same_vfs(kStatefulMount, kRootDir)) {
		INFO("%s is not mounted.", kStatefulMount);
		exit(1);
	}

	/* Verify encrypted partition is missing or not already mounted. */
	if (access(kEncryptedMount, R_OK) == 0 &&
	    !same_vfs(kEncryptedMount, kStatefulMount)) {
		INFO("%s already appears to be mounted.", kEncryptedMount);
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

		if (same_vfs(bind->dst, kStatefulMount)) {
			INFO("%s already bind mounted.", bind->dst);
			exit(1);
		}
	}

	INFO("VFS mount state sanity check ok.");
}

int device_details(void)
{
	uint8_t system_key[DIGEST_LENGTH];
	TPM_PERMANENT_FLAGS pflags;
	int old_lockbox = -1;

	printf("TPM: %s\n", has_tpm ? "yes" : "no");
	if (has_tpm) {
		printf("TPM Owned: %s\n", TlclGetPermanentFlags(&pflags) ?
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

	return EXIT_SUCCESS;
}

void init_tpm(void)
{
	int tpm;

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
}

int main(int argc, char *argv[])
{
	int okay;

	INFO_INIT("Starting.");
	init_tpm();

	if (argc > 1) {
		if (!strcmp(argv[1], "device"))
			return device_details();
		if (!strcmp(argv[1], "finalize"))
			return finalize_from_cmdline(argc > 2 ? argv[2] : NULL);

		fprintf(stderr, "Usage: %s [device|finalize]\n",
			argv[0]);
		return 1;
	}

	check_mount_states();

	okay = setup_encrypted();
	if (!okay) {
		INFO("Setup failed -- clearing files and retrying.");
		unlink(kEncryptedKey);
		unlink(kEncryptedBlock);
		okay = setup_encrypted();
	}

	INFO("Done.");

	/* Continue boot. */
	return !okay;
}
