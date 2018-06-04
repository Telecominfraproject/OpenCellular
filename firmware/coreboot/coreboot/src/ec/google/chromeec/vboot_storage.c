/*
 * This file is part of the coreboot project.
 *
 * Copyright 2016 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <assert.h>
#include <console/console.h>
#include <ec/google/chromeec/ec.h>
#include <vendorcode/google/chromeos/chromeos.h>

#define VBOOT_HASH_VSLOT 0
#define VBOOT_HASH_VSLOT_MASK (1 << (VBOOT_HASH_VSLOT))

int vboot_save_hash(void *digest, size_t digest_size)
{
	const int slot = VBOOT_HASH_VSLOT;
	uint32_t lock_status;
	int num_slots;

	/* Ensure the digests being saved match the EC's slot size. */
	assert(digest_size == EC_VSTORE_SLOT_SIZE);

	if (google_chromeec_vstore_write(slot, digest, digest_size))
		return -1;

	/* Assert the slot is locked on successful write. */
	num_slots = google_chromeec_vstore_info(&lock_status);

	/* Normalize to be 0 based. If num_slots returned 0 then it'll be -1. */
	num_slots--;

	if (num_slots < slot) {
		printk(BIOS_ERR, "Not enough vstore slots for vboot hash: %d\n",
			num_slots + 1);
		return -1;
	}

	if ((lock_status & VBOOT_HASH_VSLOT_MASK) == 0) {
		printk(BIOS_ERR, "Vstore slot not locked after write.\n");
		return -1;
	}

	return 0;
}

int vboot_retrieve_hash(void *digest, size_t digest_size)
{
	/* Ensure the digests being saved match the EC's slot size. */
	assert(digest_size == EC_VSTORE_SLOT_SIZE);

	return google_chromeec_vstore_read(VBOOT_HASH_VSLOT, digest);
}
