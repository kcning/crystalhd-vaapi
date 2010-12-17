#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <vector>

const uint8_t ff_sqrt_tab[256]={
  0, 16, 23, 28, 32, 36, 40, 43, 46, 48, 51, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 77, 79, 80, 82, 84, 85, 87, 88, 90,
 91, 92, 94, 95, 96, 98, 99,100,102,103,104,105,107,108,109,110,111,112,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,144,145,146,147,148,149,150,151,151,152,153,154,155,156,156,
157,158,159,160,160,161,162,163,164,164,165,166,167,168,168,169,170,171,171,172,173,174,174,175,176,176,177,178,179,179,180,181,
182,182,183,184,184,185,186,186,187,188,188,189,190,190,191,192,192,193,194,194,195,196,196,197,198,198,199,200,200,201,202,202,
203,204,204,205,205,206,207,207,208,208,209,210,210,211,212,212,213,213,214,215,215,216,216,217,218,218,219,219,220,220,221,222,
222,223,223,224,224,225,226,226,227,227,228,228,229,230,230,231,231,232,232,233,233,234,235,235,236,236,237,237,238,238,239,239,
240,240,241,242,242,243,243,244,244,245,245,246,246,247,247,248,248,249,249,250,250,251,251,252,252,253,253,254,254,255,255,255
};

const uint8_t ff_log2_tab[256]={
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

const uint8_t av_reverse[256]={
0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF,
};

#include "get_bits.h"
#include "golomb.h"

#define ERROR(msg, args...) \
	do { fprintf(stderr, msg, ##args); exit(EXIT_FAILURE); } while (0)

enum NALU_T {
	NALU_UNKNOWN	= 0x00,
	NALU_SLICE	= 0x01,
	NALU_SLICE_DPA	= 0x02,
	NALU_SLICE_DPB	= 0x03,
	NALU_SLICE_DPC	= 0x04,
	NALU_SLICE_IDR	= 0x05,	/* NALU_PRIO != 0 */
	NALU_SEI	= 0x06, /* NALU_PRIO == 0 */
	NALU_SPS	= 0x07,
	NALU_PPS	= 0x08,
	NALU_AUD	= 0x09, /* NALU_PRIO == 0 */
	NALU_EOSEQ	= 0x0a, /* NALU_PRIO == 0 */
	NALU_EOS	= 0x0b, /* NALU_PRIO == 0 */
	NALU_FILLER	= 0x0c, /* NALU_PRIO == 0 */
	NALU_SPS_EXT	= 0x0d,
	NALU_PNAL	= 0x0e,
	NALU_SSPS	= 0x0f,
	/* 16 (0x10) ~ 18 (0x12) reserved */
	NALU_SLNP	= 0x13,
	NALU_SLE	= 0x14,
	/* 21 ~ 23 reserved */
	/* 24 ~ 31 unspecified */
};

enum NALU_PRIO_T {
	NALU_PRIO_DISPOSABLE	= 0x00,
	NALU_PRIO_LOW		= 0x01,
	NALU_PRIO_HIGH		= 0x02,
	NALU_PRIO_HIGHEST	= 0x03,
};

const char * const string_of_NALU_T(NALU_T t)
{
	switch (t)
	{
#define NALUT(type) case NALU_##type: return "NALU_" #type;
		NALUT(UNKNOWN);
		NALUT(SLICE);
		NALUT(SLICE_DPA);
		NALUT(SLICE_DPB);
		NALUT(SLICE_DPC);
		NALUT(SLICE_IDR);
		NALUT(SEI);
		NALUT(SPS);
		NALUT(PPS);
		NALUT(AUD);
		NALUT(EOSEQ);
		NALUT(EOS);
		NALUT(FILLER);
		NALUT(SPS_EXT);
		NALUT(PNAL);
		NALUT(SSPS);
		NALUT(SLNP);
		NALUT(SLE);
#undef NALUT
	}
	return "<unknown>";
}

const char * const string_of_NALU_PRIO_T(NALU_PRIO_T t)
{
	switch (t)
	{
#define NALUPRIOT(type) case NALU_PRIO_##type: return "NALU_PRIO_" #type;
		NALUPRIOT(DISPOSABLE);
		NALUPRIOT(LOW);
		NALUPRIOT(HIGH);
		NALUPRIOT(HIGHEST);
#undef NALUT
	}
	return "<unknown>";
}

enum PROFILE_T {
	PROFILE_CAVLC_444	= 44,
	PROFILE_BASELINE	= 66,
	PROFILE_MAIN		= 77,
	PROFILE_EXTENDED	= 88,
	PROFILE_HIGH		= 100,
	PROFILE_HIGH_10		= 110,
	PROFILE_HIGH_422	= 122,
	PROFILE_HIGH_444	= 244,
};

const char * const string_of_PROFILE_T(PROFILE_T t)
{
	switch (t)
	{
#define PROFILET(type) case PROFILE_##type: return "PROFILE_" #type;
		PROFILET(CAVLC_444);
		PROFILET(BASELINE);
		PROFILET(MAIN);
		PROFILET(EXTENDED);
		PROFILET(HIGH);
		PROFILET(HIGH_10);
		PROFILET(HIGH_422);
		PROFILET(HIGH_444);
#undef NALUT
	}
	return "<unknown>";
}
#define eprintf(name, fmt, args...) printf("  %-44s : " fmt "\n", name, ##args)

void scaling_list() {
	printf("!!! scaling_list() unimplemented !!!");
	exit(-127);
}

void decode_SPS(unsigned char **data, unsigned char *eos)
{
	GetBitContext gbc, *gb = &gbc;
	init_get_bits(gb, *data, (eos - *data) << 3);

	int tmp;

	eprintf("forbidden_zero_bit", "0x%02x", get_bits1(gb));
	tmp = get_bits(gb, 2);
	eprintf("nal_ref_idc", "0x%02x, %s", tmp, string_of_NALU_PRIO_T(NALU_PRIO_T(tmp)));
	tmp = get_bits(gb, 5);
	eprintf("nal_unit_type", "0x%02x, %s", tmp, string_of_NALU_T(NALU_T(tmp)));
	int profile_idc = get_bits(gb, 8);
	eprintf("profile_idc", "0x%02x, %s", profile_idc, string_of_PROFILE_T(PROFILE_T(profile_idc)));
	eprintf("constraint_set0_flag", "0x%02x", get_bits1(gb));
	eprintf("constraint_set1_flag", "0x%02x", get_bits1(gb));
	eprintf("constraint_set2_flag", "0x%02x", get_bits1(gb));
	eprintf("constraint_set3_flag", "0x%02x", get_bits1(gb));
	eprintf("constraint_set4_flag", "0x%02x", get_bits1(gb));
	eprintf("constraint_set5_flag", "0x%02x", get_bits1(gb));
	eprintf("reserved_zero_2bit", "0x%02x", get_bits(gb, 2));
	tmp = get_bits(gb, 8);
	eprintf("level_idc", "0x%02x (%d)", tmp, tmp);
	tmp = get_ue_golomb_31(gb);
	eprintf("sps_id", "0x%08x (%d)", tmp, tmp);
	if ( profile_idc == 44 || profile_idc == 83 || profile_idc == 86 ||
	     profile_idc == 100 || profile_idc == 110 || profile_idc == 118 ||
	     profile_idc == 122 || profile_idc == 128 || profile_idc == 244 )
	{
		int chroma_format_idc = get_ue_golomb_31(gb);
		eprintf("  chroma_format_idc", "0x%02x (%d)", chroma_format_idc, chroma_format_idc);
		if (tmp == 3) // chroma_format_idc == 3
			eprintf("    seperate_colour_plane_flag", "0x%02x", get_bits1(gb));
		tmp = get_ue_golomb_31(gb);
		eprintf("  bit_depth_luma_minus8", "0x%02x (%d)", tmp, tmp);
		tmp = get_ue_golomb_31(gb);
		eprintf("  bit_depth_chroma_minus8", "0x%02x (%d)", tmp, tmp);
		eprintf("  qpprime_y_zero_transform_bypass_flag", "0x%02x", get_bits1(gb));
		eprintf("  seq_scaling_matrix_present_flag", "0x%02x", tmp = get_bits1(gb));
		if ( tmp ) // seq_scaling_matrix_present_flag
		{
			for (int i = 0; i < ((chroma_format_idc != 3)?8:12); ++i)
			{
				eprintf("    seq_scaling_list_present_flag", "0x%02x", tmp = get_bits1(gb));
				if ( tmp ) // seq_scaling_list_present_flag
				{
					if (i < 6)
						scaling_list();
					else
						scaling_list();
				}
			}
		}
	}
	tmp = get_ue_golomb_31(gb);
	eprintf("log2_max_frame_num_minus4", "0x%04x (%d)", tmp, tmp);
	int pic_order_cnt_type = get_ue_golomb_31(gb);
	eprintf("pic_order_cnt_type", "0x%04x (%d)", pic_order_cnt_type, pic_order_cnt_type);
	if ( pic_order_cnt_type == 0 )
	{
		tmp = get_ue_golomb_31(gb);
		eprintf("  log2_max_pix_order_cnt_lsb_minus4", "0x%04x (%d)", tmp, tmp);
	}
	else if ( pic_order_cnt_type == 1 )
	{
		eprintf("  delta_pic_order_always_zero_flag", "0x%02x", get_bits1(gb));
		tmp = get_se_golomb(gb);
		eprintf("  offset_for_non_ref_pic", "0x%08x (%d)", tmp, tmp);
		tmp = get_se_golomb(gb);
		eprintf("  offset_for_top_to_bottom_field", "0x%08x (%d)", tmp, tmp);
		int num_ref_frames_in_pic_order_cnt_cycle = get_ue_golomb_31(gb);
		eprintf("  num_ref_frames_in_pic_order_cnt_cycle", "0x%08x (%d)",
			num_ref_frames_in_pic_order_cnt_cycle, num_ref_frames_in_pic_order_cnt_cycle);
		for (int i = 0;i < num_ref_frames_in_pic_order_cnt_cycle; ++i)
		{
			tmp = get_se_golomb(gb);
			eprintf("    offset_for_ref_frame[%d]", "0x%08x (%d)", i, tmp, tmp);
		}
	}
	tmp = get_ue_golomb_31(gb);
	eprintf("max_num_ref_frames", "0x%08x (%d)", tmp, tmp);
	eprintf("gaps_in_frame_num_value_allowed_flag", "0x%02x", get_bits1(gb));
	tmp = get_ue_golomb_31(gb);
	eprintf("pic_width_in_mbs_minus1", "0x%08x (%d)", tmp, tmp);
	tmp = get_ue_golomb_31(gb);
	eprintf("pic_height_in_map_units_minus1", "0x%08x (%d)", tmp, tmp);
	eprintf("frame_mbs_only_flag", "0x%02x", tmp = get_bits1(gb));
	if ( !tmp ) // frame_mbs_only_flag
		eprintf("  mb_adaptive_frame_field_flag", "0x%02x", get_bits1(gb));
	eprintf("direct_8x8_inference_flag", "0x%02x", get_bits1(gb));
	eprintf("frame_cropping_flag", "0x%02x", tmp = get_bits1(gb));
	if ( tmp ) // frame_cropping_flag
	{
		tmp = get_ue_golomb_31(gb);
		eprintf("  frame_crop_left_offset", "0x%08x (%d)", tmp, tmp);
		tmp = get_ue_golomb_31(gb);
		eprintf("  frame_crop_right_offset", "0x%08x (%d)", tmp, tmp);
		tmp = get_ue_golomb_31(gb);
		eprintf("  frame_crop_top_offset", "0x%08x (%d)", tmp, tmp);
		tmp = get_ue_golomb_31(gb);
		eprintf("  frame_crop_bottom_offset", "0x%08x (%d)", tmp, tmp);
	}
	eprintf("vui_parameters_present_flag", "0x%02x", tmp = get_bits1(gb));
	if ( tmp ) // vui_parameters_present_flag
	{
		// vui_parameters()
		eprintf("  aspect_ratio_info_present_flag", "0x%02x", tmp = get_bits1(gb));
		if ( tmp ) // aspect ratio info present flag
		{
			tmp = get_bits(gb, 8);
			eprintf("    aspect_ratio_idc", "0x%02x (%d)", tmp, tmp);
			if (tmp == 999 /*EXTENDED_SAR */)
			{
				tmp = get_bits(gb, 16);
				eprintf("      sar_width", "0x%04x (%d)", tmp, tmp);
				tmp = get_bits(gb, 16);
				eprintf("      sar_height", "0x%04x (%d)", tmp, tmp);
			}
		}
		eprintf("  overscan_info_present_flag", "0x%02x", tmp = get_bits1(gb));
		if ( tmp ) // overscan_info_present_flag
		{
			eprintf("    overscan_appropriate_flag", "0x%02x", get_bits1(gb));
		}
		eprintf("  video_signal_type_present_flag", "0x%02x", tmp = get_bits1(gb));
		if ( tmp ) // video_signal_type_present_flag
		{
			tmp = get_bits(gb, 3);
			eprintf("    video_format", "0x%02x (%d)", tmp, tmp);
			eprintf("    video_full_range_flag", "0x%02x", get_bits1(gb));
			eprintf("    colour_description_present_flag", "0x%02x", tmp = get_bits1(gb));
			if ( tmp ) // colour_description_present_flag
			{
				tmp = get_bits(gb, 8);
				eprintf("      colour_primaries", "0x%02x (%d)", tmp, tmp);
				tmp = get_bits(gb, 8);
				eprintf("      transfer_characteristics", "0x%02x (%d)", tmp, tmp);
				tmp = get_bits(gb, 8);
				eprintf("      matrix_coefficients", "0x%02x (%d)", tmp, tmp);
			}
		}
		eprintf("  chroma_loc_info_present_flag", "0x%02x", tmp = get_bits1(gb));
		if ( tmp ) // chroma_loc_info_present_flag
		{
			tmp = get_ue_golomb_31(gb);
			eprintf("    chroma_sample_loc_type_top_field", "0x%08x (%d)", tmp, tmp);
			tmp = get_ue_golomb_31(gb);
			eprintf("    chroma_sample_loc_type_bottom_field", "0x%08x (%d)", tmp, tmp);
		}
		eprintf("  timing_info_present_flag", "0x%02x", tmp = get_bits1(gb));
		if ( tmp ) // timing_info_present_flag
		{
			tmp = get_bits(gb, 32);
			eprintf("    num_units_in_tick", "0x%08x (%d)", tmp, tmp);
			tmp = get_bits(gb, 32);
			eprintf("    time_scale", "0x%08x (%d)", tmp, tmp);
			eprintf("    fixed_frame_rate_flag", "0x%02x", get_bits1(gb));
		}
		int nal_hdr_parameters_present_flag = get_bits1(gb);
		eprintf("  nal_hdr_parameters_present_flag", "0x%02x", nal_hdr_parameters_present_flag);
		if ( tmp ) // nal_hdr_parameters_present_flag
		{
			// hrd_parameters
			int cpb_cnt_minus1 = get_ue_golomb_31(gb);
			eprintf("    cpb_cnt_minus1", "0x%08x (%d)", cpb_cnt_minus1, cpb_cnt_minus1);
			tmp = get_bits(gb, 4);
			eprintf("    bit_rate_scale", "0x%08x (%d)", tmp, tmp);
			tmp = get_bits(gb, 4);
			eprintf("    cpb_size_scale", "0x%08x (%d)", tmp, tmp);
			for (int i = 0;i < cpb_cnt_minus1; ++i)
			{
				tmp = get_ue_golomb_31(gb);
				eprintf("      bit_rate_value_minus1[%d]", "0x%08x (%d)", i, tmp, tmp);
				tmp = get_ue_golomb_31(gb);
				eprintf("      cpb_size_value_minus1[%d]", "0x%08x (%d)", i, tmp, tmp);
				eprintf("      cpb_flag[%d]", "0x%02x", i, get_bits1(gb));
			}
			tmp = get_bits(gb, 5);
			eprintf("    initial_cpb_removal_delay_length_minus1", "0x%08x (%d)", tmp, tmp);
			tmp = get_bits(gb, 5);
			eprintf("    cpb_removal_delay_length_minus1", "0x%08x (%d)", tmp, tmp);
			tmp = get_bits(gb, 5);
			eprintf("    dpb_output_delay_length_minus1", "0x%08x (%d)", tmp, tmp);
			tmp = get_bits(gb, 5);
			eprintf("    time_offset_length", "0x%08x (%d)", tmp, tmp);
		}
		int vcl_hdr_parameters_present_flag = get_bits1(gb);
		eprintf("  vcl_hdr_parameters_present_flag", "0x%02x", vcl_hdr_parameters_present_flag);
		if ( tmp ) // vcl_hdr_parameters_present_flag
		{
			// hrd_parameters (same as above)
			int cpb_cnt_minus1 = get_ue_golomb_31(gb);
			eprintf("    cpb_cnt_minus1", "0x%08x (%d)", cpb_cnt_minus1, cpb_cnt_minus1);
			tmp = get_bits(gb, 4);
			eprintf("    bit_rate_scale", "0x%08x (%d)", tmp, tmp);
			tmp = get_bits(gb, 4);
			eprintf("    cpb_size_scale", "0x%08x (%d)", tmp, tmp);
			for (int i = 0;i < cpb_cnt_minus1; ++i)
			{
				tmp = get_ue_golomb_31(gb);
				eprintf("      bit_rate_value_minus1", "[i=%d] 0x%08x (%d)", i, tmp, tmp);
				tmp = get_ue_golomb_31(gb);
				eprintf("      cpb_size_value_minus1", "[i=%d] 0x%08x (%d)", i, tmp, tmp);
				eprintf("      cpb_flag", "[i=%d] 0x%02x", i, get_bits1(gb));
			}
			tmp = get_bits(gb, 5);
			eprintf("    initial_cpb_removal_delay_length_minus1", "0x%08x (%d)", tmp, tmp);
			tmp = get_bits(gb, 5);
			eprintf("    cpb_removal_delay_length_minus1", "0x%08x (%d)", tmp, tmp);
			tmp = get_bits(gb, 5);
			eprintf("    dpb_output_delay_length_minus1", "0x%08x (%d)", tmp, tmp);
			tmp = get_bits(gb, 5);
			eprintf("    time_offset_length", "0x%08x (%d)", tmp, tmp);
		}
		if ( nal_hdr_parameters_present_flag || vcl_hdr_parameters_present_flag )
		{
			eprintf("    low_delay_hrd_flag", "0x%02x", get_bits1(gb));
		}
		eprintf("  pic_struct_present_flag", "0x%02x", get_bits1(gb));
		eprintf("  bitstream_restriction_flag", "0x%02x", tmp = get_bits1(gb));
		if ( tmp ) // bitstream_restriction_flag
		{
			eprintf("    motion_vectors_over_pic_boundaries_flag", "0x%02x", get_bits1(gb));
			tmp = get_ue_golomb_31(gb);
			eprintf("    max_bytes_per_pic_denom", "0x%08x (%d)", tmp, tmp);
			tmp = get_ue_golomb_31(gb);
			eprintf("    max_bits_per_mb_denom", "0x%08x (%d)", tmp, tmp);
			tmp = get_ue_golomb_31(gb);
			eprintf("    log2_max_mv_length_horizontal", "0x%08x (%d)", tmp, tmp);
			tmp = get_ue_golomb_31(gb);
			eprintf("    log2_max_mv_length_vertical", "0x%08x (%d)", tmp, tmp);
			tmp = get_ue_golomb_31(gb);
			eprintf("    num_reorder_frames", "0x%08x (%d)", tmp, tmp);
			tmp = get_ue_golomb_31(gb);
			eprintf("    max_dec_frame_buffering", "0x%08x (%d)", tmp, tmp);
		}
	}
	*data = gbc.buffer_ptr;
	--*data;
}

void decode_PPS(unsigned char **data, unsigned char *eos)
{
}

void decode_SLICE_IDR(unsigned char **data, unsigned char *eos)
{
	// skip until we hit EOS or another startcode
	while (*data != eos)
	{
		int *n = reinterpret_cast<int *>(*data);
		if (*n == 0x01000000 || (*n & 0x00ffffff) == 0x00010000)
		{
			*data -= 4;
			break;
		}
		++*data;
	}
}

int main(int argc, char *argv[])
{
	unsigned char *addr;
	int fd;
	struct stat sb;

	if (argc != 2)
		ERROR("Usage: %s filename\n", argv[0]);

	fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		ERROR("open `%s' failed!\n", argv[1]);

	if (fstat(fd, &sb) == -1)	// obtain file size
		ERROR("fstat `%s' failed!\n", argv[1]);

	addr = static_cast<unsigned char *>(mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
	if (addr == MAP_FAILED)
		ERROR("mmap() failed!\n");

	// scan the buffer and escape
	//	00 00 03 00
	//	00 00 03 01
	//	00 00 03 02
	//	00 00 03 03
	std::vector<unsigned char> buf;
	buf.reserve(sb.st_size);
	unsigned char *i;
	for (i = addr;i < addr + sb.st_size; ++i)
	{
		int *n = reinterpret_cast<int *>(i);
		if (i < addr + sb.st_size - 4 && (*n == 0x00030000 || *n == 0x01030000 || *n == 0x02030000 || *n == 0x03030000))
		{
			buf.push_back(0x00);
			buf.push_back(0x00);
			i += 3;
		}
		buf.push_back(*i);
	}

	bool got_nalu = false;
	for (i = &(*buf.begin());i < &(*buf.end()); ++i)
	{
		int *n = reinterpret_cast<int *>(i);
		if (*n == 0x01000000)
		{
			i += 4;
			got_nalu = true;
		}
		if ((*n & 0x00ffffff) == 0x00010000)
		{
			i += 3;
			got_nalu = true;
		}
		if (got_nalu)
		{
			printf("\ngot NALU %s:\n", string_of_NALU_T(NALU_T(*i & 0x1f)));
			switch (*i & 0x1f)
			{
#define DECODE(nalu_type, data, eos) case NALU_##nalu_type: decode_##nalu_type(data, eos); break;
				DECODE(SPS, &i, &(*buf.end()));
				DECODE(PPS, &i, &(*buf.end()));
				DECODE(SLICE_IDR, &i, &(*buf.end()));
#undef DECODE
			}
			got_nalu = false;
		}
		printf("%02x ", *i);
	}
	puts("");

	exit(EXIT_SUCCESS);
}

