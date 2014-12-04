/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for host library vboot2 key functions
 */

#include <stdio.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_key2.h"

#include "test_common.h"

/* Test only the algorithms we use */
struct alg_combo {
	const char *name;
	enum vb2_signature_algorithm sig_alg;
	enum vb2_hash_algorithm hash_alg;
};

static const struct alg_combo test_algs[] = {
	{"RSA2048/SHA-256", VB2_SIG_RSA2048, VB2_HASH_SHA256},
	{"RSA4096/SHA-256", VB2_SIG_RSA4096, VB2_HASH_SHA256},
	{"RSA8192/SHA-512", VB2_SIG_RSA8192, VB2_HASH_SHA512},
};

static void private_key_tests(const struct alg_combo *combo,
			      const char *pemfile)
{
	struct vb2_private_key *key, *k2;
	const struct vb2_private_key *ckey;
	struct vb2_packed_private_key *pkey;
	const char *testfile = "test.vbprik2";
	const char *notapem = "not_a_pem";
	const char *testdesc = "test desc";
	const struct vb2_guid test_guid = {.raw = {0xaa}};
	uint8_t *buf, *buf2;
	uint32_t bufsize;

	TEST_SUCC(vb2_private_key_read_pem(&key, pemfile), "Read pem - good");
	TEST_PTR_NEQ(key, NULL, "  key_ptr");
	TEST_PTR_NEQ(key->rsa_private_key, NULL, "  rsa_private_key");
	TEST_PTR_EQ(key->desc, NULL, "  desc");
	vb2_private_key_free(key);

	TEST_EQ(vb2_private_key_read_pem(&key, "no_such_key"),
		VB2_ERROR_READ_PEM_FILE_OPEN, "Read pem - no key");
	TEST_PTR_EQ(key, NULL, "  key_ptr");

	vb2_write_file(testfile, (const uint8_t *)notapem, sizeof(notapem));
	TEST_EQ(vb2_private_key_read_pem(&key, testfile),
		VB2_ERROR_READ_PEM_RSA, "Read pem - not a pem");
	unlink(testfile);

	TEST_SUCC(vb2_private_key_read_pem(&key, pemfile), "Read pem - good2");
	TEST_SUCC(vb2_private_key_set_desc(key, testdesc), "Set desc");
	TEST_PTR_NEQ(key->desc, NULL, "  desc");
	TEST_PTR_NEQ(key->desc, testdesc, "  made a copy");
	TEST_EQ(strcmp(key->desc, testdesc), 0, "  right contents");
	TEST_SUCC(vb2_private_key_set_desc(key, NULL), "Clear desc");
	TEST_PTR_EQ(key->desc, NULL, "  desc");
	TEST_SUCC(vb2_private_key_set_desc(key, testdesc), "Set desc");
	vb2_private_key_free(key);

	TEST_SUCC(vb2_private_key_read_pem(&key, pemfile), "Read pem - good3");
	TEST_SUCC(vb2_private_key_set_desc(key, testdesc), "Set desc");
	key->hash_alg = combo->hash_alg;
	key->sig_alg = combo->sig_alg;
	key->guid = test_guid;

	unlink(testfile);

	TEST_EQ(vb2_private_key_read(&k2, testfile),
		VB2_ERROR_READ_FILE_OPEN, "Read key no file");
	TEST_EQ(vb2_private_key_write(key, "no/such/dir"),
		VB2_ERROR_PRIVATE_KEY_WRITE_FILE, "Write key to bad path");

	TEST_SUCC(vb2_private_key_write(key, testfile), "Write key good");
	TEST_SUCC(vb2_private_key_read(&k2, testfile), "Read key good");
	TEST_PTR_NEQ(k2, NULL, "  key_ptr");
	TEST_EQ(k2->sig_alg, key->sig_alg, "  sig alg");
	TEST_EQ(k2->hash_alg, key->hash_alg, "  hash alg");
	TEST_EQ(memcmp(&k2->guid, &key->guid, sizeof(k2->guid)), 0, "  guid");
	TEST_EQ(strcmp(k2->desc, testdesc), 0, "  desc");
	vb2_private_key_free(k2);

	TEST_SUCC(vb2_read_file(testfile, &buf, &bufsize), "Read key raw");
	pkey = (struct vb2_packed_private_key *)buf;

	/* Make a backup of the good buffer so we can mangle it */
	buf2 = malloc(bufsize);
	memcpy(buf2, buf, bufsize);

	TEST_SUCC(vb2_private_key_unpack(&k2, buf, bufsize),
		  "Unpack private key good");
	vb2_private_key_free(k2);

	memcpy(buf, buf2, bufsize);
	pkey->c.magic = VB2_MAGIC_PACKED_KEY;
	TEST_EQ(vb2_private_key_unpack(&k2, buf, bufsize),
		VB2_ERROR_UNPACK_PRIVATE_KEY_MAGIC,
		"Unpack private key bad magic");
	TEST_PTR_EQ(k2, NULL, "  key_ptr");

	memcpy(buf, buf2, bufsize);
	pkey->c.desc_size++;
	TEST_EQ(vb2_private_key_unpack(&k2, buf, bufsize),
		VB2_ERROR_UNPACK_PRIVATE_KEY_HEADER,
		"Unpack private key bad header");

	memcpy(buf, buf2, bufsize);
	pkey->key_size += pkey->c.total_size;
	TEST_EQ(vb2_private_key_unpack(&k2, buf, bufsize),
		VB2_ERROR_UNPACK_PRIVATE_KEY_DATA,
		"Unpack private key bad data size");

	memcpy(buf, buf2, bufsize);
	pkey->c.struct_version_major++;
	TEST_EQ(vb2_private_key_unpack(&k2, buf, bufsize),
		VB2_ERROR_UNPACK_PRIVATE_KEY_STRUCT_VERSION,
		"Unpack private key bad struct version");

	memcpy(buf, buf2, bufsize);
	pkey->c.struct_version_minor++;
	TEST_SUCC(vb2_private_key_unpack(&k2, buf, bufsize),
		  "Unpack private key minor version");
	vb2_private_key_free(k2);

	memcpy(buf, buf2, bufsize);
	pkey->key_size -= 32;
	TEST_EQ(vb2_private_key_unpack(&k2, buf, bufsize),
		VB2_ERROR_UNPACK_PRIVATE_KEY_RSA,
		"Unpack private key bad rsa data");

	memcpy(buf, buf2, bufsize);
	pkey->sig_alg = VB2_SIG_NONE;
	TEST_EQ(vb2_private_key_unpack(&k2, buf, bufsize),
		VB2_ERROR_UNPACK_PRIVATE_KEY_HASH,
		"Unpack private key hash but has data");

	free(buf);
	free(buf2);
	unlink(testfile);

	TEST_EQ(vb2_private_key_hash(&ckey, VB2_HASH_INVALID),
		VB2_ERROR_PRIVATE_KEY_HASH,
		"Hash key invalid");
	TEST_PTR_EQ(ckey, NULL, "  key_ptr");

	TEST_SUCC(vb2_private_key_hash(&ckey, combo->hash_alg), "Hash key");
	TEST_PTR_NEQ(ckey, NULL, "  key_ptr");
	TEST_EQ(ckey->hash_alg, combo->hash_alg, "  hash_alg");
	TEST_EQ(ckey->sig_alg, VB2_SIG_NONE, "  sig_alg");
	TEST_EQ(memcmp(&ckey->guid, vb2_hash_guid(combo->hash_alg),
		       sizeof(ckey->guid)), 0, "  guid");

	TEST_SUCC(vb2_private_key_write(ckey, testfile), "Write hash key");
	TEST_SUCC(vb2_private_key_read(&key, testfile), "Read hash key");
	unlink(testfile);
}

static void public_key_tests(const struct alg_combo *combo,
			     const char *keybfile)
{
	struct vb2_public_key *key, k2;
	struct vb2_packed_key *pkey;
	const char *testfile = "test.vbpubk2";
	const char *testdesc = "test desc";
	const struct vb2_guid test_guid = {.raw = {0xbb}};
	const uint32_t test_version = 0xcc01;
	uint8_t *buf;
	uint32_t bufsize;

	TEST_EQ(vb2_public_key_read_keyb(&key, "no_such_key"),
		VB2_ERROR_READ_KEYB_DATA, "Read keyb - no file");
	TEST_PTR_EQ(key, NULL, "  key_ptr");

	TEST_SUCC(vb2_public_key_read_keyb(&key, keybfile), "Read keyb - good");
	TEST_PTR_NEQ(key, NULL, "  key_ptr");
	TEST_EQ(key->sig_alg, combo->sig_alg, "  sig_alg");
	TEST_PTR_EQ(key->desc, NULL, "  desc");
	vb2_public_key_free(key);

	bufsize = vb2_packed_key_size(combo->sig_alg);
	buf = calloc(1, bufsize);

	vb2_write_file(testfile, buf, bufsize - 1);
	TEST_EQ(vb2_public_key_read_keyb(&key, testfile),
		VB2_ERROR_READ_KEYB_SIZE, "Read keyb - bad size");
	unlink(testfile);

	vb2_write_file(testfile, buf, bufsize);
	free(buf);
	TEST_EQ(vb2_public_key_read_keyb(&key, testfile),
		VB2_ERROR_READ_KEYB_UNPACK, "Read keyb - unpack");
	unlink(testfile);

	TEST_SUCC(vb2_public_key_read_keyb(&key, keybfile), "Read keyb 2");
	TEST_SUCC(vb2_public_key_set_desc(key, testdesc), "Set desc");
	TEST_PTR_NEQ(key->desc, NULL, "  desc");
	TEST_PTR_NEQ(key->desc, testdesc, "  made a copy");
	TEST_EQ(strcmp(key->desc, testdesc), 0, "  right contents");
	TEST_SUCC(vb2_public_key_set_desc(key, NULL), "Clear desc");
	TEST_PTR_EQ(key->desc, NULL, "  desc");
	TEST_SUCC(vb2_public_key_set_desc(key, testdesc), "Set desc");
	vb2_public_key_free(key);

	TEST_SUCC(vb2_public_key_read_keyb(&key, keybfile), "Read keyb 3");
	TEST_SUCC(vb2_public_key_set_desc(key, testdesc), "Set desc");
	key->hash_alg = combo->hash_alg;
	key->guid = &test_guid;
	key->version = test_version;

	TEST_SUCC(vb2_public_key_pack(&pkey, key), "Pack public key");
	TEST_PTR_NEQ(pkey, NULL, "  key_ptr");
	TEST_EQ(pkey->hash_alg, key->hash_alg, "  hash_alg");
	TEST_EQ(pkey->sig_alg, key->sig_alg, "  sig_alg");
	TEST_EQ(pkey->key_version, key->version, "  version");
	TEST_EQ(memcmp(&pkey->guid, key->guid, sizeof(pkey->guid)), 0,
		"  guid");
	TEST_EQ(strcmp(vb2_common_desc(pkey), key->desc), 0, "  desc");
	TEST_SUCC(vb2_unpack_key(&k2, (uint8_t *)pkey, pkey->c.total_size),
		  "Unpack public key");
	TEST_EQ(key->arrsize, k2.arrsize, "  arrsize");
	TEST_EQ(key->n0inv, k2.n0inv, "  n0inv");
	TEST_EQ(memcmp(key->n, k2.n, key->arrsize * sizeof(uint32_t)), 0,
		"  n");
	TEST_EQ(memcmp(key->rr, k2.rr, key->arrsize * sizeof(uint32_t)), 0,
		"  rr");

	TEST_SUCC(vb2_write_object(testfile, pkey), "Write packed key");
	free(pkey);

	TEST_SUCC(vb2_packed_key_read(&pkey, testfile), "Read packed key");
	TEST_PTR_NEQ(pkey, NULL, "  key_ptr");
	unlink(testfile);

	pkey->hash_alg = VB2_HASH_INVALID;
	TEST_SUCC(vb2_write_object(testfile, pkey), "Write bad packed key");
	free(pkey);

	TEST_EQ(vb2_packed_key_read(&pkey, testfile),
		VB2_ERROR_READ_PACKED_KEY, "Read bad packed key");
	TEST_PTR_EQ(pkey, NULL, "  key_ptr");
	unlink(testfile);

	TEST_EQ(vb2_packed_key_read(&pkey, testfile),
		VB2_ERROR_READ_PACKED_KEY_DATA, "Read missing packed key");

	key->sig_alg = VB2_SIG_INVALID;
	TEST_EQ(vb2_public_key_pack(&pkey, key),
		VB2_ERROR_PUBLIC_KEY_PACK_SIZE,
		"Pack invalid sig alg");
	vb2_public_key_free(key);

	TEST_EQ(vb2_public_key_hash(&k2, VB2_HASH_INVALID),
		VB2_ERROR_PUBLIC_KEY_HASH,
		"Hash key invalid");

	TEST_SUCC(vb2_public_key_hash(&k2, combo->hash_alg), "Hash key");
	TEST_EQ(k2.hash_alg, combo->hash_alg, "  hash_alg");
	TEST_EQ(k2.sig_alg, VB2_SIG_NONE, "  sig_alg");
	TEST_EQ(memcmp(k2.guid, vb2_hash_guid(combo->hash_alg),
		       sizeof(*k2.guid)), 0, "  guid");

	TEST_SUCC(vb2_public_key_pack(&pkey, &k2), "Pack public hash key");
	TEST_PTR_NEQ(pkey, NULL, "  key_ptr");
	TEST_SUCC(vb2_unpack_key(&k2, (uint8_t *)pkey, pkey->c.total_size),
		  "Unpack public hash key");
	free(pkey);
}

static int test_algorithm(const struct alg_combo *combo, const char *keys_dir)
{
	int rsa_bits = vb2_rsa_sig_size(combo->sig_alg) * 8;
	char pemfile[1024];
	char keybfile[1024];

	printf("***Testing algorithm: %s\n", combo->name);

	sprintf(pemfile, "%s/key_rsa%d.pem", keys_dir, rsa_bits);
	sprintf(keybfile, "%s/key_rsa%d.keyb", keys_dir, rsa_bits);

	private_key_tests(combo, pemfile);
	public_key_tests(combo, keybfile);

	return 0;
}

int main(int argc, char *argv[]) {

	if (argc == 2) {
		int i;

		for (i = 0; i < ARRAY_SIZE(test_algs); i++) {
			if (test_algorithm(test_algs + i, argv[1]))
				return 1;
		}
	} else {
		fprintf(stderr, "Usage: %s <keys_dir>", argv[0]);
		return -1;
	}

	return gTestSuccess ? 0 : 255;
}
