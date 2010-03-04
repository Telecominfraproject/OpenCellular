// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utility for manipulating verified boot firmware images.
//

#include "firmware_utility.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>  // Needed for UINT16_MAX.
#include <stdlib.h>
#include <unistd.h>

#include <iostream>

extern "C" {
#include "file_keys.h"
#include "firmware_image.h"
#include "padding.h"
#include "rsa_utility.h"
#include "sha_utility.h"
#include "utility.h"
}

extern int errno;
using std::cerr;

namespace vboot_reference {

FirmwareUtility::FirmwareUtility():
    image_(NULL),
    root_key_pub_(NULL),
    firmware_version_(-1),
    firmware_key_version_(-1),
    firmware_sign_algorithm_(-1),
    is_generate_(false),
    is_verify_(false) {
}

FirmwareUtility::~FirmwareUtility() {
  RSAPublicKeyFree(root_key_pub_);
  FirmwareImageFree(image_);
}

void FirmwareUtility::PrintUsage(void) {
  cerr <<
      "Utility to generate/verify a verified boot firmware image\n\n"
      "Usage: firmware_utility <--generate|--verify> [OPTIONS]\n\n"
      "For \"--verify\",  required OPTIONS are:\n"
      "--in <infile>\t\t\tVerified boot firmware image to verify.\n"
      "--root_key_pub <pubkeyfile>\tPre-processed public root key "
      "to use for verification.\n\n"
      "For \"--generate\", required OPTIONS are:\n"
      "--root_key <privkeyfile>\tPrivate root key file\n"
      "--firmware_sign_key <privkeyfile>\tPrivate signing key file\n"
      "--firmware_sign_key_pub <pubkeyfile>\tPre-processed public signing"
      " key\n"
      "--firmware_sign_algorithm <algoid>\tSigning algorithm to use\n"
      "--firmware_key_version <version#>\tSigning Key Version#\n"
      "--firmware_version <version#>\tFirmware Version#\n"
      "--in <infile>\t\t\tFirmware Image to sign\n"
      "--out <outfile>\t\t\tOutput file for verified boot firmware image\n\n"
      "<algoid> (for --sign-algorithm) is one of the following:\n";
  for (int i = 0; i < kNumAlgorithms; i++) {
    cerr << i << " for " << algo_strings[i] << "\n";
  }
  cerr << "\n\n";
}

bool FirmwareUtility::ParseCmdLineOptions(int argc, char* argv[]) {
  int option_index;
  static struct option long_options[] = {
    {"root_key", 1, 0, 0},
    {"root_key_pub", 1, 0, 0},
    {"firmware_sign_key", 1, 0, 0},
    {"firmware_sign_key_pub", 1, 0, 0},
    {"firmware_sign_algorithm", 1, 0, 0},
    {"firmware_key_version", 1, 0, 0},
    {"firmware_version", 1, 0, 0},
    {"in", 1, 0, 0},
    {"out", 1, 0, 0},
    {"generate", 0, 0, 0},
    {"verify", 0, 0, 0},
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
        case 0:  // root_key
          root_key_file_ = optarg;
          break;
        case 1:  // root_key_pub
          root_key_pub_file_ = optarg;
          break;
        case 2:  // firmware_sign_key
          firmware_key_file_ = optarg;
          break;
        case 3:  // firmware_sign_key_pub
          firmware_key_pub_file_ = optarg;
          break;
        case 4:  // firmware_sign_algorithm
          errno = 0;  // strtol() returns an error via errno
          firmware_sign_algorithm_ = strtol(optarg,
                                            reinterpret_cast<char**>(NULL), 10);
          if (errno)
            return false;
          break;
        case 5:  // firmware_key_version
          errno = 0;
          firmware_key_version_ = strtol(optarg,
                                         reinterpret_cast<char**>(NULL), 10);
          if (errno)
            return false;
          break;
        case 6:  // firmware_version
          errno = 0;
          firmware_version_ = strtol(optarg,
                                     reinterpret_cast<char**>(NULL), 10);
          if (errno)
            return false;
          break;
        case 7:  // in
          in_file_ = optarg;
          break;
        case 8:  // out
          out_file_ = optarg;
          break;
        case 9:  // generate
          is_generate_ = true;
          break;
        case 10: // verify
          is_verify_ = true;
          break;
      }
    }
  }
  return CheckOptions();
}


void FirmwareUtility::OutputSignedImage(void) {
  if (image_) {
    if (!WriteFirmwareImage(out_file_.c_str(), image_)) {
        cerr << "Couldn't write verified boot image to file "
                  << out_file_ <<".\n";
    }
  }
}

bool FirmwareUtility::GenerateSignedImage(void) {
  uint32_t firmware_sign_key_pub_len;
  uint8_t* header_checksum;
  DigestContext ctx;
  image_ = FirmwareImageNew();

  Memcpy(image_->magic, FIRMWARE_MAGIC, FIRMWARE_MAGIC_SIZE);

  // Copy pre-processed public signing key.
  image_->firmware_sign_algorithm = (uint16_t) firmware_sign_algorithm_;
  image_->firmware_sign_key = BufferFromFile(
      firmware_key_pub_file_.c_str(),
      &firmware_sign_key_pub_len);
  if (!image_->firmware_sign_key)
    return false;
  image_->firmware_key_version = firmware_key_version_;

  // Update header length.
  image_->header_len = GetFirmwareHeaderLen(image_);

  // Calculate header checksum.
  DigestInit(&ctx, SHA512_DIGEST_ALGORITHM);
  DigestUpdate(&ctx, reinterpret_cast<uint8_t*>(&image_->header_len),
               sizeof(image_->header_len));
  DigestUpdate(&ctx,
               reinterpret_cast<uint8_t*>(&image_->firmware_sign_algorithm),
               sizeof(image_->firmware_sign_algorithm));
  DigestUpdate(&ctx, image_->firmware_sign_key,
               RSAProcessedKeySize(image_->firmware_sign_algorithm));
  DigestUpdate(&ctx, reinterpret_cast<uint8_t*>(&image_->firmware_key_version),
               sizeof(image_->firmware_key_version));
  header_checksum = DigestFinal(&ctx);
  Memcpy(image_->header_checksum, header_checksum, SHA512_DIGEST_SIZE);
  Free(header_checksum);

  image_->firmware_version = firmware_version_;
  image_->firmware_len = 0;
  // TODO(gauravsh): Populate this with the right bytes once we decide
  // what goes into the preamble.
  Memset(image_->preamble, 'P', FIRMWARE_PREAMBLE_SIZE);
  image_->firmware_data = BufferFromFile(in_file_.c_str(),
                                         &image_->firmware_len);
  if (!image_)
    return false;
  // Generate and add the signatures.
  if (!AddFirmwareKeySignature(image_, root_key_file_.c_str())) {
    cerr << "Couldn't write key signature to verified boot image.\n";
    return false;
  }

  if (!AddFirmwareSignature(image_, firmware_key_file_.c_str())) {
    cerr << "Couldn't write firmware signature to verified boot image.\n";
    return false;
  }
  return true;
}

bool FirmwareUtility::VerifySignedImage(void) {
  int error;
  root_key_pub_ = RSAPublicKeyFromFile(root_key_pub_file_.c_str());
  image_ = ReadFirmwareImage(in_file_.c_str());

  if (!root_key_pub_) {
    cerr << "Couldn't read pre-processed public root key.\n";
    return false;
  }

  if (!image_) {
    cerr << "Couldn't read firmware image or malformed image.\n";
    return false;
  }
  if (!(error = VerifyFirmwareImage(root_key_pub_, image_,
                                    0)))  // Trusted Mode.
    return true;
  cerr << VerifyFirmwareErrorString(error) << "\n";
  return false;;
}

bool FirmwareUtility::CheckOptions(void) {
  if (is_generate_ == is_verify_) {
    cerr << "One of --generate or --verify must be specified.\n";
    return false;
  }
  // Common required options.
  if (in_file_.empty()) {
    cerr << "No input file specified." << "\n";
    return false;
  }
  // Required options for --verify.
  if (is_verify_ && root_key_pub_file_.empty()) {
    cerr << "No pre-processed public root key file specified." << "\n";
    return false;
  }
  // Required options for --generate.
  if (is_generate_) {
    if (root_key_file_.empty()) {
      cerr << "No root key file specified." << "\n";
      return false;
    }
    if (firmware_version_ <= 0 || firmware_version_ > UINT16_MAX) {
      cerr << "Invalid or no firmware version specified." << "\n";
      return false;
    }
    if (firmware_key_file_.empty()) {
      cerr << "No signing key file specified." << "\n";
      return false;
    }
    if (firmware_key_pub_file_.empty()) {
      cerr << "No pre-processed public signing key file specified." << "\n";
      return false;
    }
    if (firmware_key_version_ <= 0 || firmware_key_version_ > UINT16_MAX) {
      cerr << "Invalid or no key version specified." << "\n";
      return false;
    }
    if (firmware_sign_algorithm_ < 0 ||
        firmware_sign_algorithm_ >= kNumAlgorithms) {
      cerr << "Invalid or no signing key algorithm specified." << "\n";
      return false;
    }
    if (out_file_.empty()) {
      cerr <<"No output file specified." << "\n";
      return false;
    }
  }
  return true;
}


}  // namespace vboot_reference

int main(int argc, char* argv[]) {
  vboot_reference::FirmwareUtility fu;
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
