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

#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "debug.h"
#include "crystalhd_video_h264.h"

const h264_level_t h264_levels[] =
{
	{ 10,   1485,    99,   152064,     64,    175,  64, 64,  0, 2, 0, 0, 1 },
	{ 11,   3000,   396,   345600,    192,    500, 128, 64,  0, 2, 0, 0, 1 },
	{ 12,   6000,   396,   912384,    384,   1000, 128, 64,  0, 2, 0, 0, 1 },
	{ 13,  11880,   396,   912384,    768,   2000, 128, 64,  0, 2, 0, 0, 1 },
	{ 20,  11880,   396,   912384,   2000,   2000, 128, 64,  0, 2, 0, 0, 1 },
	{ 21,  19800,   792,  1824768,   4000,   4000, 256, 64,  0, 2, 0, 0, 0 },
	{ 22,  20250,  1620,  3110400,   4000,   4000, 256, 64,  0, 2, 0, 0, 0 },
	{ 30,  40500,  1620,  3110400,  10000,  10000, 256, 32, 22, 2, 0, 1, 0 },
	{ 31, 108000,  3600,  6912000,  14000,  14000, 512, 16, 60, 4, 1, 1, 0 },
	{ 32, 216000,  5120,  7864320,  20000,  20000, 512, 16, 60, 4, 1, 1, 0 },
	{ 40, 245760,  8192, 12582912,  20000,  25000, 512, 16, 60, 4, 1, 1, 0 },
	{ 41, 245760,  8192, 12582912,  50000,  62500, 512, 16, 24, 2, 1, 1, 0 },
	{ 42, 522240,  8704, 13369344,  50000,  62500, 512, 16, 24, 2, 1, 1, 1 },
	{ 50, 589824, 22080, 42393600, 135000, 135000, 512, 16, 24, 2, 1, 1, 1 },
	{ 51, 983040, 36864, 70778880, 240000, 240000, 512, 16, 24, 2, 1, 1, 1 },
	{ 0 }
};

static inline int h264_validate_levels(
		int profile_idc,
		int level_idc,
		VAPictureParameterBufferH264 * const pic_param,
		VASliceParameterBufferH264 * const slice_param
	)
{
	int mbs = (pic_param->picture_width_in_mbs_minus1 + 1) *
		  (pic_param->picture_height_in_mbs_minus1 + 1);
	int dpb = mbs * 384 * CRYSTALHD_MIN(16,
			CRYSTALHD_MAX3(pic_param->num_ref_frames, pic_param->frame_num, 5));
	int cpb_factor = (profile_idc == H264_PROFILE_HIGH) ? 5 : 4 ;


	const h264_level_t *l = h264_levels;
	while ( l->level_idc != 0 && l->level_idc != level_idc )
		++l;

	if ( l->frame_size < mbs ||
	     l->frame_size * 8 < (pic_param->picture_width_in_mbs_minus1 + 1) * (pic_param->picture_width_in_mbs_minus1 + 1) ||
	     l->frame_size * 8 < (pic_param->picture_height_in_mbs_minus1 + 1) * (pic_param->picture_height_in_mbs_minus1 + 1) )
		return 1;

	if (dpb > l->dpb )
		return 1;

	/* vbv_bitrate? */
	//if ( ((l->bitrate * cbp_factor) / 4) < (???) )
	//	return 1;
	
	/* vbv_buffer? */
	/* mv_range? */
	/* interlaced? */
	/* fake_interlaced? */
	/* fps_den? */

	return 0;
}

static inline int h264_get_level_idc(
		int profile_idc,
		VAPictureParameterBufferH264 * const pic_param,
		VASliceParameterBufferH264 * const slice_param
	)
{
	const h264_level_t *l = h264_levels;

	while (l[1].level_idc && h264_validate_levels(profile_idc, l->level_idc, pic_param, slice_param))
		++l;

	if (l[1].level_idc)
		++l;

	return l->level_idc;
}

VAStatus crystalhd_render_picture_parameter_buffer_h264(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	buffered_picture_parameter_buffer = obj_buffer;
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_render_slice_parameter_buffer_h264(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	uint8_t data[3000] = { 0x00 };
	bs_t bs;
	bs_t *s = &bs;
	object_surface_p obj_surface = SURFACE(obj_context->current_render_target);
	VAPictureParameterBufferH264 * const pic_param = ((object_buffer_p)buffered_picture_parameter_buffer)->buffer_data;
	VASliceParameterBufferH264 * const slice_param = obj_buffer->buffer_data;

	bs_init( s, data, sizeof(data) / sizeof(*data) );

	/* SPS header */
	bs_realign( s );
	
	bs_write( s, 24, 0x000001 );						/* start_code */
	bs_write( s, 8, NAL_HIGHEST_SPS );					/* nal */

	int profile_idc;
	if (pic_param->pic_fields.bits.transform_8x8_mode_flag)
	{
		profile_idc = H264_PROFILE_HIGH;
	}
	else if (slice_param->cabac_init_idc ||
		 pic_param->pic_fields.bits.weighted_pred_flag )
	{
		profile_idc = H264_PROFILE_MAIN;
	}
	else
	{
		profile_idc = H264_PROFILE_BASELINE;
	}
	bs_write( s, 8, profile_idc );						/* profile idc */
	bs_write( s, 1, profile_idc == H264_PROFILE_BASELINE );			/* constraint_set0 */
	bs_write( s, 1, profile_idc <= H264_PROFILE_MAIN );			/* constraint_set1 */
	bs_write( s, 1, 0 );							/* constraint_set2 */
	bs_write( s, 5, 0 );							/* reserved */
	bs_write( s, 8, h264_get_level_idc(
				profile_idc, pic_param, slice_param) );		/* level_idc */
	bs_write_ue( s, 0x00 );							/* sps_id */

	if ( profile_idc >= H264_PROFILE_HIGH )
	{
		bs_write_ue( s, pic_param->seq_fields.bits.chroma_format_idc );	/* chroma_format_idc */
		bs_write_ue( s, pic_param->bit_depth_luma_minus8 );		/* bit_depth_luma_minus8 */
		bs_write_ue( s, pic_param->bit_depth_chroma_minus8 );		/* bit_depth_chroma_minus8 */
		bs_write( s, 1, 1 );						/* qpprime_y_zero_transform_bypass */
		bs_write( s, 1, 0 );						/* seq_scaling_matrix_present_flag */
	}

	bs_write_ue( s, pic_param->seq_fields.bits.log2_max_frame_num_minus4 );	/* log2_max_frame_num_minus4 */
	bs_write_ue( s, pic_param->seq_fields.bits.pic_order_cnt_type );	/* pic_order_cnt_type */
	if ( pic_param->seq_fields.bits.pic_order_cnt_type == 0 )
	{
		bs_write_ue( s, pic_param->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 );
										/* log2_max_pic_order_cnt_lsb_minus4 */
	}
#if 0
	/* TODO */
	else if ( pic_param->seq_fields.bits.pic_order_cnt_type == 1 )
	{
		int i;
		bs_write( s, 1, pic_param->seq_fields.bits.delta_pic_order_always_zero_flag );
										/* delta_pic_order_always_zero_flag */
		bs_write_se( s, offset_for_non_ref_pic );			/* offset_for_non_ref_pic */
		bs_write_se( s, offset_for_top_to_bottom_field );		/* offset_for_top_to_bottom_field */
		bs_write_ue( s, num_ref_frames_in_poc_cycle );			/* num_ref_frames_in_poc_cycle */

		for ( i = 0; i < num_ref_frames_in_poc_cycle; i++ )
			bs_write_se( s, offset_for_ref_frame[i] );		/* offset_for_ref_frame */
	}
#endif
	bs_write_ue( s, pic_param->num_ref_frames );				/* num_ref_frames */
	bs_write( s, 1, pic_param->seq_fields.bits.gaps_in_frame_num_value_allowed_flag );
										/* gaps_in_frame_num_value_allowed_flag */
	bs_write_ue( s, pic_param->picture_width_in_mbs_minus1 );		/* picture_width_in_mbs_minus1 */
	if ( pic_param->seq_fields.bits.frame_mbs_only_flag )
		bs_write_ue( s, pic_param->picture_height_in_mbs_minus1 );	/* picture_height_in_mbs_minus1 */
	else /* interlaced */
		bs_write_ue( s, (pic_param->picture_height_in_mbs_minus1 + 1) / 2 - 1 );
	bs_write( s, 1, pic_param->seq_fields.bits.frame_mbs_only_flag );	/* frame_mbs_only_flag */
	if ( !pic_param->seq_fields.bits.frame_mbs_only_flag )
		bs_write( s, 1, pic_param->seq_fields.bits.mb_adaptive_frame_field_flag );
										/* mb_adaptive_frame_field_flag */
	bs_write( s, 1, pic_param->seq_fields.bits.direct_8x8_inference_flag );	/* direct_8x8_inference_flag */
	bs_write( s, 1, 0 );							/* crop? */
	bs_write( s, 1, 0 );							/* vui? */

	bs_rbsp_trailing( s );
	bs_flush( s );

	/* PPS header */
	bs_realign( s );

	bs_write( s, 24, 0x000001 );							/* start_code */
	bs_write( s, 8, NAL_HIGHEST_PPS );						/* nal */

	bs_write_ue( s, 0x00 );								/* pps_id */
	bs_write_ue( s, 0x00 );								/* sps_id */
	bs_write( s, 1, pic_param->pic_fields.bits.entropy_coding_mode_flag );		/* entrophy_coding_mode_flag */
	bs_write( s, 1, pic_param->pic_fields.bits.pic_order_present_flag );		/* pic_order_present_flag */
	bs_write_ue( s, pic_param->num_slice_groups_minus1 );				/* num_slice_groups_minus1 */
	if ( pic_param->num_slice_groups_minus1 > 0 )
	{
		bs_write_ue( s, pic_param->slice_group_map_type );			/* slice_group_map_type */
		switch (pic_param->slice_group_map_type) {
			/* TODO: 0, 2, 6 */
#if 0
		case 0:
			for (int i = 0; i <= pic_param->num_slice_groups_minus1; i++ )
			{
				bs_write_ue( s, run_length_minus1[i] );			/* no run_length_group_minus1[i] */
			}
			break;
		case 2:
			for (int i = 0; i <= pic_param->num_slice_groups_minus1; i++ )
			{
				bs_write_ue( s, top_left[i] );				/* no top_left[i] */
				bs_write_ue( s, bottom_right[i] );			/* no bottom_right[i] */
			}
			break;
#endif
		case 3:
		case 4:
		case 5:
			bs_write( s, 1, 0 );						/* no slice_group_change_direction_flag */
			bs_write_ue( s, pic_param->slice_group_change_rate_minus1 );	/* slice_group_change_rate_minus1 */
			break;
#if 0
		case 6:
			bs_write_ue( s, pic_size_in_map_units_minus1 );			/* no pic_size_in_map_units_minus1 */
			for (int i = 0; i <= pic_size_in_map_units_minus1; i++ )
			{
				/* write slice list here */
			}
			break;
#endif

		default:
			vaStatus = VA_STATUS_ERROR_UNKNOWN;
			goto error;
		}
	}
	bs_write_ue( s, slice_param->num_ref_idx_l0_active_minus1);			/* num_ref_idx_l0_active_minus_1 */
	bs_write_ue( s, slice_param->num_ref_idx_l1_active_minus1);			/* num_ref_idx_l1_active_minus_1 */
	bs_write( s, 1, pic_param->pic_fields.bits.weighted_pred_flag);			/* weighted_pred_flag */
	bs_write( s, 2, pic_param->pic_fields.bits.weighted_bipred_idc);		/* weighted_bipred_idc */
	bs_write_se( s, pic_param->pic_init_qp_minus26);				/* pic_init_qp_minus26 */
	bs_write_se( s, pic_param->pic_init_qs_minus26);				/* pic_init_qs_minus26 */
	bs_write_se( s, pic_param->chroma_qp_index_offset);				/* chroma_qp_index_offset */
	bs_write( s, 1, pic_param->pic_fields.bits.deblocking_filter_control_present_flag);	/* deblocking_filter_control_present_flag */
	bs_write( s, 1, pic_param->pic_fields.bits.constrained_intra_pred_flag);	/* constrained_intra_pred_flag */
	bs_write( s, 1, pic_param->pic_fields.bits.reference_pic_flag);			/* reference_pic_flag */
#if 0
	if ( pic_param->pic_fields.bits.transform_8x8_mode_flag )			/* what's cqm? */
	{
		/* TODO: scaling_matrix_present_flag, scaling_list,  */
		bs_write( s, 1, pic_param->pic_fields.bits.transform_8x8_mode_flag );	/* transform_8x8_mode_flag */
		bs_write( s, 1, scaling_matrix_present_flag );				/* no scaling_matrix_present_flag */
		if ( cqm )
		{
		}
		bs_write_se( s, pic_param->second_chroma_qp_index_offset );		/* second_chroma_qp_index_offset */
	}
#endif
	bs_rbsp_trailing( s );
	bs_flush( s );

	obj_surface->metadata = (uint8_t *)malloc(bs.p - bs.p_start);
	obj_surface->metadata_size = bs.p - bs.p_start;
	memcpy(obj_surface->metadata, bs.p_start, bs.p - bs.p_start);

	crystalhd_DestroyBuffer(ctx, obj_buffer->base.id);
	crystalhd_DestroyBuffer(ctx, buffered_picture_parameter_buffer->base.id);
	buffered_picture_parameter_buffer = NULL;

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;

error:
	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_render_slice_data_buffer_h264(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	object_surface_p obj_surface = SURFACE(obj_context->current_render_target);
	int i;

	obj_surface->data = (uint8_t *)malloc((3 + obj_buffer->element_size) * obj_buffer->num_elements + 4);
	if (NULL == obj_surface->data)
	{
		INSTRUMENT_RET;
		return VA_STATUS_ERROR_ALLOCATION_FAILED;
	}
	obj_surface->data_size = (3 + obj_buffer->element_size) * obj_buffer->num_elements + 4;

	for (i = 0; i < obj_buffer->num_elements; ++i)
	{
		obj_surface->data[obj_buffer->element_size * i + 0] = 0x00;
		obj_surface->data[obj_buffer->element_size * i + 1] = 0x00;
		obj_surface->data[obj_buffer->element_size * i + 2] = 0x01;
		memcpy(obj_surface->data + (obj_buffer->element_size * i) + 3,
			obj_buffer->buffer_data, obj_buffer->element_size * obj_buffer->num_elements);
	}

	obj_surface->data[obj_surface->data_size - 4] = 0x00;
	obj_surface->data[obj_surface->data_size - 3] = 0x00;
	obj_surface->data[obj_surface->data_size - 2] = 0x01;
	obj_surface->data[obj_surface->data_size - 1] = NAL_DISPOSABLE_EOSEQ;

	crystalhd_DestroyBuffer(ctx, obj_buffer->base.id);

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}
