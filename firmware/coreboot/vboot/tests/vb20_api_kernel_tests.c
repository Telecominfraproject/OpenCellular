/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for kernel verification library, api layer
 */

#include <stdio.h>

#include "2sysincludes.h"
#include "2api.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2rsa.h"
#include "2secdata.h"
#include "vb2_common.h"
#include "test_common.h"

/* Common context for tests */
static uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE]
	__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
static struct vb2_context cc;
static struct vb2_shared_data *sd;
static struct vb2_fw_preamble *fwpre;
static struct vb2_kernel_preamble *kpre;
static struct vb2_packed_key *kdkey;
static const char fw_kernel_key_data[36] = "Test kernel key data";
static char kernel_data[0x4008] = "Sure it's a kernel...";

/* Mocked function data */

static struct {
	struct vb2_gbb_header h;
	struct vb2_packed_key recovery_key;
	char recovery_key_data[32];
} mock_gbb;

static int mock_read_res_fail_on_call;
static int mock_unpack_key_retval;
static int mock_read_gbb_header_retval;
static int mock_load_kernel_keyblock_retval;
static int mock_load_kernel_preamble_retval;

/* Type of test to reset for */
enum reset_type {
	FOR_PHASE1,
	FOR_PHASE2,
	FOR_PHASE3,
};

static void reset_common_data(enum reset_type t)
{
	struct vb2_packed_key *k;

	memset(workbuf, 0xaa, sizeof(workbuf));

	memset(&cc, 0, sizeof(cc));
	cc.workbuf = workbuf;
	cc.workbuf_size = sizeof(workbuf);

	vb2_init_context(&cc);
	sd = vb2_get_sd(&cc);

	vb2_nv_init(&cc);

	vb2_secdatak_create(&cc);
	vb2_secdatak_init(&cc);
	vb2_secdatak_set(&cc, VB2_SECDATAK_VERSIONS, 0x20002);

	mock_read_res_fail_on_call = 0;
	mock_unpack_key_retval = VB2_SUCCESS;
	mock_read_gbb_header_retval = VB2_SUCCESS;
	mock_load_kernel_keyblock_retval = VB2_SUCCESS;
	mock_load_kernel_preamble_retval = VB2_SUCCESS;

	/* Recovery key in mock GBB */
	mock_gbb.recovery_key.algorithm = 11;
	mock_gbb.recovery_key.key_offset =
		vb2_offset_of(&mock_gbb.recovery_key,
			      &mock_gbb.recovery_key_data);
	mock_gbb.recovery_key.key_size = sizeof(mock_gbb.recovery_key_data);
	strcpy(mock_gbb.recovery_key_data, "The recovery key");
	mock_gbb.h.recovery_key_offset =
		vb2_offset_of(&mock_gbb, &mock_gbb.recovery_key);
	mock_gbb.h.recovery_key_size =
		mock_gbb.recovery_key.key_offset +
		mock_gbb.recovery_key.key_size;


	if (t == FOR_PHASE1) {
		uint8_t *kdata;

		/* Create mock firmware preamble in the context */
		sd->workbuf_preamble_offset = cc.workbuf_used;
		fwpre = (struct vb2_fw_preamble *)
			(cc.workbuf + sd->workbuf_preamble_offset);
		k = &fwpre->kernel_subkey;
		kdata = (uint8_t *)fwpre + sizeof(*fwpre);
		memcpy(kdata, fw_kernel_key_data, sizeof(fw_kernel_key_data));
		k->algorithm = 7;
		k->key_offset = vb2_offset_of(k, kdata);
		k->key_size = sizeof(fw_kernel_key_data);
		sd->workbuf_preamble_size = sizeof(*fwpre) + k->key_size;
		vb2_set_workbuf_used(&cc, sd->workbuf_preamble_offset +
				     sd->workbuf_preamble_size);

	} else if (t == FOR_PHASE2) {
		struct vb2_signature *sig;
		struct vb2_digest_context dc;
		uint8_t *sdata;

		/* Create mock kernel data key */
		sd->workbuf_data_key_offset = cc.workbuf_used;
		kdkey = (struct vb2_packed_key *)
			(cc.workbuf + sd->workbuf_data_key_offset);
		kdkey->algorithm = VB2_ALG_RSA2048_SHA256;
		sd->workbuf_data_key_size = sizeof(*kdkey);
		vb2_set_workbuf_used(&cc, sd->workbuf_data_key_offset +
				     sd->workbuf_data_key_size);

		/* Create mock kernel preamble in the context */
		sd->workbuf_preamble_offset = cc.workbuf_used;
		kpre = (struct vb2_kernel_preamble *)
			(cc.workbuf + sd->workbuf_preamble_offset);
		sdata = (uint8_t *)kpre + sizeof(*kpre);

		sig = &kpre->body_signature;
		sig->data_size = sizeof(kernel_data);
		sig->sig_offset = vb2_offset_of(sig, sdata);
		sig->sig_size = VB2_SHA512_DIGEST_SIZE;

		vb2_digest_init(&dc, VB2_HASH_SHA256);
		vb2_digest_extend(&dc, (const uint8_t *)kernel_data,
				  sizeof(kernel_data));
		vb2_digest_finalize(&dc, sdata, sig->sig_size);

		sd->workbuf_preamble_size = sizeof(*kpre) + sig->sig_size;
		sd->vblock_preamble_offset =
			0x10000 - sd->workbuf_preamble_size;
		vb2_set_workbuf_used(&cc, sd->workbuf_preamble_offset +
				     sd->workbuf_preamble_size);

	} else {
		/* Set flags and versions for roll-forward */
		sd->kernel_version = 0x20004;
		sd->kernel_version_secdatak = 0x20002;
		sd->flags |= VB2_SD_FLAG_KERNEL_SIGNED;
		cc.flags |= VB2_CONTEXT_ALLOW_KERNEL_ROLL_FORWARD;
	}
};

/* Mocked functions */

int vb2ex_read_resource(struct vb2_context *ctx,
			enum vb2_resource_index index,
			uint32_t offset,
			void *buf,
			uint32_t size)
{
	uint8_t *rptr;
	uint32_t rsize;

	if (--mock_read_res_fail_on_call == 0)
		return VB2_ERROR_MOCK;

	switch(index) {
	case VB2_RES_GBB:
		rptr = (uint8_t *)&mock_gbb;
		rsize = sizeof(mock_gbb);
		break;
	default:
		return VB2_ERROR_EX_READ_RESOURCE_INDEX;
	}

	if (offset > rsize || offset + size > rsize)
		return VB2_ERROR_EX_READ_RESOURCE_SIZE;

	memcpy(buf, rptr + offset, size);
	return VB2_SUCCESS;
}

int vb2_read_gbb_header(struct vb2_context *ctx, struct vb2_gbb_header *gbb)
{
	memcpy(gbb, &mock_gbb.h, sizeof(*gbb));
	return mock_read_gbb_header_retval;
}

int vb2_load_kernel_keyblock(struct vb2_context *ctx)
{
	return mock_load_kernel_keyblock_retval;
}

int vb2_load_kernel_preamble(struct vb2_context *ctx)
{
	return mock_load_kernel_preamble_retval;
}

int vb2_unpack_key_buffer(struct vb2_public_key *key,
		   const uint8_t *buf,
		   uint32_t size)
{
	const struct vb2_packed_key *k = (const struct vb2_packed_key *)buf;

	key->arrsize = 0;
	key->hash_alg = vb2_crypto_to_hash(k->algorithm);
	return mock_unpack_key_retval;
}

int vb2_verify_digest(const struct vb2_public_key *key,
		      struct vb2_signature *sig,
		      const uint8_t *digest,
		      const struct vb2_workbuf *wb)
{
	if (memcmp(digest, (uint8_t *)sig + sig->sig_offset, sig->sig_size))
		return VB2_ERROR_VDATA_VERIFY_DIGEST;

	return VB2_SUCCESS;
}

/* Tests */

static void phase1_tests(void)
{
	struct vb2_packed_key *k;
	uint32_t old_preamble_offset;

	/* Test successful call */
	reset_common_data(FOR_PHASE1);
	old_preamble_offset = sd->workbuf_preamble_offset;
	TEST_SUCC(vb2api_kernel_phase1(&cc), "phase1 good");
	TEST_EQ(sd->workbuf_preamble_size, 0, "  no more fw preamble");
	/* Make sure normal key was loaded */
	TEST_EQ(sd->workbuf_kernel_key_offset, old_preamble_offset,
		"  workbuf key offset");
	k = (struct vb2_packed_key *)
		(cc.workbuf + sd->workbuf_kernel_key_offset);
	TEST_EQ(sd->workbuf_kernel_key_size, k->key_offset + k->key_size,
		"  workbuf key size");
	TEST_EQ(cc.workbuf_used,
		vb2_wb_round_up(sd->workbuf_kernel_key_offset +
				sd->workbuf_kernel_key_size),
		"  workbuf used");
	TEST_EQ(k->algorithm, 7, "  key algorithm");
	TEST_EQ(k->key_size, sizeof(fw_kernel_key_data), "  key_size");
	TEST_EQ(memcmp((uint8_t *)k + k->key_offset, fw_kernel_key_data,
		       k->key_size), 0, "  key data");
	TEST_EQ(sd->kernel_version_secdatak, 0x20002, "  secdatak version");

	/* Test successful call in recovery mode */
	reset_common_data(FOR_PHASE1);
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	/* No preamble loaded in recovery mode */
	cc.workbuf_used = old_preamble_offset = sd->workbuf_preamble_offset;
	sd->workbuf_preamble_offset = sd->workbuf_preamble_size = 0;
	TEST_SUCC(vb2api_kernel_phase1(&cc), "phase1 rec good");
	TEST_EQ(sd->workbuf_preamble_size, 0, "no more fw preamble");
	/* Make sure recovery key was loaded */
	TEST_EQ(sd->workbuf_kernel_key_offset, old_preamble_offset,
		"  workbuf key offset");
	k = (struct vb2_packed_key *)
		(cc.workbuf + sd->workbuf_kernel_key_offset);
	TEST_EQ(sd->workbuf_kernel_key_size, k->key_offset + k->key_size,
		"  workbuf key size");
	TEST_EQ(cc.workbuf_used,
		vb2_wb_round_up(sd->workbuf_kernel_key_offset +
				sd->workbuf_kernel_key_size),
		"  workbuf used");
	TEST_EQ(k->algorithm, 11, "  key algorithm");
	TEST_EQ(k->key_size, sizeof(mock_gbb.recovery_key_data), "  key_size");
	TEST_EQ(memcmp((uint8_t *)k + k->key_offset,
		       mock_gbb.recovery_key_data, k->key_size), 0,
		"  key data");
	TEST_EQ(sd->kernel_version_secdatak, 0x20002, "  secdatak version");

	/* Bad secdatak causes failure in normal mode only */
	reset_common_data(FOR_PHASE1);
	cc.secdatak[0] ^= 0x33;
	TEST_EQ(vb2api_kernel_phase1(&cc), VB2_ERROR_SECDATAK_CRC,
		"phase1 bad secdata");
	reset_common_data(FOR_PHASE1);

	cc.secdatak[0] ^= 0x33;
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	TEST_SUCC(vb2api_kernel_phase1(&cc), "phase1 bad secdata rec");
	TEST_EQ(sd->kernel_version_secdatak, 0, "  secdatak version");

	/* Failures while reading recovery key */
	reset_common_data(FOR_PHASE1);
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	cc.workbuf_used = cc.workbuf_size + VB2_WORKBUF_ALIGN -
			vb2_wb_round_up(sizeof(struct vb2_gbb_header));
	TEST_EQ(vb2api_kernel_phase1(&cc), VB2_ERROR_GBB_WORKBUF,
		"phase1 rec workbuf gbb header");

	reset_common_data(FOR_PHASE1);
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	mock_read_gbb_header_retval = VB2_ERROR_MOCK;
	TEST_EQ(vb2api_kernel_phase1(&cc), VB2_ERROR_MOCK,
		"phase1 rec gbb read header");

	reset_common_data(FOR_PHASE1);
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	mock_gbb.h.recovery_key_size = cc.workbuf_size - 1;
	TEST_EQ(vb2api_kernel_phase1(&cc),
		VB2_ERROR_API_KPHASE1_WORKBUF_REC_KEY,
		"phase1 rec workbuf key");

	reset_common_data(FOR_PHASE1);
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	mock_read_res_fail_on_call = 1;
	TEST_EQ(vb2api_kernel_phase1(&cc), VB2_ERROR_MOCK,
		"phase1 rec gbb read key");

	/* Failures while parsing subkey from firmware preamble */
	reset_common_data(FOR_PHASE1);
	sd->workbuf_preamble_size = 0;
	TEST_EQ(vb2api_kernel_phase1(&cc), VB2_ERROR_API_KPHASE1_PREAMBLE,
		"phase1 fw preamble");
}

static void load_kernel_vblock_tests(void)
{
	reset_common_data(FOR_PHASE1);
	TEST_SUCC(vb2api_load_kernel_vblock(&cc), "load vblock good");

	reset_common_data(FOR_PHASE1);
	mock_load_kernel_keyblock_retval = VB2_ERROR_MOCK;
	TEST_EQ(vb2api_load_kernel_vblock(&cc), VB2_ERROR_MOCK,
		"load vblock bad keyblock");

	reset_common_data(FOR_PHASE1);
	mock_load_kernel_preamble_retval = VB2_ERROR_MOCK;
	TEST_EQ(vb2api_load_kernel_vblock(&cc), VB2_ERROR_MOCK,
		"load vblock bad preamble");
}

static void get_kernel_size_tests(void)
{
	uint32_t offs, size;

	reset_common_data(FOR_PHASE2);
	offs = size = 0;
	TEST_SUCC(vb2api_get_kernel_size(&cc, &offs, &size), "get size good");
	TEST_EQ(offs, 0x10000, "  offset");
	TEST_EQ(size, sizeof(kernel_data), "  size");

	/* Don't need to pass pointers */
	reset_common_data(FOR_PHASE2);
	TEST_SUCC(vb2api_get_kernel_size(&cc, NULL, NULL), "get size null");

	reset_common_data(FOR_PHASE2);
	sd->workbuf_preamble_size = 0;
	TEST_EQ(vb2api_get_kernel_size(&cc, &offs, &size),
		VB2_ERROR_API_GET_KERNEL_SIZE_PREAMBLE,
		"get size no preamble");
}

static void verify_kernel_data_tests(void)
{
	reset_common_data(FOR_PHASE2);
	TEST_SUCC(vb2api_verify_kernel_data(&cc, kernel_data,
					    sizeof(kernel_data)),
		  "verify data good");

	reset_common_data(FOR_PHASE2);
	sd->workbuf_preamble_size = 0;
	TEST_EQ(vb2api_verify_kernel_data(&cc, kernel_data,
					  sizeof(kernel_data)),
		VB2_ERROR_API_VERIFY_KDATA_PREAMBLE, "verify no preamble");

	reset_common_data(FOR_PHASE2);
	TEST_EQ(vb2api_verify_kernel_data(&cc, kernel_data,
					  sizeof(kernel_data) + 1),
		VB2_ERROR_API_VERIFY_KDATA_SIZE, "verify size");

	reset_common_data(FOR_PHASE2);
	cc.workbuf_used = cc.workbuf_size + VB2_WORKBUF_ALIGN -
			vb2_wb_round_up(sizeof(struct vb2_digest_context));
	TEST_EQ(vb2api_verify_kernel_data(&cc, kernel_data,
					  sizeof(kernel_data)),
		VB2_ERROR_API_VERIFY_KDATA_WORKBUF, "verify workbuf");

	reset_common_data(FOR_PHASE2);
	sd->workbuf_data_key_size = 0;
	TEST_EQ(vb2api_verify_kernel_data(&cc, kernel_data,
					  sizeof(kernel_data)),
		VB2_ERROR_API_VERIFY_KDATA_KEY, "verify no key");

	reset_common_data(FOR_PHASE2);
	mock_unpack_key_retval = VB2_ERROR_MOCK;
	TEST_EQ(vb2api_verify_kernel_data(&cc, kernel_data,
					  sizeof(kernel_data)),
		VB2_ERROR_MOCK, "verify unpack key");

	reset_common_data(FOR_PHASE2);
	kdkey->algorithm = VB2_ALG_COUNT;
	TEST_EQ(vb2api_verify_kernel_data(&cc, kernel_data,
					  sizeof(kernel_data)),
		VB2_ERROR_SHA_INIT_ALGORITHM, "verify hash init");

	reset_common_data(FOR_PHASE2);
	cc.workbuf_used = cc.workbuf_size -
			vb2_wb_round_up(sizeof(struct vb2_digest_context));
	TEST_EQ(vb2api_verify_kernel_data(&cc, kernel_data,
					  sizeof(kernel_data)),
		VB2_ERROR_API_CHECK_HASH_WORKBUF_DIGEST, "verify hash workbuf");

	reset_common_data(FOR_PHASE2);
	kernel_data[3] ^= 0xd0;
	TEST_EQ(vb2api_verify_kernel_data(&cc, kernel_data,
					  sizeof(kernel_data)),
		VB2_ERROR_VDATA_VERIFY_DIGEST, "verify hash digest");
	kernel_data[3] ^= 0xd0;
}

static void phase3_tests(void)
{
	uint32_t v;

	reset_common_data(FOR_PHASE3);
	TEST_SUCC(vb2api_kernel_phase3(&cc), "phase3 good");
	vb2_secdatak_get(&cc, VB2_SECDATAK_VERSIONS, &v);
	TEST_EQ(v, 0x20004, "  version");

	reset_common_data(FOR_PHASE3);
	sd->kernel_version = 0x20001;
	TEST_SUCC(vb2api_kernel_phase3(&cc), "phase3 no rollback");
	vb2_secdatak_get(&cc, VB2_SECDATAK_VERSIONS, &v);
	TEST_EQ(v, 0x20002, "  version");

	reset_common_data(FOR_PHASE3);
	sd->flags &= ~VB2_SD_FLAG_KERNEL_SIGNED;
	TEST_SUCC(vb2api_kernel_phase3(&cc), "phase3 unsigned kernel");
	vb2_secdatak_get(&cc, VB2_SECDATAK_VERSIONS, &v);
	TEST_EQ(v, 0x20002, "  version");

	reset_common_data(FOR_PHASE3);
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	TEST_SUCC(vb2api_kernel_phase3(&cc), "phase3 recovery");
	vb2_secdatak_get(&cc, VB2_SECDATAK_VERSIONS, &v);
	TEST_EQ(v, 0x20002, "  version");

	reset_common_data(FOR_PHASE3);
	cc.flags &= ~VB2_CONTEXT_ALLOW_KERNEL_ROLL_FORWARD;
	TEST_SUCC(vb2api_kernel_phase3(&cc), "phase3 no rollforward");
	vb2_secdatak_get(&cc, VB2_SECDATAK_VERSIONS, &v);
	TEST_EQ(v, 0x20002, "  version");

	reset_common_data(FOR_PHASE3);
	sd->status &= ~VB2_SD_STATUS_SECDATAK_INIT;
	TEST_EQ(vb2api_kernel_phase3(&cc),
		VB2_ERROR_SECDATAK_SET_UNINITIALIZED, "phase3 set fail");
}

int main(int argc, char* argv[])
{
	phase1_tests();
	load_kernel_vblock_tests();
	get_kernel_size_tests();
	verify_kernel_data_tests();
	phase3_tests();

	return gTestSuccess ? 0 : 255;
}
