/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Display functions used in kernel selection.
 */

#include "bmpblk_font.h"
#include "gbb_header.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_nvstorage.h"

static uint32_t disp_current_screen = VB_SCREEN_BLANK;
static uint32_t disp_width = 0, disp_height = 0;


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



/* Return a fixed string representing the HWID */
static char *VbHWID(VbCommonParams* cparams) {
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)cparams->gbb_data;
  if (0 == gbb->hwid_size ||
      gbb->hwid_offset > cparams->gbb_size ||
      gbb->hwid_offset + gbb->hwid_size > cparams->gbb_size) {
    VBDEBUG(("VbHWID(): invalid hwid offset/size\n"));
    return "{INVALID}";
  }
  return (char*)((uint8_t*)gbb + gbb->hwid_offset);
}


/* TODO: We could cache the font info to speed things up, by making the
 * in-memory font structure distinct from the in-flash version.  We'll do that
 * Real Soon Now. Until then, we just repeat the same linear search every time.
 */
typedef FontArrayHeader VbFont_t;

static VbFont_t *VbInternalizeFontData(FontArrayHeader *fonthdr) {
  /* Just return the raw data pointer for now. */
  return (VbFont_t *)fonthdr;
}

static void VbDoneWithFontForNow(VbFont_t *ptr) {
  /* Nothing. */
}

static ImageInfo *VbFindFontGlyph(VbFont_t *font, uint32_t ascii,
                                  void **bufferptr, uint32_t *buffersize) {
  uint8_t *ptr, *firstptr;
  uint32_t max;
  uint32_t i;
  FontArrayEntryHeader *entry;

  ptr = (uint8_t *)font;
  max = ((FontArrayHeader *)ptr)->num_entries;
  ptr += sizeof(FontArrayHeader);
  firstptr = ptr;

  /* Simple linear search. */
  for(i=0; i<max; i++)
  {
    entry = (FontArrayEntryHeader *)ptr;
    if (entry->ascii == ascii) {
      /* Note: We're assuming the glpyh is uncompressed. That's true
       * because the bmpblk_font tool doesn't compress anything. The
       * bmpblk_utility does, but it compresses the entire font blob at once,
       * and we've already uncompressed that before we got here.
       */
      *bufferptr = ptr + sizeof(FontArrayEntryHeader);
      *buffersize = entry->info.original_size;
      return &(entry->info);
    }
    ptr += sizeof(FontArrayEntryHeader)+entry->info.compressed_size;
  }

  /* NOTE: We must return something valid. We'll just use the first glyph in the
   * font structure (so it should be something distinct).
   */
  entry = (FontArrayEntryHeader *)firstptr;
  *bufferptr = firstptr + sizeof(FontArrayEntryHeader);
  *buffersize = entry->info.original_size;
  return &(entry->info);
}

/* Try to display the specified text at a particular position. */
static void VbRenderTextAtPos(char *text, int right_to_left,
                              uint32_t x, uint32_t y, VbFont_t *font) {
  int i;
  ImageInfo *image_info = 0;
  void *buffer;
  uint32_t buffersize;
  uint32_t cur_x = x, cur_y = y;

  if (!text || !font) {
    VBDEBUG(("  VbRenderTextAtPos: invalid args\n"));
    return;
  }

  for (i=0; text[i]; i++) {

    if (text[i] == '\n') {
      if (!image_info)
        image_info = VbFindFontGlyph(font, text[i], &buffer, &buffersize);
      cur_x = x;
      cur_y += image_info->height;
      continue;
    }

    image_info = VbFindFontGlyph(font, text[i], &buffer, &buffersize);

    if (right_to_left) {
      cur_x -= image_info->width;
    }

    if (VBERROR_SUCCESS != VbExDisplayImage(cur_x, cur_y, buffer, buffersize)) {
      VBDEBUG(("  VbRenderTextAtPos: can't display ascii 0x%x\n", text[i]));
    }

    if (!right_to_left) {
      cur_x += image_info->width;
    }
  }
}


/* Display a screen from the GBB. */
VbError_t VbDisplayScreenFromGBB(VbCommonParams* cparams, uint32_t screen,
                                 VbNvContext *vncptr) {
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)cparams->gbb_data;
  uint8_t* bmpfv = NULL;
  void* fullimage = NULL;
  BmpBlockHeader* hdr;
  ScreenLayout* layout;
  ImageInfo* image_info;
  uint32_t screen_index;
  uint32_t localization = 0;
  VbError_t retval = VBERROR_UNKNOWN;   /* Assume error until proven ok */
  uint32_t inoutsize;
  uint32_t offset;
  uint32_t i;
  VbFont_t *font;
  char *text_to_show;
  int rtol = 0;

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
  VbNvGet(vncptr, VBNV_LOCALIZATION_INDEX, &localization);
  if (localization >= hdr->number_of_localizations) {
    localization = 0;
    VbNvSet(vncptr, VBNV_LOCALIZATION_INDEX, localization);
  }

  /* Calculate offset of screen layout = start of screen stuff +
   * correct locale + correct screen. */
  offset = sizeof(BmpBlockHeader) +
    localization * hdr->number_of_screenlayouts * sizeof(ScreenLayout) +
    screen_index * sizeof(ScreenLayout);
  layout = (ScreenLayout*)(bmpfv + offset);

  /* Display all bitmaps for the image */
  for (i = 0; i < MAX_IMAGE_IN_LAYOUT; i++) {
    if (layout->images[i].image_info_offset) {
      offset = layout->images[i].image_info_offset;
      image_info = (ImageInfo*)(bmpfv + offset);
      fullimage = bmpfv + offset + sizeof(ImageInfo);
      inoutsize = image_info->original_size;
      if (inoutsize && image_info->compression != COMPRESS_NONE) {
        fullimage = VbExMalloc(inoutsize);
        retval = VbExDecompress(bmpfv + offset + sizeof(ImageInfo),
                                image_info->compressed_size,
                                image_info->compression,
                                fullimage, &inoutsize);
        if (VBERROR_SUCCESS != retval) {
          VbExFree(fullimage);
          goto VbDisplayScreenFromGBB_exit;
        }
      }

      switch(image_info->format) {
      case FORMAT_BMP:
        retval = VbExDisplayImage(layout->images[i].x, layout->images[i].y,
                                  fullimage, inoutsize);
        break;

      case FORMAT_FONT:
        /* The uncompressed blob is our font structure. Cache it as needed. */
        font = VbInternalizeFontData(fullimage);

        /* TODO: handle text in general here */
        if (TAG_HWID == image_info->tag || TAG_HWID_RTOL == image_info->tag) {
          text_to_show = VbHWID(cparams);
          rtol = (TAG_HWID_RTOL == image_info->tag);
        } else {
          text_to_show = "";
          rtol = 0;
        }

        VbRenderTextAtPos(text_to_show, rtol,
                          layout->images[i].x, layout->images[i].y, font);

        VbDoneWithFontForNow(font);
        break;

      default:
        VBDEBUG(("VbDisplayScreenFromGBB(): unsupported ImageFormat %d\n",
                 image_info->format));
        retval = VBERROR_INVALID_GBB;
      }

      if (COMPRESS_NONE != image_info->compression)
        VbExFree(fullimage);

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
VbError_t VbDisplayScreen(VbCommonParams* cparams, uint32_t screen, int force,
                          VbNvContext *vncptr) {
  VbError_t retval;

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
  if (VBERROR_SUCCESS == VbDisplayScreenFromGBB(cparams, screen, vncptr))
    return VBERROR_SUCCESS;

  /* If the screen wasn't in the GBB bitmaps, fall back to a default screen. */
  return VbExDisplayScreen(screen);
}


static void Uint8ToString(char *buf, uint8_t val) {
  const char *trans = "0123456789abcdef";
  *buf++ = trans[val >> 4];
  *buf = trans[val & 0xF];
}

static void FillInSha1Sum(char *outbuf, VbPublicKey* key) {
  uint8_t* buf = ((uint8_t *)key) + key->key_offset;
  uint64_t buflen = key->key_size;
  uint8_t* digest = DigestBuf(buf, buflen, SHA1_DIGEST_ALGORITHM);
  int i;
  for (i=0; i<SHA1_DIGEST_SIZE; i++) {
    Uint8ToString(outbuf, digest[i]);
    outbuf += 2;
  }
  *outbuf = '\0';
  VbExFree(digest);
}

#define DEBUG_INFO_SIZE 512

/* Display debug info to the screen */
VbError_t VbDisplayDebugInfo(VbCommonParams* cparams, VbNvContext *vncptr) {
  VbSharedDataHeader* shared = (VbSharedDataHeader*)cparams->shared_data_blob;
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)cparams->gbb_data;
  char buf[DEBUG_INFO_SIZE] = "";
  char sha1sum[SHA1_DIGEST_SIZE * 2 + 1];
  uint32_t used = 0;
  uint32_t i;

  /* Redisplay the current screen, to overwrite any previous debug output */
  VbDisplayScreen(cparams, disp_current_screen, 1, vncptr);

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
                           vncptr->raw[i], 16, 2);
  }

  /* Add dev_boot_usb flag */
  VbNvGet(vncptr, VBNV_DEV_BOOT_USB, &i);
  used += Strncat(buf + used, "\ndev_boot_usb: ", DEBUG_INFO_SIZE - used);
  used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used, i, 10, 0);

  /* Add TPM versions */
  used += Strncat(buf + used, "\nTPM: fwver=0x", DEBUG_INFO_SIZE - used);
  used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used,
                         shared->fw_version_tpm, 16, 8);
  used += Strncat(buf + used, " kernver=0x", DEBUG_INFO_SIZE - used);
  used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used,
                         shared->kernel_version_tpm, 16, 8);

  /* Add GBB flags */
  used += Strncat(buf + used, "\ngbb.flags: 0x", DEBUG_INFO_SIZE - used);
  if (gbb->major_version == GBB_MAJOR_VER && gbb->minor_version >= 1) {
    used += Uint64ToString(buf + used, DEBUG_INFO_SIZE - used,
                           gbb->flags, 16, 8);
  } else {
    used += Strncat(buf + used, "0 (default)", DEBUG_INFO_SIZE - used);
  }

  /* Add sha1sum for Root & Recovery keys */
  FillInSha1Sum(sha1sum,
                (VbPublicKey*)((uint8_t*)gbb + gbb->rootkey_offset));
  used += Strncat(buf + used, "\ngbb.rootkey: ", DEBUG_INFO_SIZE - used);
  used += Strncat(buf + used, sha1sum, DEBUG_INFO_SIZE - used);
  FillInSha1Sum(sha1sum,
                (VbPublicKey*)((uint8_t*)gbb + gbb->recovery_key_offset));
  used += Strncat(buf + used, "\ngbb.recovery_key: ", DEBUG_INFO_SIZE - used);
  used += Strncat(buf + used, sha1sum, DEBUG_INFO_SIZE - used);

  /* If we're in dev-mode, show the kernel subkey that we expect, too. */
  if (0 == shared->recovery_reason) {
    FillInSha1Sum(sha1sum, &shared->kernel_subkey);
    used += Strncat(buf + used, "\nkernel_subkey: ", DEBUG_INFO_SIZE - used);
    used += Strncat(buf + used, sha1sum, DEBUG_INFO_SIZE - used);
  }

  /* Make sure we finish with a newline */
  used += Strncat(buf + used, "\n", DEBUG_INFO_SIZE - used);

  /* TODO: add more interesting data:
   * - Information on current disks */

  buf[DEBUG_INFO_SIZE - 1] = '\0';
  return VbExDisplayDebugInfo(buf);
}


VbError_t VbCheckDisplayKey(VbCommonParams* cparams, uint32_t key,
                            VbNvContext *vncptr) {

  if ('\t' == key) {
    /* Tab = display debug info */
    return VbDisplayDebugInfo(cparams, vncptr);

  } else if (VB_KEY_LEFT == key || VB_KEY_RIGHT == key ||
             VB_KEY_DOWN == key || VB_KEY_UP == key) {
    /* Arrow keys = change localization */
    uint32_t loc = 0;
    uint32_t count = 0;

    VbNvGet(vncptr, VBNV_LOCALIZATION_INDEX, &loc);
    if (VBERROR_SUCCESS != VbGetLocalizationCount(cparams, &count))
      loc = 0;  /* No localization count (bad GBB?), so set to 0 (default) */
    else if (VB_KEY_RIGHT == key || VB_KEY_UP == key)
      loc = (loc < count - 1 ? loc + 1 : 0);
    else
      loc = (loc > 0 ? loc - 1 : count - 1);
    VBDEBUG(("VbCheckDisplayKey() - change localization to %d\n", (int)loc));
    VbNvSet(vncptr, VBNV_LOCALIZATION_INDEX, loc);

    /* Force redraw of current screen */
    return VbDisplayScreen(cparams, disp_current_screen, 1, vncptr);
  }

  return VBERROR_SUCCESS;
}
