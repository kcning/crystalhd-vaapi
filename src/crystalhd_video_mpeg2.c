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
#include "crystalhd_video_mpeg2.h"

VAStatus crystalhd_prepare_decoder_mpeg2(
	VADriverContextP ctx,
	object_context_p obj_context
)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_SUCCESS;

	BC_INPUT_FORMAT bcInputFormat = {
		.FGTEnable	= FALSE,
		.MetaDataEnable	= FALSE,
		.Progressive	= TRUE,
		.OptFlags	= vdecFrameRate23_97, /* FIXME: Should we enable BD mode and max frame rate mode for LINK? */
		.mSubtype	= BC_MSUBTYPE_MPEG2VIDEO,
		.width		= obj_context->picture_width,
		.height		= obj_context->picture_height,
		.startCodeSz	= 3,
		.pMetaData	= obj_context->metadata,
		.metaDataSz	= obj_context->metadata_size,
		.bEnableScaling	= FALSE,
		/* we're not using HW Scaling so ScalingParams is irrelevant, but we still set it up here */
		.ScalingParams	= {
			.sWidth		= obj_context->picture_width,
			.sHeight	= obj_context->picture_height,
			.DNR		= 0, /* What's DNR? Do Not Resize??? */
			.Reserved1	= 0,
			.Reserved2	= NULL,
			.Reserved3	= 0,
			.Reserved4	= NULL,
		},
	};

	if ( BC_STS_SUCCESS != DtsOpenDecoder(driver_data->hdev, BC_STREAM_TYPE_ES) )
	{
		vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	if ( BC_STS_SUCCESS != DtsSetInputFormat(driver_data->hdev, &bcInputFormat) )
	{
		vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error_CloseDecoder;
	}

	if ( BC_STS_SUCCESS != DtsStartDecoder(driver_data->hdev) )
	{
		vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error_CloseDecoder;
	}

	if ( BC_STS_SUCCESS != DtsStartCapture(driver_data->hdev) )
	{
		vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error_StopDecoder;
	}

	return vaStatus;

error_StopDecoder:
	DtsStopDecoder(driver_data->hdev);

error_CloseDecoder:
	DtsCloseDecoder(driver_data->hdev);

error:
	return vaStatus;
}

VAStatus crystalhd_begin_picture_mpeg2(
		VADriverContextP ctx,
		VAContextID context,
		VASurfaceID render_target
	)
{
	INIT_DRIVER_DATA;
	object_context_p obj_context = CONTEXT(context);

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_render_iqmatrix_buffer_mpeg2(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

//	if (0 != obj_context->last_iqmatrix_buffer_id)
//		crystalhd_DestroyBuffer(ctx, obj_context->last_iqmatrix_buffer_id);

	obj_context->last_iqmatrix_buffer_id = obj_buffer->base.id;

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_render_picture_parameter_buffer_mpeg2(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

//	if (0 != obj_context->last_picture_param_buffer_id)
//		crystalhd_DestroyBuffer(ctx, obj_context->last_picture_param_buffer_id);

	obj_context->last_picture_param_buffer_id = obj_buffer->base.id;

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_render_slice_parameter_buffer_mpeg2(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

//	if (0 != obj_context->last_slice_param_buffer_id)
//		crystalhd_DestroyBuffer(ctx, obj_context->last_slice_param_buffer_id);

	obj_context->last_slice_param_buffer_id = obj_buffer->base.id;

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_render_slice_data_buffer_mpeg2(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	object_surface_p obj_surface = SURFACE(obj_context->current_render_target);
	int i;

	obj_surface->data = obj_buffer->buffer_data;
	obj_surface->data_size = obj_buffer->element_size * obj_buffer->num_elements;

	return VA_STATUS_SUCCESS;
}

enum MPEG2Startcode
{
	MPEG2_STARTCODE_PIC		= 0x00000100,
	MPEG2_STARTCODE_SEQ_HDR		= 0x000001b3,
	MPEG2_STARTCODE_EXT		= 0x000001b5,
	MPEG2_STARTCODE_EOSEQ		= 0x000001b7,
	MPEG2_STARTCODE_GROUP_OF_PIC	= 0x000001b8,
};

enum MPEG2ExtensionStartcodeIdentifier
{
	MPEG2_EXTID_SEQ_EXT		= 0x01,
	MPEG2_EXTID_SEQ_DISP_EXT	= 0x02,
	MPEG2_EXTID_PIC_CODING		= 0x08,
};

VAStatus crystalhd_end_picture_mpeg2(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_surface_p obj_surface
	)
{
	INIT_DRIVER_DATA;

	object_buffer_p iqmatrix_buf = BUFFER(obj_context->last_iqmatrix_buffer_id);
	object_buffer_p pic_param_buf = BUFFER(obj_context->last_picture_param_buffer_id);
	object_buffer_p slice_param_buf = BUFFER(obj_context->last_slice_param_buffer_id);

	VAIQMatrixBufferMPEG2 *iqmatrix = (VAIQMatrixBufferMPEG2 *)iqmatrix_buf->buffer_data;
	VAPictureParameterBufferMPEG2 *pic_param = (VAPictureParameterBufferMPEG2 *)pic_param_buf->buffer_data;
	VASliceParameterBufferMPEG2 *slice_param = (VASliceParameterBufferMPEG2 *)slice_param_buf->buffer_data;

	uint8_t metadata[250] = { 0 };
	bs_t bs, *s = &bs;
	bs_init( s, metadata, sizeof(metadata) / sizeof(*metadata) );

	// Video Metadata, things marked IGNORED is irrelevant (any value will work)

	// Picture Header
	bs_realign( s );
	bs_write32( s, MPEG2_STARTCODE_SEQ_HDR );						// startcode
	bs_write( s, 12, pic_param->horizontal_size & 0x0fff );					// horizontal_size_value
	bs_write( s, 12, pic_param->vertical_size & 0x0fff );					// vertical_size_value
	bs_write( s, 4, 0x00 );									// IGNORED: aspect_ratio_information
	bs_write( s, 4, 0x00 );									// IGNORED: frame_rate_code
	bs_write( s, 18, 0x000000 );								// IGNORED: bit_rate_value
	bs_write1( s, 1 );									// marker_bit (always 0x01)
	bs_write( s, 10, 0x0000 );								// IGNORED: vbv_buffer_size_value
	bs_write1( s, 0 );									// constrained_parameters_flag
	bs_write1( s, iqmatrix->load_intra_quantiser_matrix );					// load_intra_quantiser_matrix
	if ( iqmatrix->load_intra_quantiser_matrix )
		for (int i = 0;i < 64; ++i)							// intra_quantiser_matrix[64]
			bs_write( s, 8, iqmatrix->intra_quantiser_matrix[i] );
	bs_write1( s, iqmatrix->load_non_intra_quantiser_matrix );				// load_non_intra_quantiser_matrix
	if ( iqmatrix->load_non_intra_quantiser_matrix )
		for (int i = 0;i < 64; ++i)							// non_intra_quantiser_matrix[64]
			bs_write( s, 8, iqmatrix->non_intra_quantiser_matrix[i] );
	bs_flush( s );

	// Sequence Extension
	bs_write32( s, MPEG2_STARTCODE_EXT );							// startcode
	bs_write( s, 4, MPEG2_EXTID_SEQ_EXT );							// extension_startcode_identifier
	bs_write( s, 8, 0x00 );									// IGNORED: profile_and_level_indication
	bs_write1( s, pic_param->picture_coding_extension.bits.progressive_frame );		// progressive_sequence
	bs_write( s, 2, 0x00 );									// IGNORED: chroma_format
	bs_write( s, 2, (pic_param->horizontal_size >> 14) & 0x03 );				// horizontal_size_extension
	bs_write( s, 2, (pic_param->vertical_size >> 14) & 0x03 );				// vertical_size_extension
	bs_write( s, 12, 0x0000 );								// IGNORED: bit_rate_extension
	bs_write1( s, 0x01 );									// marker_bit (always 0x01)
	bs_write( s, 8, 0x00 );									// IGNORED: vbv_buffer_size_extension
	bs_write1( s, 0x00 );									// FIXME: low_delay
	bs_write( s, 2, 0x00 );									// IGNORED: frame_rate_extension_n
	bs_write( s, 5, 0x00 );									// IGNORED: frame_rate_extension_d
	bs_flush( s );

	// Sequence Display Extension
	bs_write32( s, MPEG2_STARTCODE_EXT );							// startcode
	bs_write( s, 4, MPEG2_EXTID_SEQ_DISP_EXT );						// extension_startcode_identifier
	bs_write( s, 3, 0x00 );									// IGNORED: video_format
	bs_write1( s, 0x01 );									// colour_description
	if ( 0x01 ) // colour_description
	{
		// 0x02 = Unspecified. see spec 6.3.6
		bs_write( s, 8, 0x02 );								// colour_primaries
		bs_write( s, 8, 0x02 );								// transfer_characteristics
		bs_write( s, 8, 0x02 );								// matrix_coefficients
	}
	bs_write( s, 14, pic_param->horizontal_size & 0x3fff );					// display_horizontal_size
	bs_write1( s, 0x01 );									// marker bit (always 0x01)
	bs_write( s, 14, pic_param->vertical_size & 0x3fff );					// display_vertical_size
	bs_flush( s );

	// Group of Picture Header
	bs_write32( s, MPEG2_STARTCODE_GROUP_OF_PIC );						// startcode
	bs_write( s, 25, 0x00001000 );								// IGNORED: time_code (bit 13 is the marker bit)
												//	    see 6.3.8
	bs_write1( s, 0x01 );									// FIXME: closed_gop
	bs_write1( s, 0x00 );									// FIXME: broken_link
	bs_flush( s );

	// Picture
	bs_write32( s, MPEG2_STARTCODE_PIC );							// startcode
	bs_write( s, 10, 0x00000 );								// FIXME: temporal_reference
	bs_write( s, 3, pic_param->picture_coding_type );					// picture_coding_type
	bs_write( s, 16, 0x00ffff );								// FIXME: vbv_delay
	if ( pic_param->picture_coding_type == 2 || pic_param->picture_coding_type == 3 )
	{
		bs_write1( s, 0x00 );								// FIXME: full_pel_forward_vector
		bs_write( s, 3, 0x00 );								// FIXME: forward_f_code
	}
	if ( pic_param->picture_coding_type == 3 )
	{
		bs_write1( s, 0x00 );								// FIXME: full_pel_backward_vector
		bs_write( s, 3, 0x00 );								// FIXME: backward_f_code
	}
	// FIXME:
	//while ( nextbits() == '1' ) {
	//	bs_write1( s, 0x01 );								// extra_bit_picture
	//	bs_write( s, 8, 0x00 );								// FIXME: extra_information_picture
	//}
	bs_write1( s, 0x00 );									// extra_bit_picture (always 0x00)
	bs_flush( s );

	// Picture Coding Extension
	bs_write32( s, MPEG2_STARTCODE_EXT );							// startcode
	bs_write( s, 4, MPEG2_EXTID_PIC_CODING );						// extension_startcode_identifier
	bs_write( s, 16, pic_param->f_code );							// f_code, packed all value in one int.
												// f_code[0][0] /* forward horizontal */
												// f_code[0][1] /* forward vertical */
												// f_code[1][0] /* backward horizontal */
												// f_code[1][1] /* backward vertical */
	bs_write( s, 2, pic_param->picture_coding_extension.bits.intra_dc_precision );		// intra_dc_precision
	bs_write( s, 2, pic_param->picture_coding_extension.bits.picture_structure );		// picture_structure
	bs_write1( s, pic_param->picture_coding_extension.bits.top_field_first );		// top_field_first
	bs_write1( s, pic_param->picture_coding_extension.bits.frame_pred_frame_dct );		// frame_pred_frame_dct
	bs_write1( s, pic_param->picture_coding_extension.bits.concealment_motion_vectors );	// concealment_motion_vectors
	bs_write1( s, pic_param->picture_coding_extension.bits.q_scale_type );			// q_scale_type
	bs_write1( s, pic_param->picture_coding_extension.bits.intra_vlc_format );		// intra_vlc_format
	bs_write1( s, pic_param->picture_coding_extension.bits.alternate_scan );		// alternate_scan
	bs_write1( s, pic_param->picture_coding_extension.bits.repeat_first_field );		// repeat_first_field
	bs_write1( s, pic_param->picture_coding_extension.bits.progressive_frame );		// chroma_420_type, shall equal to progressive_frame
												// see 6.3.10
	bs_write1( s, pic_param->picture_coding_extension.bits.progressive_frame );		// progressive_frame
	bs_write1( s, 0x00 );									// FIXME: composite_display_flag
	if ( 0x00 ) // composite_display_flag
	{
		bs_write1( s, 0x00 );								// FIXME: v_axis
		bs_write( s, 3, 0x00 );								// FIXME: field_sequence
		bs_write1( s, 0x00 );								// FIXME: sub_carrier
		bs_write( s, 7, 0x00 );								// FIXME: burst_amplitude
		bs_write( s, 8, 0x00 );								// FIXME: sub_carrier_phase
	}
	bs_flush( s );

	BC_STATUS sts;

	sts = DtsProcInput(driver_data->hdev, bs.p_start, bs.p - bs.p_start, 0, FALSE);
	if (sts != BC_STS_SUCCESS)
	{
		crystalhd__error_message("%s: Unable to send metadata. status = %d\n", __func__, sts);
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	sts = DtsProcInput(driver_data->hdev, obj_surface->data, obj_surface->data_size, 0, FALSE);
	if (sts != BC_STS_SUCCESS)
	{
		crystalhd__error_message("%s: Unable to send data. status = %d\n", __func__, sts);
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	uint8_t eoseq[] = { 0x00, 0x00, 0x01, 0xb7 };
	sts = DtsProcInput(driver_data->hdev, eoseq, sizeof(eoseq) / sizeof(*eoseq), 0, FALSE);
	if (sts != BC_STS_SUCCESS)
	{
		crystalhd__error_message("%s: Unable to send data. status = %d\n", __func__, sts);
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	// FIXME: TODO: free buffers here

	return VA_STATUS_SUCCESS;
}
