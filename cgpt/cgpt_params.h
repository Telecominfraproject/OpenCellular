// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VBOOT_REFERENCE_CGPT_CGPT_PARAMS_H_
#define VBOOT_REFERENCE_CGPT_CGPT_PARAMS_H_

#include "cgpt.h"

// This file defines the internal methods that use the user-mode cgpt programatically.
// This is the interface for the callers such as the cgpt tool or the C++ post installer
// executable.

typedef struct CgptCreateParams {
  char *drive_name;
  int zap;
} CgptCreateParams;

typedef struct CgptAddParams {
  char *drive_name;
  uint32_t partition;
  uint64_t begin;
  uint64_t size;
  Guid type_guid;
  Guid unique_guid;
  char *label;
  int successful;
  int tries;
  int priority;
  uint16_t raw_value;
  int set_begin;
  int set_size;
  int set_type;
  int set_unique;
  int set_successful;
  int set_tries;
  int set_priority;
  int set_raw;
} CgptAddParams;

typedef struct CgptShowParams {
  char *drive_name;
  int numeric;
  int verbose;
  int quick;
  uint32_t partition;
  int single_item;
  int debug;

  // This is filled in by the relevant methods in CgptShow.c
  int num_partitions;
} CgptShowParams;

typedef struct CgptRepairParams {
  char *drive_name;
  int verbose;
} CgptRepairParams;

typedef struct CgptBootParams {
  char *drive_name;
  uint32_t partition;
  char *bootfile;
  int create_pmbr;
} CgptBootParams;

typedef struct CgptPrioritizeParams {
  char *drive_name;

  uint32_t set_partition;
  int set_friends;
  int max_priority;
  int orig_priority;
} CgptPrioritizeParams;

typedef struct CgptFindParams {
  char *drive_name;

  int verbose;
  int set_unique;
  int set_type;
  int set_label;
  int oneonly;
  int numeric;
  uint8_t *matchbuf;
  uint64_t matchlen;
  uint64_t matchoffset;
  uint8_t *comparebuf;
  Guid unique_guid;
  Guid type_guid;
  char *label;
  int hits;
  int match_partnum;           // 0 for no match, 1-N for match
} CgptFindParams;

typedef struct CgptLegacyParams {
  char *drive_name;
  int efipart;
} CgptLegacyParams;

// create related methods.
int CgptCreate(CgptCreateParams *params);

// add/attribute/details related methods
int CgptAdd(CgptAddParams *params);
int CgptSetAttributes(CgptAddParams *params);
int CgptGetPartitionDetails(CgptAddParams *params);

// boot related methods.
int CgptBoot(CgptBootParams *params);
int CgptGetBootPartitionNumber(CgptBootParams *params);

// show/get related methods.
int CgptShow(CgptShowParams *params);
int CgptGetNumNonEmptyPartitions(CgptShowParams *params);

// repair related methods.
int CgptRepair(CgptRepairParams *params);

// priority related methods.
int CgptPrioritize(CgptPrioritizeParams *params);

// find related methods.
void CgptFind(CgptFindParams *params);

// legacy related methods.
int CgptLegacy(CgptLegacyParams *params);

#endif  // VBOOT_REFERENCE_CGPT_CGPT_PARAMS_H_
