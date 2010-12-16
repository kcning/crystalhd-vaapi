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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <va/va.h>

#include "bitstream.h"
#include "debug.h"
#include "crystalhd_drv_video.h"
#include "crystalhd_video_h264.h"
#include "x264_encoder_set.h"

VAStatus crystalhd_begin_picture_h264(
		VADriverContextP ctx,
		VAContextID context,
		VASurfaceID render_target
	)
{
	INIT_DRIVER_DATA;
	object_context_p obj_context = CONTEXT(context);

	obj_context->last_h264_sps_id = 0;
	obj_context->last_h264_pps_id = 0;

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_render_iqmatrix_buffer_h264(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	if (0 != obj_context->last_iqmatrix_buffer_id)
		crystalhd_DestroyBuffer(ctx, obj_context->last_iqmatrix_buffer_id);

	obj_context->last_iqmatrix_buffer_id = obj_buffer->base.id;

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_render_picture_parameter_buffer_h264(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	if (0 != obj_context->last_picture_param_buffer_id)
		crystalhd_DestroyBuffer(ctx, obj_context->last_picture_param_buffer_id);

	obj_context->last_picture_param_buffer_id = obj_buffer->base.id;

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_render_slice_parameter_buffer_h264(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	if (0 != obj_context->last_slice_param_buffer_id)
		crystalhd_DestroyBuffer(ctx, obj_context->last_slice_param_buffer_id);

	obj_context->last_slice_param_buffer_id = obj_buffer->base.id;

	return VA_STATUS_SUCCESS;
}

static inline
VAStatus crystalhd_render_sps_pps_h264(
		VADriverContextP ctx,
		object_context_p obj_context
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	uint8_t metadata[500] = { 0x00 };
	bs_t bs;

	object_surface_p obj_surface = SURFACE(obj_context->current_render_target);
	if (NULL == obj_surface)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
		goto error;
	}

	object_buffer_p pic_param_buf = BUFFER(obj_context->last_picture_param_buffer_id);
	object_buffer_p slice_param_buf = BUFFER(obj_context->last_slice_param_buffer_id);
	object_buffer_p iqmatrix_buf = BUFFER(obj_context->last_iqmatrix_buffer_id);
	if (NULL == pic_param_buf || NULL == slice_param_buf || NULL == iqmatrix_buf)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_BUFFER;
		goto error;
	}

	VAPictureParameterBufferH264 * const pic_param = pic_param_buf->buffer_data;
	VASliceParameterBufferH264 * const slice_param = slice_param_buf->buffer_data;
	VAIQMatrixBufferH264 * const iqmatrix = iqmatrix_buf->buffer_data;
	if (NULL == pic_param || NULL == slice_param || NULL == iqmatrix)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_BUFFER;
		goto error;
	}

	bs_init( &bs, metadata, sizeof(metadata) / sizeof(*metadata) );

	x264_sps_t sps;
	x264_pps_t pps;

	x264_sps_init( &sps, obj_context->last_h264_sps_id++, pic_param, slice_param, obj_context );
	x264_pps_init( &pps, obj_context->last_h264_pps_id++, &sps, pic_param, slice_param, iqmatrix );

	x264_sps_write( &bs, &sps );
	x264_pps_write( &bs, &pps );

	obj_surface->metadata = (uint8_t *)malloc(bs.p - bs.p_start);
	obj_surface->metadata_size = bs.p - bs.p_start;
	memcpy(obj_surface->metadata, bs.p_start, bs.p - bs.p_start);

	vaStatus = VA_STATUS_SUCCESS;

error:
	return vaStatus;
}

VAStatus crystalhd_render_slice_data_buffer_h264(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	object_surface_p obj_surface = SURFACE(obj_context->current_render_target);
	int i;

	obj_surface->data = (uint8_t *)realloc(obj_surface->data, obj_surface->data_size + (4 + obj_buffer->element_size) * obj_buffer->num_elements);
	if (NULL == obj_surface->data)
	{
		return VA_STATUS_ERROR_ALLOCATION_FAILED;
	}

	for (i = 0; i < obj_buffer->num_elements; ++i)
	{
		obj_surface->data[obj_surface->data_size + obj_buffer->element_size * i + 0] = (obj_buffer->element_size & 0xff000000) >> 24;
		obj_surface->data[obj_surface->data_size + obj_buffer->element_size * i + 1] = (obj_buffer->element_size & 0x00ff0000) >> 16;
		obj_surface->data[obj_surface->data_size + obj_buffer->element_size * i + 2] = (obj_buffer->element_size & 0x0000ff00) >> 8;
		obj_surface->data[obj_surface->data_size + obj_buffer->element_size * i + 3] = (obj_buffer->element_size & 0x000000ff);
		memcpy(obj_surface->data + obj_surface->data_size + (obj_buffer->element_size * i) + 4,
			obj_buffer->buffer_data, obj_buffer->element_size * obj_buffer->num_elements);
	}
	obj_surface->data_size = obj_surface->data_size + (4 + obj_buffer->element_size) * obj_buffer->num_elements;

	crystalhd_DestroyBuffer(ctx, obj_buffer->base.id);

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_end_picture_h264(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_surface_p obj_surface
	)
{
	INIT_DRIVER_DATA;

	crystalhd_render_sps_pps_h264(ctx, obj_context);

	BC_STATUS sts;
	if ( NULL != obj_surface->metadata && obj_surface->metadata_size > 0)
	{
		sts = DtsProcInput(driver_data->hdev, obj_surface->metadata, obj_surface->metadata_size, 0, FALSE);
		if (sts != BC_STS_SUCCESS)
		{
			crystalhd__error_message("%s: Unable to send metadata (surface). status = %d\n", __func__, sts);
			return VA_STATUS_ERROR_OPERATION_FAILED;
		}
	}

	if ( NULL != obj_context->metadata && obj_context->metadata_size > 0)
	{
		sts = DtsProcInput(driver_data->hdev, obj_context->metadata, obj_context->metadata_size, 0, FALSE);
		if (sts != BC_STS_SUCCESS)
		{
			crystalhd__error_message("%s: Unable to send metadata (context). status = %d\n", __func__, sts);
			return VA_STATUS_ERROR_OPERATION_FAILED;
		}
	}

	sts = DtsProcInput(driver_data->hdev, obj_surface->data, obj_surface->data_size, 0, FALSE);
	if (sts != BC_STS_SUCCESS)
	{
		crystalhd__error_message("%s: Unable to send data. status = %d\n", __func__, sts);
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	return VA_STATUS_SUCCESS;
}
