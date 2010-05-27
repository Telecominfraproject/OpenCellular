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
#include "cryptolib.h"
#include "file_keys.h"
#include "kernel_image.h"
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
                                padding_(0),
                                kernel_len_(0),
                                is_generate_(false),
                                is_verify_(false),
                                is_describe_(false),
                                is_only_vblock_(false) {
}

KernelUtility::~KernelUtility() {
  RSAPublicKeyFree(firmware_key_pub_);
  KernelImageFree(image_);
}

void KernelUtility::PrintUsage(void) {
  cerr << "\n"
      "Utility to generate/verify/describe a verified boot kernel image\n"
      "\n"
      "Usage: kernel_utility <--generate|--verify|--describe> [OPTIONS]\n"
      "\n"
      "For \"--describe\", the required OPTIONS are:\n"
      "  --in <infile>\t\t\t\tSigned boot image to describe.\n"
      "\n"
      "For \"--verify\",  required OPTIONS are:\n"
      "  --in <infile>\t\t\t\tSigned boot image to verify.\n"
      "  --firmware_key_pub <pubkeyfile>\tPre-processed public firmware key\n"
      "\n"
      "For \"--generate\", required OPTIONS are:\n"
      "  --firmware_key <privkeyfile>\t\tPrivate firmware signing key file\n"
      "  --kernel_key <privkeyfile>\t\tPrivate kernel signing key file\n"
      "  --kernel_key_pub <pubkeyfile>\t\tPre-processed public kernel signing"
      " key\n"
      "  --firmware_sign_algorithm <algoid>\tSigning algorithm for firmware\n"
      "  --kernel_sign_algorithm <algoid>\tSigning algorithm for kernel\n"
      "  --kernel_key_version <number>\t\tKernel signing key version number\n"
      "  --kernel_version <number>\t\tKernel Version number\n"
      "  --config <file>\t\t\tEmbedded kernel command-line parameters\n"
      "  --bootloader <file>\t\t\tEmbedded bootloader stub\n"
      "  --vmlinuz <file>\t\t\tEmbedded kernel image\n"
      "  --out <outfile>\t\t\tOutput file for verified boot image\n"
      "\n"
      "Optional arguments for \"--generate\" are:\n"
      "  --vblock\t\t\t\tJust output the verification block\n"
      "  --padding\t\t\t\tPad the header to this size\n"
      "\n"
      "<algoid> (for --*_sign_algorithm) is one of the following:\n";
  for (int i = 0; i < kNumAlgorithms; i++) {
    cerr << "  " << i << " for " << algo_strings[i] << "\n";
  }
  cerr << "\n\n";
}

bool KernelUtility::ParseCmdLineOptions(int argc, char* argv[]) {
  int option_index, i;
  char *e = 0;
  enum {
    OPT_FIRMWARE_KEY = 1000,
    OPT_FIRMWARE_KEY_PUB,
    OPT_KERNEL_KEY,
    OPT_KERNEL_KEY_PUB,
    OPT_FIRMWARE_SIGN_ALGORITHM,
    OPT_KERNEL_SIGN_ALGORITHM,
    OPT_KERNEL_KEY_VERSION,
    OPT_KERNEL_VERSION,
    OPT_IN,
    OPT_OUT,
    OPT_GENERATE,
    OPT_VERIFY,
    OPT_DESCRIBE,
    OPT_VBLOCK,
    OPT_BOOTLOADER,
    OPT_VMLINUZ,
    OPT_CONFIG,
    OPT_PADDING,
  };
  static struct option long_options[] = {
    {"firmware_key", 1, 0,              OPT_FIRMWARE_KEY            },
    {"firmware_key_pub", 1, 0,          OPT_FIRMWARE_KEY_PUB        },
    {"kernel_key", 1, 0,                OPT_KERNEL_KEY              },
    {"kernel_key_pub", 1, 0,            OPT_KERNEL_KEY_PUB          },
    {"firmware_sign_algorithm", 1, 0,   OPT_FIRMWARE_SIGN_ALGORITHM },
    {"kernel_sign_algorithm", 1, 0,     OPT_KERNEL_SIGN_ALGORITHM   },
    {"kernel_key_version", 1, 0,        OPT_KERNEL_KEY_VERSION      },
    {"kernel_version", 1, 0,            OPT_KERNEL_VERSION          },
    {"in", 1, 0,                        OPT_IN                      },
    {"out", 1, 0,                       OPT_OUT                     },
    {"generate", 0, 0,                  OPT_GENERATE                },
    {"verify", 0, 0,                    OPT_VERIFY                  },
    {"describe", 0, 0,                  OPT_DESCRIBE                },
    {"vblock", 0, 0,                    OPT_VBLOCK                  },
    {"bootloader", 1, 0,                OPT_BOOTLOADER              },
    {"vmlinuz", 1, 0,                   OPT_VMLINUZ                 },
    {"config", 1, 0,                    OPT_CONFIG                  },
    {"padding", 1, 0,                   OPT_PADDING                 },
    {NULL, 0, 0, 0}
  };
  while ((i = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
    switch (i) {
    case '?':
      return false;
      break;
    case OPT_FIRMWARE_KEY:
      firmware_key_file_ = optarg;
      break;
    case OPT_FIRMWARE_KEY_PUB:
      firmware_key_pub_file_ = optarg;
      break;
    case OPT_KERNEL_KEY:
      kernel_key_file_ = optarg;
      break;
    case OPT_KERNEL_KEY_PUB:
      kernel_key_pub_file_ = optarg;
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
    case OPT_KERNEL_SIGN_ALGORITHM:
      errno = 0;
      kernel_sign_algorithm_ = strtol(optarg, &e, 0);
      if (!*optarg || (e && *e)) {
        cerr << "Invalid argument to --"
             << long_options[option_index].name
             << ": " << optarg << "\n";
        return false;
      }
      break;
    case OPT_KERNEL_KEY_VERSION:
      errno = 0;
      kernel_key_version_ = strtol(optarg, &e, 0);
      if (!*optarg || (e && *e)) {
        cerr << "Invalid argument to --"
             << long_options[option_index].name
             << ": " << optarg << "\n";
        return false;
      }
      break;
    case OPT_KERNEL_VERSION:
      errno = 0;
      kernel_version_ = strtol(optarg, &e, 0);
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
    case OPT_BOOTLOADER:
      bootloader_file_ = optarg;
      break;
    case OPT_VMLINUZ:
      vmlinuz_file_ = optarg;
      break;
    case OPT_CONFIG:
      config_file_ = optarg;
      break;
    case OPT_PADDING:
      padding_ = strtol(optarg, &e, 0);
      if (!*optarg || (e && *e)) {
        cerr << "Invalid argument to --"
             << long_options[option_index].name
             << ": " << optarg << "\n";
        return false;
      }
      break;
    }
  }
  return CheckOptions();
}

void KernelUtility::OutputSignedImage(void) {
  if (image_) {
    if (!WriteKernelImage(out_file_.c_str(), image_, is_only_vblock_)) {
      cerr << "Couldn't write verified boot kernel image to file "
           << out_file_ <<".\n";
    }
  }
}

void KernelUtility::DescribeSignedImage(void) {
  image_ = ReadKernelImage(in_file_.c_str());
  if (!image_) {
    cerr << "Couldn't read kernel image or malformed image.\n";
    return;
  }
  PrintKernelImage(image_);
}

bool KernelUtility::GenerateSignedImage(void) {
  uint64_t kernel_key_pub_len;
  image_ = KernelImageNew();

  Memcpy(image_->magic, KERNEL_MAGIC, KERNEL_MAGIC_SIZE);

  // TODO(gauravsh): make this a command line option.
  image_->header_version = 1;
  if (padding_)
    image_->padded_header_size = padding_;
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
  CalculateKernelHeaderChecksum(image_, image_->header_checksum);

  image_->kernel_version = kernel_version_;

  image_->kernel_data = GenerateKernelBlob(vmlinuz_file_.c_str(),
                                           config_file_.c_str(),
                                           bootloader_file_.c_str(),
                                           &image_->kernel_len,
                                           &image_->bootloader_offset,
                                           &image_->bootloader_size);
  if (!image_->kernel_data)
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
  // Ensure that only one of --{describe|generate|verify} is set.
  if (!((is_describe_ && !is_generate_ && !is_verify_) ||
        (!is_describe_ && is_generate_ && !is_verify_) ||
        (!is_describe_ && !is_generate_ && is_verify_))) {
    cerr << "One (and only one) of --describe, --generate or --verify "
         << "must be specified.\n";
    return false;
  }
  // Common required options.
  // Required options for --describe.
  if (is_describe_) {
    if (in_file_.empty()) {
      cerr << "No input file specified.\n";
      return false;
    }
  }
  // Required options for --verify.
  if (is_verify_) {
    if (firmware_key_pub_file_.empty()) {
      cerr << "No pre-processed public firmware key file specified.\n";
      return false;
    }
    if (in_file_.empty()) {
      cerr << "No input file specified.\n";
      return false;
    }
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
    if (config_file_.empty()) {
      cerr << "No config file specified.\n";
      return false;
    }
    if (bootloader_file_.empty()) {
      cerr << "No bootloader file specified.\n";
      return false;
    }
    if (vmlinuz_file_.empty()) {
      cerr << "No vmlinuz file specified.\n";
      return false;
    }
  }
  return true;
}

}  // namespace vboot_reference

int main(int argc, char* argv[]) {
  vboot_reference::KernelUtility ku;
  if (!ku.ParseCmdLineOptions(argc, argv)) {
    ku.PrintUsage();
    return -1;
  }
  if (ku.is_describe()) {
    ku.DescribeSignedImage();
  }
  else if (ku.is_generate()) {
    if (!ku.GenerateSignedImage())
      return -1;
    ku.OutputSignedImage();
  }
  else if (ku.is_verify()) {
    cerr << "Verification ";
    if (ku.VerifySignedImage())
      cerr << "SUCCESS.\n";
    else
      cerr << "FAILURE.\n";
  }
  return 0;
}
