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

VAStatus
crystalhd_render_iqmatrix_buffer_h264(
	VADriverContextP ctx,
	object_context_p obj_context,
	object_buffer_p obj_buffer
);

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

VAStatus
crystalhd_begin_picture_h264(
	VADriverContextP ctx,
	VAContextID context,
	VASurfaceID render_target
);

VAStatus
crystalhd_end_picture_h264(
	VADriverContextP ctx,
	object_context_p obj_context,
	object_surface_p obj_surface
);

#endif
