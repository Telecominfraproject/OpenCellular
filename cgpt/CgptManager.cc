// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "CgptManager.h"

extern "C" {
#include "cgpt_params.h"
}

using std::string;

// We don't use these variables for the libcgpt version.
const char* progname = "";
const char* command = "";

// This file implements the C++ wrapper methods over the C cgpt methods.

CgptManager::CgptManager():
  is_initialized_(false) {
}

CgptManager::~CgptManager() {
}

CgptErrorCode CgptManager::Initialize(const string& device_name) {
  device_name_ = device_name;
  is_initialized_ = true;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::ClearAll() {
  if (!is_initialized_)
    return kCgptNotInitialized;

  CgptCreateParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.zap = 0;

  int retval = cgpt_create(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  return kCgptSuccess;
}

CgptErrorCode CgptManager::AddPartition(const string& label,
                              const Guid& partition_type_guid,
                              const Guid& unique_id,
                              uint64_t beginning_offset,
                              uint64_t num_sectors) {
  if (!is_initialized_)
    return kCgptNotInitialized;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.label = const_cast<char *>(label.c_str());

  params.type_guid = partition_type_guid;
  params.set_type = 1;

  params.begin = beginning_offset;
  params.set_begin = 1;

  params.size = num_sectors;
  params.set_size = 1;

  if (!IsZero(&unique_id)) {
     params.unique_guid = unique_id;
     params.set_unique = 1;
  }

  int retval = cgpt_add(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetNumNonEmptyPartitions(uint8_t* num_partitions) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!num_partitions)
    return kCgptInvalidArgument;

  CgptShowParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  int retval = cgpt_get_num_non_empty_partitions(&params);

  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *num_partitions = params.num_partitions;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::SetPmbr(uint32_t boot_partition_number,
                                   const string& boot_file_name,
                                   bool should_create_legacy_partition) {
  if (!is_initialized_)
    return kCgptNotInitialized;

  CgptBootParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  if (!boot_file_name.empty())
    params.bootfile = const_cast<char *>(boot_file_name.c_str());

  params.partition = boot_partition_number;
  params.create_pmbr = should_create_legacy_partition;

  int retval = cgpt_boot(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetPmbrBootPartitionNumber(
                                    uint32_t* boot_partition) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!boot_partition)
    return kCgptInvalidArgument;

  CgptBootParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());

  int retval = cgpt_get_boot_partition_number(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *boot_partition = params.partition;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::SetSuccessful(
                               uint32_t partition_number,
                               bool is_successful) {
  if (!is_initialized_)
    return kCgptNotInitialized;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  params.successful = is_successful;
  params.set_successful = true;

  int retval = cgpt_set_attributes(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetSuccessful(uint32_t partition_number,
                                         bool* is_successful) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!is_successful)
    return kCgptInvalidArgument;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  int retval = cgpt_get_partition_details(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *is_successful = params.successful;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::SetNumTriesLeft(uint32_t partition_number,
                                           int numTries) {
  if (!is_initialized_)
    return kCgptNotInitialized;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  params.tries = numTries;
  params.set_tries = true;

  int retval = cgpt_set_attributes(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetNumTriesLeft(uint32_t partition_number,
                                           int* numTries) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!numTries)
    return kCgptInvalidArgument;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  int retval = cgpt_get_partition_details(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *numTries = params.tries;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::SetPriority(uint32_t partition_number,
                                       uint8_t priority) {
  if (!is_initialized_)
    return kCgptNotInitialized;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  params.priority = priority;
  params.set_priority = true;

  int retval = cgpt_set_attributes(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetPriority(uint32_t partition_number,
                                       uint8_t* priority) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!priority)
    return kCgptInvalidArgument;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  int retval = cgpt_get_partition_details(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *priority = params.priority;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetBeginningOffset(uint32_t partition_number,
                                              uint64_t* offset) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!offset)
    return kCgptInvalidArgument;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  int retval = cgpt_get_partition_details(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *offset = params.begin;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetNumSectors(uint32_t partition_number,
                                         uint64_t* num_sectors) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!num_sectors)
    return kCgptInvalidArgument;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  int retval = cgpt_get_partition_details(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *num_sectors = params.size;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetPartitionTypeId(uint32_t partition_number,
                                              Guid* type_id) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!type_id)
    return kCgptInvalidArgument;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  int retval = cgpt_get_partition_details(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *type_id = params.type_guid;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetPartitionUniqueId(uint32_t partition_number,
                                                Guid* unique_id) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!unique_id)
    return kCgptInvalidArgument;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.partition = partition_number;

  int retval = cgpt_get_partition_details(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *unique_id = params.unique_guid;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::GetPartitionNumberByUniqueId(
                    const Guid& unique_id,
                    uint32_t* partition_number) const {
  if (!is_initialized_)
    return kCgptNotInitialized;

  if (!partition_number)
    return kCgptInvalidArgument;

  CgptAddParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.unique_guid = unique_id;
  params.set_unique = 1;

  int retval = cgpt_get_partition_details(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  *partition_number = params.partition;
  return kCgptSuccess;
}

CgptErrorCode CgptManager::SetHighestPriority(uint32_t partition_number,
                                              uint8_t highest_priority) {
  if (!is_initialized_)
    return kCgptNotInitialized;

  CgptPrioritizeParams params;
  memset(&params, 0, sizeof(params));

  params.drive_name = const_cast<char *>(device_name_.c_str());
  params.set_partition = partition_number;
  params.max_priority = highest_priority;

  int retval = cgpt_prioritize(&params);
  if (retval != CGPT_OK)
    return kCgptUnknownError;

  return kCgptSuccess;
}

CgptErrorCode CgptManager::SetHighestPriority(uint32_t partition_number) {
  // The internal implementation in cgpt_prioritize automatically computes the
  // right priority number if we supply 0 for the highest_priority argument.
  return SetHighestPriority(partition_number, 0);
}

CgptErrorCode CgptManager::Validate() {
  if (!is_initialized_)
    return kCgptNotInitialized;

  uint8_t num_partitions;

  // GetNumNonEmptyPartitions does the check for GptSanityCheck.
  // so call it (ignore the num_partitions result) and just return
  // its success/failure result.
  return GetNumNonEmptyPartitions(&num_partitions);
}
