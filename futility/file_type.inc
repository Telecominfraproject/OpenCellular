/* -*- mode:c -*-
 *
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*        enum              --type           desc */
FILE_TYPE(UNKNOWN,          "unknown",       "not something we know about")
FILE_TYPE(PUBKEY,           "pubkey",        "VbPublicKey (.vbpubk)")
FILE_TYPE(KEYBLOCK,         "keyblock",      "VbKeyBlock")
FILE_TYPE(FW_PREAMBLE,      "fw_pre",        "VbFirmwarePreamble (VBLOCK_A/B)")
FILE_TYPE(GBB,              "gbb",           "GBB")
FILE_TYPE(BIOS_IMAGE,       "bios",          "Chrome OS BIOS image")
FILE_TYPE(OLD_BIOS_IMAGE,   "oldbios",       "Cr-48 Chrome OS BIOS image")
FILE_TYPE(KERN_PREAMBLE,    "kernel",        "kernel preamble/partition")
FILE_TYPE(RAW_FIRMWARE,     "fwblob",        "raw firmware blob (FW_MAIN_A/B)")
FILE_TYPE(RAW_KERNEL,       "vmlinuz",       "raw linux kernel")
FILE_TYPE(CHROMIUMOS_DISK,  "disk_img",      "chromiumos disk image")
FILE_TYPE(PRIVKEY,          "prikey",        "VbPrivateKey (.vbprivk)")
FILE_TYPE(VB2_PUBKEY,       "pubkey21",      "vb21 public key (.vbpubk2)")
FILE_TYPE(VB2_PRIVKEY,	    "prikey21",      "vb21 private key (.vbprik2)")
FILE_TYPE(PEM,		    "pem",           "RSA private key (.pem)")
