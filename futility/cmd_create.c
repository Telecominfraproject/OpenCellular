/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include <openssl/pem.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2id.h"
#include "2rsa.h"
#include "util_misc.h"
#include "vb2_common.h"
#include "vb2_struct.h"

#include "host_key.h"
#include "host_key2.h"
#include "host_misc2.h"

#include "futility.h"
#include "futility_options.h"

/* Command line options */
enum {
	OPT_OUTFILE = 1000,
	OPT_VERSION,
	OPT_DESC,
	OPT_ID,
	OPT_HASH_ALG,
	OPT_HELP,
};

#define DEFAULT_VERSION 1
#define DEFAULT_HASH VB2_HASH_SHA256;

static char *infile, *outfile, *outext;
static uint32_t opt_version = DEFAULT_VERSION;
enum vb2_hash_algorithm opt_hash_alg = DEFAULT_HASH;
static char *opt_desc;
static struct vb2_id opt_id;
static int force_id;

static const struct option long_opts[] = {
	{"version",  1, 0, OPT_VERSION},
	{"desc",     1, 0, OPT_DESC},
	{"id",       1, 0, OPT_ID},
	{"hash_alg", 1, 0, OPT_HASH_ALG},
	{"help",     0, 0, OPT_HELP},
	{NULL, 0, 0, 0}
};

static void print_help(int argc, char *argv[])
{
	struct vb2_text_vs_enum *entry;

	printf("\n"
"Usage:  " MYNAME " %s [options] <INFILE> [<BASENAME>]\n", argv[0]);
	printf("\n"
"Create a keypair from an RSA key (.pem file).\n"
"\n"
"Options:\n"
"\n"
"  --version <number>          Key version (default %d)\n"
"  --hash_alg <number>         Hashing algorithm to use:\n",
		DEFAULT_VERSION);
	for (entry = vb2_text_vs_hash; entry->name; entry++)
		printf("                                %d / %s%s\n",
		       entry->num, entry->name,
		       entry->num == VB2_HASH_SHA256 ? " (default)" : "");
	printf(
"  --id <id>                   Identifier for this keypair (vb21 only)\n"
"  --desc <text>               Human-readable description (vb21 only)\n"
"\n");

}

static int vb1_make_keypair()
{
	VbPrivateKey *privkey = 0;
	VbPublicKey *pubkey = 0;
	RSA *rsa_key = 0;
	uint8_t *keyb_data = 0;
	uint32_t keyb_size;
	enum vb2_signature_algorithm sig_alg;
	uint64_t vb1_algorithm;
	FILE *fp;
	int ret = 1;

	fp = fopen(infile, "rb");
	if (!fp) {
		fprintf(stderr, "Unable to open %s\n", infile);
		goto done;
	}

	rsa_key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
	fclose(fp);

	if (!rsa_key) {
		fprintf(stderr, "Unable to read RSA key from %s\n", infile);
		goto done;
	}

	sig_alg = vb2_rsa_sig_alg(rsa_key);
	if (sig_alg == VB2_SIG_INVALID) {
		fprintf(stderr, "Unsupported sig algorithm in RSA key\n");
		goto done;
	}

	/* combine the sig_alg with the hash_alg to get the vb1 algorithm */
	vb1_algorithm = (sig_alg - VB2_SIG_RSA1024) * 3
		+ opt_hash_alg - VB2_HASH_SHA1;

	/* Create the private key */
	privkey = (VbPrivateKey *)malloc(sizeof(VbPrivateKey));
	if (!privkey)
		goto done;

	privkey->rsa_private_key = rsa_key;
	privkey->algorithm = vb1_algorithm;

	/* Write it out */
	strcpy(outext, ".vbprivk");
	if (0 != PrivateKeyWrite(outfile, privkey)) {
		fprintf(stderr, "unable to write private key\n");
		goto done;
	}
	fprintf(stderr, "wrote %s\n", outfile);

	/* Create the public key */
	ret = vb_keyb_from_rsa(rsa_key, &keyb_data, &keyb_size);
	if (ret) {
		fprintf(stderr, "couldn't extract the public key\n");
		goto done;
	}

	pubkey = PublicKeyAlloc(keyb_size, vb1_algorithm, opt_version);
	if (!pubkey)
		goto done;
	memcpy(GetPublicKeyData(pubkey), keyb_data, keyb_size);

	/* Write it out */
	strcpy(outext, ".vbpubk");
	if (0 != PublicKeyWrite(outfile, pubkey)) {
		fprintf(stderr, "unable to write public key\n");
		goto done;
	}
	fprintf(stderr, "wrote %s\n", outfile);

	ret = 0;

done:
	free(privkey);
	free(pubkey);
	free(keyb_data);
	RSA_free(rsa_key);
	return ret;
}

static int vb2_make_keypair()
{
	struct vb2_private_key *privkey = 0;
	struct vb2_public_key *pubkey = 0;
	RSA *rsa_key = 0;
	uint8_t *keyb_data = 0;
	uint32_t keyb_size;
	enum vb2_signature_algorithm sig_alg;
	uint8_t *pubkey_buf = 0;
	int has_priv = 0;

	FILE *fp;
	int ret = 1;

	fp = fopen(infile, "rb");
	if (!fp) {
		fprintf(stderr, "Unable to open %s\n", infile);
		goto done;
	}

	rsa_key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);

	if (!rsa_key) {
		/* Check if the PEM contains only a public key */
		fseek(fp, 0, SEEK_SET);
		rsa_key = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
	}
	fclose(fp);
	if (!rsa_key) {
		fprintf(stderr, "Unable to read RSA key from %s\n", infile);
		goto done;
	}
	/* Public keys doesn't have the private exponent */
	has_priv = !!rsa_key->d;
	if (!has_priv)
		fprintf(stderr, "%s has a public key only.\n", infile);

	sig_alg = vb2_rsa_sig_alg(rsa_key);
	if (sig_alg == VB2_SIG_INVALID) {
		fprintf(stderr, "Unsupported sig algorithm in RSA key\n");
		goto done;
	}

	if (has_priv) {
		/* Create the private key */
		privkey = calloc(1, sizeof(*privkey));
		if (!privkey) {
			fprintf(stderr, "Unable to allocate the private key\n");
			goto done;
		}

		privkey->rsa_private_key = rsa_key;
		privkey->sig_alg = sig_alg;
		privkey->hash_alg = opt_hash_alg;
		if (opt_desc && vb2_private_key_set_desc(privkey, opt_desc)) {
			fprintf(stderr, "Unable to set the private key description\n");
			goto done;
		}
	}

	/* Create the public key */
	if (vb2_public_key_alloc(&pubkey, sig_alg)) {
		fprintf(stderr, "Unable to allocate the public key\n");
		goto done;
	}

	/* Extract the keyb blob */
	if (vb_keyb_from_rsa(rsa_key, &keyb_data, &keyb_size)) {
		fprintf(stderr, "Couldn't extract the public key\n");
		goto done;
	}

	/*
	 * Copy the keyb blob to the public key's buffer, because that's where
	 * vb2_unpack_key_data() and vb2_public_key_pack() expect to find it.
	 */
	pubkey_buf = vb2_public_key_packed_data(pubkey);
	memcpy(pubkey_buf, keyb_data, keyb_size);

	/* Fill in the internal struct pointers */
	if (vb2_unpack_key_data(pubkey, pubkey_buf, keyb_size)) {
		fprintf(stderr, "Unable to unpack the public key blob\n");
		goto done;
	}

	pubkey->hash_alg = opt_hash_alg;
	pubkey->version = opt_version;
	if (opt_desc && vb2_public_key_set_desc(pubkey, opt_desc)) {
		fprintf(stderr, "Unable to set pubkey description\n");
		goto done;
	}

	/* Update the IDs */
	if (!force_id) {
		uint8_t *digest = DigestBuf(keyb_data, keyb_size,
					    SHA1_DIGEST_ALGORITHM);
		memcpy(&opt_id, digest, sizeof(opt_id));
		free(digest);
	}

	memcpy((struct vb2_id *)pubkey->id, &opt_id, sizeof(opt_id));

	/* Write them out */
	if (has_priv) {
		privkey->id = opt_id;
		strcpy(outext, ".vbprik2");
		if (vb2_private_key_write(privkey, outfile)) {
			fprintf(stderr, "unable to write private key\n");
			goto done;
		}
		fprintf(stderr, "wrote %s\n", outfile);
	}

	strcpy(outext, ".vbpubk2");
	if (vb2_public_key_write(pubkey, outfile)) {
		fprintf(stderr, "unable to write public key\n");
		goto done;
	}
	fprintf(stderr, "wrote %s\n", outfile);

	ret = 0;

done:
	RSA_free(rsa_key);
	if (privkey)				/* prevent double-free */
		privkey->rsa_private_key = 0;
	vb2_private_key_free(privkey);
	vb2_public_key_free(pubkey);
	free(keyb_data);
	return ret;
}

static int do_create(int argc, char *argv[])
{
	int errorcnt = 0;
	char *e, *s;
	int i, r, len, remove_ext = 0;

	while ((i = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
		switch (i) {

		case OPT_VERSION:
			opt_version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"invalid version \"%s\"\n", optarg);
				errorcnt = 1;
			}
			break;

		case OPT_DESC:
			opt_desc = optarg;
			break;

		case OPT_ID:
			if (VB2_SUCCESS != vb2_str_to_id(optarg, &opt_id)) {
				fprintf(stderr, "invalid id \"%s\"\n",
					optarg);
				errorcnt = 1;
			}
			force_id = 1;
			break;

		case OPT_HASH_ALG:
			if (!vb2_lookup_hash_alg(optarg, &opt_hash_alg)) {
				fprintf(stderr,
					"invalid hash_alg \"%s\"\n", optarg);
				errorcnt++;
			}
			break;

		case OPT_HELP:
			print_help(argc, argv);
			return !!errorcnt;

		case '?':
			if (optopt)
				fprintf(stderr, "Unrecognized option: -%c\n",
					optopt);
			else
				fprintf(stderr, "Unrecognized option\n");
			errorcnt++;
			break;
		case ':':
			fprintf(stderr, "Missing argument to -%c\n", optopt);
			errorcnt++;
			break;
		case 0:				/* handled option */
			break;
		default:
			DIE;
		}
	}

	/* If we don't have an input file already, we need one */
	if (!infile) {
		if (argc - optind <= 0) {
			fprintf(stderr, "ERROR: missing input filename\n");
			errorcnt++;
		} else {
			infile = argv[optind++];
		}
	}

	if (errorcnt) {
		print_help(argc, argv);
		return 1;
	}

	/* Decide how to determine the output filenames. */
	if (argc > optind) {
		s = argv[optind++];		/* just use this */
	} else {
		s = infile;			/* based on pem file name */
		remove_ext = 1;
	}

	/* Make an extra-large copy to leave room for filename extensions */
	len = strlen(s) + 20;
	outfile = (char *)malloc(len);
	if (!outfile) {
		fprintf(stderr, "ERROR: malloc() failed\n");
		return 1;
	}
	strcpy(outfile, s);

	if (remove_ext) {
		/* Find the last '/' if any, then the last '.' before that. */
		s = strrchr(outfile, '/');
		if (!s)
			s = outfile;
		s = strrchr(s, '.');
		/* Cut off the extension */
		if (s)
			*s = '\0';
	}
	/* Remember that spot for later */
	outext = outfile + strlen(outfile);

	/* Okay, do it */
	if (vboot_version == VBOOT_VERSION_1_0)
		r = vb1_make_keypair();
	else
		r = vb2_make_keypair();

	free(outfile);
	return r;
}

DECLARE_FUTIL_COMMAND(create, do_create, VBOOT_VERSION_ALL,
		      "Create a keypair from an RSA .pem file");
