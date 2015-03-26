#include "fmap.h"
#include "file_type.h"
#include "futility.h"
#include "traversal.h"

/* These are the expected areas, in order of traversal */
struct bios_fmap_s bios_area[] = {
	{BIOS_FMAP_GBB,	       "GBB",         "GBB Area"},
	{BIOS_FMAP_FW_MAIN_A,  "FW_MAIN_A",   "Firmware A Data"},
	{BIOS_FMAP_FW_MAIN_B,  "FW_MAIN_B",   "Firmware B Data"},
	{BIOS_FMAP_VBLOCK_A,   "VBLOCK_A",    "Firmware A Key"},
	{BIOS_FMAP_VBLOCK_B,   "VBLOCK_B",    "Firmware B Key"},
};
BUILD_ASSERT(ARRAY_SIZE(bios_area) == NUM_BIOS_COMPONENTS);


void fmap_limit_area(FmapAreaHeader *ah, uint32_t len)
{
	uint32_t sum = ah->area_offset + ah->area_size;
	if (sum < ah->area_size || sum > len) {
		Debug("%s(%s) 0x%x + 0x%x > 0x%x\n",
		      __func__, ah->area_name,
		      ah->area_offset, ah->area_size, len);
		ah->area_offset = 0;
		ah->area_size = 0;
	}
}

enum futil_file_type ft_recognize_bios_image(uint8_t *buf, uint32_t len)
{
	FmapHeader *fmap;
	int i;

	fmap = fmap_find(buf, len);
	if (!fmap)
		return FILE_TYPE_UNKNOWN;

	for (i = 0; i < NUM_BIOS_COMPONENTS; i++)
		if (!fmap_find_by_name(buf, len, fmap,
				       bios_area[i].name, 0))
			break;
	if (i == NUM_BIOS_COMPONENTS)
		return FILE_TYPE_BIOS_IMAGE;

	for (i = 0; i < NUM_BIOS_COMPONENTS; i++)
		if (!fmap_find_by_name(buf, len, fmap,
				       bios_area[i].oldname, 0))
			break;
	if (i == NUM_BIOS_COMPONENTS)
		return FILE_TYPE_OLD_BIOS_IMAGE;

	return FILE_TYPE_UNKNOWN;
}
