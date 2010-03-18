// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utility for manipulating verified boot kernel images.
//

#include "kernel_utility.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>  // Needed for UINT16_MAX.
#include <stdlib.h>
#include <unistd.h>

#include <iostream>

extern "C" {
#include "file_keys.h"
#include "kernel_image.h"
#include "padding.h"
#include "rsa_utility.h"
#include "sha_utility.h"
#include "utility.h"
}

extern int errno;
using std::cerr;

namespace vboot_reference {

KernelUtility::KernelUtility(): image_(NULL),
                                firmware_key_pub_(NULL),
                                header_version_(1),
                                firmware_sign_algorithm_(-1),
                                kernel_sign_algorithm_(-1),
                                kernel_key_version_(-1),
                                kernel_version_(-1),
                                is_generate_(false),
                                is_verify_(false) {
  // Populate kernel config options with defaults.
  options_.version[0] = 1;
  options_.version[1] = 0;
  options_.kernel_len = 0;
  options_.kernel_load_addr = 0;
  options_.kernel_entry_addr = 0;
}

KernelUtility::~KernelUtility() {
  RSAPublicKeyFree(firmware_key_pub_);
  KernelImageFree(image_);
}

void KernelUtility::PrintUsage(void) {
  cerr <<
      "Utility to generate/verify a verified boot kernel image\n\n"
      "Usage: kernel_utility <--generate|--verify> [OPTIONS]\n\n"
      "For \"--verify\",  required OPTIONS are:\n"
      "--in <infile>\t\t\tVerified boot kernel image to verify.\n"
      "--firmware_key_pub <pubkeyfile>\tPre-processed public firmware key "
      "to use for verification.\n\n"
      "For \"--generate\", required OPTIONS are:\n"
      "--firmware_key <privkeyfile>\tPrivate firmware signing key file\n"
      "--kernel_key <privkeyfile>\tPrivate kernel signing key file\n"
      "--kernel_key_pub <pubkeyfile>\tPre-processed public kernel signing"
      " key\n"
      "--firmware_sign_algorithm <algoid>\tSigning algorithm used by "
      "the firmware\n"
      "--kernel_sign_algorithm <algoid>\tSigning algorithm to use for kernel\n"
      "--kernel_key_version <version#>\tKernel signing Key Version#\n"
      "--kernel_version <version#>\tKernel Version#\n"
      "--in <infile>\t\tKernel Image to sign\n"
      "--out <outfile>\t\tOutput file for verified boot Kernel image\n\n"
      "Optional arguments for \"--generate\" include:\n"
      "--config_version <version>\n"
      "--kernel_load_addr <addr>\n"
      "--kernel_entry_addr <addr>\n\n"
      "<algoid> (for --*_sign_algorithm) is one of the following:\n";
  for (int i = 0; i < kNumAlgorithms; i++) {
    cerr << i << " for " << algo_strings[i] << "\n";
  }
  cerr << "\n\n";
}

bool KernelUtility::ParseCmdLineOptions(int argc, char* argv[]) {
  int option_index;
  static struct option long_options[] = {
    {"firmware_key", 1, 0, 0},
    {"firmware_key_pub", 1, 0, 0},
    {"kernel_key", 1, 0, 0},
    {"kernel_key_pub", 1, 0, 0},
    {"firmware_sign_algorithm", 1, 0, 0},
    {"kernel_sign_algorithm", 1, 0, 0},
    {"kernel_key_version", 1, 0, 0},
    {"kernel_version", 1, 0, 0},
    {"in", 1, 0, 0},
    {"out", 1, 0, 0},
    {"generate", 0, 0, 0},
    {"verify", 0, 0, 0},
    {"config_version", 1, 0, 0},
    {"kernel_load_addr", 1, 0, 0},
    {"kernel_entry_addr", 1, 0, 0},
    {NULL, 0, 0, 0}
  };
  while (1) {
    int i = getopt_long(argc, argv, "", long_options, &option_index);
    if (-1 == i)  // Done with option processing.
      break;
    if ('?' == i)  // Invalid option found.
      return false;

    if (0 == i) {
      switch (option_index) {
        case 0:  // firmware_key
          firmware_key_file_ = optarg;
          break;
        case 1:  // firmware_key_pub
          firmware_key_pub_file_ = optarg;
          break;
        case 2:  // kernel_key
          kernel_key_file_ = optarg;
          break;
        case 3:  // kernel_key_pub
          kernel_key_pub_file_ = optarg;
          break;
        case 4:  // firmware_sign_algorithm
          errno = 0;  // strtol() returns an error via errno
          firmware_sign_algorithm_ = strtol(optarg,
                                            reinterpret_cast<char**>(NULL), 10);
          if (errno)
            return false;
          break;
        case 5:  // kernel_sign_algorithm
          errno = 0;
          kernel_sign_algorithm_ = strtol(optarg,
                                          reinterpret_cast<char**>(NULL), 10);
          if (errno)
            return false;
          break;
        case 6:  // kernel_key_version
          errno = 0;
          kernel_key_version_ = strtol(optarg,
                                       reinterpret_cast<char**>(NULL), 10);
          if (errno)
            return false;
          break;
        case 7:  // kernel_version
          errno = 0;
          kernel_version_ = strtol(optarg,
                                   reinterpret_cast<char**>(NULL), 10);
          if (errno)
            return false;
          break;
        case 8:  // in
          in_file_ = optarg;
          break;
        case 9:  // out
          out_file_ = optarg;
          break;
        case 10:  // generate
          is_generate_ = true;
          break;
        case 11:  // verify
          is_verify_ = true;
          break;
        case 12:  // config_version
          if (2 != sscanf(optarg, "%d.%d", &options_.version[0],
                          &options_.version[1]))
            return false;
          break;
        case 13:  // kernel_load_addr
          errno = 0;
          options_.kernel_load_addr =
              strtol(optarg, reinterpret_cast<char**>(NULL), 10);
          if (errno)
            return false;
          break;
        case 14:  // kernel_entry_addr
          errno = 0;
          options_.kernel_entry_addr =
              strtol(optarg, reinterpret_cast<char**>(NULL), 10);

          if (errno)
            return false;
          break;
      }
    }
  }
  return CheckOptions();
}

void KernelUtility::OutputSignedImage(void) {
  if (image_) {
    if (!WriteKernelImage(out_file_.c_str(), image_)) {
      cerr << "Couldn't write verified boot kernel image to file "
           << out_file_ <<".\n";
    }
  }
}

bool KernelUtility::GenerateSignedImage(void) {
  uint64_t kernel_key_pub_len;
  uint8_t* header_checksum;
  DigestContext ctx;
  image_ = KernelImageNew();

  Memcpy(image_->magic, KERNEL_MAGIC, KERNEL_MAGIC_SIZE);

  // TODO(gauravsh): make this a command line option.
  image_->header_version = 1;
  image_->firmware_sign_algorithm = (uint16_t) firmware_sign_algorithm_;
  // Copy pre-processed public signing key.
  image_->kernel_sign_algorithm = (uint16_t) kernel_sign_algorithm_;
  image_->kernel_sign_key = BufferFromFile(kernel_key_pub_file_.c_str(),
                                           &kernel_key_pub_len);
  if (!image_->kernel_sign_key)
    return false;
  image_->kernel_key_version = kernel_key_version_;

  // Update header length.
  image_->header_len = GetKernelHeaderLen(image_);

  // Calculate header checksum.
  DigestInit(&ctx, SHA512_DIGEST_ALGORITHM);
  DigestUpdate(&ctx, reinterpret_cast<uint8_t*>(&image_->header_version),
               sizeof(image_->header_version));
  DigestUpdate(&ctx, reinterpret_cast<uint8_t*>(&image_->header_len),
               sizeof(image_->header_len));
  DigestUpdate(&ctx,
               reinterpret_cast<uint8_t*>(&image_->firmware_sign_algorithm),
               sizeof(image_->firmware_sign_algorithm));
  DigestUpdate(&ctx,
               reinterpret_cast<uint8_t*>(&image_->kernel_sign_algorithm),
               sizeof(image_->kernel_sign_algorithm));
  DigestUpdate(&ctx, reinterpret_cast<uint8_t*>(&image_->kernel_key_version),
               sizeof(image_->kernel_key_version));
  DigestUpdate(&ctx, image_->kernel_sign_key,
               RSAProcessedKeySize(image_->kernel_sign_algorithm));
  header_checksum = DigestFinal(&ctx);
  Memcpy(image_->header_checksum, header_checksum, SHA512_DIGEST_SIZE);
  Free(header_checksum);

  image_->kernel_version = kernel_version_;
  image_->options.version[0] = options_.version[0];
  image_->options.version[1] = options_.version[1];
  image_->options.kernel_load_addr = options_.kernel_load_addr;
  image_->options.kernel_entry_addr = options_.kernel_entry_addr;
  image_->kernel_data = BufferFromFile(in_file_.c_str(),
                                       &image_->options.kernel_len);
  if (!image_)
    return false;
  // Generate and add the signatures.
  if (!AddKernelKeySignature(image_, firmware_key_file_.c_str())) {
    cerr << "Couldn't write key signature to verified boot kernel image.\n";
    return false;
  }

  if (!AddKernelSignature(image_, kernel_key_file_.c_str())) {
    cerr << "Couldn't write firmware signature to verified boot kernel image.\n";
    return false;
  }
  return true;
}

bool KernelUtility::VerifySignedImage(void) {
  int error;
  firmware_key_pub_ = RSAPublicKeyFromFile(firmware_key_pub_file_.c_str());
  image_ = ReadKernelImage(in_file_.c_str());

  if (!firmware_key_pub_) {
    cerr << "Couldn't read pre-processed public root key.\n";
    return false;
  }

  if (!image_) {
    cerr << "Couldn't read kernel image or malformed image.\n";
    return false;
  }
  if (!(error = VerifyKernelImage(firmware_key_pub_, image_, 0)))
    return true;
  cerr << VerifyKernelErrorString(error) << "\n";
  return false;
}

bool KernelUtility::CheckOptions(void) {
  if (is_generate_ == is_verify_) {
    cerr << "One of --generate or --verify must be specified.\n";
    return false;
  }
  // Common required options.
  if (in_file_.empty()) {
    cerr << "No input file specified.\n";
    return false;
  }
  // Required options for --verify.
  if (is_verify_ && firmware_key_pub_file_.empty()) {
    cerr << "No pre-processed public firmware key file specified.\n";
    return false;
  }
  // Required options for --generate.
  if (is_generate_) {
    if (firmware_key_file_.empty()) {
      cerr << "No firmware key file specified.\n";
      return false;
    }
    if (kernel_key_file_.empty()) {
      cerr << "No kernel key file specified.\n";
      return false;
    }
    if (kernel_key_pub_file_.empty()) {
      cerr << "No pre-processed public kernel key file specified\n";
      return false;
    }
    if (kernel_key_version_ <= 0 || kernel_key_version_ > UINT16_MAX) {
      cerr << "Invalid or no kernel key version specified.\n";
      return false;
    }
    if (firmware_sign_algorithm_ < 0 ||
        firmware_sign_algorithm_ >= kNumAlgorithms) {
      cerr << "Invalid or no firmware signing key algorithm specified.\n";
      return false;
    }
    if (kernel_sign_algorithm_ < 0 ||
        kernel_sign_algorithm_ >= kNumAlgorithms) {
      cerr << "Invalid or no kernel signing key algorithm specified.\n";
      return false;
    }
    if (kernel_version_ <=0 || kernel_version_ > UINT16_MAX) {
      cerr << "Invalid or no kernel version specified.\n";
      return false;
    }
    if (out_file_.empty()) {
      cerr <<"No output file specified.\n";
      return false;
    }
  }
  return true;
}

}  // namespace vboot_reference

int main(int argc, char* argv[]) {
  vboot_reference::KernelUtility fu;
  if (!fu.ParseCmdLineOptions(argc, argv)) {
    fu.PrintUsage();
    return -1;
  }
  if (fu.is_generate()) {
    if (!fu.GenerateSignedImage())
      return -1;
    fu.OutputSignedImage();
  }
  if (fu.is_verify()) {
    cerr << "Verification ";
    if (fu.VerifySignedImage())
      cerr << "SUCCESS.\n";
    else
      cerr << "FAILURE.\n";
  }
  return 0;
}
