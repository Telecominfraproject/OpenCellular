/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - entry points for kernel selection
 */

#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"


/* Global variables */
static uint32_t disp_current_screen = VB_SCREEN_BLANK;
static uint32_t disp_width = 0, disp_height = 0;
static VbNvContext vnc;


#ifdef CHROMEOS_ENVIRONMENT
/* Global variable accessors for unit tests */
VbNvContext* VbApiKernelGetVnc(void) {
  return &vnc;
}
#endif


/* Set recovery request */
static void VbSetRecoveryRequest(uint32_t recovery_request) {
  VBDEBUG(("VbSetRecoveryRequest(%d)\n", (int)recovery_request));
  VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, recovery_request);
}


/* Get the number of localizations in the GBB bitmap data. */
static VbError_t VbGetLocalizationCount(VbCommonParams* cparams,
                                        uint32_t* count) {
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)cparams->gbb_data;
  BmpBlockHeader* hdr;

  /* Default to 0 on error */
  *count = 0;

  /* Make sure the bitmap data is inside the GBB and is non-zero in size */
  if (0 == gbb->bmpfv_size ||
      gbb->bmpfv_offset > cparams->gbb_size ||
      gbb->bmpfv_offset + gbb->bmpfv_size > cparams->gbb_size) {
    return VBERROR_INVALID_GBB;
  }

  /* Sanity-check the bitmap block header */
  hdr = (BmpBlockHeader *)(((uint8_t*)gbb) + gbb->bmpfv_offset);
  if ((0 != Memcmp(hdr->signature, BMPBLOCK_SIGNATURE,
                   BMPBLOCK_SIGNATURE_SIZE)) ||
      (hdr->major_version > BMPBLOCK_MAJOR_VERSION) ||
      ((hdr->major_version == BMPBLOCK_MAJOR_VERSION) &&
       (hdr->minor_version > BMPBLOCK_MINOR_VERSION))) {
    return VBERROR_INVALID_BMPFV;
  }

  *count = hdr->number_of_localizations;
  return VBERROR_SUCCESS;
}


/* Display a screen from the GBB. */
static VbError_t VbDisplayScreenFromGBB(VbCommonParams* cparams,
                                        uint32_t screen) {
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)cparams->gbb_data;
  uint8_t* bmpfv = NULL;
  BmpBlockHeader* hdr;
  ScreenLayout* layout;
  ImageInfo* image_info;
  uint32_t screen_index;
  uint32_t localization = 0;
  VbError_t retval = VBERROR_UNKNOWN;  /* Assume error until proven
                                        * successful */
  uint32_t offset;
  uint32_t i;

  /* Make sure the bitmap data is inside the GBB and is non-zero in size */
  if (0 == gbb->bmpfv_size ||
      gbb->bmpfv_offset > cparams->gbb_size ||
      gbb->bmpfv_offset + gbb->bmpfv_size > cparams->gbb_size) {
    VBDEBUG(("VbDisplayScreenFromGBB(): invalid bmpfv offset/size\n"));
    return VBERROR_INVALID_GBB;
  }

  /* Copy bitmap data from GBB into RAM for speed */
  bmpfv = (uint8_t*)VbExMalloc(gbb->bmpfv_size);
  Memcpy(bmpfv, ((uint8_t*)gbb) + gbb->bmpfv_offset, gbb->bmpfv_size);

  /* Sanity-check the bitmap block header */
  hdr = (BmpBlockHeader *)bmpfv;
  if ((0 != Memcmp(hdr->signature, BMPBLOCK_SIGNATURE,
                   BMPBLOCK_SIGNATURE_SIZE)) ||
      (hdr->major_version > BMPBLOCK_MAJOR_VERSION) ||
      ((hdr->major_version == BMPBLOCK_MAJOR_VERSION) &&
       (hdr->minor_version > BMPBLOCK_MINOR_VERSION))) {
    VBDEBUG(("VbDisplayScreenFromGBB(): invalid/too new bitmap header\n"));
    retval = VBERROR_INVALID_BMPFV;
    goto VbDisplayScreenFromGBB_exit;
  }

  /* Translate screen ID into index.  Note that not all screens are in the
   * GBB. */
  /* TODO: ensure screen IDs match indices?  Having this translation
   * here is awful. */
  switch (screen) {
    case VB_SCREEN_DEVELOPER_WARNING:
      screen_index = 0;
      break;
    case VB_SCREEN_RECOVERY_REMOVE:
      screen_index = 1;
      break;
    case VB_SCREEN_RECOVERY_NO_GOOD:
      screen_index = 2;
      break;
    case VB_SCREEN_RECOVERY_INSERT:
      screen_index = 3;
      break;
    case VB_SCREEN_BLANK:
    case VB_SCREEN_DEVELOPER_EGG:
    default:
      /* Screens which aren't in the GBB */
      VBDEBUG(("VbDisplayScreenFromGBB(): screen %d not in the GBB\n",
               (int)screen));
      retval = VBERROR_INVALID_SCREEN_INDEX;
      goto VbDisplayScreenFromGBB_exit;
  }
  if (screen_index >= hdr->number_of_screenlayouts) {
    VBDEBUG(("VbDisplayScreenFromGBB(): screen %d index %d not in the GBB\n",
             (int)screen, (int)screen_index));
    retval = VBERROR_INVALID_SCREEN_INDEX;
    goto VbDisplayScreenFromGBB_exit;
  }

  /* Clip localization to the number of localizations present in the GBB */
  VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &localization);
  if (localization >= hdr->number_of_localizations) {
    localization = 0;
    VbNvSet(&vnc, VBNV_LOCALIZATION_INDEX, localization);
  }

  /* Calculate offset of screen layout = start of screen stuff +
   * correct locale + correct screen. */
  offset = sizeof(BmpBlockHeader) +
      localization * hdr->number_of_screenlayouts * sizeof(ScreenLayout) +
      screen_index * sizeof(ScreenLayout);
  VBDEBUG(("VbDisplayScreenFromGBB(): scr_%d_%d at offset 0x%x\n",
           localization, screen_index, offset));
  layout = (ScreenLayout*)(bmpfv + offset);

  /* Display all bitmaps for the image */
  for (i = 0; i < MAX_IMAGE_IN_LAYOUT; i++) {
    if (layout->images[i].image_info_offset) {
      offset = layout->images[i].image_info_offset;
      image_info = (ImageInfo*)(bmpfv + offset);
      VBDEBUG(("VbDisplayScreenFromGBB: image %d: %dx%d+%d+%d %d/%d"
               "tag %d at 0x%x\n",
               i, image_info->width, image_info->height,
               layout->images[i].x, layout->images[i].y,
               image_info->compressed_size, image_info->original_size,
               image_info->tag, offset));

      retval = VbExDisplayImage(layout->images[i].x, layout->images[i].y,
                                image_info, bmpfv + offset + sizeof(ImageInfo));
      if (VBERROR_SUCCESS != retval)
        goto VbDisplayScreenFromGBB_exit;
    }
  }

  /* Successful if all bitmaps displayed */
  retval = VBERROR_SUCCESS;

VbDisplayScreenFromGBB_exit:

  /* Free the bitmap data copy */
  VbExFree(bmpfv);
  return retval;
}


/* Display a screen, initializing the display if necessary.  If force!=0,
 * redisplays the screen even if it's the same as the current screen. */
static VbError_t VbDisplayScreen(VbCommonParams* cparams, uint32_t screen,
                                 int force) {
  VbError_t retval;

  VBDEBUG(("VbDisplayScreen(%d, %d)\n", (int)screen, force));

  /* Initialize display if necessary */
  if (!disp_width) {
    retval = VbExDisplayInit(&disp_width, &disp_height);
    if (VBERROR_SUCCESS != retval)
      return retval;
  }

  /* If the requested screen is the same as the current one, we're done. */
  if (disp_current_screen == screen && 0 == force)
    return VBERROR_SUCCESS;

  /* If the screen is blank, turn off the backlight; else turn it on. */
  VbExDisplayBacklight(VB_SCREEN_BLANK == screen ? 0 : 1);

  /* Request the screen */
  disp_current_screen = screen;

  /* Look in the GBB first */
  if (VBERROR_SUCCESS == VbDisplayScreenFromGBB(cparams, screen))
    return VBERROR_SUCCESS;

  /* If the screen wasn't in the GBB bitmaps, fall back to a default screen. */
  return VbExDisplayScreen(screen);
}


#define DEBUG_INFO_SIZE 512

/* Display debug info to the screen */
static VbError_t VbDisplayDebugInfo(VbCommonParams* cparams) {
  VbSharedDataHeader* shared = (VbSharedDataHeader*)cparams->shared_data_blob;
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)cparams->gbb_data;
  char buf[DEBUG_INFO_SIZE] = "";
  uint32_t used = 0;
  uint32_t i;

  /* Redisplay the current screen, to overwrite any previous debug output */
  VbDisplayScreen(cparams, disp_current_screen, 1);

  /* Add hardware ID */
  used += Strncat(buf + used, "HWID: ", DEBUG_INFO_SIZE - used);
  if (0 == gbb->hwid_size ||
      gbb->hwid_offset > cparams->gbb_size ||
      gbb->hwid_offset + gbb->hwid_size > cparams->gbb_size) {
    VBDEBUG(("VbCheckDisplayKey(): invalid hwid offset/size\n"));
    used += Strncat(buf + used, "(INVALID)", DEBUG_INFO_SIZE - used);
  } else {
    used += Strncat(buf + used, (char*)((uint8_t*)gbb + gbb->hwid_offset),
                    DEBUG_INFO_SIZE - used);
  }

  /* Add recovery reason */
  used += Strncat(buf + used, "\nrecovery_reason: 0x", DEBUG_INFO_SIZE - used);
  used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used,
                         shared->recovery_reason, 16, 2);

  /* Add VbSharedData flags */
  used += Strncat(buf + used, "\nVbSD.flags: 0x", DEBUG_INFO_SIZE - used);
  used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used,
                         shared->flags, 16, 8);

  /* Add raw contents of VbNvStorage */
  used += Strncat(buf + used, "\nVbNv.raw:", DEBUG_INFO_SIZE - used);
  for (i = 0; i < VBNV_BLOCK_SIZE; i++) {
    used += Strncat(buf + used, " ", DEBUG_INFO_SIZE - used);
    used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used,
                           vnc.raw[i], 16, 2);
  }

  /* Add dev_boot_usb flag */
  VbNvGet(&vnc, VBNV_DEV_BOOT_USB, &i);
  used += Strncat(buf + used, "\ndev_boot_usb: ", DEBUG_INFO_SIZE - used);
  used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used, i, 10, 0);

  /* Add TPM versions */
  used += Strncat(buf + used, "\nTPM: fwver=0x", DEBUG_INFO_SIZE - used);
  used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used,
                         shared->fw_version_tpm, 16, 8);
  used += Strncat(buf + used, " kernver=0x", DEBUG_INFO_SIZE - used);
  used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used,
                         shared->kernel_version_tpm, 16, 8);

  /* Make sure we finish with a newline */
  used += Strncat(buf + used, "\n", DEBUG_INFO_SIZE - used);

  /* TODO: add more interesting data:
   * - SHA1 of kernel subkey (assuming we always set it in VbSelectFirmware,
   *   even in recovery mode, where we just copy it from the root key)
   * - Information on current disks
   * - Anything else interesting from VbNvStorage */

  buf[DEBUG_INFO_SIZE - 1] = '\0';
  VBDEBUG(("VbCheckDisplayKey() wants to show '%s'\n", buf));
  return VbExDisplayDebugInfo(buf);
}


static VbError_t VbCheckDisplayKey(VbCommonParams* cparams, uint32_t key) {

  if ('\t' == key) {
    /* Tab = display debug info */
    return VbDisplayDebugInfo(cparams);

  } else if (VB_KEY_LEFT == key || VB_KEY_RIGHT == key) {
    /* Arrow keys = change localization */
    uint32_t loc = 0;
    uint32_t count = 0;

    VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &loc);
    if (VBERROR_SUCCESS != VbGetLocalizationCount(cparams, &count))
      loc = 0;  /* No localization count (bad GBB?), so set to 0 (default) */
    else if (VB_KEY_RIGHT == key)
      loc = (loc < count - 1 ? loc + 1 : 0);
    else
      loc = (loc > 0 ? loc - 1 : count - 1);
    VBDEBUG(("VbCheckDisplayKey() - change localization to %d\n", (int)loc));
    VbNvSet(&vnc, VBNV_LOCALIZATION_INDEX, loc);

    /* Force redraw of current screen */
    return VbDisplayScreen(cparams, disp_current_screen, 1);
  }

  return VBERROR_SUCCESS;
}


/* Attempt loading a kernel from the specified type(s) of disks.  If
 * successful, sets p->disk_handle to the disk for the kernel and returns
 * VBERROR_SUCCESS.
 *
 * Returns VBERROR_NO_DISK_FOUND if no disks of the specified type were found.
 *
 * May return other VBERROR_ codes for other failures. */
uint32_t VbTryLoadKernel(VbCommonParams* cparams, LoadKernelParams* p,
                         uint32_t get_info_flags) {
  VbError_t retval = VBERROR_UNKNOWN;
  VbDiskInfo* disk_info = NULL;
  uint32_t disk_count = 0;
  uint32_t i;

  VBDEBUG(("VbTryLoadKernel() start, get_info_flags=0x%x\n",
          (unsigned)get_info_flags));

  p->disk_handle = NULL;

  /* Find disks */
  if (VBERROR_SUCCESS != VbExDiskGetInfo(&disk_info, &disk_count,
                                         get_info_flags))
    disk_count = 0;

  VBDEBUG(("VbTryLoadKernel() found %d disks\n", (int)disk_count));
  if (0 == disk_count) {
    VbSetRecoveryRequest(VBNV_RECOVERY_RW_NO_DISK);
    return VBERROR_NO_DISK_FOUND;
  }

  /* Loop over disks */
  for (i = 0; i < disk_count; i++) {
    VBDEBUG(("VbTryLoadKernel() trying disk %d\n", (int)i));
    p->disk_handle = disk_info[i].handle;
    p->bytes_per_lba = disk_info[i].bytes_per_lba;
    p->ending_lba = disk_info[i].lba_count - 1;
    retval = LoadKernel(p);
    VBDEBUG(("VbTryLoadKernel() LoadKernel() returned %d\n", retval));

    /* Stop now if we found a kernel */
    /* TODO: If recovery requested, should track the farthest we get, instead
     * of just returning the value from the last disk attempted. */
    if (VBERROR_SUCCESS == retval)
      break;
  }

  /* If we didn't succeed, don't return a disk handle */
  if (VBERROR_SUCCESS != retval)
    p->disk_handle = NULL;

  VbExDiskFreeInfo(disk_info, p->disk_handle);

  /* Pass through return code.  Recovery reason (if any) has already been set
   * by LoadKernel(). */
  return retval;
}


/* Handle a normal boot. */
VbError_t VbBootNormal(VbCommonParams* cparams, LoadKernelParams* p) {

  /* Force dev_boot_usb flag disabled.  This ensures the flag will be
   * initially disabled if the user later transitions back into
   * developer mode. */
  VbNvSet(&vnc, VBNV_DEV_BOOT_USB, 0);

  /* Boot from fixed disk only */
  return VbTryLoadKernel(cparams, p, VB_DISK_FLAG_FIXED);
}


/* Developer mode delays.  All must be multiples of DEV_DELAY_INCREMENT */
#define DEV_DELAY_INCREMENT 250  /* Delay each loop, in msec */
#define DEV_DELAY_BEEP1 20000    /* Beep for first time at this time */
#define DEV_DELAY_BEEP2 21000    /* Beep for second time at this time */
#define DEV_DELAY_TIMEOUT 30000  /* Give up at this time */

/* Handle a developer-mode boot */
VbError_t VbBootDeveloper(VbCommonParams* cparams, LoadKernelParams* p) {
  uint32_t delay_time = 0;
  uint32_t allow_usb = 0;

  /* Check if USB booting is allowed */
  VbNvGet(&vnc, VBNV_DEV_BOOT_USB, &allow_usb);

  /* Show the dev mode warning screen */
  VbDisplayScreen(cparams, VB_SCREEN_DEVELOPER_WARNING, 0);

  /* Loop for dev mode warning delay */
  for (delay_time = 0; delay_time < DEV_DELAY_TIMEOUT;
       delay_time += DEV_DELAY_INCREMENT) {
    uint32_t key;

    if (VbExIsShutdownRequested())
      return VBERROR_SHUTDOWN_REQUESTED;

    if (DEV_DELAY_BEEP1 == delay_time || DEV_DELAY_BEEP2 == delay_time)
      VbExBeep(DEV_DELAY_INCREMENT, 400);
    else
      VbExSleepMs(DEV_DELAY_INCREMENT);

    /* Handle keypress */
    key = VbExKeyboardRead();
    switch (key) {
      case '\r':
      case ' ':
      case 0x1B:
        /* Enter, space, or ESC = reboot to recovery */
        VBDEBUG(("VbBootDeveloper() - user pressed ENTER/SPACE/ESC"));
        VbSetRecoveryRequest(VBNV_RECOVERY_RW_DEV_SCREEN);
        return 1;
      case 0x04:
        /* Ctrl+D = dismiss warning; advance to timeout */
        VBDEBUG(("VbBootDeveloper() - user pressed Ctrl+D; skip delay\n"));
        delay_time = DEV_DELAY_TIMEOUT;
        break;
      case 0x15:
        /* Ctrl+U = try USB boot, or beep if failure */
        VBDEBUG(("VbBootDeveloper() - user pressed Ctrl+U; try USB\n"));
        if (!allow_usb) {
          VBDEBUG(("VbBootDeveloper() - USB booting is disabled\n"));
          VbExBeep(DEV_DELAY_INCREMENT / 2, 400);
          VbExSleepMs(DEV_DELAY_INCREMENT / 2);
          VbExBeep(DEV_DELAY_INCREMENT / 2, 400);
        } else if (VBERROR_SUCCESS ==
                   VbTryLoadKernel(cparams, p, VB_DISK_FLAG_REMOVABLE)) {
          VBDEBUG(("VbBootDeveloper() - booting USB\n"));
          return VBERROR_SUCCESS;
        } else {
          VBDEBUG(("VbBootDeveloper() - no kernel found on USB\n"));
          VbExBeep(DEV_DELAY_INCREMENT, 400);
          /* Clear recovery requests from failed kernel loading, so
           * that powering off at this point doesn't put us into
           * recovery mode. */
          VbSetRecoveryRequest(VBNV_RECOVERY_NOT_REQUESTED);
        }
        break;
      default:
        VbCheckDisplayKey(cparams, key);
        break;
        /* TODO: xyzzy easter egg check */
    }
  }

  /* Timeout or Ctrl+D; attempt loading from fixed disk */
  VBDEBUG(("VbBootDeveloper() - trying fixed disk\n"));
  return VbTryLoadKernel(cparams, p, VB_DISK_FLAG_FIXED);
}


/* Delay between disk checks in recovery mode */
#define REC_DELAY_INCREMENT 250

/* Handle a recovery-mode boot */
VbError_t VbBootRecovery(VbCommonParams* cparams, LoadKernelParams* p) {
  VbSharedDataHeader* shared = (VbSharedDataHeader*)cparams->shared_data_blob;
  uint32_t retval;
  int i;

  VBDEBUG(("VbBootRecovery() start\n"));

  /* If dev mode switch is off, require removal of all external media. */
  if (!(shared->flags & VBSD_BOOT_DEV_SWITCH_ON)) {
    VbDiskInfo* disk_info = NULL;
    uint32_t disk_count = 0;

    VBDEBUG(("VbBootRecovery() forcing device removal\n"));

    while (1) {
      if (VBERROR_SUCCESS != VbExDiskGetInfo(&disk_info, &disk_count,
          VB_DISK_FLAG_REMOVABLE))
        disk_count = 0;
      VbExDiskFreeInfo(disk_info, NULL);

      if (0 == disk_count) {
        VbDisplayScreen(cparams, VB_SCREEN_BLANK, 0);
        break;
      }

      VBDEBUG(("VbBootRecovery() waiting for %d disks to be removed\n",
               (int)disk_count));

      VbDisplayScreen(cparams, VB_SCREEN_RECOVERY_REMOVE, 0);

      /* Scan keyboard more frequently than media, since x86 platforms
       * don't like to scan USB too rapidly. */
      for (i = 0; i < 4; i++) {
        VbCheckDisplayKey(cparams, VbExKeyboardRead());
        if (VbExIsShutdownRequested())
          return VBERROR_SHUTDOWN_REQUESTED;
        VbExSleepMs(REC_DELAY_INCREMENT);
      }
    }
  }

  /* Loop and wait for a recovery image */
  while (1) {
    VBDEBUG(("VbBootRecovery() attempting to load kernel\n"));
    retval = VbTryLoadKernel(cparams, p, VB_DISK_FLAG_REMOVABLE);

    /* Clear recovery requests from failed kernel loading, since we're
     * already in recovery mode.  Do this now, so that powering off after
     * inserting an invalid disk doesn't leave us stuck in recovery mode. */
    VbSetRecoveryRequest(VBNV_RECOVERY_NOT_REQUESTED);

    if (VBERROR_SUCCESS == retval)
      break;  /* Found a recovery kernel */

    VbDisplayScreen(cparams, VBERROR_NO_DISK_FOUND == retval ?
                    VB_SCREEN_RECOVERY_INSERT : VB_SCREEN_RECOVERY_NO_GOOD, 0);

    /* Scan keyboard more frequently than media, since x86 platforms don't like
     * to scan USB too rapidly. */
    for (i = 0; i < 4; i++) {
      VbCheckDisplayKey(cparams, VbExKeyboardRead());
      if (VbExIsShutdownRequested())
        return VBERROR_SHUTDOWN_REQUESTED;
      VbExSleepMs(REC_DELAY_INCREMENT);
    }
  }

  return VBERROR_SUCCESS;
}


VbError_t VbSelectAndLoadKernel(VbCommonParams* cparams,
                                VbSelectAndLoadKernelParams* kparams) {
  VbSharedDataHeader* shared = (VbSharedDataHeader*)cparams->shared_data_blob;
  VbError_t retval = VBERROR_SUCCESS;
  LoadKernelParams p;
  uint32_t tpm_status = 0;

  VBDEBUG(("VbSelectAndLoadKernel() start\n"));

  /* Start timer */
  shared->timer_vb_select_and_load_kernel_enter = VbExGetTimer();

  VbExNvStorageRead(vnc.raw);
  VbNvSetup(&vnc);

  /* Clear output params in case we fail */
  kparams->disk_handle = NULL;
  kparams->partition_number = 0;
  kparams->bootloader_address = 0;
  kparams->bootloader_size = 0;
  Memset(kparams->partition_guid, 0, sizeof(kparams->partition_guid));

  /* Read the kernel version from the TPM.  Ignore errors in recovery mode. */
  tpm_status = RollbackKernelRead(&shared->kernel_version_tpm);
  if (0 != tpm_status) {
    VBDEBUG(("Unable to get kernel versions from TPM\n"));
    if (!shared->recovery_reason) {
      VbSetRecoveryRequest(VBNV_RECOVERY_RW_TPM_ERROR);
      retval = VBERROR_TPM_READ_KERNEL;
      goto VbSelectAndLoadKernel_exit;
    }
  }
  shared->kernel_version_tpm_start = shared->kernel_version_tpm;

  /* Fill in params for calls to LoadKernel() */
  Memset(&p, 0, sizeof(p));
  p.shared_data_blob = cparams->shared_data_blob;
  p.shared_data_size = cparams->shared_data_size;
  p.gbb_data = cparams->gbb_data;
  p.gbb_size = cparams->gbb_size;
  p.kernel_buffer = kparams->kernel_buffer;
  p.kernel_buffer_size = kparams->kernel_buffer_size;
  p.nv_context = &vnc;
  p.boot_flags = 0;
  if (shared->flags & VBSD_BOOT_DEV_SWITCH_ON)
    p.boot_flags |= BOOT_FLAG_DEVELOPER;

  /* Handle separate normal and developer firmware builds. */
#if defined(VBOOT_FIRMWARE_TYPE_NORMAL)
  /* Normal-type firmware always acts like the dev switch is off. */
  p.boot_flags &= ~BOOT_FLAG_DEVELOPER;
#elif defined(VBOOT_FIRMWARE_TYPE_DEVELOPER)
  /* Developer-type firmware fails if the dev switch is off. */
  if (!(p.boot_flags & BOOT_FLAG_DEVELOPER)) {
    /* Dev firmware should be signed with a key that only verifies
     * when the dev switch is on, so we should never get here. */
    VBDEBUG(("Developer firmware called with dev switch off!\n"));
    VbSetRecoveryRequest(VBNV_RECOVERY_RW_DEV_MISMATCH);
    retval = VBERROR_DEV_FIRMWARE_SWITCH_MISMATCH;
    goto VbSelectAndLoadKernel_exit;
  }
#else
  /* Recovery firmware, or merged normal+developer firmware.  No
   * need to override flags. */
#endif

  /* Select boot path */
  if (shared->recovery_reason) {
    /* Recovery boot */
    p.boot_flags |= BOOT_FLAG_RECOVERY;
    retval = VbBootRecovery(cparams, &p);
    VbDisplayScreen(cparams, VB_SCREEN_BLANK, 0);

  } else if (p.boot_flags & BOOT_FLAG_DEVELOPER) {
    /* Developer boot */
    retval = VbBootDeveloper(cparams, &p);
    VbDisplayScreen(cparams, VB_SCREEN_BLANK, 0);

  } else {
    /* Normal boot */
    retval = VbBootNormal(cparams, &p);

    if ((1 == shared->firmware_index) && (shared->flags & VBSD_FWB_TRIED)) {
      /* Special cases for when we're trying a new firmware B.  These are
       * needed because firmware updates also usually change the kernel key,
       * which means that the B firmware can only boot a new kernel, and the
       * old firmware in A can only boot the previous kernel. */

      /* Don't advance the TPM if we're trying a new firmware B, because we
       * don't yet know if the new kernel will successfully boot.  We still
       * want to be able to fall back to the previous firmware+kernel if the
       * new firmware+kernel fails. */

      /* If we found only invalid kernels, reboot and try again.  This allows
       * us to fall back to the previous firmware+kernel instead of giving up
       * and going to recovery mode right away.  We'll still go to recovery
       * mode if we run out of tries and the old firmware can't find a kernel
       * it likes. */
      if (VBERROR_INVALID_KERNEL_FOUND == retval) {
        VBDEBUG(("Trying firmware B, and only found invalid kernels.\n"));
        VbSetRecoveryRequest(VBNV_RECOVERY_NOT_REQUESTED);
        goto VbSelectAndLoadKernel_exit;
      }
    } else {
      /* Not trying a new firmware B. */
      /* See if we need to update the TPM. */
      VBDEBUG(("Checking if TPM kernel version needs advancing\n"));
      if (shared->kernel_version_tpm > shared->kernel_version_tpm_start) {
        tpm_status = RollbackKernelWrite(shared->kernel_version_tpm);
        if (0 != tpm_status) {
          VBDEBUG(("Error writing kernel versions to TPM.\n"));
          VbSetRecoveryRequest(VBNV_RECOVERY_RW_TPM_ERROR);
          retval = VBERROR_TPM_WRITE_KERNEL;
          goto VbSelectAndLoadKernel_exit;
        }
      }
    }
  }

  if (VBERROR_SUCCESS != retval)
    goto VbSelectAndLoadKernel_exit;

  /* Save disk parameters */
  kparams->disk_handle = p.disk_handle;
  kparams->partition_number = (uint32_t)p.partition_number;
  kparams->bootloader_address = p.bootloader_address;
  kparams->bootloader_size = (uint32_t)p.bootloader_size;
  Memcpy(kparams->partition_guid, p.partition_guid,
         sizeof(kparams->partition_guid));

  /* Lock the kernel versions.  Ignore errors in recovery mode. */
  tpm_status = RollbackKernelLock();
  if (0 != tpm_status) {
    VBDEBUG(("Error locking kernel versions.\n"));
    if (!shared->recovery_reason) {
      VbSetRecoveryRequest(VBNV_RECOVERY_RW_TPM_ERROR);
      retval = VBERROR_TPM_LOCK_KERNEL;
      goto VbSelectAndLoadKernel_exit;
    }
  }

VbSelectAndLoadKernel_exit:

  if (vnc.raw_changed)
    VbExNvStorageWrite(vnc.raw);

  /* Stop timer */
  shared->timer_vb_select_and_load_kernel_exit = VbExGetTimer();

  VBDEBUG(("VbSelectAndLoadKernel() returning %d\n", (int)retval));

  /* Pass through return value from boot path */
  return retval;
}
