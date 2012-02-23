// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for CgptManager class.
#include <string>

#include "../cgpt/CgptManager.h"

// We use some specific GUID constants for some of the tests,
// so pulling in cgpt.h. Make sure this is included after
// CgptManager.h so that we can test the actual usage of
// CgptManager.h as the post-install package would use.
// Example: this would catch an incorrect usage of a GUID
// that's defined only in cgpt.h and being accidentally
// used in CgptManager.h (which should not have any cgpt.h
// dependencies).
extern "C" {
#include "../cgpt/cgpt.h"
}

#include <base/logging.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <uuid/uuid.h>

using std::string;

static const Guid p2guid = {{{0, 1, 2, 3, 4, {2, 2, 2, 2, 2, 2}}}};
static const Guid p3guid = {{{0, 6, 5, 4, 2, {3, 3, 3, 3, 3, 3}}}};

#define EXPECT_SUCCESS(c) EXPECT_EQ(kCgptSuccess, c)

// The --v flag controls the log verbosity level.
DEFINE_int32(v, 0, 0);

// This class unit tests the CgptManager class.
class CgptManagerUnitTest : public ::testing::Test {
public:
  CgptManagerUnitTest() {
    // Even though the post-installer doesn't use any methods that require
    // uuid_generate, for the unit test we use those methods, so we need to
    // set the uuid_generator.
    uuid_generator = uuid_generate;
  }

  void SetUp() {
    const string device_name = "/tmp/DummyFileForCgptManagerTests.bin";

    CreateDummyDevice(device_name);

    LOG(INFO) << "Initializing cgpt with " << device_name;
    EXPECT_SUCCESS(cgpt_manager.Initialize(device_name));
    EXPECT_SUCCESS(cgpt_manager.ClearAll());

    CheckPartitionCount(0);
  }

  virtual ~CgptManagerUnitTest() { }

protected:
  CgptManager cgpt_manager;

  void CreateDummyDevice(const string& dummy_device) {
    FILE* fp = fopen(dummy_device.c_str(), "w");

    ASSERT_TRUE(fp != NULL);

    const int kNumSectors = 1000;
    const int kSectorSize = 512;
    const char kFillChar = '7'; // Some character, value doesn't matter.

    for(int i = 0; i < kNumSectors * kSectorSize; i++) {
      EXPECT_EQ(kFillChar, fputc(kFillChar, fp));
    }

    fclose(fp);
  }

  void CheckEquality(string field,
                     uint64_t expected,
                     uint64_t actual) {


    VLOG(1) << field << ":"
            << "Expected = " << expected
            << ";Actual = " << actual;

    EXPECT_EQ(expected, actual);
  }


  void CheckGuidEquality(string field,
                         const Guid& expected_id,
                         const Guid& actual_id) {
    char expected_id_str[GUID_STRLEN];
    GuidToStr(&expected_id, expected_id_str, sizeof(expected_id_str));

    char actual_id_str[GUID_STRLEN];
    GuidToStr(&actual_id, actual_id_str, sizeof(actual_id_str));

    VLOG(1) << field << ":"
            << "Expected = " << expected_id_str
            << ";Actual = " << actual_id_str;

    EXPECT_TRUE(GuidEqual(&expected_id, &actual_id));
  }

  // Checks if the current number of partitions in the device matches
  // the value of expected_num_partitions.
  void CheckPartitionCount(uint8 expected_num_partitions) {
    uint8_t actual_num_partitions;
    EXPECT_SUCCESS(cgpt_manager.GetNumNonEmptyPartitions(
                                    &actual_num_partitions));

    CheckEquality("NumPartitions",
                  expected_num_partitions,
                  actual_num_partitions);
  }

  void SetAndCheckSuccessfulBit(uint32_t partition_number,
                                bool expected_is_successful) {
    EXPECT_SUCCESS(cgpt_manager.SetSuccessful(partition_number,
                                              expected_is_successful));

    bool actual_is_successful;
    EXPECT_SUCCESS(cgpt_manager.GetSuccessful(partition_number,
                                              &actual_is_successful));
    EXPECT_EQ(expected_is_successful, actual_is_successful);
  }


  void SetAndCheckNumTriesLeft(uint32_t partition_number,
                               int expected_num_tries) {
    EXPECT_SUCCESS(cgpt_manager.SetNumTriesLeft(partition_number,
                                                expected_num_tries));

    int actual_num_tries;
    EXPECT_SUCCESS(cgpt_manager.GetNumTriesLeft(partition_number,
                                                &actual_num_tries));
    CheckEquality("NumTries", expected_num_tries, actual_num_tries);
  }

  void SetAndCheckPriority(uint32_t partition_number,
                           uint8_t expected_priority) {
    EXPECT_SUCCESS(cgpt_manager.SetPriority(partition_number,
                                            expected_priority));

    uint8_t actual_priority;
    EXPECT_SUCCESS(cgpt_manager.GetPriority(partition_number,
                                            &actual_priority));
    CheckEquality("Priority", expected_priority, actual_priority);
  }

  void CheckPriority(uint32_t partition_number,
                     uint8_t expected_priority) {
    uint8_t actual_priority;
    EXPECT_SUCCESS(cgpt_manager.GetPriority(partition_number,
                                            &actual_priority));
    CheckEquality("Priority", expected_priority, actual_priority);
  }


  void CheckBeginningOffset(uint32_t partition_number,
                            uint64_t expected_offset) {
    uint64_t actual_offset;
    EXPECT_SUCCESS(cgpt_manager.GetBeginningOffset(partition_number,
                                                   &actual_offset));
    CheckEquality("Beginning Offset", expected_offset, actual_offset);
  }


  void CheckNumSectors(uint32_t partition_number,
                       uint64_t expected_num_sectors) {
    uint64_t actual_num_sectors;
    EXPECT_SUCCESS(cgpt_manager.GetNumSectors(partition_number,
                                              &actual_num_sectors));
    CheckEquality("Num Sectors", expected_num_sectors, actual_num_sectors);
  }


  void CheckPartitionTypeId(int partition_number,
                            const Guid& expected_partition_type_id) {
    // Get the partition type id and check if it matches the expected value.
    Guid actual_partition_type_id;
    EXPECT_SUCCESS(cgpt_manager.GetPartitionTypeId(partition_number,
                                                   &actual_partition_type_id));

    CheckGuidEquality("PartitionTypeId",
                      expected_partition_type_id,
                      actual_partition_type_id);
  }

  void CheckPartitionUniqueId(int partition_number,
                            const Guid& expected_partition_unique_id) {
    // Get the partition unique id and check if it matches the expected value.
    Guid actual_partition_unique_id;
    EXPECT_SUCCESS(cgpt_manager.GetPartitionUniqueId(
                                    partition_number,
                                    &actual_partition_unique_id));

    CheckGuidEquality("PartitionUniqueId",
                      expected_partition_unique_id,
                      actual_partition_unique_id);
  }

  void CheckPartitionNumberByUniqueId(const Guid& unique_id,
                                      uint32_t expected_partition_number) {
    // Get the partition number for the unique id and check
    // if it matches the expected value.
    uint32_t actual_partition_number;
    EXPECT_SUCCESS(cgpt_manager.GetPartitionNumberByUniqueId(
                                    unique_id,
                                    &actual_partition_number));

    CheckEquality("PartitionNumberForUniqueId",
                  expected_partition_number,
                  actual_partition_number);
  }


  void CreateBootFile(const string& boot_file_name) {
    FILE* fp = fopen(boot_file_name.c_str(), "w");

    ASSERT_TRUE(fp != NULL);

    const int kNumSectors = 1;
    const int kSectorSize = 512;
    const char kFillChar = '8'; // Some character, value doesn't matter.

    for(int i = 0; i < kNumSectors * kSectorSize; i++) {
      EXPECT_EQ(kFillChar, fputc(kFillChar, fp));
    }

    fclose(fp);
  }

private:
  DISALLOW_COPY_AND_ASSIGN(CgptManagerUnitTest);
};

TEST_F(CgptManagerUnitTest, AutoPrioritizationTest) {
  EXPECT_SUCCESS(cgpt_manager.AddPartition("k1",
                                           guid_chromeos_kernel,
                                           guid_unused,
                                           100,
                                           10));
  CheckPartitionCount(1);

  EXPECT_SUCCESS(cgpt_manager.AddPartition("k2",
                                           guid_chromeos_kernel,
                                           p2guid,
                                           200,
                                           20));
  CheckPartitionCount(2);

  EXPECT_SUCCESS(cgpt_manager.AddPartition("k3",
                                           guid_chromeos_kernel,
                                           p3guid,
                                           300,
                                           30));
  CheckPartitionCount(3);

  uint8_t expectedk1Priority = 1;
  uint8_t expectedk2Priority = 2;
  uint8_t expectedk3Priority = 0;

  // Calling SetAndCheckPriority will do a set and get of the above priorities.
  SetAndCheckPriority(1, expectedk1Priority);
  SetAndCheckPriority(2, expectedk2Priority);
  SetAndCheckPriority(3, expectedk3Priority);

  EXPECT_SUCCESS(cgpt_manager.SetHighestPriority(1));

  expectedk1Priority = 2;
  expectedk2Priority = 1;

  CheckPriority(1, expectedk1Priority);
  CheckPriority(2, expectedk2Priority);
  CheckPriority(3, expectedk3Priority);
}


TEST_F(CgptManagerUnitTest, AddPartitionTest) {
  int p2_offset = 200;
  int p2_size = 20;
  int p3_offset = 300;
  int p3_size = 30;

  VLOG(1) << "Adding various types of partitions ... ";
  EXPECT_SUCCESS(cgpt_manager.AddPartition("data stuff",
                                           guid_linux_data,
                                           guid_unused,
                                           100,
                                           10));
  CheckPartitionCount(1);

  EXPECT_SUCCESS(cgpt_manager.AddPartition("kernel stuff",
                                           guid_chromeos_kernel,
                                           p2guid,
                                           p2_offset,
                                           p2_size));
  CheckPartitionCount(2);

  EXPECT_SUCCESS(cgpt_manager.AddPartition("rootfs stuff",
                                           guid_chromeos_rootfs,
                                           p3guid,
                                           p3_offset,
                                           p3_size));
  CheckPartitionCount(3);

  uint32_t pmbr_boot_partition_number = 4;
  EXPECT_SUCCESS(cgpt_manager.AddPartition("ESP stuff",
                                           guid_efi,
                                           guid_unused,
                                           400,
                                           40));
  CheckPartitionCount(4);

  EXPECT_SUCCESS(cgpt_manager.AddPartition("fture stuff",
                                           guid_chromeos_reserved,
                                           guid_unused,
                                           500,
                                           50));
  CheckPartitionCount(5);

  Guid guid_random =  {{{0x2364a860, 0xbf63, 0x42fb, 0xa8, 0x3d,
                        {0x9a, 0xd3, 0xe0, 0x57, 0xfc, 0xf5}}}};

  EXPECT_SUCCESS(cgpt_manager.AddPartition("random stuff",
                                           guid_random,
                                           guid_unused,
                                           600,
                                           60));

  CheckPartitionCount(6);

  string boot_file_name = "/tmp/BootFileForCgptManagerTests.bin";
  LOG(INFO) << "Adding EFI partition to PMBR with bootfile: "
            << boot_file_name;

  CreateBootFile(boot_file_name);
  EXPECT_SUCCESS(cgpt_manager.SetPmbr(pmbr_boot_partition_number,
                                      boot_file_name,
                                      true));

  VLOG(1) << "Checking if contents of GPT match values set in AddPartition.";

  uint32_t actual_boot_partition_number;
  EXPECT_SUCCESS(cgpt_manager.GetPmbrBootPartitionNumber(
                                       &actual_boot_partition_number));
  EXPECT_EQ(pmbr_boot_partition_number, actual_boot_partition_number);

  // Set the successful attribute for some partition to various Values
  // and check if the settings are preserved.
  SetAndCheckSuccessfulBit(2, true);
  SetAndCheckSuccessfulBit(2, false);

  // Set the number of tries for some partition to various Values
  // and check if the settings are preserved.
  SetAndCheckNumTriesLeft(2, 6);
  SetAndCheckNumTriesLeft(2, 5);

  // Set the priority for some partition to various Values
  // and check if the settings are preserved.
  SetAndCheckPriority(2, 2);
  SetAndCheckPriority(2, 0);

  // Check if the beginning offset for some of the partitions
  // are the same as what was set in AddPartition.
  CheckBeginningOffset(2, p2_offset);
  CheckBeginningOffset(3, p3_offset);

  // Check if the number of sectors for some of the partitions
  // are same as what was set in AddPartition.
  CheckNumSectors(2, p2_size);
  CheckNumSectors(3, p3_size);

  // Check if the partition type IDs for some of the partitions
  // are same as what was set in AddPartition.
  CheckPartitionTypeId(2, guid_chromeos_kernel);
  CheckPartitionTypeId(4, guid_efi);

  // Check if the partition unique IDs for some of the partitions
  // same as what was set in AddPartition.
  CheckPartitionUniqueId(2, p2guid);
  CheckPartitionUniqueId(3, p3guid);

  // Check if the partition numbers for some of the partitions are
  // retrievable by their unique IDs set in AddPartition.
  CheckPartitionNumberByUniqueId(p2guid, 2);
  CheckPartitionNumberByUniqueId(p3guid, 3);
}

int main(int argc, char **argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  ::testing::InitGoogleTest(&argc, argv);

  // VLOG(2) logs at level -2. So if user gives --v=2, we should
  // set MinLogLevel to -2, so VLOG(2) and VLOG(1) will show up.
  logging::SetMinLogLevel(-FLAGS_v);
  return RUN_ALL_TESTS();
}
