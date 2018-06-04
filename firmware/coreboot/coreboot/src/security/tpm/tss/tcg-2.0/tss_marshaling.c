/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <arch/early_variables.h>
#include <commonlib/iobuf.h>
#include <console/console.h>
#include <stdlib.h>
#include <string.h>

#include "tss_marshaling.h"

static uint16_t tpm_tag CAR_GLOBAL;  /* Depends on the command type. */

#define unmarshal_TPM_CAP(a, b) ibuf_read_be32(a, b)
#define unmarshal_TPM_CC(a, b) ibuf_read_be32(a, b)
#define unmarshal_TPM_PT(a, b) ibuf_read_be32(a, b)
#define unmarshal_TPM_HANDLE(a, b) ibuf_read_be32(a, b)

#define marshal_TPM_HANDLE(a, b) obuf_write_be32(a, b)
#define marshal_TPMI_ALG_HASH(a, b) obuf_write_be16(a, b)

static int marshal_startup(struct obuf *ob, struct tpm2_startup *cmd_body)
{
	return obuf_write_be16(ob, cmd_body->startup_type);
}

static int marshal_get_capability(struct obuf *ob,
				   struct tpm2_get_capability *cmd_body)
{
	int rc = 0;

	rc |= obuf_write_be32(ob, cmd_body->capability);
	rc |= obuf_write_be32(ob, cmd_body->property);
	rc |= obuf_write_be32(ob, cmd_body->propertyCount);

	return rc;
}

static int marshal_TPM2B(struct obuf *ob, TPM2B *data)
{
	int rc = 0;

	rc |= obuf_write_be16(ob, data->size);
	rc |= obuf_write(ob, data->buffer, data->size);

	return rc;
}

static int marshal_TPMA_NV(struct obuf *ob, TPMA_NV *nv)
{
	uint32_t v;

	memcpy(&v, nv, sizeof(v));
	return obuf_write_be32(ob, v);
}

static int marshal_TPMS_NV_PUBLIC(struct obuf *ob, TPMS_NV_PUBLIC *nvpub)
{
	int rc = 0;

	rc |= marshal_TPM_HANDLE(ob, nvpub->nvIndex);
	rc |= marshal_TPMI_ALG_HASH(ob, nvpub->nameAlg);
	rc |= marshal_TPMA_NV(ob, &nvpub->attributes);
	rc |= marshal_TPM2B(ob, &nvpub->authPolicy.b);
	rc |= obuf_write_be16(ob, nvpub->dataSize);

	return rc;
}

static int marshal_TPMT_HA(struct obuf *ob, TPMT_HA *tpmtha)
{
	int rc = 0;

	rc |= marshal_TPMI_ALG_HASH(ob, tpmtha->hashAlg);
	rc |= obuf_write(ob, tpmtha->digest.sha256,
			sizeof(tpmtha->digest.sha256));

	return rc;
}

static int marshal_TPML_DIGEST_VALUES(struct obuf *ob,
				       TPML_DIGEST_VALUES *dvalues)
{
	int i;
	int rc = 0;

	rc |= obuf_write_be32(ob, dvalues->count);
	for (i = 0; i < dvalues->count; i++)
		rc |= marshal_TPMT_HA(ob, &dvalues->digests[i]);

	return rc;
}

static int marshal_session_header(struct obuf *ob,
				   struct tpm2_session_header *session_header)
{
	int rc = 0;
	struct obuf ob_sz;
	size_t prev_written;

	/* Snapshot current location to place size of header. */
	if (obuf_splice_current(ob, &ob_sz, sizeof(uint32_t)) < 0)
		return -1;

	/* Write a size placeholder. */
	rc |= obuf_write_be32(ob, 0);

	/* Keep track of session header data size by tracking num written. */
	prev_written = obuf_nr_written(ob);

	rc |= obuf_write_be32(ob, session_header->session_handle);
	rc |= obuf_write_be16(ob, session_header->nonce_size);
	rc |= obuf_write(ob, session_header->nonce, session_header->nonce_size);
	rc |= obuf_write_be8(ob, session_header->session_attrs);
	rc |= obuf_write_be16(ob, session_header->auth_size);
	rc |= obuf_write(ob, session_header->auth, session_header->auth_size);

	/* Fill back in proper size of session header. */
	rc |= obuf_write_be32(&ob_sz, obuf_nr_written(ob) - prev_written);

	return rc;
}

/*
 * Common session header can include one or two handles and an empty
 * session_header structure.
 */
static int marshal_common_session_header(struct obuf *ob,
					  const uint32_t *handles,
					  size_t handle_count)
{
	size_t i;
	struct tpm2_session_header session_header;
	int rc = 0;

	car_set_var(tpm_tag, TPM_ST_SESSIONS);

	for (i = 0; i < handle_count; i++)
		rc |= marshal_TPM_HANDLE(ob, handles[i]);

	memset(&session_header, 0, sizeof(session_header));
	session_header.session_handle = TPM_RS_PW;
	rc |= marshal_session_header(ob, &session_header);

	return rc;
}

static int marshal_nv_define_space(struct obuf *ob,
				    struct tpm2_nv_define_space_cmd *nvd_in)
{
	const uint32_t handle[] = { TPM_RH_PLATFORM };
	struct obuf ob_sz;
	size_t prev_written;
	int rc = 0;

	rc |= marshal_common_session_header(ob, handle, ARRAY_SIZE(handle));
	rc |= marshal_TPM2B(ob, &nvd_in->auth.b);

	/* Snapshot current location to place size field. */
	if (obuf_splice_current(ob, &ob_sz, sizeof(uint16_t)) < 0)
		return -1;

	/* Put placeholder for size */
	rc |= obuf_write_be16(ob, 0);

	/* Keep track of nv define space data size by tracking num written. */
	prev_written = obuf_nr_written(ob);

	rc |= marshal_TPMS_NV_PUBLIC(ob, &nvd_in->publicInfo);
	rc |= obuf_write_be16(&ob_sz, obuf_nr_written(ob) - prev_written);

	return rc;
}

static int marshal_nv_write(struct obuf *ob,
			     struct tpm2_nv_write_cmd *command_body)
{
	int rc = 0;
	uint32_t handles[] = { TPM_RH_PLATFORM, command_body->nvIndex };

	rc |= marshal_common_session_header(ob, handles, ARRAY_SIZE(handles));
	rc |= marshal_TPM2B(ob, &command_body->data.b);
	rc |= obuf_write_be16(ob, command_body->offset);

	return rc;
}

static int marshal_nv_write_lock(struct obuf *ob,
				  struct tpm2_nv_write_lock_cmd *command_body)
{
	uint32_t handles[] = { TPM_RH_PLATFORM, command_body->nvIndex };

	return marshal_common_session_header(ob, handles, ARRAY_SIZE(handles));
}

static int marshal_pcr_extend(struct obuf *ob,
			       struct tpm2_pcr_extend_cmd *command_body)
{
	int rc = 0;
	uint32_t handles[] = { command_body->pcrHandle };

	rc |= marshal_common_session_header(ob, handles, ARRAY_SIZE(handles));
	rc |= marshal_TPML_DIGEST_VALUES(ob, &command_body->digests);

	return rc;
}

static int marshal_nv_read(struct obuf *ob,
			    struct tpm2_nv_read_cmd *command_body)
{
	int rc = 0;
	uint32_t handles[] = { TPM_RH_PLATFORM, command_body->nvIndex };

	rc |= marshal_common_session_header(ob, handles, ARRAY_SIZE(handles));
	rc |= obuf_write_be16(ob, command_body->size);
	rc |= obuf_write_be16(ob, command_body->offset);

	return rc;
}

/* TPM2_Clear command does not require paramaters. */
static int marshal_clear(struct obuf *ob)
{
	const uint32_t handle[] = { TPM_RH_PLATFORM };

	return marshal_common_session_header(ob, handle, ARRAY_SIZE(handle));
}

static int marshal_selftest(struct obuf *ob,
			     struct tpm2_self_test *command_body)
{
	return obuf_write_be8(ob, command_body->yes_no);
}

static int marshal_hierarchy_control(struct obuf *ob,
			struct tpm2_hierarchy_control_cmd *command_body)
{
	int rc = 0;
	struct tpm2_session_header session_header;

	car_set_var(tpm_tag, TPM_ST_SESSIONS);

	rc |= marshal_TPM_HANDLE(ob, TPM_RH_PLATFORM);
	memset(&session_header, 0, sizeof(session_header));
	session_header.session_handle = TPM_RS_PW;
	rc |= marshal_session_header(ob, &session_header);

	rc |= marshal_TPM_HANDLE(ob, command_body->enable);
	rc |= obuf_write_be8(ob, command_body->state);

	return rc;
}

static int marshal_cr50_vendor_command(struct obuf *ob, void *command_body)
{
	int rc = 0;
	uint16_t *sub_command = command_body;

	switch (*sub_command) {
	case TPM2_CR50_SUB_CMD_NVMEM_ENABLE_COMMITS:
		rc |= obuf_write_be16(ob, *sub_command);
		break;
	case TPM2_CR50_SUB_CMD_TURN_UPDATE_ON:
		rc |= obuf_write_be16(ob, sub_command[0]);
		rc |= obuf_write_be16(ob, sub_command[1]);
		break;
	default:
		/* Unsupported subcommand. */
		printk(BIOS_WARNING, "Unsupported cr50 subcommand: 0x%04x\n",
			*sub_command);
		rc = -1;
		break;
	}
	return rc;
}

int tpm_marshal_command(TPM_CC command, void *tpm_command_body, struct obuf *ob)
{
	struct obuf ob_hdr;
	const size_t hdr_sz = sizeof(uint16_t) + 2 * sizeof(uint32_t);
	int rc = 0;

	car_set_var(tpm_tag, TPM_ST_NO_SESSIONS);

	if (obuf_splice_current(ob, &ob_hdr, hdr_sz) < 0)
		return -1;

	/* Write TPM command header with placeholder field values. */
	rc |= obuf_write_be16(ob, 0);
	rc |= obuf_write_be32(ob, 0);
	rc |= obuf_write_be32(ob, command);

	if (rc != 0)
		return rc;

	switch (command) {
	case TPM2_Startup:
		rc |= marshal_startup(ob, tpm_command_body);
		break;

	case TPM2_GetCapability:
		rc |= marshal_get_capability(ob, tpm_command_body);
		break;

	case TPM2_NV_Read:
		rc |= marshal_nv_read(ob, tpm_command_body);
		break;

	case TPM2_NV_DefineSpace:
		rc |= marshal_nv_define_space(ob, tpm_command_body);
		break;

	case TPM2_NV_Write:
		rc |= marshal_nv_write(ob, tpm_command_body);
		break;

	case TPM2_NV_WriteLock:
		rc |= marshal_nv_write_lock(ob, tpm_command_body);
		break;

	case TPM2_SelfTest:
		rc |= marshal_selftest(ob, tpm_command_body);
		break;

	case TPM2_Hierarchy_Control:
		rc |= marshal_hierarchy_control(ob, tpm_command_body);
		break;

	case TPM2_Clear:
		rc |= marshal_clear(ob);
		break;

	case TPM2_PCR_Extend:
		rc |= marshal_pcr_extend(ob, tpm_command_body);
		break;

	case TPM2_CR50_VENDOR_COMMAND:
		rc |= marshal_cr50_vendor_command(ob, tpm_command_body);
		break;

	default:
		printk(BIOS_INFO, "%s:%d:Request to marshal unsupported command %#x\n",
		       __FILE__, __LINE__, command);
		rc = -1;
	}

	if (rc != 0)
		return rc;

	/* Fix up the command header with known values. */
	rc |= obuf_write_be16(&ob_hdr, car_get_var(tpm_tag));
	rc |= obuf_write_be32(&ob_hdr, obuf_nr_written(ob));

	return rc;
}

static int unmarshal_get_capability(struct ibuf *ib,
				     struct get_cap_response *gcr)
{
	int i;
	int rc = 0;

	rc |= ibuf_read_be8(ib, &gcr->more_data);
	rc |= unmarshal_TPM_CAP(ib, &gcr->cd.capability);

	if (rc != 0)
		return rc;

	switch (gcr->cd.capability) {
	case TPM_CAP_TPM_PROPERTIES:
		if (ibuf_read_be32(ib, &gcr->cd.data.tpmProperties.count))
			return -1;
		if (gcr->cd.data.tpmProperties.count > ARRAY_SIZE
		    (gcr->cd.data.tpmProperties.tpmProperty)) {
			printk(BIOS_INFO, "%s:%s:%d - %d - too many properties\n",
			       __FILE__, __func__, __LINE__,
			       gcr->cd.data.tpmProperties.count);
			return -1;
		}
		for (i = 0; i < gcr->cd.data.tpmProperties.count; i++) {
			TPMS_TAGGED_PROPERTY *pp;

			pp = gcr->cd.data.tpmProperties.tpmProperty + i;
			rc |= unmarshal_TPM_PT(ib, &pp->property);
			rc |= ibuf_read_be32(ib, &pp->value);
		}
		break;
	default:
		printk(BIOS_ERR,
		       "%s:%d - unable to unmarshal capability response",
		       __func__, __LINE__);
		printk(BIOS_ERR, " for %d\n", gcr->cd.capability);
		rc = -1;
		break;
	}

	return rc;
}

static int unmarshal_TPM2B_MAX_NV_BUFFER(struct ibuf *ib,
					  TPM2B_MAX_NV_BUFFER *nv_buffer)
{
	if (ibuf_read_be16(ib, &nv_buffer->t.size))
		return -1;

	nv_buffer->t.buffer = ibuf_oob_drain(ib, nv_buffer->t.size);

	if (nv_buffer->t.buffer == NULL) {
		printk(BIOS_ERR, "%s:%d - "
		       "size mismatch: expected %d, remaining %zd\n",
		       __func__, __LINE__, nv_buffer->t.size,
			ibuf_remaining(ib));
		return -1;
	}

	return 0;
}

static int unmarshal_nv_read(struct ibuf *ib, struct nv_read_response *nvr)
{
	/* Total size of the parameter field. */
	if (ibuf_read_be32(ib, &nvr->params_size))
		return -1;

	if (unmarshal_TPM2B_MAX_NV_BUFFER(ib, &nvr->buffer))
		return -1;

	if (nvr->params_size !=
	    (nvr->buffer.t.size + sizeof(nvr->buffer.t.size))) {
		printk(BIOS_ERR,
		       "%s:%d - parameter/buffer %d/%d size mismatch",
		       __func__, __LINE__, nvr->params_size,
		       nvr->buffer.t.size);
		return -1;
	}

	/*
	 * Let's ignore the authorisation section. It should be 5 bytes total,
	 * just confirm that this is the case and report any discrepancy.
	 */
	if (ibuf_remaining(ib) != 5)
		printk(BIOS_ERR,
		       "%s:%d - unexpected authorisation seciton size %zd\n",
		       __func__, __LINE__, ibuf_remaining(ib));

	ibuf_oob_drain(ib, ibuf_remaining(ib));

	return 0;
}

static int unmarshal_vendor_command(struct ibuf *ib,
				     struct vendor_command_response *vcr)
{
	if (ibuf_read_be16(ib, &vcr->vc_subcommand))
		return -1;

	switch (vcr->vc_subcommand) {
	case TPM2_CR50_SUB_CMD_NVMEM_ENABLE_COMMITS:
		break;
	case TPM2_CR50_SUB_CMD_TURN_UPDATE_ON:
		return ibuf_read_be8(ib, &vcr->num_restored_headers);
		break;
	default:
		printk(BIOS_ERR,
		       "%s:%d - unsupported vendor command %#04x!\n",
		       __func__, __LINE__, vcr->vc_subcommand);
		return -1;
	}

	return 0;
}

struct tpm2_response *tpm_unmarshal_response(TPM_CC command, struct ibuf *ib)
{
	static struct tpm2_response tpm2_static_resp CAR_GLOBAL;
	struct tpm2_response *tpm2_resp = car_get_var_ptr(&tpm2_static_resp);
	int rc = 0;

	rc |= ibuf_read_be16(ib, &tpm2_resp->hdr.tpm_tag);
	rc |= ibuf_read_be32(ib, &tpm2_resp->hdr.tpm_size);
	rc |= unmarshal_TPM_CC(ib, &tpm2_resp->hdr.tpm_code);

	if (rc != 0)
		return NULL;

	if (ibuf_remaining(ib) == 0) {
		if (tpm2_resp->hdr.tpm_size != ibuf_nr_read(ib))
			printk(BIOS_ERR,
			       "%s: size mismatch in response to command %#x\n",
			       __func__, command);
		return tpm2_resp;
	}

	switch (command) {
	case TPM2_Startup:
		break;

	case TPM2_GetCapability:
		rc |= unmarshal_get_capability(ib, &tpm2_resp->gc);
		break;

	case TPM2_NV_Read:
		rc |= unmarshal_nv_read(ib, &tpm2_resp->nvr);
		break;

	case TPM2_Hierarchy_Control:
	case TPM2_Clear:
	case TPM2_NV_DefineSpace:
	case TPM2_NV_Write:
	case TPM2_NV_WriteLock:
	case TPM2_PCR_Extend:
		/* Session data included in response can be safely ignored. */
		ibuf_oob_drain(ib, ibuf_remaining(ib));
		break;

	case TPM2_CR50_VENDOR_COMMAND:
		rc |= unmarshal_vendor_command(ib, &tpm2_resp->vcr);
		break;

	default:
		{
			size_t i;
			size_t sz_left;
			const uint8_t *data;

			printk(BIOS_INFO, "%s:%d:"
			       "Request to unmarshal unexpected command %#x,"
			       " code %#x",
			       __func__, __LINE__, command,
			       tpm2_resp->hdr.tpm_code);

			sz_left = ibuf_remaining(ib);
			data = ibuf_oob_drain(ib, sz_left);

			for (i = 0; i < sz_left; i++) {
				if (!(i % 16))
					printk(BIOS_INFO, "\n");
				printk(BIOS_INFO, "%2.2x ", data[i]);
			}
		}
		printk(BIOS_INFO, "\n");
		return NULL;
	}

	if (ibuf_remaining(ib)) {
		printk(BIOS_INFO,
		       "%s:%d got %d bytes back in response to %#x,"
		       " failed to parse (%zd)\n",
		       __func__, __LINE__, tpm2_resp->hdr.tpm_size,
		       command, ibuf_remaining(ib));
		return NULL;
	}

	/* The entire message have been parsed. */
	return tpm2_resp;
}
