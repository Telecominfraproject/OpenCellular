/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Boot descriptor block host functions
 */

#ifndef VBOOT_REFERENCE_BDB_HOST_H_
#define VBOOT_REFERENCE_BDB_HOST_H_

#include <stdlib.h>
#include <openssl/pem.h>
#include "bdb_struct.h"

/*****************************************************************************/
/*
Expected calling sequence:

Load and check just the header
bdb_check_header(buf, size);

Load and verify the entire BDB
bdb_verify(buf, size, bdb_key_hash, dev_mode_flag);

	bdb_check_header() again - paranoia against bad storage devices

	bdb_check_key() on BDB key
	bdb_sha256() on BDB key
	Compare with appropriate root key hash
	If dev_mode_flag(), mismatch is not fatal

	bdb_check_sig() on BDB header sig
	bdb_sha256() on OEM area 1, RW datakey
	bdb_rsa_verify() on digest using BDB key

	bdb_check_key() on RW datakey

	bdb_check_data() on RW data
	bdb_check_sig() on data sig
	bdb_sha256() on data, OEM area 1, hashes
	bdb_rsa_verify() on digest using RW datakey

Check RW datakey version.  If normal boot from primary BDB, roll forward
Check data version.  If normal boot from primary BDB, roll forward
*/

/*****************************************************************************/
/* Codes for functions returning numeric error codes */

enum bdb_host_return_code {
	/* All/any of bdb_return_code, and the following... */

	/* Other errors */
	BDB_ERROR_HOST = 200,
};

/*****************************************************************************/
/* Functions */

/**
 * Like strncpy, but guaranteeing null termination
 */
char *strzcpy(char *dest, const char *src, size_t size);

/**
 * Read a file.
 *
 * Caller must free() the returned buffer.
 *
 * @param filename	Path to file
 * @param size_ptr	Destination for size of buffer
 * @return A newly allocated buffer containing the data, or NULL if error.
 */
uint8_t *read_file(const char *filename, uint32_t *size_ptr);

/**
 * Write a file.
 *
 * @param buf		Data to write
 * @param size		Size of data in bytes
 * @return 0 if success, non-zero error code if error.
 */
int write_file(const char *filename, const void *buf, uint32_t size);

/**
 * Read a PEM from a file.
 *
 * Caller must free the PEM with RSA_free().
 *
 * @param filename	Path to file
 * @return A newly allocated PEM object, or NULL if error.
 */
struct rsa_st *read_pem(const char *filename);

/**
 * Create a BDB public key object.
 *
 * Caller must free() the returned key.
 *
 * @param filename	Path to file containing public key (.keyb)
 * @param key_version	Version for key
 * @param desc		Description.  Optional; may be NULL.
 * @return A newly allocated public key, or NULL if error.
 */
struct bdb_key *bdb_create_key(const char *filename,
			       uint32_t key_version,
			       const char *desc);

/**
 * Create a BDB signature object.
 *
 * Caller must free() the returned signature.
 *
 * @param data		Data to sign
 * @param size		Size of data in bytes
 * @param key		PEM key
 * @param sig_alg	Signature algorithm
 * @param desc		Description.  Optional; may be NULL.
 * @return A newly allocated signature, or NULL if error.
 */
struct bdb_sig *bdb_create_sig(const void *data,
			       size_t size,
			       struct rsa_st *key,
			       uint32_t sig_alg,
			       const char *desc);

struct bdb_create_params
{
	/* Load address */
	uint64_t bdb_load_address;

	/* OEM areas.  Size may be 0, in which case the buffer is ignored */
	uint8_t *oem_area_0;
	uint32_t oem_area_0_size;
	uint8_t *oem_area_1;
	uint32_t oem_area_1_size;

	/* Public BDB key and datakey */
	struct bdb_key *bdbkey;
	struct bdb_key *datakey;

	/* Private BDB key and datakey */
	struct rsa_st *private_bdbkey;
	struct rsa_st *private_datakey;

	/* Descriptions for header and data signatures */
	char *header_sig_description;
	char *data_sig_description;

	/* Data description and version */
	char *data_description;
	uint32_t data_version;

	/* Data hashes and count */
	struct bdb_hash *hash;
	uint32_t num_hashes;
};

/**
 * Create a new BDB
 *
 * Caller must free() returned object.
 *
 * @param p		Creation parameters
 * @return A newly allocated BDB, or NULL if error.
 */
struct bdb_header *bdb_create(struct bdb_create_params *p);

/*****************************************************************************/

#endif /* VBOOT_REFERENCE_BDB_HOST_H_ */
