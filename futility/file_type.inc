/* -*- mode:c -*-
 *
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This declares the file types that we can handle. Note that the order may be
 * important for types with recognizer functions, since we generally want to to
 * look for big things first.
 */

/*
 *        enum              --type           desc
 *        recognizer function
 *        show function
 *        sign function
 */
FILE_TYPE(BIOS_IMAGE,       "bios",          "Chrome OS BIOS image",
	  R_(ft_recognize_bios_image),
	  S_(ft_show_bios),
	  S_(ft_sign_bios))
FILE_TYPE(OLD_BIOS_IMAGE,   "oldbios",       "Cr-48 Chrome OS BIOS image",
	  R_(ft_recognize_bios_image),
	  S_(ft_show_bios),
	  S_(ft_sign_bios))
FILE_TYPE(GBB,              "gbb",           "GBB",
	  R_(ft_recognize_gbb),
	  S_(ft_show_gbb),
	  NONE)
FILE_TYPE(FW_PREAMBLE,      "fw_pre",        "VbFirmwarePreamble (VBLOCK_A/B)",
	  R_(ft_recognize_vblock1),
	  S_(ft_show_fw_preamble),
	  NONE)
FILE_TYPE(KERN_PREAMBLE,    "kernel",        "kernel preamble/partition",
	  R_(ft_recognize_vblock1),
	  S_(ft_show_kernel_preamble),
	  S_(ft_sign_kern_preamble))
FILE_TYPE(KEYBLOCK,         "keyblock",      "VbKeyBlock",
	  R_(ft_recognize_vblock1),
	  S_(ft_show_keyblock),
	  NONE)
FILE_TYPE(PUBKEY,           "pubkey",        "VbPublicKey (.vbpubk)",
	  R_(ft_recognize_vb1_key),
	  S_(ft_show_pubkey),
	  S_(ft_sign_pubkey))
FILE_TYPE(PRIVKEY,          "prikey",        "VbPrivateKey (.vbprivk)",
	  R_(ft_recognize_vb1_key),
	  S_(ft_show_privkey),
	  NONE)
FILE_TYPE(VB2_PUBKEY,       "pubkey21",      "vb21 public key (.vbpubk2)",
	  R_(ft_recognize_vb2_key),
	  S_(ft_show_vb2_pubkey),
	  NONE)
FILE_TYPE(VB2_PRIVKEY,	    "prikey21",      "vb21 private key (.vbprik2)",
	  R_(ft_recognize_vb2_key),
	  S_(ft_show_vb2_privkey),
	  NONE)
FILE_TYPE(PEM,		    "pem",           "RSA private key (.pem)",
	  R_(ft_recognize_pem),
	  S_(ft_show_pem),
	  NONE)
FILE_TYPE(RAW_FIRMWARE,     "fwblob",        "raw firmware blob (FW_MAIN_A/B)",
	  NONE,
	  NONE,
	  S_(ft_sign_raw_firmware))
FILE_TYPE(RAW_KERNEL,       "vmlinuz",       "raw linux kernel",
	  NONE,
	  NONE,
	  S_(ft_sign_raw_kernel))
FILE_TYPE(CHROMIUMOS_DISK,  "disk_img",      "chromiumos disk image",
	  NONE,
	  NONE,
	  NONE)
FILE_TYPE(RWSIG,            "rwsig",         "RW device image",
	  R_(ft_recognize_rwsig),
	  S_(ft_show_rwsig),
	  S_(ft_sign_rwsig))
/* Firmware for USB Type-C power adapters */
FILE_TYPE(USBPD1,           "usbpd1",        "USB-PD charger image (v1.0)",
	  R_(ft_recognize_usbpd1),
	  S_(ft_show_usbpd1),
	  S_(ft_sign_usbpd1))
