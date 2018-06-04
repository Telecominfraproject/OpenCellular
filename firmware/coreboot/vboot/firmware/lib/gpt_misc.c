/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "2sysincludes.h"
#include "2common.h"

#include "sysincludes.h"
#include "cgptlib.h"
#include "cgptlib_internal.h"
#include "crc32.h"
#include "gpt.h"
#include "utility.h"
#include "vboot_api.h"


/**
 * Allocate and read GPT data from the drive.
 *
 * The sector_bytes and gpt_drive_sectors fields should be filled on input.  The
 * primary and secondary header and entries are filled on output.
 *
 * Returns 0 if successful, 1 if error.
 */
int AllocAndReadGptData(VbExDiskHandle_t disk_handle, GptData *gptdata)
{
	uint64_t max_entries_bytes = MAX_NUMBER_OF_ENTRIES * sizeof(GptEntry);
	int primary_valid = 0, secondary_valid = 0;

	/* No data to be written yet */
	gptdata->modified = 0;
	/* This should get overwritten by GptInit() */
	gptdata->ignored = 0;

	/* Allocate all buffers */
	gptdata->primary_header = (uint8_t *)malloc(gptdata->sector_bytes);
	gptdata->secondary_header =
		(uint8_t *)malloc(gptdata->sector_bytes);
	gptdata->primary_entries = (uint8_t *)malloc(max_entries_bytes);
	gptdata->secondary_entries = (uint8_t *)malloc(max_entries_bytes);

	if (gptdata->primary_header == NULL ||
	    gptdata->secondary_header == NULL ||
	    gptdata->primary_entries == NULL ||
	    gptdata->secondary_entries == NULL)
		return 1;

	/* Read primary header from the drive, skipping the protective MBR */
	if (0 != VbExDiskRead(disk_handle, 1, 1, gptdata->primary_header)) {
		VB2_DEBUG("Read error in primary GPT header\n");
		memset(gptdata->primary_header, 0, gptdata->sector_bytes);
	}

	/* Only read primary GPT if the primary header is valid */
	GptHeader* primary_header = (GptHeader*)gptdata->primary_header;
	if (0 == CheckHeader(primary_header, 0,
			gptdata->streaming_drive_sectors,
			gptdata->gpt_drive_sectors,
			gptdata->flags,
			gptdata->sector_bytes)) {
		primary_valid = 1;
		uint64_t entries_bytes =
				(uint64_t)primary_header->number_of_entries
				* primary_header->size_of_entry;
		uint64_t entries_sectors = entries_bytes
					/ gptdata->sector_bytes;
		if (0 != VbExDiskRead(disk_handle,
				      primary_header->entries_lba,
				      entries_sectors,
				      gptdata->primary_entries)) {
			VB2_DEBUG("Read error in primary GPT entries\n");
			primary_valid = 0;
		}
	} else {
		VB2_DEBUG("Primary GPT header is %s\n",
			  memcmp(primary_header->signature,
				 GPT_HEADER_SIGNATURE_IGNORED,
				 GPT_HEADER_SIGNATURE_SIZE)
			  ? "invalid" : "being ignored");
	}

	/* Read secondary header from the end of the drive */
	if (0 != VbExDiskRead(disk_handle, gptdata->gpt_drive_sectors - 1, 1,
			      gptdata->secondary_header)) {
		VB2_DEBUG("Read error in secondary GPT header\n");
		memset(gptdata->secondary_header, 0, gptdata->sector_bytes);
	}

	/* Only read secondary GPT if the secondary header is valid */
	GptHeader* secondary_header = (GptHeader*)gptdata->secondary_header;
	if (0 == CheckHeader(secondary_header, 1,
			gptdata->streaming_drive_sectors,
			gptdata->gpt_drive_sectors,
			gptdata->flags,
			gptdata->sector_bytes)) {
		secondary_valid = 1;
		uint64_t entries_bytes =
				(uint64_t)secondary_header->number_of_entries
				* secondary_header->size_of_entry;
		uint64_t entries_sectors = entries_bytes
				/ gptdata->sector_bytes;
		if (0 != VbExDiskRead(disk_handle,
				      secondary_header->entries_lba,
				      entries_sectors,
				      gptdata->secondary_entries)) {
			VB2_DEBUG("Read error in secondary GPT entries\n");
			secondary_valid = 0;
		}
	} else {
		VB2_DEBUG("Secondary GPT header is %s\n",
			  memcmp(secondary_header->signature,
				 GPT_HEADER_SIGNATURE_IGNORED,
				 GPT_HEADER_SIGNATURE_SIZE)
			  ? "invalid" : "being ignored");
	}

	/* Return 0 if least one GPT header was valid */
	return (primary_valid || secondary_valid) ? 0 : 1;
}

/**
 * Write any changes for the GPT data back to the drive, then free the buffers.
 *
 * Returns 0 if successful, 1 if error.
 */
int WriteAndFreeGptData(VbExDiskHandle_t disk_handle, GptData *gptdata)
{
	int skip_primary = 0;
	GptHeader *header;
	uint64_t entries_bytes, entries_sectors;
	int ret = 1;

	header = (GptHeader *)gptdata->primary_header;
	if (!header)
		header = (GptHeader *)gptdata->secondary_header;
	if (!header)
		return 1;  /* No headers at all, so nothing to write */

	entries_bytes = (uint64_t)header->number_of_entries
			* header->size_of_entry;
	entries_sectors = entries_bytes / gptdata->sector_bytes;

	/*
	 * TODO(namnguyen): Preserve padding between primary GPT header and
	 * its entries.
	 */
	uint64_t entries_lba = GPT_PMBR_SECTORS + GPT_HEADER_SECTORS;
	if (gptdata->primary_header) {
		GptHeader *h = (GptHeader *)(gptdata->primary_header);
		entries_lba = h->entries_lba;

		if (gptdata->ignored & MASK_PRIMARY) {
			VB2_DEBUG("Not updating primary GPT: "
				  "marked to be ignored.\n");
			skip_primary = 1;
		} else if (gptdata->modified & GPT_MODIFIED_HEADER1) {
			if (!memcmp(h->signature, GPT_HEADER_SIGNATURE2,
				    GPT_HEADER_SIGNATURE_SIZE)) {
				VB2_DEBUG("Not updating primary GPT: "
					  "legacy mode is enabled.\n");
				skip_primary = 1;
			} else {
				VB2_DEBUG("Updating GPT header 1\n");
				if (0 != VbExDiskWrite(disk_handle, 1, 1,
						       gptdata->primary_header))
					goto fail;
			}
		}
	}

	if (gptdata->primary_entries && !skip_primary) {
		if (gptdata->modified & GPT_MODIFIED_ENTRIES1) {
			VB2_DEBUG("Updating GPT entries 1\n");
			if (0 != VbExDiskWrite(disk_handle, entries_lba,
					       entries_sectors,
					       gptdata->primary_entries))
				goto fail;
		}
	}

	entries_lba = (gptdata->gpt_drive_sectors - entries_sectors -
		GPT_HEADER_SECTORS);
	if (gptdata->secondary_header && !(gptdata->ignored & MASK_SECONDARY)) {
		GptHeader *h = (GptHeader *)(gptdata->secondary_header);
		entries_lba = h->entries_lba;
		if (gptdata->modified & GPT_MODIFIED_HEADER2) {
			VB2_DEBUG("Updating GPT header 2\n");
			if (0 != VbExDiskWrite(disk_handle,
					       gptdata->gpt_drive_sectors - 1, 1,
					       gptdata->secondary_header))
				goto fail;
		}
	}

	if (gptdata->secondary_entries && !(gptdata->ignored & MASK_SECONDARY)){
		if (gptdata->modified & GPT_MODIFIED_ENTRIES2) {
			VB2_DEBUG("Updating GPT entries 2\n");
			if (0 != VbExDiskWrite(disk_handle,
					       entries_lba, entries_sectors,
					       gptdata->secondary_entries))
				goto fail;
		}
	}

	ret = 0;

fail:
	/* Avoid leaking memory on disk write failure */
	if (gptdata->primary_header)
		free(gptdata->primary_header);
	if (gptdata->primary_entries)
		free(gptdata->primary_entries);
	if (gptdata->secondary_entries)
		free(gptdata->secondary_entries);
	if (gptdata->secondary_header)
		free(gptdata->secondary_header);

	/* Success */
	return ret;
}

int IsUnusedEntry(const GptEntry *e)
{
	static Guid zero = {{{0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}}}};
	return !memcmp(&zero, (const uint8_t*)(&e->type), sizeof(zero));
}

/*
 * Func: GptGetEntrySize
 * Desc: This function returns size(in lba) of a partition represented by
 * given GPT entry.
 */
size_t GptGetEntrySizeLba(const GptEntry *e)
{
	return (e->ending_lba - e->starting_lba + 1);
}

/*
 * Func: GptGetEntrySize
 * Desc: This function returns size(in bytes) of a partition represented by
 * given GPT entry.
 */
size_t GptGetEntrySizeBytes(const GptData *gpt, const GptEntry *e)
{
	return GptGetEntrySizeLba(e) * gpt->sector_bytes;
}
