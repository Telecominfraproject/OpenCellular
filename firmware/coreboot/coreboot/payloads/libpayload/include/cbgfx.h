/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2015 Google, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <libpayload.h>
#include <arch/types.h>
#include <stddef.h>

/*
 * API error codes
 */
#define CBGFX_SUCCESS			0
/* unknown error */
#define CBGFX_ERROR_UNKNOWN		1
/* failed to initialize cbgfx library */
#define CBGFX_ERROR_INIT		2
/* drawing beyond screen or canvas boundary */
#define CBGFX_ERROR_BOUNDARY		3
/* invalid parameter */
#define CBGFX_ERROR_INVALID_PARAMETER	4
/* bitmap error: signature mismatch */
#define CBGFX_ERROR_BITMAP_SIGNATURE	0x10
/* bitmap error: unsupported format */
#define CBGFX_ERROR_BITMAP_FORMAT	0x11
/* bitmap error: invalid data */
#define CBGFX_ERROR_BITMAP_DATA		0x12
/* bitmap error: scaling out of range */
#define CBGFX_ERROR_SCALE_OUT_OF_RANGE	0x13
/* invalid framebuffer info */
#define CBGFX_ERROR_FRAMEBUFFER_INFO	0x14
/* invalid framebuffer address */
#define CBGFX_ERROR_FRAMEBUFFER_ADDR	0x15
/* portrait screen not supported */
#define CBGFX_ERROR_PORTRAIT_SCREEN	0x16

struct fraction {
	int32_t n;
	int32_t d;
};

struct scale {
	struct fraction x;
	struct fraction y;
};

struct vector {
	union {
		int32_t x;
		int32_t width;
	};
	union {
		int32_t y;
		int32_t height;
	};
};

struct rect {
	struct vector offset;
	struct vector size;
};

struct rgb_color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

/*
 * Resolution of scale parameters used to describe height, width, coordinate,
 * etc. relative to the canvas. For example, if it's 100, scales range from 0 to
 * 100%.
 */
#define CANVAS_SCALE		100

/*
 * The coordinate system is expected to have (0, 0) at top left corner with
 * y values increasing towards bottom of screen.
 */

/**
 * draw a box filled with a color on screen
 *
 * box: .offset points the coordinate of the top left corner and .size specifies
 * width and height of the box. Both are relative to the canvas size thus scale
 * from 0 to CANVAS_SCALE (0 to 100%).
 * rgb: RGB color of the box.
 *
 * return: CBGFX_* error codes
 */
int draw_box(const struct rect *box, const struct rgb_color *rgb);

/**
 * Clear the canvas
 */
int clear_canvas(const struct rgb_color *rgb);

/**
 * Clear the screen
 */
int clear_screen(const struct rgb_color *rgb);

/**
 * Draw a bitmap image using position and size relative to the canvas
 *
 * @param[in] bitmap	Pointer to the bitmap data, starting from file header
 * @param[in] size	Size of the bitmap data
 * @param[in] pos_rel	Coordinate of the pivot relative to the canvas
 * @param[in] dim_rel	Width and height of the image relative to the canvas
 *                      width and height. They must not exceed 1 (=100%). If one
 *                      is zero, it's derived from the other to keep the aspect
 *                      ratio.
 * @param[in] flags     lower 8 bits is Pivot position. Use PIVOT_H_* and
 *                      PIVOT_V_* flags.
 *                      Bit 9 is bit to indicate if we invert the rendering.
 *                      0 = render image as is, 1 = invert image.
 *
 * @return CBGFX_* error codes
 *
 * 'Pivot' is a point of the image based on which the image is positioned.
 * For example, if a pivot is set to PIVOT_H_CENTER|PIVOT_V_CENTER, the image is
 * positioned so that pos_rel matches the center of the image.
 */
int draw_bitmap(const void *bitmap, size_t size,
		const struct scale *pos_rel, const struct scale *dim_rel,
		uint32_t flags);

/* Pivot flags. See the draw_bitmap description. */
#define PIVOT_H_LEFT	(1 << 0)
#define PIVOT_H_CENTER	(1 << 1)
#define PIVOT_H_RIGHT	(1 << 2)
#define PIVOT_V_TOP	(1 << 3)
#define PIVOT_V_CENTER	(1 << 4)
#define PIVOT_V_BOTTOM	(1 << 5)
#define PIVOT_MASK      0x000000ff

/* invert flag */
#define INVERT_SHIFT    8
#define INVERT_COLORS   (1 << INVERT_SHIFT)

/**
 * Draw a bitmap image at screen coordinate with no scaling
 *
 * @param[in] bitmap	Pointer to the bitmap data, starting from file header
 * @param[in] size	Size of the bitmap data
 * @param[in] pos       Screen coordinate of upper left corner of the image.
 *
 * @return CBGFX_* error codes
 */
int draw_bitmap_direct(const void *bitmap, size_t size,
		       const struct vector *top_left);

/**
 * Get width and height of projected image
 *
 * @param[in] bitmap	Pointer to the bitmap data, starting from file header
 * @param[in] sz	Size of the bitmap data
 * @param[i/o] dim_rel	Width and height of the image relative to the canvas
 *                      width and height. They must not exceed 1 (=100%).
 *                      On return, it contains automatically calculated width
 *                      and/or height.
 *
 * @return CBGFX_* error codes
 *
 * It returns the width and height of the projected image. If the input height
 * is zero, it's derived from the input width to keep the aspect ratio, and vice
 * versa. If both are zero, the width and the height which can project the image
 * in the original size are returned.
 */
int get_bitmap_dimension(const void *bitmap, size_t sz, struct scale *dim_rel);
