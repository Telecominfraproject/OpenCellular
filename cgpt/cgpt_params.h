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
  char *driveName;
  int zap;
} CgptCreateParams;

typedef struct CgptAddParams {
  char *driveName;
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
  char *driveName;
  int numeric;
  int verbose;
  int quick;
  uint32_t partition;
  int single_item;
} CgptShowParams;

typedef struct CgptRepairParams {
  char *driveName;
  int verbose;
} CgptRepairParams;

typedef struct CgptBootParams {
  char *driveName;
  uint32_t partition;
  char *bootfile;
  int create_pmbr;
} CgptBootParams;

typedef struct CgptPrioritizeParams {
  char *driveName;

  uint32_t set_partition;
  int set_friends;
  int max_priority;
  int orig_priority;
} CgptPrioritizeParams;

typedef struct CgptFindParams {
  char *driveName;

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

int cgpt_create(CgptCreateParams *params);
int cgpt_add(CgptAddParams *params);
int cgpt_boot(CgptBootParams *params);
int cgpt_show(CgptShowParams *params);
int cgpt_repair(CgptRepairParams *params);
int cgpt_prioritize(CgptPrioritizeParams *params);
void cgpt_find(CgptFindParams *params);
#endif  // VBOOT_REFERENCE_CGPT_CGPT_PARAMS_H_
