// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VBOOT_REFERENCE_CGPT_CGPTMANAGER_H_
#define VBOOT_REFERENCE_CGPT_CGPTMANAGER_H_

#include <string>
#include "gpt.h"

// This file defines a simple C++ wrapper class interface for the cgpt methods.

// These are the possible error codes that can be returned by the CgptManager.
typedef enum CgptErrorCode
{
  kCgptSuccess = 0,
  kCgptNotInitialized = 1,
  kCgptUnknownError = 2,
  kCgptInvalidArgument = 3,
} CgptErrorCode;


// CgptManager exposes methods to manipulate the Guid Partition Table as needed
// for ChromeOS scenarios.
class CgptManager {
  public:
    // Default constructor. The Initialize method must be called before
    // any other method can be called on this class.
    CgptManager();

    // Destructor. Automatically closes any opened device.
    ~CgptManager();

    // Opens the given device_name (e.g. "/dev/sdc") and initializes
    // with the Guid Partition Table of that device. This is the first method
    // that should be called on this class.  Otherwise those methods will
    // return kCgptNotInitialized.
    // Returns kCgptSuccess or an appropriate error code.
    // This device is automatically closed when this object is destructed.
    CgptErrorCode Initialize(const std::string& device_name);

    // Clears all the existing contents of the GPT and PMBR on the current
    // device.
    CgptErrorCode ClearAll();

    // Adds a new partition at the end of the existing partitions
    // with the new label, type, unique Id, offset and size.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode AddPartition(const std::string& label,
                     const Guid& partition_type_guid,
                     const Guid& unique_id,
                     uint64_t beginning_offset,
                     uint64_t num_sectors);

    // Populates num_partitions parameter with the number of partitions
    // that are currently on this device and not empty.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetNumNonEmptyPartitions(uint8_t* num_partitions) const;

    // Sets the Protective Master Boot Record on this device with the given
    // boot_partition number after populating the MBR with the contents of the
    // given boot_file_name. It also creates a legacy partition if
    // should_create_legacy_partition is true.
    // Note: Strictly speaking, the PMBR is not part of the GPT, but it is
    // included here for ease of use.
    CgptErrorCode SetPmbr(uint32_t boot_partition_number,
                          const std::string& boot_file_name,
                          bool should_create_legacy_partition);

    // Populates boot_partition with the partition number that's set to
    // boot in the PMBR.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetPmbrBootPartitionNumber(uint32_t* boot_partition) const;

    // Sets the "successful" attribute of the given kernelPartition to 0 or 1
    // based on the value of is_successful being true (1) or false(0)
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode SetSuccessful(uint32_t partition_number, bool is_successful);

    // Populates is_successful to true if the successful attribute in the
    // given kernelPartition is non-zero, or to false if it's zero.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetSuccessful(uint32_t partition_number,
                                bool* is_successful) const;

    // Sets the "NumTriesLeft" attribute of the given kernelPartition to
    // the given num_tries_left value.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode SetNumTriesLeft(uint32_t partition_number,
                                  int num_tries_left);

    // Populates the num_tries_left parameter with the value of the
    // NumTriesLeft attribute of the given kernelPartition.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetNumTriesLeft(uint32_t partition_number,
                                  int* num_tries_left) const;

    // Sets the "Priority" attribute of the given kernelPartition to
    // the given priority value.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode SetPriority(uint32_t partition_number,
                              uint8_t priority);

    // Populates the priority parameter with the value of the Priority
    // attribute of the given kernelPartition.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetPriority(uint32_t partition_number,
                              uint8_t* priority) const;

    // Populates the offset parameter with the beginning offset of the
    // given partition.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetBeginningOffset(uint32_t partition_number,
                                     uint64_t* offset) const;

    // Populates the number of sectors in the given partition.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetNumSectors(uint32_t partition_number,
                                uint64_t* num_sectors) const;

    // Populates the type_id parameter with the partition type id
    // (these are the standard ids for kernel, rootfs, etc.)
    // of the partition corresponding to the given partition_number.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetPartitionTypeId(uint32_t partition_number,
                                     Guid* type_id) const;

    // Populates the unique_id parameter with the Guid that uniquely identifies
    // the given partition_number.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetPartitionUniqueId(uint32_t partition_number,
                                       Guid* unique_id) const;

    // Populates the partition_number parameter with the partition number of
    // the partition which is uniquely identified by the given unique_id.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode GetPartitionNumberByUniqueId(
                      const Guid& unique_id,
                      uint32_t* partition_number) const;

    // Sets the "Priority" attribute of given kernelPartition to the value
    // specified in higestPriority parameter. In addition, also reduces the
    // priorities of all the other kernel partitions, if necessary, to ensure
    // no other partition has a higher priority. It does preserve the relative
    // ordering among the remaining partitions and doesn't touch the partitions
    // whose priorities are zero.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode SetHighestPriority(uint32_t partition_number,
                                     uint8_t highest_priority);

    // Same as SetHighestPriority above but works without having to explicitly
    // give a value for highest_priority. The internal implementation figures
    // out the best highest number that needs to be given depending on the
    // existing priorities.
    // Returns kCgptSuccess or an appropriate error code.
    CgptErrorCode SetHighestPriority(uint32_t partition_number);

    // Runs the sanity checks on the CGPT and MBR and
    // Returns kCgptSuccess if everything is valid or an appropriate error code
    // if there's anything invalid or if there's any error encountered during
    // the validation.
    CgptErrorCode Validate();

  private:
    std::string device_name_;
    bool is_initialized_;

    CgptManager(const CgptManager &);
    void operator=(const CgptManager &);
};

#endif  // VBOOT_REFERENCE_CGPT_CGPTMANAGER_H_
