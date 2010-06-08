// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VBOOT_REFERENCE_FIRMWARE_UTILITY_H_
#define VBOOT_REFERENCE_FIRMWARE_UTILITY_H_

#include <string>

class FirmwareImage;
struct RSAPublicKey;

namespace vboot_reference {

// A class for handling verified boot firmware images.
class FirmwareUtility {
 public:
  FirmwareUtility();
  ~FirmwareUtility();

  // Print usage to stderr.
  void PrintUsage(void);

  // Parse command line options and populate data members.
  // Return true on success, false on failure.
  bool ParseCmdLineOptions(int argc, char* argv[]);

  // Print descriptio of verified boot firmware image.
  void DescribeSignedImage();

  // Generate a verified boot image by reading firmware data from in_file_.
  // Return true on success, false on failure.
  bool GenerateSignedImage();

  // Verify a previously generated signed firmware image using the root key read
  // from [root_key_pub_file_].
  bool VerifySignedImage();

  // Output the verified boot image to out_file_.
  void OutputSignedImage();


  bool is_generate() { return is_generate_; }
  bool is_verify() { return is_verify_; }
  bool is_describe() { return is_describe_; }

 private:

  // Check if all options were specified and sane.
  // Return true on success, false on failure.
  bool CheckOptions();

  FirmwareImage* image_;
  RSAPublicKey* root_key_pub_;
  std::string root_key_file_;
  std::string root_key_pub_file_;
  std::string firmware_key_file_;
  std::string firmware_key_pub_file_;
  std::string subkey_in_file_;  // Existing key signature header.
  std::string in_file_;
  std::string out_file_;
  std::string kernel_subkey_sign_pub_file_;
  int firmware_key_version_;
  int firmware_sign_algorithm_;
  int firmware_version_;
  int kernel_subkey_sign_algorithm_;
  bool is_generate_;  // Are we generating a new image?
  bool is_verify_;  // Are we just verifying an already signed image?
  bool is_describe_; // Should we print out description of the image?
  bool is_only_vblock_;  // Should we just output the verification block?
  bool is_subkey_out_;  // Should we just output the subkey header?
};

}  // namespace vboot_reference

#endif  // VBOOT_REFERENCE_FIRMWARE_UTILITY_H_
