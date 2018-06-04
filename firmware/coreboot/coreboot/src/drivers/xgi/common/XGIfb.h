/*
 * This file is part of the coreboot project.
 *
 * File taken verbatim from the Linux xgifb driver (v3.18.5)
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

#ifndef _LINUX_XGIFB
#define _LINUX_XGIFB
#include "vgatypes.h"
#include "vb_struct.h"

enum xgifb_display_type {
	XGIFB_DISP_NONE = 0,
	XGIFB_DISP_CRT,
	XGIFB_DISP_LCD,
	XGIFB_DISP_TV,
};

#define HASVB_NONE	    0x00
#define HASVB_301	    0x01
#define HASVB_LVDS	    0x02
#define HASVB_TRUMPION	    0x04
#define HASVB_LVDS_CHRONTEL 0x10
#define HASVB_302	    0x20
#define HASVB_CHRONTEL	    0x80

enum XGI_CHIP_TYPE {
	XG40 = 32,
	XG42,
	XG20 = 48,
	XG21,
	XG27,
};

enum xgi_tvtype {
	TVMODE_NTSC = 0,
	TVMODE_PAL,
	TVMODE_HIVISION,
	TVTYPE_PALM,
	TVTYPE_PALN,
	TVTYPE_NTSCJ,
	TVMODE_TOTAL
};

enum xgi_tv_plug {
	TVPLUG_UNKNOWN = 0,
	TVPLUG_COMPOSITE = 1,
	TVPLUG_SVIDEO = 2,
	TVPLUG_COMPOSITE_AND_SVIDEO = 3,
	TVPLUG_SCART = 4,
	TVPLUG_YPBPR_525i = 5,
	TVPLUG_YPBPR_525P = 6,
	TVPLUG_YPBPR_750P = 7,
	TVPLUG_YPBPR_1080i = 8,
	TVPLUG_TOTAL
};

struct XGIfb_info {
	unsigned long XGIfb_id;
	int    chip_id;                 /* PCI ID of detected chip */
	int    memory;                  /* video memory in KB which XGIfb manages */
	int    heapstart;               /* heap start (= XGIfb "mem" argument) in KB */
	unsigned char fbvidmode;        /* current XGIfb mode */

	unsigned char XGIfb_version;
	unsigned char XGIfb_revision;
	unsigned char XGIfb_patchlevel;

	unsigned char XGIfb_caps;       /* XGIfb capabilities */

	int    XGIfb_tqlen;             /* turbo queue length (in KB) */

	unsigned int XGIfb_pcibus;      /* The card's PCI ID */
	unsigned int XGIfb_pcislot;
	unsigned int XGIfb_pcifunc;

	unsigned char XGIfb_lcdpdc;     /* PanelDelayCompensation */

	unsigned char XGIfb_lcda;       /* Detected status of LCDA for low res/text modes */

	char reserved[235];             /* for future use */
};

struct xgifb_video_info {
	struct fb_info *fb_info;
	struct xgi_hw_device_info hw_info;
	struct vb_device_info dev_info;

	int mode_idx;
	int rate_idx;

	u32 pseudo_palette[17];

	int           chip_id;
	unsigned int  video_size;
	phys_addr_t   video_base;
	void __iomem *video_vbase;
	phys_addr_t   mmio_base;
	unsigned long mmio_size;
	void __iomem *mmio_vbase;
	unsigned long vga_base;
	int mtrr;

	int    video_bpp;
	int    video_cmap_len;
	int    video_width;
	int    video_height;
	int    video_vwidth;
	int    video_vheight;
	int    org_x;
	int    org_y;
	int    video_linelength;
	unsigned int refresh_rate;

	enum xgifb_display_type display2; /* the second display output type */
	bool display2_force;
	unsigned char hasVB;
	unsigned char TV_type;
	unsigned char TV_plug;

	struct XGI21_LVDSCapStruct lvds_data;

	enum XGI_CHIP_TYPE chip;
	unsigned char revision_id;

	unsigned short DstColor;
	unsigned long  XGI310_AccelDepth;
	unsigned long  CommandReg;

	unsigned int   pcibus;
	unsigned int   pcislot;
	unsigned int   pcifunc;

	unsigned short subsysvendor;
	unsigned short subsysdevice;

	char reserved[236];
};

#endif
