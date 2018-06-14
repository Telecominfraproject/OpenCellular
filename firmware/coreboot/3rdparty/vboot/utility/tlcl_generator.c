/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* This program generates partially filled TPM datagrams and other compile-time
 * constants (e.g. structure sizes and offsets).  Compile this file---and ONLY
 * this file---with -fpack-struct.  We take advantage of the fact that the
 * (packed) TPM structures layout (mostly) match the TPM request and response
 * datagram layout.  When they don't completely match, some fixing is necessary
 * (see PCR_SELECTION_FIX below).
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "sysincludes.h"
#include "tlcl_internal.h"
#include "tpmextras.h"
#include "tss_constants.h"

/* See struct Command below.  This structure represent a field in a TPM
 * command.  [name] is the field name.  [visible] is 1 if the field is
 * modified by the run-time.  Non-visible fields are initialized at build time
 * and remain constant.  [size] is the field size in bytes.  [value] is the
 * fixed value of non-visible fields.
 */
typedef struct Field {
  const char* name;
  int visible;
  int offset;
  int size;
  uint32_t value;     /* large enough for all initializers */
  struct Field* next;
} Field;

/* This structure is used to build (at build time) and manipulate (at firmware
 * or emulation run time) buffers containing TPM datagrams.  [name] is the name
 * of a TPM command.  [size] is the size of the command buffer in bytes, when
 * known.  [max_size] is the maximum size allowed for variable-length commands
 * (such as Read and Write).  [fields] is a link-list of command fields.
 */
typedef struct Command {
  const char* name;
  int size;
  int max_size;
  Field* fields;
  struct Command* next;
} Command;

/* Adds a field to a command, and makes its offset visible.  The fields must be
 * added at increasing offsets.
 */
static void AddVisibleField(Command* cmd, const char* name, int offset) {
  Field* fld = (Field*) calloc(1, sizeof(Field));
  if (cmd->fields != NULL) {
    assert(offset > fn->offset);
  }
  fld->next = cmd->fields;
  cmd->fields = fld;
  fld->name = name;
  fld->visible = 1;
  fld->offset = offset;
}

/* Adds a constant field with its value.  The fields must be added at
 * increasing offsets.
 */
static void AddInitializedField(Command* cmd, int offset,
                                int size, uint32_t value) {
  Field* fld = (Field*) calloc(1, sizeof(Field));
  fld->next = cmd->fields;
  cmd->fields = fld;
  fld->name = NULL;
  fld->visible = 0;
  fld->size = size;
  fld->offset = offset;
  fld->value = value;
}

/* Create a structure representing a TPM command datagram.
 */
Command* newCommandWithTag(TPM_COMMAND_CODE code, int size, TPM_TAG tag) {
  Command* cmd = (Command*) calloc(1, sizeof(Command));
  cmd->size = size;
  AddInitializedField(cmd, 0, sizeof(TPM_TAG), tag);
  AddInitializedField(cmd, sizeof(TPM_TAG), sizeof(uint32_t), size);
  AddInitializedField(cmd, sizeof(TPM_TAG) + sizeof(uint32_t),
                      sizeof(TPM_COMMAND_CODE), code);
  return cmd;
}

Command* newCommand(TPM_COMMAND_CODE code, int size) {
  return newCommandWithTag(code, size, TPM_TAG_RQU_COMMAND);
}

/* BuildXXX builds TPM command XXX.
 */
Command* BuildDefineSpaceCommand(void) {
  int nv_data_public = kTpmRequestHeaderLength;
  int nv_index = nv_data_public + offsetof(TPM_NV_DATA_PUBLIC, nvIndex);
  int nv_pcr_info_read = nv_data_public +
    offsetof(TPM_NV_DATA_PUBLIC, pcrInfoRead);
  int read_locality = nv_pcr_info_read +
    offsetof(TPM_PCR_INFO_SHORT, localityAtRelease);
  int nv_pcr_info_write = nv_data_public +
    offsetof(TPM_NV_DATA_PUBLIC, pcrInfoWrite);
  int write_locality = nv_pcr_info_write +
    offsetof(TPM_PCR_INFO_SHORT, localityAtRelease);
  int nv_permission = nv_data_public +
    offsetof(TPM_NV_DATA_PUBLIC, permission);
  int nv_permission_tag =
    nv_permission + offsetof(TPM_NV_ATTRIBUTES, tag);
  int nv_permission_attributes =
    nv_permission + offsetof(TPM_NV_ATTRIBUTES, attributes);
  int nv_datasize = nv_data_public +
    offsetof(TPM_NV_DATA_PUBLIC, dataSize);

  int size = kTpmRequestHeaderLength + sizeof(TPM_NV_DATA_PUBLIC) +
      kEncAuthLength;
  Command* cmd = newCommand(TPM_ORD_NV_DefineSpace, size);
  cmd->name = "tpm_nv_definespace_cmd";

  AddVisibleField(cmd, "index", nv_index);
  AddVisibleField(cmd, "pcr_info_read", nv_pcr_info_read);
  AddVisibleField(cmd, "pcr_info_write", nv_pcr_info_write);
  AddVisibleField(cmd, "perm", nv_permission_attributes);
  AddVisibleField(cmd, "size", nv_datasize);

  AddInitializedField(cmd, nv_data_public, sizeof(uint16_t),
                      TPM_TAG_NV_DATA_PUBLIC);
  AddInitializedField(cmd, nv_pcr_info_read, sizeof(uint16_t), 3);
  AddInitializedField(cmd, read_locality, sizeof(TPM_LOCALITY_SELECTION),
                      TPM_ALL_LOCALITIES);
  AddInitializedField(cmd, nv_pcr_info_write, sizeof(uint16_t), 3);
  AddInitializedField(cmd, write_locality, sizeof(TPM_LOCALITY_SELECTION),
                      TPM_ALL_LOCALITIES);
  AddInitializedField(cmd, nv_permission_tag, sizeof(TPM_STRUCTURE_TAG),
                      TPM_TAG_NV_ATTRIBUTES);
  return cmd;
}

/* BuildXXX builds TPM command XXX.
 */
Command* BuildWriteCommand(void) {
  Command* cmd = newCommand(TPM_ORD_NV_WriteValue, 0);
  cmd->name = "tpm_nv_write_cmd";
  cmd->max_size = TPM_LARGE_ENOUGH_COMMAND_SIZE;
  AddVisibleField(cmd, "index", kTpmRequestHeaderLength);
  AddVisibleField(cmd, "length", kTpmRequestHeaderLength + 8);
  AddVisibleField(cmd, "data", kTpmRequestHeaderLength + 12);
  return cmd;
}

Command* BuildReadCommand(void) {
  int size = kTpmRequestHeaderLength + kTpmReadInfoLength;
  Command* cmd = newCommand(TPM_ORD_NV_ReadValue, size);
  cmd->name = "tpm_nv_read_cmd";
  AddVisibleField(cmd, "index", kTpmRequestHeaderLength);
  AddVisibleField(cmd, "length", kTpmRequestHeaderLength + 8);
  return cmd;
}

Command* BuildPCRReadCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(uint32_t);
  Command* cmd = newCommand(TPM_ORD_PcrRead, size);
  cmd->name = "tpm_pcr_read_cmd";
  AddVisibleField(cmd, "pcrNum", kTpmRequestHeaderLength);
  return cmd;
}

Command* BuildPPAssertCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(TPM_PHYSICAL_PRESENCE);
  Command* cmd = newCommand(TSC_ORD_PhysicalPresence, size);
  cmd->name = "tpm_ppassert_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_PHYSICAL_PRESENCE),
                      TPM_PHYSICAL_PRESENCE_PRESENT);
  return cmd;
}

Command* BuildPPEnableCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(TPM_PHYSICAL_PRESENCE);
  Command* cmd = newCommand(TSC_ORD_PhysicalPresence, size);
  cmd->name = "tpm_ppenable_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_PHYSICAL_PRESENCE),
                      TPM_PHYSICAL_PRESENCE_CMD_ENABLE);
  return cmd;
}

Command* BuildFinalizePPCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(TPM_PHYSICAL_PRESENCE);
  Command* cmd = newCommand(TSC_ORD_PhysicalPresence, size);
  cmd->name = "tpm_finalizepp_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_PHYSICAL_PRESENCE),
                      TPM_PHYSICAL_PRESENCE_CMD_ENABLE |
                      TPM_PHYSICAL_PRESENCE_HW_DISABLE |
                      TPM_PHYSICAL_PRESENCE_LIFETIME_LOCK);
  return cmd;
}

Command* BuildPPLockCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(TPM_PHYSICAL_PRESENCE);
  Command* cmd = newCommand(TSC_ORD_PhysicalPresence, size);
  cmd->name = "tpm_pplock_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_PHYSICAL_PRESENCE),
                      TPM_PHYSICAL_PRESENCE_LOCK);
  return cmd;
}

Command* BuildStartupCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(TPM_STARTUP_TYPE);
  Command* cmd = newCommand(TPM_ORD_Startup, size);
  cmd->name = "tpm_startup_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_STARTUP_TYPE),
                      TPM_ST_CLEAR);
  return cmd;
}

Command* BuildSaveStateCommand(void) {
  int size = kTpmRequestHeaderLength;
  Command* cmd = newCommand(TPM_ORD_SaveState, size);
  cmd->name = "tpm_savestate_cmd";
  return cmd;
}

Command* BuildResumeCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(TPM_STARTUP_TYPE);
  Command* cmd = newCommand(TPM_ORD_Startup, size);
  cmd->name = "tpm_resume_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_STARTUP_TYPE),
                      TPM_ST_STATE);
  return cmd;
}

Command* BuildSelftestfullCommand(void) {
  int size = kTpmRequestHeaderLength;
  Command* cmd = newCommand(TPM_ORD_SelfTestFull, size);
  cmd->name = "tpm_selftestfull_cmd";
  return cmd;
}

Command* BuildContinueSelfTestCommand(void) {
  int size = kTpmRequestHeaderLength;
  Command* cmd = newCommand(TPM_ORD_ContinueSelfTest, size);
  cmd->name = "tpm_continueselftest_cmd";
  return cmd;
}

Command* BuildReadPubekCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(TPM_NONCE);
  Command* cmd = newCommand(TPM_ORD_ReadPubek, size);
  cmd->name = "tpm_readpubek_cmd";
  AddVisibleField(cmd, "antiReplay", kTpmRequestHeaderLength);
  return cmd;
}

Command* BuildForceClearCommand(void) {
  int size = kTpmRequestHeaderLength;
  Command* cmd = newCommand(TPM_ORD_ForceClear, size);
  cmd->name = "tpm_forceclear_cmd";
  return cmd;
}

Command* BuildPhysicalEnableCommand(void) {
  int size = kTpmRequestHeaderLength;
  Command* cmd = newCommand(TPM_ORD_PhysicalEnable, size);
  cmd->name = "tpm_physicalenable_cmd";
  return cmd;
}

Command* BuildPhysicalDisableCommand(void) {
  int size = kTpmRequestHeaderLength;
  Command* cmd = newCommand(TPM_ORD_PhysicalDisable, size);
  cmd->name = "tpm_physicaldisable_cmd";
  return cmd;
}

Command* BuildPhysicalSetDeactivatedCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(uint8_t);
  Command* cmd = newCommand(TPM_ORD_PhysicalSetDeactivated, size);
  cmd->name = "tpm_physicalsetdeactivated_cmd";
  AddVisibleField(cmd, "deactivated", kTpmRequestHeaderLength);
  return cmd;
}

Command* BuildExtendCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(uint32_t) + kPcrDigestLength;
  Command* cmd = newCommand(TPM_ORD_Extend, size);
  cmd->name = "tpm_extend_cmd";
  AddVisibleField(cmd, "pcrNum", kTpmRequestHeaderLength);
  AddVisibleField(cmd, "inDigest", kTpmRequestHeaderLength + sizeof(uint32_t));
  return cmd;
}

Command* BuildGetFlagsCommand(void) {
  int size = (kTpmRequestHeaderLength +
              sizeof(TPM_CAPABILITY_AREA) +   /* capArea */
              sizeof(uint32_t) +              /* subCapSize */
              sizeof(uint32_t));              /* subCap */

  Command* cmd = newCommand(TPM_ORD_GetCapability, size);
  cmd->name = "tpm_getflags_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_CAPABILITY_AREA), TPM_CAP_FLAG);
  AddInitializedField(cmd, kTpmRequestHeaderLength +
                      sizeof(TPM_CAPABILITY_AREA),
                      sizeof(uint32_t), sizeof(uint32_t));
  AddInitializedField(cmd, kTpmRequestHeaderLength +
                      sizeof(TPM_CAPABILITY_AREA) + sizeof(uint32_t),
                      sizeof(uint32_t), TPM_CAP_FLAG_PERMANENT);
  return cmd;
}

Command* BuildGetSTClearFlagsCommand(void) {
  int size = (kTpmRequestHeaderLength +
              sizeof(TPM_CAPABILITY_AREA) +   /* capArea */
              sizeof(uint32_t) +              /* subCapSize */
              sizeof(uint32_t));              /* subCap */

  Command* cmd = newCommand(TPM_ORD_GetCapability, size);
  cmd->name = "tpm_getstclearflags_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_CAPABILITY_AREA), TPM_CAP_FLAG);
  AddInitializedField(cmd, kTpmRequestHeaderLength +
                      sizeof(TPM_CAPABILITY_AREA),
                      sizeof(uint32_t), sizeof(uint32_t));
  AddInitializedField(cmd, kTpmRequestHeaderLength +
                      sizeof(TPM_CAPABILITY_AREA) + sizeof(uint32_t),
                      sizeof(uint32_t), TPM_CAP_FLAG_VOLATILE);
  return cmd;
}

Command* BuildGetSpaceInfoCommand(void) {
  int size = (kTpmRequestHeaderLength +
              sizeof(TPM_CAPABILITY_AREA) +   /* capArea */
              sizeof(uint32_t) +              /* subCapSize */
              sizeof(uint32_t));              /* subCap */

  Command* cmd = newCommand(TPM_ORD_GetCapability, size);
  cmd->name = "tpm_getspaceinfo_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_CAPABILITY_AREA), TPM_CAP_NV_INDEX);
  AddInitializedField(cmd, kTpmRequestHeaderLength +
                      sizeof(TPM_CAPABILITY_AREA),
                      sizeof(uint32_t), sizeof(uint32_t));
  AddVisibleField(cmd, "index", kTpmRequestHeaderLength +
                  sizeof(TPM_CAPABILITY_AREA) + sizeof(uint32_t));
  return cmd;
}

Command* BuildGetOwnershipCommand(void) {
  int size = (kTpmRequestHeaderLength +
              sizeof(TPM_CAPABILITY_AREA) +   /* capArea */
              sizeof(uint32_t) +              /* subCapSize */
              sizeof(uint32_t));              /* subCap */

  Command* cmd = newCommand(TPM_ORD_GetCapability, size);
  cmd->name = "tpm_getownership_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_CAPABILITY_AREA), TPM_CAP_PROPERTY);
  AddInitializedField(cmd, kTpmRequestHeaderLength +
                      sizeof(TPM_CAPABILITY_AREA),
                      sizeof(uint32_t), sizeof(uint32_t));
  AddInitializedField(cmd, kTpmRequestHeaderLength +
                      sizeof(TPM_CAPABILITY_AREA) + sizeof(uint32_t),
                      sizeof(uint32_t), TPM_CAP_PROP_OWNER);
  return cmd;
}

Command* BuildGetRandomCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(uint32_t);
  Command* cmd = newCommand(TPM_ORD_GetRandom, size);
  cmd->name = "tpm_get_random_cmd";
  AddVisibleField(cmd, "bytesRequested", kTpmRequestHeaderLength);
  return cmd;
}

Command* BuildGetVersionValCommand(void) {
  int size = (kTpmRequestHeaderLength +
              sizeof(TPM_CAPABILITY_AREA) +   /* capArea */
              sizeof(uint32_t));              /* subCapSize */

  Command* cmd = newCommand(TPM_ORD_GetCapability, size);
  cmd->name = "tpm_getversionval_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_CAPABILITY_AREA), TPM_CAP_GET_VERSION_VAL);
  AddInitializedField(cmd, kTpmRequestHeaderLength +
                      sizeof(TPM_CAPABILITY_AREA),
                      sizeof(uint32_t), 0);
  return cmd;
}

Command* BuildIFXFieldUpgradeInfoRequest2Command(void) {
  int size = (kTpmRequestHeaderLength +
              sizeof(TPM_IFX_FieldUpgradeInfoRequest2) +
              sizeof(uint16_t));
  Command* cmd = newCommand(TPM_ORD_FieldUpgrade, size);
  cmd->name = "tpm_ifx_fieldupgradeinforequest2_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength,
                      sizeof(TPM_IFX_FieldUpgradeInfoRequest2),
                      TPM_IFX_FieldUpgradeInfoRequest2);
  AddInitializedField(cmd, kTpmRequestHeaderLength +
                      sizeof(TPM_IFX_FieldUpgradeInfoRequest2),
                      sizeof(uint16_t), 0);
  return cmd;
}

Command* BuildOIAPCommand(void) {
  int size = kTpmRequestHeaderLength;
  Command* cmd = newCommand(TPM_ORD_OIAP, size);
  cmd->name = "tpm_oiap_cmd";
  return cmd;
}

Command* BuildOSAPCommand(void) {
  int size = kTpmRequestHeaderLength + sizeof(uint16_t) + sizeof(uint32_t) +
             sizeof(TPM_NONCE);
  Command* cmd = newCommand(TPM_ORD_OSAP, size);
  cmd->name = "tpm_osap_cmd";
  AddVisibleField(cmd, "entityType", kTpmRequestHeaderLength);
  AddVisibleField(cmd, "entityValue",
                  kTpmRequestHeaderLength + sizeof(uint16_t));
  AddVisibleField(
      cmd, "nonceOddOSAP",
      kTpmRequestHeaderLength + sizeof(uint16_t) + sizeof(uint32_t));
  return cmd;
}

Command* BuildTakeOwnershipCommand(void) {
  Command* cmd = newCommandWithTag(TPM_ORD_TakeOwnership, 624,
                                   TPM_TAG_RQU_AUTH1_COMMAND);
  cmd->name = "tpm_takeownership_cmd";
  int offset = kTpmRequestHeaderLength;
  AddInitializedField(cmd, offset, sizeof(uint16_t), TPM_PID_OWNER);
  offset += sizeof(uint16_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t), TPM_RSA_2048_LEN);
  offset += sizeof(uint32_t);
  AddVisibleField(cmd, "encOwnerAuth", offset);
  offset += sizeof(uint8_t[TPM_RSA_2048_LEN]);
  AddInitializedField(cmd, offset, sizeof(uint32_t), TPM_RSA_2048_LEN);
  offset += sizeof(uint32_t);
  AddVisibleField(cmd, "encSrkAuth", offset);
  offset += sizeof(uint8_t[TPM_RSA_2048_LEN]);

  /* The remainder are the srkParams struct TPM_KEY12 contents. */
  AddInitializedField(cmd, offset, sizeof(uint16_t), TPM_TAG_KEY12);
  offset += sizeof(uint16_t);
  AddInitializedField(cmd, offset, sizeof(uint16_t), 0);
  offset += sizeof(uint16_t);
  AddInitializedField(cmd, offset, sizeof(uint16_t), TPM_KEY_USAGE_STORAGE);
  offset += sizeof(uint16_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t), 0 /* keyFlags */);
  offset += sizeof(uint32_t);
  AddInitializedField(cmd, offset, sizeof(uint8_t), TPM_AUTH_ALWAYS);
  offset += sizeof(uint8_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t), TPM_ALG_RSA);
  offset += sizeof(uint32_t);
  AddInitializedField(cmd, offset, sizeof(uint16_t),
                      TPM_ES_RSAESOAEP_SHA1_MGF1);
  offset += sizeof(uint16_t);
  AddInitializedField(cmd, offset, sizeof(uint16_t), TPM_SS_NONE);
  offset += sizeof(uint16_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t),
                      3 * sizeof(uint32_t) /* algorithmParams.parmSize */);
  offset += sizeof(uint32_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t),
                      2048 /* algorithmParms.parms.keyLength */);
  offset += sizeof(uint32_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t),
                      2 /* algorithmParms.parms.numPrimes */);
  offset += sizeof(uint32_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t),
                      0 /* algorithmParms.parms.exponentSize */);
  offset += sizeof(uint32_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t), 0 /* PCRInfoSize */);
  offset += sizeof(uint32_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t), 0 /* pubkey.keyLength */);
  offset += sizeof(uint32_t);
  AddInitializedField(cmd, offset, sizeof(uint32_t), 0 /* encDataSize */);
  offset += sizeof(uint32_t);

  /* Allocate space for the auth block. */
  offset += kTpmRequestAuthBlockLength;

  assert(offset == cmd->size);

  return cmd;
}

Command* BuildCreateDelegationFamilyCommand(void) {
  int size = kTpmRequestHeaderLength + 3 * sizeof(uint32_t) + sizeof(uint8_t);
  Command* cmd = newCommand(TPM_ORD_Delegate_Manage, size);
  cmd->name = "tpm_create_delegation_family_cmd";
  AddInitializedField(cmd, kTpmRequestHeaderLength, sizeof(uint32_t),
                      0 /* familyID */);
  AddInitializedField(cmd, kTpmRequestHeaderLength + sizeof(uint32_t),
                      sizeof(uint32_t), TPM_FAMILY_CREATE);
  AddInitializedField(cmd, kTpmRequestHeaderLength + 2 * sizeof(uint32_t),
                      sizeof(uint32_t), sizeof(uint8_t) /* opDataSize */);
  AddVisibleField(cmd, "familyLabel",
                  kTpmRequestHeaderLength + 3 * sizeof(uint32_t));
  return cmd;
}

Command* BuildReadDelegationFamilyTableCommand(void) {
  Command* cmd =
      newCommand(TPM_ORD_Delegate_ReadTable, kTpmRequestHeaderLength);
  cmd->name = "tpm_delegate_read_table_cmd";
  return cmd;
}

/* Output the fields of a structure.
 */
void OutputFields(Field* fld) {
  /*
   * Field order is reversed.
   */
  if (fld != NULL) {
    OutputFields(fld->next);
    if (fld->visible) {
      printf("  uint16_t %s;\n", fld->name);
    }
  }
}

/* Outputs a structure initializer.
 */
int OutputBytes_(Command* cmd, Field* fld) {
  int cursor = 0;
  int i;
  /*
   * Field order is reversed.
   */
  if (fld != NULL) {
    cursor = OutputBytes_(cmd, fld->next);
  } else {
    return 0;
  }
  if (!fld->visible) {
    /*
     * Catch up missing fields.
     */
    assert(fld->offset >= cursor);
    for (i = 0; i < fld->offset - cursor; i++) {
      printf("0, ");
    }
    cursor = fld->offset;
    switch (fld->size) {
    case 1:
      printf("0x%x, ", fld->value);
      cursor += 1;
      break;
    case 2:
      printf("0x%x, 0x%x, ", fld->value >> 8, fld->value & 0xff);
      cursor += 2;
      break;
    case 4:
      printf("0x%x, 0x%x, 0x%x, 0x%x, ", fld->value >> 24,
             (fld->value >> 16) & 0xff,
             (fld->value >> 8) & 0xff,
             fld->value & 0xff);
      cursor += 4;
      break;
    default:
      fprintf(stderr, "invalid field size %d\n", fld->size);
      exit(1);
      break;
    }
  }
  return cursor;
}

/* Helper to output a structure initializer.
 */
void OutputBytes(Command* cmd) {
  (void) OutputBytes_(cmd, cmd->fields);
}

void OutputFieldPointers(Command* cmd, Field* fld) {
  if (fld == NULL) {
    return;
  } else {
    OutputFieldPointers(cmd, fld->next);
    if (fld->visible) {
      printf("%d, ", fld->offset);
    }
  }
}

/* Outputs the structure initializers for all commands.
 */
void OutputCommands(Command* cmd) {
  if (cmd == NULL) {
    return;
  } else {
    printf("const struct s_%s{\n  uint8_t buffer[%d];\n",
           cmd->name, cmd->size == 0 ? cmd->max_size : cmd->size);
    OutputFields(cmd->fields);
    printf("} %s = {{", cmd->name);
    OutputBytes(cmd);
    printf("},\n");
    OutputFieldPointers(cmd, cmd->fields);
    printf("};\n\n");
  }
  OutputCommands(cmd->next);
}

Command* (*builders[])(void) = {
  BuildDefineSpaceCommand,
  BuildWriteCommand,
  BuildReadCommand,
  BuildPCRReadCommand,
  BuildPPAssertCommand,
  BuildPPEnableCommand,
  BuildPPLockCommand,
  BuildFinalizePPCommand,
  BuildStartupCommand,
  BuildSaveStateCommand,
  BuildResumeCommand,
  BuildSelftestfullCommand,
  BuildContinueSelfTestCommand,
  BuildReadPubekCommand,
  BuildForceClearCommand,
  BuildPhysicalDisableCommand,
  BuildPhysicalEnableCommand,
  BuildPhysicalSetDeactivatedCommand,
  BuildGetFlagsCommand,
  BuildGetSTClearFlagsCommand,
  BuildGetSpaceInfoCommand,
  BuildGetOwnershipCommand,
  BuildGetRandomCommand,
  BuildExtendCommand,
  BuildGetVersionValCommand,
  BuildIFXFieldUpgradeInfoRequest2Command,
  BuildOIAPCommand,
  BuildOSAPCommand,
  BuildTakeOwnershipCommand,
  BuildCreateDelegationFamilyCommand,
  BuildReadDelegationFamilyTableCommand,
};

static void FreeFields(Field* fld) {
  if (fld != NULL) {
    Field* next_field = fld->next;
    free(fld);
    FreeFields(next_field);
  }
}

static void FreeCommands(Command* cmd) {
  if (cmd != NULL) {
    Command* next_command = cmd->next;
    FreeFields(cmd->fields);
    free(cmd);
    FreeCommands(next_command);
  }
}

int main(void) {
  Command* commands = NULL;
  int i;
  for (i = 0; i < sizeof(builders) / sizeof(builders[0]); i++) {
    Command* cmd = builders[i]();
    cmd->next = commands;
    commands = cmd;
  }

  printf("/* This file is automatically generated */\n\n");
  OutputCommands(commands);
  printf("const int kWriteInfoLength = %d;\n", (int) sizeof(TPM_WRITE_INFO));

  FreeCommands(commands);
  return 0;
}
