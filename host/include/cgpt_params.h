// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VBOOT_REFERENCE_CGPT_CGPT_PARAMS_H_
#define VBOOT_REFERENCE_CGPT_CGPT_PARAMS_H_
#include <stdint.h>

#include "gpt.h"

enum {
  CGPT_OK = 0,
  CGPT_FAILED,
};

typedef struct CgptCreateParams {
  char *drive_name;
  int zap;
  uint64_t size;
  uint64_t padding;
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
  uint32_t raw_value;
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
  int match_partnum;           /* 1-based; 0 means no match */
} CgptFindParams;

typedef struct CgptLegacyParams {
  char *drive_name;
  int efipart;
} CgptLegacyParams;

#endif  // VBOOT_REFERENCE_CGPT_CGPT_PARAMS_H_
