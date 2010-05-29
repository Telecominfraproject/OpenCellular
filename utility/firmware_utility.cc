// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utility for manipulating verified boot firmware images.
//

#include "firmware_utility.h"

#include <getopt.h>
#include <stdio.h>
#include <stdint.h>  // Needed for UINT16_MAX.
#include <stdlib.h>
#include <unistd.h>

#include <iostream>

extern "C" {
#include "cryptolib.h"
#include "file_keys.h"
#include "firmware_image.h"
#include "stateful_util.h"
}

using std::cerr;

// Macro to determine the size of a field structure in the FirmwareImage
// structure.
#define FIELD_LEN(field) (sizeof(((FirmwareImage*)0)->field))

namespace vboot_reference {

FirmwareUtility::FirmwareUtility():
    image_(NULL),
    root_key_pub_(NULL),
    firmware_key_version_(-1),
    firmware_sign_algorithm_(-1),
    firmware_version_(-1),
    is_generate_(false),
    is_verify_(false),
    is_describe_(false),
    is_only_vblock_(false),
    is_subkey_out_(false) {
}

FirmwareUtility::~FirmwareUtility() {
  RSAPublicKeyFree(root_key_pub_);
  FirmwareImageFree(image_);
}

void FirmwareUtility::PrintUsage(void) {
  cerr <<
      "Utility to generate/verify a verified boot firmware image\n"
      "\n"
      "Usage: firmware_utility <--generate|--verify> [OPTIONS]\n"
      "\n"
      "For \"--verify\",  required OPTIONS are:\n"
      "  --in <infile>\t\t\tVerified boot firmware image to verify.\n"
      "  --root_key_pub <pubkeyfile>\tPre-processed public root key "
      "to use for verification.\n"
      "\n"
      "For \"--generate\", required OPTIONS are:\n"
      "  --root_key <privkeyfile>\t\tPrivate root key file\n"
      "  --firmware_key_pub <pubkeyfile>\tPre-processed public signing"
      " key\n"
      "  --firmware_sign_algorithm <algoid>\tSigning algorithm to use\n"
      "  --firmware_key_version <version#>\tSigning Key Version#\n"
      "OR\n"
      "  --subkey_in <subkeyfile>\t\tExisting key signature header\n"
      "\n"
      "  --firmware_key <privkeyfile>\tPrivate signing key file\n"
      "  --firmware_version <version#>\tFirmware Version#\n"
      "  --in <infile>\t\t\tFirmware Image to sign\n"
      "  --out <outfile>\t\tOutput file for verified boot firmware image\n"
      "\n"
      "Optional:\n"
      "  --subkey_out\t\t\tJust output the subkey (key verification) header\n"
      "  --vblock\t\t\tJust output the verification block\n"
      "\n"
      "<algoid> (for --sign-algorithm) is one of the following:\n";
  for (int i = 0; i < kNumAlgorithms; i++) {
    cerr << i << " for " << algo_strings[i] << "\n";
  }
  cerr << "\n\n";
}

bool FirmwareUtility::ParseCmdLineOptions(int argc, char* argv[]) {
  int option_index, i;
  char *e = 0;
  enum {
    OPT_ROOT_KEY = 1000,
    OPT_ROOT_KEY_PUB,
    OPT_FIRMWARE_KEY,
    OPT_FIRMWARE_KEY_PUB,
    OPT_SUBKEY_IN,
    OPT_FIRMWARE_SIGN_ALGORITHM,
    OPT_FIRMWARE_KEY_VERSION,
    OPT_FIRMWARE_VERSION,
    OPT_IN,
    OPT_OUT,
    OPT_GENERATE,
    OPT_VERIFY,
    OPT_DESCRIBE,
    OPT_VBLOCK,
    OPT_SUBKEY_OUT,
  };
  static struct option long_options[] = {
    {"root_key", 1, 0,                  OPT_ROOT_KEY                },
    {"root_key_pub", 1, 0,              OPT_ROOT_KEY_PUB            },
    {"firmware_key", 1, 0,              OPT_FIRMWARE_KEY            },
    {"firmware_key_pub", 1, 0,          OPT_FIRMWARE_KEY_PUB        },
    {"subkey_in", 1, 0,                 OPT_SUBKEY_IN               },
    {"firmware_sign_algorithm", 1, 0,   OPT_FIRMWARE_SIGN_ALGORITHM },
    {"firmware_key_version", 1, 0,      OPT_FIRMWARE_KEY_VERSION    },
    {"firmware_version", 1, 0,          OPT_FIRMWARE_VERSION        },
    {"in", 1, 0,                        OPT_IN                      },
    {"out", 1, 0,                       OPT_OUT                     },
    {"generate", 0, 0,                  OPT_GENERATE                },
    {"verify", 0, 0,                    OPT_VERIFY                  },
    {"describe", 0, 0,                  OPT_DESCRIBE                },
    {"vblock", 0, 0,                    OPT_VBLOCK                  },
    {"subkey_out", 0, 0,                OPT_SUBKEY_OUT              },
    {NULL, 0, 0, 0}
  };
  while ((i = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
    switch (i) {
      case '?':
        return false;
        break;
      case OPT_ROOT_KEY:
        root_key_file_ = optarg;
        break;
      case OPT_ROOT_KEY_PUB:
        root_key_pub_file_ = optarg;
        break;
      case OPT_FIRMWARE_KEY:
        firmware_key_file_ = optarg;
        break;
      case OPT_FIRMWARE_KEY_PUB:
        firmware_key_pub_file_ = optarg;
        break;
      case OPT_SUBKEY_IN:
        subkey_in_file_ = optarg;
        break;
      case OPT_FIRMWARE_SIGN_ALGORITHM:
        firmware_sign_algorithm_ = strtol(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          cerr << "Invalid argument to --"
               << long_options[option_index].name
               << ": " << optarg << "\n";
          return false;
        }
        break;
      case OPT_FIRMWARE_KEY_VERSION:
        firmware_key_version_ = strtol(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          cerr << "Invalid argument to --"
               << long_options[option_index].name
               << ": " << optarg << "\n";
          return false;
        }
        break;
      case OPT_FIRMWARE_VERSION:
        firmware_version_ = strtol(optarg, &e, 0);
        if (!*optarg || (e && *e)) {
          cerr << "Invalid argument to --"
               << long_options[option_index].name
               << ": " << optarg << "\n";
          return false;
        }
        break;
      case OPT_IN:
        in_file_ = optarg;
        break;
      case OPT_OUT:
        out_file_ = optarg;
        break;
      case OPT_GENERATE:
        is_generate_ = true;
        break;
      case OPT_VERIFY:
          is_verify_ = true;
          break;
      case OPT_DESCRIBE:
        is_describe_ = true;
        break;
      case OPT_VBLOCK:
        is_only_vblock_ = true;
        break;
      case OPT_SUBKEY_OUT:
        is_subkey_out_ = true;
        break;
    }
  }
  return CheckOptions();
}


void FirmwareUtility::OutputSignedImage(void) {
  if (image_) {
    if (!WriteFirmwareImage(out_file_.c_str(), image_,
                            is_only_vblock_,
                            is_subkey_out_)) {
        cerr << "Couldn't write verified boot image to file "
                  << out_file_ <<".\n";
    }
  }
}

void FirmwareUtility::DescribeSignedImage(void) {
  image_ = ReadFirmwareImage(in_file_.c_str());
  if (!image_) {
    cerr << "Couldn't read firmware image or malformed image.\n";
  }
  PrintFirmwareImage(image_);
}

bool FirmwareUtility::GenerateSignedImage(void) {
  uint64_t firmware_sign_key_pub_len;

  image_ = FirmwareImageNew();
  Memcpy(image_->magic, FIRMWARE_MAGIC, FIRMWARE_MAGIC_SIZE);

  if (subkey_in_file_.empty()) {
    // We muse generate the firmware key signature header (subkey header)
    // ourselves.
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
    CalculateFirmwareHeaderChecksum(image_, image_->header_checksum);

    image_->firmware_version = firmware_version_;
    image_->firmware_len = 0;

    // Generate and add the key signatures.
    if (!AddFirmwareKeySignature(image_, root_key_file_.c_str())) {
      cerr << "Couldn't write key signature to verified boot image.\n";
      return false;
    }
  } else {
    // Use existing subkey header.
    MemcpyState st;
    uint8_t* subkey_header_buf = NULL;
    uint64_t subkey_len;
    int header_len;
    int firmware_sign_key_len;
    uint8_t header_checksum[FIELD_LEN(header_checksum)];

    subkey_header_buf = BufferFromFile(subkey_in_file_.c_str(), &subkey_len);
    if (!subkey_header_buf) {
      cerr << "Couldn't read subkey header from file %s\n"
           << subkey_in_file_.c_str();
      return false;
    }
    st.remaining_len = subkey_len;
    st.remaining_buf = subkey_header_buf;
    st.overrun = 0;

    // TODO(gauravsh): This is basically the same code as the first half of
    // of ReadFirmwareImage(). Refactor to eliminate code duplication.

    StatefulMemcpy(&st, &image_->header_len, FIELD_LEN(header_len));
    StatefulMemcpy(&st, &image_->firmware_sign_algorithm,
                   FIELD_LEN(firmware_sign_algorithm));

    // Valid Algorithm?
    if (image_->firmware_sign_algorithm >= kNumAlgorithms) {
      Free(subkey_header_buf);
      return NULL;
    }

    // Compute size of pre-processed RSA public key and signature.
    firmware_sign_key_len = RSAProcessedKeySize(image_->firmware_sign_algorithm);

    // Check whether the header length is correct.
    header_len = GetFirmwareHeaderLen(image_);
    if (header_len != image_->header_len) {
      debug("Header length mismatch. Got: %d Expected: %d\n",
            image_->header_len, header_len);
      Free(subkey_header_buf);
      return NULL;
    }

    // Read pre-processed public half of the sign key.
    StatefulMemcpy(&st, &image_->firmware_key_version,
                 FIELD_LEN(firmware_key_version));
    image_->firmware_sign_key = (uint8_t*) Malloc(firmware_sign_key_len);
    StatefulMemcpy(&st, image_->firmware_sign_key, firmware_sign_key_len);
    StatefulMemcpy(&st, image_->header_checksum, FIELD_LEN(header_checksum));

    // Check whether the header checksum matches.
    CalculateFirmwareHeaderChecksum(image_, header_checksum);
    if (SafeMemcmp(header_checksum, image_->header_checksum,
                   FIELD_LEN(header_checksum))) {
      debug("Invalid firmware header checksum!\n");
      Free(subkey_header_buf);
      return NULL;
    }

    // Read key signature.
    StatefulMemcpy(&st, image_->firmware_key_signature,
                   FIELD_LEN(firmware_key_signature));

    Free(subkey_header_buf);
    if (st.overrun || st.remaining_len != 0)  // Overrun or underrun.
      return false;
    return true;
  }

  // TODO(gauravsh): Populate this with the right bytes once we decide
  // what goes into the preamble.
  Memset(image_->preamble, 'P', FIRMWARE_PREAMBLE_SIZE);
  image_->firmware_data = BufferFromFile(in_file_.c_str(),
                                         &image_->firmware_len);
  if (!image_->firmware_data)
    return false;

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
  if (VERIFY_FIRMWARE_SUCCESS ==
      (error = VerifyFirmwareImage(root_key_pub_, image_)))
    return true;
  cerr << VerifyFirmwareErrorString(error) << "\n";
  return false;;
}

bool FirmwareUtility::CheckOptions(void) {
  // Ensure that only one of --{describe|generate|verify} is set.
  if (!((is_describe_ && !is_generate_ && !is_verify_) ||
        (!is_describe_ && is_generate_ && !is_verify_) ||
        (!is_describe_ && !is_generate_ && is_verify_))) {
    cerr << "One (and only one) of --describe, --generate or --verify "
         << "must be specified.\n";
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
    if (subkey_in_file_.empty()) {
      // Root key, kernel signing public key, and firmware signing
      // algorithm are required to generate the key signature header.
      if (root_key_file_.empty()) {
        cerr << "No root key file specified." << "\n";
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
    }
    if (firmware_key_file_.empty()) {
      cerr << "No signing key file specified." << "\n";
        return false;
      }
    if (firmware_version_ <= 0 || firmware_version_ > UINT16_MAX) {
        cerr << "Invalid or no firmware version specified." << "\n";
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
  if (fu.is_describe()) {
    fu.DescribeSignedImage();
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
