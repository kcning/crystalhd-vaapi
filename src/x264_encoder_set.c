/*****************************************************************************
 * set: header writing
 *****************************************************************************
 * Copyright (C) 2003-2010 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *		  Loren Merritt <lorenm@u.washington.edu>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>
#include <string.h>
#include <va/va.h>

#include "common.h"
#include "debug.h"
#include "x264_encoder_set.h"

#define bs_write_ue bs_write_ue_big

static inline void transpose( uint8_t *buf, int w )
{
#define XCHG(type,a,b) do{ type t = a; a = b; b = t; } while(0)
	for( int i = 0; i < w; i++ )
		for( int j = 0; j < i; j++ )
			XCHG( uint8_t, buf[w*i+j], buf[w*j+i] );
#undef XCHG
}

const x264_level_t x264_levels[] =
{
	{ 10,   1485,    99,   152064,     64,    175,  64, 64,  0, 2, 0, 0, 1 },
	{  9,   1485,    99,   152064,    128,    350,  64, 64,  0, 2, 0, 0, 1 }, /* "1b" */
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

static inline int __estimate_level_idc(
	int profile_idc,
	VAPictureParameterBufferH264 const * const pic_param,
	VASliceParameterBufferH264 const * const slice_param
)
{
	int mbs = (pic_param->picture_width_in_mbs_minus1 + 1) * (pic_param->picture_height_in_mbs_minus1 + 1);
	int dpb = mbs * 384 * CRYSTALHD_MIN(16, CRYSTALHD_MAX3(pic_param->num_ref_frames, pic_param->frame_num, 5));
	int cbp_factor = ((profile_idc == PROFILE_HIGH10) ? 12 : ((profile_idc == PROFILE_HIGH) ? 5 : 4));
	crystalhd__information_message("%s: mbs = %d, dpb = %d, cbp_factor = %d\n", __func__, mbs, dpb, cbp_factor);

	const x264_level_t *l = x264_levels;

#define CHECK( test ) \
	while( l->level_idc != 0 && (test) ) \
	{ \
		crystalhd__information_message("assert \"" #test "\" for level_idc %d failed.\n", l->level_idc);\
		++l; \
	}
	CHECK( l->frame_size < mbs );
	CHECK( l->frame_size * 8 < (pic_param->picture_width_in_mbs_minus1 + 1) * (pic_param->picture_width_in_mbs_minus1 + 1) );
	CHECK( l->frame_size * 8 < (pic_param->picture_height_in_mbs_minus1 + 1) * (pic_param->picture_height_in_mbs_minus1 + 1) );
	CHECK( l->dpb < dpb );
	//CHECK( "VBV bitrate", (l->bitrate * cbp_factor) / 4, h->_param.rc.i_vbv_max_bitrate );
	//CHECK( "VBV buffer", (l->cpb * cbp_factor) / 4, h->_param.rc.i_vbv_buffer_size );
	//CHECK( "MV range", l->mv_range, h->_param.analyse.i_mv_range );
	CHECK( !l->frame_only != pic_param->pic_fields.bits.pic_order_present_flag );
	//if( h->_param.i_fps_den > 0 )
	//	CHECK( "MB rate", l->mbps, (int64_t)mbs * h->_param.i_fps_num / h->_param.i_fps_den );
#undef CHECK

	if ((l+1)->level_idc != 0)
		++l;

	return l->level_idc;
}

/* zigzags are transposed with respect to the tables in the standard */
static const uint8_t x264_zigzag_scan4[2][16] =
{{ // frame
	0,  4,  1,  2,  5,  8, 12,  9,  6,  3,  7, 10, 13, 14, 11, 15
},
{  // field
	0,  1,  4,  2,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
}};
static const uint8_t x264_zigzag_scan8[2][64] =
{{
	 0,  8,  1,  2,  9, 16, 24, 17, 10,  3,  4, 11, 18, 25, 32, 40,
	33, 26, 19, 12,  5,  6, 13, 20, 27, 34, 41, 48, 56, 49, 42, 35,
	28, 21, 14,  7, 15, 22, 29, 36, 43, 50, 57, 58, 51, 44, 37, 30,
	23, 31, 38, 45, 52, 59, 60, 53, 46, 39, 47, 54, 61, 62, 55, 63
},
{
	 0,  1,  2,  8,  9,  3,  4, 10, 16, 11,  5,  6,  7, 12, 17, 24,
	18, 13, 14, 15, 19, 25, 32, 26, 20, 21, 22, 23, 27, 33, 40, 34,
	28, 29, 30, 31, 35, 41, 48, 42, 36, 37, 38, 39, 43, 49, 50, 44,
	45, 46, 47, 51, 56, 57, 52, 53, 54, 55, 58, 59, 60, 61, 62, 63
}};

/* default quant matrices */
static const uint8_t x264_cqm_jvt4i[16] =
{
	 6,13,20,28,
	13,20,28,32,
	20,28,32,37,
	28,32,37,42
};
static const uint8_t x264_cqm_jvt4p[16] =
{
	10,14,20,24,
	14,20,24,27,
	20,24,27,30,
	24,27,30,34
};
static const uint8_t x264_cqm_jvt8i[64] =
{
	 6,10,13,16,18,23,25,27,
	10,11,16,18,23,25,27,29,
	13,16,18,23,25,27,29,31,
	16,18,23,25,27,29,31,33,
	18,23,25,27,29,31,33,36,
	23,25,27,29,31,33,36,38,
	25,27,29,31,33,36,38,40,
	27,29,31,33,36,38,40,42
};
static const uint8_t x264_cqm_jvt8p[64] =
{
	 9,13,15,17,19,21,22,24,
	13,13,17,19,21,22,24,25,
	15,17,19,21,22,24,25,27,
	17,19,21,22,24,25,27,28,
	19,21,22,24,25,27,28,30,
	21,22,24,25,27,28,30,32,
	22,24,25,27,28,30,32,33,
	24,25,27,28,30,32,33,35
};
static const uint8_t x264_cqm_flat16[64] =
{
	16,16,16,16,16,16,16,16,
	16,16,16,16,16,16,16,16,
	16,16,16,16,16,16,16,16,
	16,16,16,16,16,16,16,16,
	16,16,16,16,16,16,16,16,
	16,16,16,16,16,16,16,16,
	16,16,16,16,16,16,16,16,
	16,16,16,16,16,16,16,16
};
static const uint8_t * const x264_cqm_jvt[6] =
{
	x264_cqm_jvt4i, x264_cqm_jvt4p,
	x264_cqm_jvt4i, x264_cqm_jvt4p,
	x264_cqm_jvt8i, x264_cqm_jvt8p
};

static inline void scaling_list_write( bs_t *s, x264_pps_t *pps, int idx )
{
	const int len = idx<4 ? 16 : 64;
	const uint8_t *zigzag = idx<4 ? x264_zigzag_scan4[0] : x264_zigzag_scan8[0];
	const uint8_t *list = pps->scaling_list[idx];
	const uint8_t *def_list = (idx==CQM_4IC) ? pps->scaling_list[CQM_4IY]
							: (idx==CQM_4PC) ? pps->scaling_list[CQM_4PY]
							: x264_cqm_jvt[idx];
	if( !memcmp( list, def_list, len ) )
		bs_write( s, 1, 0 ); // scaling_list_present_flag
	else if( !memcmp( list, x264_cqm_jvt[idx], len ) )
	{
		bs_write( s, 1, 1 ); // scaling_list_present_flag
		bs_write_se( s, -8 ); // use jvt list
	}
	else
	{
		int run;
		bs_write( s, 1, 1 ); // scaling_list_present_flag

		// try run-length compression of trailing values
		for( run = len; run > 1; run-- )
			if( list[zigzag[run-1]] != list[zigzag[run-2]] )
				break;
		if( run < len && len - run < bs_size_se( (int8_t)-list[zigzag[run]] ) )
			run = len;

		for( int j = 0; j < run; j++ )
			bs_write_se( s, (int8_t)(list[zigzag[j]] - (j>0 ? list[zigzag[j-1]] : 8)) ); // delta

		if( run < len )
			bs_write_se( s, (int8_t)-list[zigzag[run]] );
	}
}

void x264_sps_init(
	x264_sps_t *sps,
	int i_id,
	VAProfile va_profile,
	VAPictureParameterBufferH264 const * const pic_param,
	VASliceParameterBufferH264 const * const slice_param,
	const object_context_p const obj_context )
{
	sps->i_id = i_id;
	sps->i_mb_width = pic_param->picture_width_in_mbs_minus1 + 1;
	sps->i_mb_height= pic_param->picture_height_in_mbs_minus1 + 1;

	/* FIXME: sps->b_qpprime_y_zero_transform_bypass = _param->rc.i_rc_method == X264_RC_CQP && _param->rc.i_qp_constant == 0; */
	sps->b_qpprime_y_zero_transform_bypass = 0;

	if ( va_profile == VAProfileH264Baseline )
		sps->i_profile_idc = PROFILE_BASELINE;
	else if ( va_profile == VAProfileH264Main )
		sps->i_profile_idc = PROFILE_MAIN;
	else if ( va_profile == VAProfileH264High )
		sps->i_profile_idc = PROFILE_HIGH;
	else
		/* should fail here = = */;

#if 0
	if( sps->b_qpprime_y_zero_transform_bypass )
		sps->i_profile_idc  = PROFILE_HIGH444_PREDICTIVE;
	else if( pic_param->bit_depth_luma_minus8 || pic_param->bit_depth_chroma_minus8 )
		sps->i_profile_idc  = PROFILE_HIGH10;
	else if( pic_param->pic_fields.bits.transform_8x8_mode_flag || pic_param->seq_fields.bits.chroma_format_idc )
		sps->i_profile_idc  = PROFILE_HIGH;
	else if( pic_param->pic_fields.bits.entropy_coding_mode_flag || pic_param->pic_fields.bits.weighted_pred_flag ||
		 pic_param->pic_fields.bits.pic_order_present_flag /* <== interlaced */
		/* || _param->i_bframe > 0 */ )
		sps->i_profile_idc  = PROFILE_MAIN;
	else
		sps->i_profile_idc  = PROFILE_BASELINE;
#endif

	sps->b_constraint_set0  = sps->i_profile_idc == PROFILE_BASELINE;
	sps->b_constraint_set1  = sps->i_profile_idc <= PROFILE_MAIN;
	sps->b_constraint_set2  = 0;
	sps->b_constraint_set3  = 0;

	sps->i_level_idc = __estimate_level_idc(sps->i_profile_idc, pic_param, slice_param);
	if( sps->i_level_idc == 9 && ( sps->i_profile_idc >= PROFILE_BASELINE && sps->i_profile_idc <= PROFILE_EXTENDED ) )
	{
		sps->b_constraint_set3 = 1; /* level 1b with Baseline, Main or Extended profile is signalled via constraint_set3 */
		sps->i_level_idc = 11;
	}
	/* High 10 Intra profile */
	sps->b_constraint_set3 = sps->i_profile_idc == PROFILE_HIGH10;

	sps->i_bit_depth_luma_minus8 = pic_param->bit_depth_luma_minus8;
	sps->i_bit_depth_chroma_minus8 = pic_param->bit_depth_chroma_minus8;

	sps->vui.i_num_reorder_frames = pic_param->seq_fields.bits.pic_order_cnt_type;
	/* extra slot with pyramid so that we don't have to override the
	 * order of forgetting old pictures */
	sps->vui.i_max_dec_frame_buffering =
	sps->i_num_ref_frames = pic_param->num_ref_frames;

	sps->i_log2_max_frame_num = pic_param->seq_fields.bits.log2_max_frame_num_minus4 + 4;

	sps->i_poc_type = pic_param->seq_fields.bits.pic_order_cnt_type;
	if( sps->i_poc_type == 0 )
	{
		sps->i_log2_max_poc_lsb = pic_param->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 + 4;
	}
	else if( sps->i_poc_type == 1 )
	{
		int i;

		/* FIXME */
		sps->b_delta_pic_order_always_zero = 1;
		sps->i_offset_for_non_ref_pic = 0;
		sps->i_offset_for_top_to_bottom_field = 0;
		sps->i_num_ref_frames_in_poc_cycle = 0;

		for( i = 0; i < pic_param->num_ref_frames; i++ )
		{
			sps->i_offset_for_ref_frame[i] = 0;
		}
	}

	sps->b_vui = 1;

	sps->b_gaps_in_frame_num_value_allowed = 0;
	sps->b_frame_mbs_only = pic_param->seq_fields.bits.frame_mbs_only_flag;
	if( !sps->b_frame_mbs_only )
		sps->i_mb_height = ( sps->i_mb_height + 1 ) & ~1;
	sps->b_mb_adaptive_frame_field = pic_param->seq_fields.bits.frame_mbs_only_flag;
	sps->b_direct8x8_inference = 1;

	sps->crop.i_left	= 0;
	sps->crop.i_top		= 0;
	sps->crop.i_right	= sps->i_mb_width*16 - obj_context->picture_width;
	sps->crop.i_bottom	= (sps->i_mb_height*16 - obj_context->picture_height) >> !pic_param->seq_fields.bits.frame_mbs_only_flag;
	sps->b_crop		= sps->crop.i_left || sps->crop.i_top || sps->crop.i_right || sps->crop.i_bottom;

	// assume square aspect ratio
	sps->vui.b_aspect_ratio_info_present = 1;
	sps->vui.i_sar_width = 1;
	sps->vui.i_sar_height = 1;
	/*
	if( param->vui.i_sar_width > 0 && param->vui.i_sar_height > 0 )
	{
		sps->vui.b_aspect_ratio_info_present = 1;
		sps->vui.i_sar_width = param->vui.i_sar_width;
		sps->vui.i_sar_height= param->vui.i_sar_height;
	}
	*/

	// assume no overscan
	sps->vui.b_overscan_info_present = 0;
	sps->vui.b_overscan_info = 0;
	/*
	sps->vui.b_overscan_info_present = ( param->vui.i_overscan ? 1 : 0 );
	if( sps->vui.b_overscan_info_present )
		sps->vui.b_overscan_info = ( param->vui.i_overscan == 2 ? 1 : 0 );
	*/

	sps->vui.b_signal_type_present = 0;
	// huh? what're these?
	sps->vui.i_vidformat = 5; // ( param->vui.i_vidformat <= 5 ? param->vui.i_vidformat : 5 );
	sps->vui.b_fullrange = 0; // ( param->vui.b_fullrange ? 1 : 0 );
	sps->vui.b_color_description_present = 0;

	// assume no color descriptions */
	sps->vui.i_colorprim	= 2;
	sps->vui.i_transfer	= 2;
	sps->vui.i_colmatrix	= 2;
	/*
	sps->vui.i_colorprim = ( param->vui.i_colorprim <=  9 ? param->vui.i_colorprim : 2 );
	sps->vui.i_transfer  = ( param->vui.i_transfer  <= 11 ? param->vui.i_transfer  : 2 );
	sps->vui.i_colmatrix = ( param->vui.i_colmatrix <=  9 ? param->vui.i_colmatrix : 2 );
	if( sps->vui.i_colorprim != 2 ||
		sps->vui.i_transfer  != 2 ||
		sps->vui.i_colmatrix != 2 )
	{
		sps->vui.b_color_description_present = 1;
	}

	if( sps->vui.i_vidformat != 5 ||
		sps->vui.b_fullrange ||
		sps->vui.b_color_description_present )
	{
		sps->vui.b_signal_type_present = 1;
	}
	*/

	/* FIXME: not sufficient for interlaced video */
	// assume no chroma loc info */
	/*
	sps->vui.b_chroma_loc_info_present = ( param->vui.i_chroma_loc ? 1 : 0 );
	if( sps->vui.b_chroma_loc_info_present )
	{
		sps->vui.i_chroma_loc_top = param->vui.i_chroma_loc;
		sps->vui.i_chroma_loc_bottom = param->vui.i_chroma_loc;
	}
	*/

	// FIXME: faking timing info
	sps->vui.b_timing_info_present = 1;
	sps->vui.i_num_units_in_tick = 0;
	sps->vui.i_time_scale = 0;
	sps->vui.b_fixed_frame_rate = 0;
	/*
	sps->vui.b_timing_info_present = param->i_timebase_num > 0 && param->i_timebase_den > 0;
	if( sps->vui.b_timing_info_present )
	{
		sps->vui.i_num_units_in_tick = param->i_timebase_num;
		sps->vui.i_time_scale = param->i_timebase_den * 2;
		sps->vui.b_fixed_frame_rate = !param->b_vfr_input;
	}
	*/

	// assume we got none of these...
	sps->vui.b_vcl_hrd_parameters_present = 0;
	sps->vui.b_nal_hrd_parameters_present = 0;
	sps->vui.b_pic_struct_present = 0;
	/*
	sps->vui.b_vcl_hrd_parameters_present = 0; // we don't support vcl hrd
	sps->vui.b_nal_hrd_parameters_present = !!param->i_nal_hrd;
	sps->vui.b_pic_struct_present = param->b_pic_struct;
	*/

	// NOTE: HRD related parts of the SPS are initialised in x264_ratecontrol_init_reconfigurable
	sps->vui.b_bitstream_restriction = 1;
	if( sps->vui.b_bitstream_restriction )
	{
		sps->vui.b_motion_vectors_over_pic_boundaries = 1;
		sps->vui.i_max_bytes_per_pic_denom = 0;
		sps->vui.i_max_bits_per_mb_denom = 0;
		// FIXME: hun???
		sps->vui.i_log2_max_mv_length_horizontal = sps->vui.i_log2_max_mv_length_vertical = 9;
		/*
		sps->vui.i_log2_max_mv_length_horizontal =
		sps->vui.i_log2_max_mv_length_vertical = (int)log2f( CRYSTALHD_MAX( 1, param->analyse.i_mv_range*4-1 ) ) + 1;
		*/
	}
}

void x264_sps_write( bs_t *s, x264_sps_t *sps )
{
	bs_realign( s );

	bs_write( s, 32, 0x00000001 );					// delim
	bs_write( s, 8, (NAL_PRIORITY_HIGHEST << 5) | NAL_SPS );	// nal header

	bs_write( s, 8, sps->i_profile_idc );
	bs_write( s, 1, sps->b_constraint_set0 );
	bs_write( s, 1, sps->b_constraint_set1 );
	bs_write( s, 1, sps->b_constraint_set2 );
	bs_write( s, 1, sps->b_constraint_set3 );

	bs_write( s, 4, 0 );	/* reserved */

	bs_write( s, 8, sps->i_level_idc );

	bs_write_ue( s, sps->i_id );

	if( sps->i_profile_idc >= PROFILE_HIGH )
	{
		bs_write_ue( s, 1 ); // chroma_format_idc = 4:2:0
		bs_write_ue( s, sps->i_bit_depth_luma_minus8 ); // bit_depth_luma_minus8
		bs_write_ue( s, sps->i_bit_depth_chroma_minus8 ); // bit_depth_chroma_minus8
		bs_write( s, 1, sps->b_qpprime_y_zero_transform_bypass );
		bs_write( s, 1, 0 ); // seq_scaling_matrix_present_flag
	}

	bs_write_ue( s, sps->i_log2_max_frame_num - 4 );
	bs_write_ue( s, sps->i_poc_type );
	if( sps->i_poc_type == 0 )
	{
		bs_write_ue( s, sps->i_log2_max_poc_lsb - 4 );
	}
	else if( sps->i_poc_type == 1 )
	{
		bs_write( s, 1, sps->b_delta_pic_order_always_zero );
		bs_write_se( s, sps->i_offset_for_non_ref_pic );
		bs_write_se( s, sps->i_offset_for_top_to_bottom_field );
		bs_write_ue( s, sps->i_num_ref_frames_in_poc_cycle );

		for( int i = 0; i < sps->i_num_ref_frames_in_poc_cycle; i++ )
			bs_write_se( s, sps->i_offset_for_ref_frame[i] );
	}
	bs_write_ue( s, sps->i_num_ref_frames );
	bs_write( s, 1, sps->b_gaps_in_frame_num_value_allowed );
	bs_write_ue( s, sps->i_mb_width - 1 );
	bs_write_ue( s, (sps->i_mb_height >> !sps->b_frame_mbs_only) - 1);
	bs_write( s, 1, sps->b_frame_mbs_only );
	if( !sps->b_frame_mbs_only )
		bs_write( s, 1, sps->b_mb_adaptive_frame_field );
	bs_write( s, 1, sps->b_direct8x8_inference );

	bs_write( s, 1, sps->b_crop );
	if( sps->b_crop )
	{
		bs_write_ue( s, sps->crop.i_left   / 2 );
		bs_write_ue( s, sps->crop.i_right  / 2 );
		bs_write_ue( s, sps->crop.i_top	/ 2 );
		bs_write_ue( s, sps->crop.i_bottom / 2 );
	}

	bs_write( s, 1, sps->b_vui );
	if( sps->b_vui )
	{
		bs_write1( s, sps->vui.b_aspect_ratio_info_present );
		if( sps->vui.b_aspect_ratio_info_present )
		{
			int i;
			static const struct { uint8_t w, h, sar; } sar[] =
			{
				{ 1,   1, 1 }, { 12, 11, 2 }, { 10, 11, 3 }, { 16, 11, 4 },
				{ 40, 33, 5 }, { 24, 11, 6 }, { 20, 11, 7 }, { 32, 11, 8 },
				{ 80, 33, 9 }, { 18, 11, 10}, { 15, 11, 11}, { 64, 33, 12},
				{ 160,99, 13}, { 0, 0, 255 }
			};
			for( i = 0; sar[i].sar != 255; i++ )
			{
				if( sar[i].w == sps->vui.i_sar_width &&
					sar[i].h == sps->vui.i_sar_height )
					break;
			}
			bs_write( s, 8, sar[i].sar );
			if( sar[i].sar == 255 ) /* aspect_ratio_idc (extended) */
			{
				bs_write( s, 16, sps->vui.i_sar_width );
				bs_write( s, 16, sps->vui.i_sar_height );
			}
		}

		bs_write1( s, sps->vui.b_overscan_info_present );
		if( sps->vui.b_overscan_info_present )
			bs_write1( s, sps->vui.b_overscan_info );

		bs_write1( s, sps->vui.b_signal_type_present );
		if( sps->vui.b_signal_type_present )
		{
			bs_write( s, 3, sps->vui.i_vidformat );
			bs_write1( s, sps->vui.b_fullrange );
			bs_write1( s, sps->vui.b_color_description_present );
			if( sps->vui.b_color_description_present )
			{
				bs_write( s, 8, sps->vui.i_colorprim );
				bs_write( s, 8, sps->vui.i_transfer );
				bs_write( s, 8, sps->vui.i_colmatrix );
			}
		}

		bs_write1( s, sps->vui.b_chroma_loc_info_present );
		if( sps->vui.b_chroma_loc_info_present )
		{
			bs_write_ue( s, sps->vui.i_chroma_loc_top );
			bs_write_ue( s, sps->vui.i_chroma_loc_bottom );
		}

		bs_write1( s, sps->vui.b_timing_info_present );
		if( sps->vui.b_timing_info_present )
		{
			bs_write32( s, sps->vui.i_num_units_in_tick );
			bs_write32( s, sps->vui.i_time_scale );
			bs_write1( s, sps->vui.b_fixed_frame_rate );
		}

		bs_write1( s, sps->vui.b_nal_hrd_parameters_present );
		if( sps->vui.b_nal_hrd_parameters_present )
		{
			bs_write_ue( s, sps->vui.hrd.i_cpb_cnt - 1 );
			bs_write( s, 4, sps->vui.hrd.i_bit_rate_scale );
			bs_write( s, 4, sps->vui.hrd.i_cpb_size_scale );

			bs_write_ue( s, sps->vui.hrd.i_bit_rate_value - 1 );
			bs_write_ue( s, sps->vui.hrd.i_cpb_size_value - 1 );

			bs_write1( s, sps->vui.hrd.b_cbr_hrd );

			bs_write( s, 5, sps->vui.hrd.i_initial_cpb_removal_delay_length - 1 );
			bs_write( s, 5, sps->vui.hrd.i_cpb_removal_delay_length - 1 );
			bs_write( s, 5, sps->vui.hrd.i_dpb_output_delay_length - 1 );
			bs_write( s, 5, sps->vui.hrd.i_time_offset_length );
		}

		bs_write1( s, sps->vui.b_vcl_hrd_parameters_present );

		if( sps->vui.b_nal_hrd_parameters_present || sps->vui.b_vcl_hrd_parameters_present )
			bs_write1( s, 0 );   /* low_delay_hrd_flag */

		bs_write1( s, sps->vui.b_pic_struct_present );
		bs_write1( s, sps->vui.b_bitstream_restriction );
		if( sps->vui.b_bitstream_restriction )
		{
			bs_write1( s, sps->vui.b_motion_vectors_over_pic_boundaries );
			bs_write_ue( s, sps->vui.i_max_bytes_per_pic_denom );
			bs_write_ue( s, sps->vui.i_max_bits_per_mb_denom );
			bs_write_ue( s, sps->vui.i_log2_max_mv_length_horizontal );
			bs_write_ue( s, sps->vui.i_log2_max_mv_length_vertical );
			bs_write_ue( s, sps->vui.i_num_reorder_frames );
			bs_write_ue( s, sps->vui.i_max_dec_frame_buffering );
		}
	}

	bs_rbsp_trailing( s );
	bs_flush( s );
}

void x264_pps_init(
	x264_pps_t *pps,
	int i_id,
	x264_sps_t const * const sps,
	VAPictureParameterBufferH264 const * const pic_param,
	VASliceParameterBufferH264 const * const slice_param,
	VAIQMatrixBufferH264 const * const iqmatrix
)
{
	pps->i_id = i_id;
	pps->i_sps_id = sps->i_id;
	pps->b_cabac = slice_param->cabac_init_idc ? 1 : 0;

	pps->b_pic_order = pic_param->pic_fields.bits.pic_order_present_flag;
	pps->i_num_slice_groups = 1;

	pps->i_num_ref_idx_l0_default_active = slice_param->num_ref_idx_l0_active_minus1 + 1;
	pps->i_num_ref_idx_l1_default_active = slice_param->num_ref_idx_l1_active_minus1 + 1;

	pps->b_weighted_pred = pic_param->pic_fields.bits.weighted_pred_flag;
	pps->b_weighted_bipred = pic_param->pic_fields.bits.weighted_bipred_idc ? 1 : 0;

	pps->i_pic_init_qp = pic_param->pic_init_qp_minus26 + 26;
	pps->i_pic_init_qs = pic_param->pic_init_qs_minus26 + 26;

	pps->i_chroma_qp_index_offset = pic_param->chroma_qp_index_offset;
	pps->b_deblocking_filter_control = 1;
	pps->b_constrained_intra_pred = pic_param->pic_fields.bits.constrained_intra_pred_flag;
	pps->b_redundant_pic_cnt = pic_param->pic_fields.bits.redundant_pic_cnt_present_flag;

	pps->b_transform_8x8_mode = pic_param->pic_fields.bits.transform_8x8_mode_flag;

	pps->scaling_list[0] = iqmatrix->ScalingList4x4[0];
	pps->scaling_list[1] = iqmatrix->ScalingList4x4[1];
	pps->scaling_list[2] = iqmatrix->ScalingList4x4[2];
	pps->scaling_list[3] = iqmatrix->ScalingList4x4[3];
	pps->scaling_list[4] = iqmatrix->ScalingList4x4[4];
	pps->scaling_list[5] = iqmatrix->ScalingList4x4[5];
	if (pic_param->pic_fields.bits.transform_8x8_mode_flag)
	{
		pps->scaling_list[4] = iqmatrix->ScalingList8x8[0];
		pps->scaling_list[5] = iqmatrix->ScalingList8x8[1];
	}
}

void x264_pps_write( bs_t *s, x264_pps_t *pps )
{
	bs_realign( s );

	bs_write( s, 32, 0x00000001 );					// delim
	bs_write( s, 8, (NAL_PRIORITY_HIGHEST << 5) | NAL_PPS );	// nal header

	bs_write_ue( s, pps->i_id );
	bs_write_ue( s, pps->i_sps_id );

	bs_write( s, 1, pps->b_cabac );
	bs_write( s, 1, pps->b_pic_order );
	bs_write_ue( s, pps->i_num_slice_groups - 1 );

	bs_write_ue( s, pps->i_num_ref_idx_l0_default_active - 1 );
	bs_write_ue( s, pps->i_num_ref_idx_l1_default_active - 1 );
	bs_write( s, 1, pps->b_weighted_pred );
	bs_write( s, 2, pps->b_weighted_bipred );

	bs_write_se( s, pps->i_pic_init_qp );
	bs_write_se( s, pps->i_pic_init_qs );
	bs_write_se( s, pps->i_chroma_qp_index_offset );

	bs_write( s, 1, pps->b_deblocking_filter_control );
	bs_write( s, 1, pps->b_constrained_intra_pred );
	bs_write( s, 1, pps->b_redundant_pic_cnt );

	/* write our cqm anyway */
	bs_write( s, 1, pps->b_transform_8x8_mode );
	bs_write( s, 1, 1 );
	scaling_list_write( s, pps, CQM_4IY );
	scaling_list_write( s, pps, CQM_4IC );
	bs_write( s, 1, 0 ); // Cr = Cb
	scaling_list_write( s, pps, CQM_4PY );
	scaling_list_write( s, pps, CQM_4PC );
	bs_write( s, 1, 0 ); // Cr = Cb
	if( pps->b_transform_8x8_mode )
	{
		scaling_list_write( s, pps, CQM_8IY+4 );
		scaling_list_write( s, pps, CQM_8PY+4 );
	}
	bs_write_se( s, pps->i_chroma_qp_index_offset );
	/*
	if( pps->b_transform_8x8_mode || pps->i_cqm_preset != X264_CQM_FLAT )
	{
		bs_write( s, 1, pps->b_transform_8x8_mode );
		bs_write( s, 1, (pps->i_cqm_preset != X264_CQM_FLAT) );
		if( pps->i_cqm_preset != X264_CQM_FLAT )
		{
			scaling_list_write( s, pps, CQM_4IY );
			scaling_list_write( s, pps, CQM_4IC );
			bs_write( s, 1, 0 ); // Cr = Cb
			scaling_list_write( s, pps, CQM_4PY );
			scaling_list_write( s, pps, CQM_4PC );
			bs_write( s, 1, 0 ); // Cr = Cb
			if( pps->b_transform_8x8_mode )
			{
				scaling_list_write( s, pps, CQM_8IY+4 );
				scaling_list_write( s, pps, CQM_8PY+4 );
			}
		}
		bs_write_se( s, pps->i_chroma_qp_index_offset );
	}
	*/

	bs_rbsp_trailing( s );
	bs_flush( s );
}
