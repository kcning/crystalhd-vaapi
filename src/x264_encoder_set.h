/*****************************************************************************
 * set.h: header writing
 *****************************************************************************
 * Copyright (C) 2003-2010 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#ifndef X264_ENCODER_SET_H
#define X264_ENCODER_SET_H

#include <va/va.h>
#include "bitstream.h"
#include "crystalhd_drv_video.h"

enum nal_unit_type_e
{
	NAL_UNKNOWN	= 0x00,
	NAL_SLICE	= 0x01,
	NAL_SLICE_DPA	= 0x02,
	NAL_SLICE_DPB	= 0x03,
	NAL_SLICE_DPC	= 0x04,
	NAL_SLICE_IDR	= 0x05,	/* ref_idc != 0 */
	NAL_SEI		= 0x06,	/* ref_idc == 0 */
	NAL_SPS		= 0x07,
	NAL_PPS		= 0x08,
	NAL_AUD		= 0x09,
	NAL_FILLER	= 0x0c,
	/* ref_idc == 0 for 0x06, 0x09, 0x0a, 0x0b, 0x0c */
};
enum nal_priority_e
{
	NAL_PRIORITY_DISPOSABLE	= 0x00,
	NAL_PRIORITY_LOW	= 0x01,
	NAL_PRIORITY_HIGH	= 0x02,
	NAL_PRIORITY_HIGHEST	= 0x03,
};

enum profile_e
{
	PROFILE_BASELINE = 66,
	PROFILE_MAIN     = 77,
	PROFILE_EXTENDED = 88,
	PROFILE_HIGH    = 100,
	PROFILE_HIGH10  = 110,
	PROFILE_HIGH422 = 122,
	PROFILE_HIGH444 = 144,
	PROFILE_HIGH444_PREDICTIVE = 244,
};

enum cqm4_e
{
	CQM_4IY = 0,
	CQM_4PY = 1,
	CQM_4IC = 2,
	CQM_4PC = 3
};

enum cqm8_e
{
	CQM_8IY = 0,
	CQM_8PY = 1
};

typedef struct
{
	int i_id;

	int i_profile_idc;
	int i_level_idc;

	int b_constraint_set0;
	int b_constraint_set1;
	int b_constraint_set2;
	int b_constraint_set3;

	int i_log2_max_frame_num;

	int i_poc_type;
	/* poc 0 */
	int i_log2_max_poc_lsb;
	/* poc 1 */
	int b_delta_pic_order_always_zero;
	int i_offset_for_non_ref_pic;
	int i_offset_for_top_to_bottom_field;
	int i_num_ref_frames_in_poc_cycle;
	int i_offset_for_ref_frame[256];

	int i_num_ref_frames;
	int b_gaps_in_frame_num_value_allowed;
	int i_mb_width;
	int i_mb_height;
	int b_frame_mbs_only;
	int b_mb_adaptive_frame_field;
	int b_direct8x8_inference;

	int i_bit_depth_luma_minus8;
	int i_bit_depth_chroma_minus8;

	int b_crop;
	struct
	{
		int i_left;
		int i_right;
		int i_top;
		int i_bottom;
	} crop;

	int b_vui;
	struct
	{
		int b_aspect_ratio_info_present;
		int i_sar_width;
		int i_sar_height;

		int b_overscan_info_present;
		int b_overscan_info;

		int b_signal_type_present;
		int i_vidformat;
		int b_fullrange;
		int b_color_description_present;
		int i_colorprim;
		int i_transfer;
		int i_colmatrix;

		int b_chroma_loc_info_present;
		int i_chroma_loc_top;
		int i_chroma_loc_bottom;

		int b_timing_info_present;
		uint32_t i_num_units_in_tick;
		uint32_t i_time_scale;
		int b_fixed_frame_rate;

		int b_nal_hrd_parameters_present;
		int b_vcl_hrd_parameters_present;

		struct
		{
			int i_cpb_cnt;
			int i_bit_rate_scale;
			int i_cpb_size_scale;
			int i_bit_rate_value;
			int i_cpb_size_value;
			int i_bit_rate_unscaled;
			int i_cpb_size_unscaled;
			int b_cbr_hrd;

			int i_initial_cpb_removal_delay_length;
			int i_cpb_removal_delay_length;
			int i_dpb_output_delay_length;
			int i_time_offset_length;
		} hrd;

		int b_pic_struct_present;
		int b_bitstream_restriction;
		int b_motion_vectors_over_pic_boundaries;
		int i_max_bytes_per_pic_denom;
		int i_max_bits_per_mb_denom;
		int i_log2_max_mv_length_horizontal;
		int i_log2_max_mv_length_vertical;
		int i_num_reorder_frames;
		int i_max_dec_frame_buffering;

		/* FIXME to complete */
	} vui;

	int b_qpprime_y_zero_transform_bypass;
} x264_sps_t;

typedef struct
{
	int i_id;
	int i_sps_id;

	int b_cabac;

	int b_pic_order;
	int i_num_slice_groups;

	int i_num_ref_idx_l0_default_active;
	int i_num_ref_idx_l1_default_active;

	int b_weighted_pred;
	int b_weighted_bipred;

	int i_pic_init_qp;
	int i_pic_init_qs;

	int i_chroma_qp_index_offset;

	int b_deblocking_filter_control;
	int b_constrained_intra_pred;
	int b_redundant_pic_cnt;

	int b_transform_8x8_mode;

	int i_cqm_preset;
	const uint8_t *scaling_list[6]; /* could be 8, but we don't allow separate Cb/Cr lists */
} x264_pps_t;

typedef struct {
int level_idc;
int mbps;        /* max macroblock processing rate (macroblocks/sec) */
int frame_size;  /* max frame size (macroblocks) */
int dpb;         /* max decoded picture buffer (bytes) */
int bitrate;     /* max bitrate (kbit/sec) */
int cpb;         /* max vbv buffer (kbit) */
int mv_range;    /* max vertical mv component range (pixels) */
int mvs_per_2mb; /* max mvs per 2 consecutive mbs. */
int slice_rate;  /* ?? */
int mincr;       /* min compression ratio */
int bipred8x8;   /* limit bipred to >=8x8 */
int direct8x8;   /* limit b_direct to >=8x8 */
int frame_only;  /* forbid interlacing */
} x264_level_t;

void x264_sps_init(
	x264_sps_t *sps,
	int i_id,
	VAProfile va_profile,
	VAPictureParameterBufferH264 const * const pic_param,
	VASliceParameterBufferH264 const * const slice_param,
	const object_context_p const obj_context
);
void x264_pps_init(
	x264_pps_t *pps,
	int i_id,
	x264_sps_t const * const sps,
	VAPictureParameterBufferH264 const * const pic_param,
	VASliceParameterBufferH264 const * const slice_param,
	VAIQMatrixBufferH264 const * const iqmatrix
);
void x264_sps_write( bs_t *s, x264_sps_t * const sps );
void x264_pps_write( bs_t *s, x264_pps_t * const pps );

#if 0
void x264_sei_recovery_point_write( x264_t *h, bs_t *s, int recovery_frame_cnt );
int  x264_sei_version_write( x264_t *h, bs_t *s );
int  x264_validate_levels( x264_t *h, int verbose );
void x264_sei_buffering_period_write( x264_t *h, bs_t *s );
void x264_sei_pic_timing_write( x264_t *h, bs_t *s );
void x264_sei_write( bs_t *s, uint8_t *payload, int payload_size, int payload_type );
void x264_filler_write( x264_t *h, bs_t *s, int filler );
#endif

#endif
