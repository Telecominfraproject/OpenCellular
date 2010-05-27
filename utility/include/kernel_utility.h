// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VBOOT_REFERENCE_KERNEL_UTILITY_H_
#define VBOOT_REFERENCE_KERNEL_UTILITY_H_

#include <string>

extern "C" {
#include  "kernel_image.h"
}

struct RSAPublicKey;

namespace vboot_reference {

// A class for handling verified boot kernel images.
class KernelUtility {
 public:
  KernelUtility();
  ~KernelUtility();

  // Print usage to stderr.
  void PrintUsage(void);

  // Parse command line options and populate data members.
  // Return true on success, false on failure.
  bool ParseCmdLineOptions(int argc, char* argv[]);

  // Print description of a verified boot kernel image.
  void DescribeSignedImage();

  // Generate a verified boot image by reading kernel data from in_file_.
  // Return true on success, false on failure.
  bool GenerateSignedImage();

  // Verify a previously generated signed firmware image using the key read
  // from [firmware_key_pub_file_].
  bool VerifySignedImage();

  // Output the verified boot kernel image to out_file_.
  void OutputSignedImage();

  bool is_generate() { return is_generate_; }
  bool is_verify() { return is_verify_; }
  bool is_describe() { return is_describe_; }

 private:

  // Check if all options were specified and sane.
  // Return true on success, false on failure.
  bool CheckOptions();

  KernelImage* image_;
  RSAPublicKey* firmware_key_pub_;  // Root key used for verification.
  std::string firmware_key_file_;  // Private key for signing the kernel key.
  std::string firmware_key_pub_file_;
  std::string kernel_key_file_;  // Private key for signing the kernel.
  std::string kernel_key_pub_file_;
  std::string config_file_;  // File containing kernel commandline parameters
  std::string bootloader_file_;  // Embedded bootloader code
  std::string vmlinuz_file_;  // Input vmlinuz to be embedded in signed blob.

  // Fields of a KernelImage. (read from the command line).
  int header_version_;
  int firmware_sign_algorithm_;
  int kernel_sign_algorithm_;
  int kernel_key_version_;
  int kernel_version_;
  int padding_;
  uint64_t kernel_len_;
  uint8_t* kernel_config_;

  std::string in_file_;
  std::string out_file_;
  bool is_generate_;  // Are we generating a new image?
  bool is_verify_;  // Are we just verifying an already signed image?
  bool is_describe_;  // Should we print out description of the image?
  bool is_only_vblock_;  // Should we just output the verification block?
  bool is_subkey_out_;  // Should we just output the subkey header?
};

}  // namespace vboot_reference

#endif  // VBOOT_REFERENCE_FIRMWARE_UTILITY_H_
