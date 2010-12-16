/*
 * Copyright (c) 2010 iWorldcom Co., Ltd. All Rights Reserved.
 *
 *    - Victor Tseng <palatis@iworldcom.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#define DTS_OUTPUT_TIMEOUT			1000

#define CRYSTALHD_MAX_PROFILES			11
#define CRYSTALHD_MAX_ENTRYPOINTS		5
#define CRYSTALHD_MAX_CONFIG_ATTRIBUTES		10
#define CRYSTALHD_MAX_IMAGE_FORMATS		5
#define CRYSTALHD_MAX_SUBPIC_FORMATS		4
#define CRYSTALHD_MAX_DISPLAY_ATTRIBUTES	4
#define CRYSTALHD_STR_VENDOR			"Broadcom Crystal HD Video Decoder " RC_FILE_VERSION

#define ALIGN(i, n)			(((i) + (n) - 1) & ~((n) - 1))
#define STRIDE(w)			(((w) + 0xf) & ~0xf)
#define SIZE_YUV420(w, h)		((h) * (STRIDE(w) + STRIDE((w) >> 1)))

#define CRYSTALHD_MIN(a, b)		(((a) < (b)) ? (a) : (b))
#define CRYSTALHD_MAX(a, b)		(((a) > (b)) ? (a) : (b))
#define CRYSTALHD_MIN3(a, b, c)		CRYSTALHD_MIN(CRYSTALHD_MIN(a, b), (c))
#define CRYSTALHD_MAX3(a, b, c)		CRYSTALHD_MAX(CRYSTALHD_MAX(a, b), (c))
#define CRYSTALHD_MIN4(a, b, c, d)	CRYSTALHD_MIN(CRYSTALHD_MIN3(a, b, c), (d))
#define CRYSTALHD_MAX4(a, b, c, d)	CRYSTALHD_MAX(CRYSTALHD_MAX3(a, b, c), (d))

#endif
