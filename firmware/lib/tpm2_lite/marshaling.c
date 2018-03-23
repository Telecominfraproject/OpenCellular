/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "2sysincludes.h"
#include "2common.h"

#include "tpm2_marshaling.h"
#include "utility.h"

static uint16_t tpm_tag;  /* Depends on the command type. */
static int ph_disabled;   /* Platform hierarchy disabled. */

static void write_be16(void *dest, uint16_t val)
{
	uint8_t *byte_dest = dest;

	byte_dest[0] = val >> 8;
	byte_dest[1] = val;
}

static void write_be32(void *dest, uint32_t val)
{
	uint8_t *byte_dest = dest;

	byte_dest[0] = val >> 24;
	byte_dest[1] = val >> 16;
	byte_dest[2] = val >> 8;
	byte_dest[3] = val;
}

static uint16_t read_be16(const void *src)
{
	const uint8_t *s = src;
	return (((uint16_t)s[0]) << 8) | (((uint16_t)s[1]) << 0);
}

static inline uint32_t read_be32(const void *src)
{
	const uint8_t *s = src;

	return (((uint32_t)s[0]) << 24) | (((uint32_t)s[1]) << 16) |
		(((uint32_t)s[2]) << 8) | (((uint32_t)s[3]) << 0);
}

/*
 * Each unmarshaling function receives a pointer to the buffer pointer and a
 * pointer to the size of data still in the buffer. The function extracts data
 * from the buffer and adjusts both buffer pointer and remaining data size.
 *
 * Should there be not enough data in the buffer to unmarshal the required
 * object, the remaining data size is set to -1 to indicate the error. The
 * remaining data size is expected to be set to zero once the last data item
 * has been extracted from the buffer.
 */

static uint8_t unmarshal_u8(void **buffer, int *buffer_space)
{
	uint8_t value;

	if (*buffer_space < sizeof(value)) {
		*buffer_space = -1; /* Indicate a failure. */
		return 0;
	}

	value = *(uint8_t *)(*buffer);
	*buffer = (void *) ((uintptr_t) (*buffer) + sizeof(value));
	*buffer_space -= sizeof(value);

	return value;
}

static uint16_t unmarshal_u16(void **buffer, int *buffer_space)
{
	uint16_t value;

	if (*buffer_space < sizeof(value)) {
		*buffer_space = -1; /* Indicate a failure. */
		return 0;
	}

	value = read_be16(*buffer);
	*buffer = (void *) ((uintptr_t) (*buffer) + sizeof(value));
	*buffer_space -= sizeof(value);

	return value;
}

static uint32_t unmarshal_u32(void **buffer, int *buffer_space)
{
	uint32_t value;

	if (*buffer_space < sizeof(value)) {
		*buffer_space = -1; /* Indicate a failure. */
		return 0;
	}

	value = read_be32(*buffer);
	*buffer = (void *) ((uintptr_t) (*buffer) + sizeof(value));
	*buffer_space -= sizeof(value);

	return value;
}

#define unmarshal_TPM_HANDLE(a, b) unmarshal_u32(a, b)
#define unmarshal_ALG_ID(a, b) unmarshal_u16(a, b)

static void unmarshal_TPM2B_MAX_NV_BUFFER(void **buffer,
					  int *size,
					  TPM2B_MAX_NV_BUFFER *nv_buffer)
{
	nv_buffer->t.size = unmarshal_u16(buffer, size);
	if (nv_buffer->t.size > *size) {
		VB2_DEBUG("size mismatch: expected %d, remaining %d\n",
			  nv_buffer->t.size, *size);
		return;
	}

	nv_buffer->t.buffer = *buffer;

	*buffer = ((uint8_t *)(*buffer)) + nv_buffer->t.size;
	*size -= nv_buffer->t.size;
}

static void unmarshal_authorization_section(void **buffer, int *size,
					    char *cmd_name)
{
	/*
	 * Let's ignore the authorisation section. It should be 5 bytes total,
	 * just confirm that this is the case and report any discrepancy.
	 */
	if (*size != 5)
		VB2_DEBUG("unexpected authorisation section size %d for %s\n",
			  *size, cmd_name);

	*buffer = ((uint8_t *)(*buffer)) + *size;
	*size = 0;
}

static void unmarshal_nv_read(void **buffer, int *size,
			      struct nv_read_response *nvr)
{
	/* Total size of the parameter field. */
	nvr->params_size = unmarshal_u32(buffer, size);
	unmarshal_TPM2B_MAX_NV_BUFFER(buffer, size, &nvr->buffer);

	if (nvr->params_size !=
	    (nvr->buffer.t.size + sizeof(nvr->buffer.t.size))) {
		VB2_DEBUG("parameter/buffer %d/%d size mismatch",
			  nvr->params_size, nvr->buffer.t.size);
		return;
	}

	if (*size < 0)
		return;

	unmarshal_authorization_section(buffer, size, "NV_Read");
}

static void unmarshal_TPM2B(void **buffer,
			    int *size,
			    TPM2B *tpm2b)
{
	tpm2b->size = unmarshal_u16(buffer, size);
	if (tpm2b->size > *size) {
		VB2_DEBUG("size mismatch: expected %d, remaining %d\n",
			  tpm2b->size, *size);
		*size = -1;
		return;
	}

	tpm2b->buffer = *buffer;

	*buffer = ((uint8_t *)(*buffer)) + tpm2b->size;
	*size -= tpm2b->size;
}

static void unmarshal_TPMS_NV_PUBLIC(void **buffer,
				     int *size,
				     TPMS_NV_PUBLIC *pub)
{
	int tpm2b_size = unmarshal_u16(buffer, size);
	if (tpm2b_size > *size) {
		VB2_DEBUG("size mismatch: expected %d, remaining %d\n",
			  tpm2b_size, *size);
		*size = -1;
		return;
	}
	*size -= tpm2b_size;

	pub->nvIndex = unmarshal_TPM_HANDLE(buffer, &tpm2b_size);
	pub->nameAlg = unmarshal_ALG_ID(buffer, &tpm2b_size);
	pub->attributes = unmarshal_u32(buffer, &tpm2b_size);
	unmarshal_TPM2B(buffer, &tpm2b_size, &pub->authPolicy);
	pub->dataSize = unmarshal_u16(buffer, &tpm2b_size);

	if (tpm2b_size != 0) {
		VB2_DEBUG("TPMS_NV_PUBLIC size doesn't match size field\n");
		*size = -1;
		return;
	}
}

static void unmarshal_nv_read_public(void **buffer, int *size,
				     struct nv_read_public_response *nv_pub)
{
	unmarshal_TPMS_NV_PUBLIC(buffer, size, &nv_pub->nvPublic);
	unmarshal_TPM2B(buffer, size, &nv_pub->nvName);

	if (*size > 0) {
		VB2_DEBUG("extra %d bytes after nvName\n", *size);
		*size = -1;
		return;
	}
}

static void unmarshal_TPML_TAGGED_TPM_PROPERTY(void **buffer, int *size,
					       TPML_TAGGED_TPM_PROPERTY *prop)
{
	prop->count = unmarshal_u32(buffer, size);

	if (prop->count != 1) {
		*size = -1;
		VB2_DEBUG("Request to unmarshal unsupported "
			  "number of properties: %u\n",
			  prop->count);
		return;
	}

	prop->tpm_property[0].property = unmarshal_u32(buffer, size);
	prop->tpm_property[0].value = unmarshal_u32(buffer, size);
}

static void unmarshal_TPMS_CAPABILITY_DATA(void **buffer, int *size,
					   TPMS_CAPABILITY_DATA *cap_data)
{
	cap_data->capability = unmarshal_u32(buffer, size);

	switch (cap_data->capability) {

	case TPM_CAP_TPM_PROPERTIES:
		unmarshal_TPML_TAGGED_TPM_PROPERTY(buffer, size,
						   &cap_data->data.
						       tpm_properties);
		break;

	default:
		*size = -1;
		VB2_DEBUG("Request to unmarshal unsupported capability %#x\n",
			  cap_data->capability);
	}
}

static void unmarshal_get_capability(void **buffer, int *size,
				     struct get_capability_response *cap)
{
	/* Total size of the parameter field. */
	cap->more_data = unmarshal_u8(buffer, size);
	unmarshal_TPMS_CAPABILITY_DATA(buffer, size, &cap->capability_data);
}


/*
 * Each marshaling function receives a pointer to the buffer to marshal into,
 * a pointer to the data item to be marshaled, and a pointer to the remaining
 * room in the buffer.
 */

 /*
  * Marshaling an arbitrary blob requires its size in addition to common
  * parameter set.
  */
static void marshal_blob(void **buffer, void *blob,
			 size_t blob_size, int *buffer_space)
{
	if (*buffer_space < blob_size) {
		*buffer_space = -1;
		return;
	}

	memcpy(*buffer, blob, blob_size);
	buffer_space -= blob_size;
	*buffer = (void *)((uintptr_t)(*buffer) + blob_size);
}

static void marshal_u8(void **buffer, uint8_t value, int *buffer_space)
{
	uint8_t *bp = *buffer;

	if (*buffer_space < sizeof(value)) {
		*buffer_space = -1;
		return;
	}

	*bp++ = value;
	*buffer = bp;
	*buffer_space -= sizeof(value);
}

static void marshal_u16(void **buffer, uint16_t value, int *buffer_space)
{
	if (*buffer_space < sizeof(value)) {
		*buffer_space = -1;
		return;
	}
	write_be16(*buffer, value);
	*buffer = (void *)((uintptr_t)(*buffer) + sizeof(value));
	*buffer_space -= sizeof(value);
}

static void marshal_u32(void **buffer, uint32_t value, int *buffer_space)
{
	if (*buffer_space < sizeof(value)) {
		*buffer_space = -1;
		return;
	}

	write_be32(*buffer, value);
	*buffer = (void *)((uintptr_t)(*buffer) + sizeof(value));
	*buffer_space -= sizeof(value);
}

#define unmarshal_TPM_CC(a, b) unmarshal_u32(a, b)
#define marshal_TPM_HANDLE(a, b, c) marshal_u32(a, b, c)
#define marshal_TPM_SU(a, b, c) marshal_u16(a, b, c)
#define marshal_ALG_ID(a, b, c) marshal_u16(a, b, c)

/*
 * For TPM2B* structures the size field (16 or 32 bits) goes before the data.
 * When marshaling, we first reserve the space for the size field, then
 * marshal the data, then fill the reserved size field with the actual size
 * of the marshaled data.
 */
typedef struct {
	int size;
	void *location;
} tpm2_marshal_size_field;

static void marshal_reserve_size_field(void **buffer,
				       tpm2_marshal_size_field *field,
				       int field_size,
				       int *buffer_space)
{
	if (field_size != sizeof(uint32_t) && field_size != sizeof(uint16_t)) {
		VB2_DEBUG("Unsupported size field size: %d\n", field_size);
		*buffer_space = -1;
		return;
	}
	if (*buffer_space < field_size) {
		*buffer_space = -1;
		return;
	}
	field->size = field_size;
	field->location = *buffer;
	*buffer_space -= field_size;
	*buffer = (void *)(((uintptr_t) *buffer) + field_size);
}

static void marshal_fill_size_field(void **buffer,
				    tpm2_marshal_size_field *field,
				    int include_size_field,
				    int *buffer_space)
{
	uintptr_t size = (uintptr_t) *buffer - (uintptr_t) field->location;

	if (*buffer_space < 0)
		return;  /* The structure did not fit. */

	if (!include_size_field)
		size -= field->size;
	if (field->size == sizeof(uint32_t))
		marshal_u32(&field->location, size, &field->size);
	else /* if (field->size == sizeof(uint16_t)) */
		marshal_u16(&field->location, size, &field->size);
}

static void marshal_session_header(void **buffer,
				   struct tpm2_session_header *session_header,
				   int *buffer_space)
{
	tpm2_marshal_size_field size_field;

	/* Skip room for the session header size. */
	marshal_reserve_size_field(buffer, &size_field,
				   sizeof(uint32_t), buffer_space);

	marshal_u32(buffer, session_header->session_handle, buffer_space);
	marshal_u16(buffer, session_header->nonce_size, buffer_space);
	marshal_blob(buffer, session_header->nonce,
		     session_header->nonce_size, buffer_space);
	marshal_u8(buffer, session_header->session_attrs, buffer_space);
	marshal_u16(buffer, session_header->auth_size, buffer_space);
	marshal_blob(buffer, session_header->auth,
		     session_header->auth_size, buffer_space);

	/* Paste in the session size. */
	marshal_fill_size_field(buffer, &size_field, 0, buffer_space);
}

static void marshal_TPM2B(void **buffer,
			  TPM2B *data,
			  int *buffer_space)
{
	size_t total_size = data->size + sizeof(data->size);

	if (total_size > *buffer_space) {
		*buffer_space = -1;
		return;
	}
	marshal_u16(buffer, data->size, buffer_space);
	memcpy(*buffer, data->buffer, data->size);
	*buffer = ((uint8_t *)(*buffer)) + data->size;
	*buffer_space -= data->size;
}

static void marshal_TPMS_NV_PUBLIC(void **buffer,
				   TPMS_NV_PUBLIC *data,
				   int *buffer_space)
{
	tpm2_marshal_size_field size_field;

	/* Skip room for the size. */
	marshal_reserve_size_field(buffer, &size_field,
				   sizeof(uint16_t), buffer_space);

	marshal_TPM_HANDLE(buffer, data->nvIndex, buffer_space);
	marshal_ALG_ID(buffer, data->nameAlg, buffer_space);
	marshal_u32(buffer, data->attributes, buffer_space);
	marshal_TPM2B(buffer, &data->authPolicy, buffer_space);
	marshal_u16(buffer, data->dataSize, buffer_space);

	/* Paste in the structure size. */
	marshal_fill_size_field(buffer, &size_field, 0, buffer_space);
}

static void marshal_nv_define_space(void **buffer,
				    struct tpm2_nv_define_space_cmd
				        *command_body,
				    int *buffer_space)
{
	struct tpm2_session_header session_header;

	/* Use platform authorization if PLATFORMCREATE is set, and owner
	 * authorization otherwise (per TPM2 Spec. Part 2. Section 31.3.1).
	 * Owner authorization with empty password will work only until
	 * ownership is taken. Platform authorization will work only until
	 * platform hierarchy is disabled (i.e. in firmware or in recovery
	 * mode).
	 */
	if (command_body->publicInfo.attributes & TPMA_NV_PLATFORMCREATE)
		marshal_TPM_HANDLE(buffer, TPM_RH_PLATFORM, buffer_space);
	else
		marshal_TPM_HANDLE(buffer, TPM_RH_OWNER, buffer_space);

	memset(&session_header, 0, sizeof(session_header));
	session_header.session_handle = TPM_RS_PW;
	marshal_session_header(buffer, &session_header, buffer_space);
	tpm_tag = TPM_ST_SESSIONS;

	marshal_TPM2B(buffer, &command_body->auth, buffer_space);
	marshal_TPMS_NV_PUBLIC(buffer, &command_body->publicInfo, buffer_space);
}

/* Determine which authorization should be used when writing or write-locking
 * an NV index.
 *
 * Use a simplified approach:
 * 1) Use platform auth for indexes defined by TPM and Platform, as
 * specified in "Registry of reserved TPM 2.0 handles and localities".
 * That will only work for indexes with PPWRITE, and until the platform
 * hierarchy is disabled.
 * 2) Use empty password auth for other indexes.
 * That will only work for indexes with AUTHWRITE and empty auth value.
 *
 * A more honest approach would require the caller to specify the
 * authorization, or would check the NV index attributes.
 * But that's not needed now, as all indexes defined by firmware are
 * in the TPM range and have PPWRITE. The indexes defined by the
 * OS are in the Owner range and have either OWNERWRITE or AUTHWRITE,
 * but we don't ever use Tlcl to write to OWNERWRITE indexes.
 */
static TPM_HANDLE get_nv_index_write_auth(TPMI_RH_NV_INDEX nvIndex)
{
	return nvIndex >= TPMI_RH_NV_INDEX_OWNER_START ?
	       nvIndex :
	       TPM_RH_PLATFORM;
}

static void marshal_nv_write(void **buffer,
			     struct tpm2_nv_write_cmd *command_body,
			     int *buffer_space)
{
	struct tpm2_session_header session_header;

	marshal_TPM_HANDLE(buffer,
			   get_nv_index_write_auth(command_body->nvIndex),
			   buffer_space);
	marshal_TPM_HANDLE(buffer, command_body->nvIndex, buffer_space);
	memset(&session_header, 0, sizeof(session_header));
	session_header.session_handle = TPM_RS_PW;
	marshal_session_header(buffer, &session_header, buffer_space);
	tpm_tag = TPM_ST_SESSIONS;

	marshal_TPM2B(buffer, &command_body->data.b, buffer_space);
	marshal_u16(buffer, command_body->offset, buffer_space);
}

static void marshal_nv_read(void **buffer,
			    struct tpm2_nv_read_cmd *command_body,
			    int *buffer_space)
{
	struct tpm2_session_header session_header;

	/* Use empty password auth if platform hierarchy is disabled */
	if (ph_disabled)
		marshal_TPM_HANDLE(buffer, command_body->nvIndex, buffer_space);
	else
		marshal_TPM_HANDLE(buffer, TPM_RH_PLATFORM, buffer_space);
	marshal_TPM_HANDLE(buffer, command_body->nvIndex, buffer_space);
	memset(&session_header, 0, sizeof(session_header));
	session_header.session_handle = TPM_RS_PW;
	marshal_session_header(buffer, &session_header, buffer_space);
	tpm_tag = TPM_ST_SESSIONS;
	marshal_u16(buffer, command_body->size, buffer_space);
	marshal_u16(buffer, command_body->offset, buffer_space);
}

static void marshal_nv_read_lock(void **buffer,
				  struct tpm2_nv_read_lock_cmd *command_body,
				  int *buffer_space)
{
	struct tpm2_session_header session_header;

	tpm_tag = TPM_ST_SESSIONS;
	marshal_TPM_HANDLE(buffer, command_body->nvIndex, buffer_space);
	marshal_TPM_HANDLE(buffer, command_body->nvIndex, buffer_space);
	memset(&session_header, 0, sizeof(session_header));
	session_header.session_handle = TPM_RS_PW;
	marshal_session_header(buffer, &session_header, buffer_space);
}

static void marshal_nv_write_lock(void **buffer,
				  struct tpm2_nv_write_lock_cmd *command_body,
				  int *buffer_space)
{
	struct tpm2_session_header session_header;

	tpm_tag = TPM_ST_SESSIONS;
	marshal_TPM_HANDLE(buffer,
			   get_nv_index_write_auth(command_body->nvIndex),
			   buffer_space);
	marshal_TPM_HANDLE(buffer, command_body->nvIndex, buffer_space);
	memset(&session_header, 0, sizeof(session_header));
	session_header.session_handle = TPM_RS_PW;
	marshal_session_header(buffer, &session_header, buffer_space);
}

static void marshal_nv_read_public(void **buffer,
				   struct tpm2_nv_read_public_cmd *command_body,
				   int *buffer_space)
{
	tpm_tag = TPM_ST_NO_SESSIONS;

	marshal_TPM_HANDLE(buffer, command_body->nvIndex, buffer_space);
}

static void marshal_hierarchy_control(void **buffer,
				      struct tpm2_hierarchy_control_cmd
				          *command_body,
				      int *buffer_space)
{
	struct tpm2_session_header session_header;

	tpm_tag = TPM_ST_SESSIONS;
	marshal_TPM_HANDLE(buffer, TPM_RH_PLATFORM, buffer_space);
	memset(&session_header, 0, sizeof(session_header));
	session_header.session_handle = TPM_RS_PW;
	marshal_session_header(buffer, &session_header, buffer_space);

	marshal_TPM_HANDLE(buffer, command_body->enable, buffer_space);
	marshal_u8(buffer, command_body->state, buffer_space);
}

static void marshal_get_capability(void **buffer,
				   struct tpm2_get_capability_cmd
				       *command_body,
				   int *buffer_space)
{
	tpm_tag = TPM_ST_NO_SESSIONS;

	marshal_u32(buffer, command_body->capability, buffer_space);
	marshal_u32(buffer, command_body->property, buffer_space);
	marshal_u32(buffer, command_body->property_count, buffer_space);
}

static void marshal_clear(void **buffer,
			  void *command_body,
			  int *buffer_space)
{
	struct tpm2_session_header session_header;

	tpm_tag = TPM_ST_SESSIONS;
	marshal_TPM_HANDLE(buffer, TPM_RH_PLATFORM, buffer_space);
	memset(&session_header, 0, sizeof(session_header));
	session_header.session_handle = TPM_RS_PW;
	marshal_session_header(buffer, &session_header, buffer_space);
}

static void marshal_self_test(void **buffer,
			      struct tpm2_self_test_cmd *command_body,
			      int *buffer_space)
{
	tpm_tag = TPM_ST_NO_SESSIONS;

	marshal_u8(buffer, command_body->full_test, buffer_space);
}

static void marshal_startup(void **buffer,
			    struct tpm2_startup_cmd *command_body,
			    int *buffer_space)
{
	tpm_tag = TPM_ST_NO_SESSIONS;

	marshal_TPM_SU(buffer, command_body->startup_type, buffer_space);
}

static void marshal_shutdown(void **buffer,
			     struct tpm2_shutdown_cmd *command_body,
			     int *buffer_space)
{
	tpm_tag = TPM_ST_NO_SESSIONS;

	marshal_TPM_SU(buffer, command_body->shutdown_type, buffer_space);
}

int tpm_marshal_command(TPM_CC command, void *tpm_command_body,
			void *buffer, int buffer_size)
{
	void *cmd_body = (uint8_t *)buffer + sizeof(struct tpm_header);
	int max_body_size = buffer_size - sizeof(struct tpm_header);
	int body_size = max_body_size;

	/* Will be modified when marshaling some commands. */
	tpm_tag = TPM_ST_NO_SESSIONS;

	switch (command) {

	case TPM2_NV_DefineSpace:
		marshal_nv_define_space(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_NV_Read:
		marshal_nv_read(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_NV_Write:
		marshal_nv_write(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_NV_ReadLock:
		marshal_nv_read_lock(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_NV_WriteLock:
		marshal_nv_write_lock(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_NV_ReadPublic:
		marshal_nv_read_public(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_Hierarchy_Control:
		marshal_hierarchy_control(&cmd_body,
					  tpm_command_body, &body_size);
		break;

	case TPM2_GetCapability:
		marshal_get_capability(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_Clear:
		marshal_clear(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_SelfTest:
		marshal_self_test(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_Startup:
		marshal_startup(&cmd_body, tpm_command_body, &body_size);
		break;

	case TPM2_Shutdown:
		marshal_shutdown(&cmd_body, tpm_command_body, &body_size);
		break;

	default:
		body_size = -1;
		VB2_DEBUG("Request to marshal unsupported command %#x\n",
			  command);
	}

	if (body_size > 0) {

		/* See how much room was taken by marshaling. */
		body_size = max_body_size - body_size;

		body_size += sizeof(struct tpm_header);

		marshal_u16(&buffer, tpm_tag, &max_body_size);
		marshal_u32(&buffer, body_size, &max_body_size);
		marshal_u32(&buffer, command, &max_body_size);
	}

	return body_size;
}

int tpm_unmarshal_response(TPM_CC command,
			   void *response_body,
			   int cr_size,
			   struct tpm2_response *response)
{
	if (cr_size < sizeof(struct tpm_header))
		return -1;

	response->hdr.tpm_tag = unmarshal_u16(&response_body, &cr_size);
	response->hdr.tpm_size = unmarshal_u32(&response_body, &cr_size);
	response->hdr.tpm_code = unmarshal_TPM_CC(&response_body, &cr_size);

	if (!cr_size) {
		if (response->hdr.tpm_size != sizeof(response->hdr))
			VB2_DEBUG("size mismatch in response to command %#x\n",
				  command);
		return 0;
	}

	switch (command) {
	case TPM2_NV_Read:
		unmarshal_nv_read(&response_body, &cr_size,
				  &response->nvr);
		break;

	case TPM2_NV_ReadPublic:
		unmarshal_nv_read_public(&response_body, &cr_size,
				         &response->nv_read_public);
		break;

	case TPM2_GetCapability:
		unmarshal_get_capability(&response_body, &cr_size,
					 &response->cap);
		break;

	case TPM2_Hierarchy_Control:
	case TPM2_NV_Write:
	case TPM2_NV_WriteLock:
	case TPM2_NV_ReadLock:
	case TPM2_Clear:
	case TPM2_SelfTest:
	case TPM2_Startup:
	case TPM2_Shutdown:
	case TPM2_NV_DefineSpace:
		/* Session data included in response can be safely ignored. */
		cr_size = 0;
		break;

	default:
		{
			int i;

			VB2_DEBUG("Request to unmarshal unexpected command %#x,"
				  " code %#x",
				  command,
				  response->hdr.tpm_code);

			for (i = 0; i < cr_size; i++) {
				if (!(i % 16))
					VB2_DEBUG_RAW("\n");
				VB2_DEBUG_RAW("%2.2x ",
					      ((uint8_t *)response_body)[i]);
			}
		}
		VB2_DEBUG("\n");
		return -1;
	}

	if (cr_size) {
		VB2_DEBUG("got %d bytes back in response to %#x,"
			  " failed to parse (%d)\n",
			  response->hdr.tpm_size,
			  command, cr_size);
		return -1;
	}

	/* The entire message have been parsed. */
	return 0;
}

uint32_t tpm_get_packet_size(const uint8_t *packet)
{
	/* 0: tag (16 bit)
	 * 2: size (32 bit)
	 */
	return read_be32(packet + 2);
}

uint32_t tpm_get_packet_response_code(const uint8_t *packet)
{
	/* 0: tag (16 bit)
	 * 2: size (32 bit)
	 * 6: resp code (32 bit)
	 */
	return read_be32(packet + 6);
}

void tpm_set_ph_disabled(int flag)
{
	ph_disabled = flag;
}

int tpm_is_ph_disabled(void)
{
	return ph_disabled;
}
