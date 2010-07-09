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

#ifndef _CRYSTALHD_VIDEO_H264_H_
#define _CRYSTALHD_VIDEO_H264_H_

#include <va/va.h>

#include "crystalhd_drv_video.h"

typedef struct {
	int level_idc;
	int mbps;		/* max macroblock processing rate (macroblocks/sec) */
	int frame_size;		/* max frame size (macroblocks) */
	int dpb;		/* max decoded picture buffer (bytes) */
	int bitrate;		/* max bitrate (kbit/sec) */
	int cpb;		/* max vbv buffer (kbit) */
	int mv_range;		/* max vertical mv component range (pixels) */
	int mvs_per_2mb;	/* max mvs per 2 consecutive mbs. */
	int slice_rate;		/* ?? */
	int mincr;		/* min compression ratio */
	int bipred8x8;		/* limit bipred to >=8x8 */
	int direct8x8;		/* limit b_direct to >=8x8 */
	int frame_only;		/* forbid interlacing */
} h264_level_t;

extern const h264_level_t h264_levels[];

enum h264_profile_e
{
	H264_PROFILE_BASELINE		= 66,
	H264_PROFILE_MAIN		= 77,
	H264_PROFILE_EXTENDED		= 88,
	H264_PROFILE_HIGH		= 100,
	H264_PROFILE_HIGH10		= 110,
	H264_PROFILE_HIGH422		= 122,
	H264_PROFILE_HIGH444		= 144,
	H264_PROFILE_HIGH444_PREDICTIVE	= 244,
};

enum NALU
{
	NAL_DISPOSABLE_EOSEQ	= 0x0a,
	NAL_HIGHEST_SLICE_IDR	= 0x65,
	NAL_HIGHEST_SPS		= 0x67,
	NAL_HIGHEST_PPS		= 0x68,
};

VAStatus
crystalhd_render_picture_parameter_buffer_h264(
	VADriverContextP ctx,
	object_context_p obj_context,
	object_buffer_p obj_buffer
);

VAStatus
crystalhd_render_slice_parameter_buffer_h264(
	VADriverContextP ctx,
	object_context_p obj_context,
	object_buffer_p obj_buffer
);

VAStatus
crystalhd_render_slice_data_buffer_h264(
	VADriverContextP ctx,
	object_context_p obj_context,
	object_buffer_p obj_buffer
);

#endif
