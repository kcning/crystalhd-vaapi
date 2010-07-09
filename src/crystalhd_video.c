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

#include "crystalhd_video.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define INIT_DRIVER_DATA	struct crystalhd_driver_data *driver_data = (struct crystalhd_driver_data *) ctx->pDriverData;

#define CONFIG(id)	((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id)	((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define SURFACE(id)	((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define BUFFER(id)	((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))
#define IMAGE(id)	((object_image_p) object_heap_lookup( &driver_data->image_heap, id ))

#define CONFIG_ID_OFFSET		0x01000000
#define CONTEXT_ID_OFFSET		0x02000000
#define SURFACE_ID_OFFSET		0x04000000
#define BUFFER_ID_OFFSET		0x08000000
#define IMAGE_ID_OFFSET			0x10000000

#define INSTRUMENT_CALL	crystalhd__information_message("%s (#%d): being called\n", __func__, __LINE__);
#define INSTRUMENT_RET	crystalhd__information_message("%s (#%d): returned\n", __func__, __LINE__);
#define INSTRUMENT_CHECKPOINT(n) crystalhd__information_message("%s (#%d): checkpoint %d\n", __func__, __LINE__, n);

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

static const h264_level_t h264_levels[] =
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

enum profile_e
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

#define CRYSTALHD_MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define CRYSTALHD_MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define CRYSTALHD_MAX3(a, b, c)	CRYSTALHD_MAX((a), CRYSTALHD_MAX((b), (c)))

#define DUMP_BUFFER(BUF, SIZE, FILENAME, ...) \
	do { \
		char dump_buf_file[200] = { 0 }; \
		sprintf(dump_buf_file, FILENAME, __VA_ARGS__); \
		FILE * dump_buf = fopen(dump_buf_file, "w"); \
		if (fwrite(BUF, SIZE, 1, dump_buf) < 0) \
			crystalhd__error_message("cannot dump buffer to %s\n", dump_buf_file); \
		fclose(dump_buf); \
	} while (0);

static void crystalhd__error_message(const char *msg, ...)
{
	va_list args;

	fprintf(stderr, "crystalhd_drv_video error: ");
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}

static void crystalhd__information_message(const char *msg, ...)
{
	va_list args;

	fprintf(stderr, "crystalhd_drv_video: ");
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}

VAStatus crystalhd_QueryConfigProfiles(
		VADriverContextP ctx,
		VAProfile *profile_list,	/* out */
		int *num_profiles		/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	int i = 0;

	//profile_list[i++] = VAProfileMPEG2Simple;
	//profile_list[i++] = VAProfileMPEG2Main;
	//profile_list[i++] = VAProfileMPEG4Simple;
	//profile_list[i++] = VAProfileMPEG4AdvancedSimple;
	//profile_list[i++] = VAProfileMPEG4Main;
	profile_list[i++] = VAProfileH264Baseline;
	profile_list[i++] = VAProfileH264Main;
	profile_list[i++] = VAProfileH264High;
	//profile_list[i++] = VAProfileVC1Simple;
	//profile_list[i++] = VAProfileVC1Main;
	//rofile_list[i++] = VAProfileVC1Advanced;
	//profile_list[i++] = VAProfileH263Baseline;
	//profile_list[i++] = VAProfileJPEGBaseline;

	/* If the assert fails then CRYSTALHD_MAX_PROFILES needs to be bigger */
	assert(i <= CRYSTALHD_MAX_PROFILES);
	*num_profiles = i;

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_QueryConfigEntrypoints(
		VADriverContextP ctx,
		VAProfile profile,
		VAEntrypoint  *entrypoint_list,	/* out */
		int *num_entrypoints		/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;

	switch (profile) {
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
				*num_entrypoints = 2;
				entrypoint_list[0] = VAEntrypointVLD;
				entrypoint_list[1] = VAEntrypointMoComp;
				break;

		case VAProfileMPEG4Simple:
		case VAProfileMPEG4AdvancedSimple:
		case VAProfileMPEG4Main:
				*num_entrypoints = 1;
				entrypoint_list[0] = VAEntrypointVLD;
				break;

		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
				*num_entrypoints = 1;
				entrypoint_list[0] = VAEntrypointVLD;
				break;

		case VAProfileVC1Simple:
		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
				*num_entrypoints = 1;
				entrypoint_list[0] = VAEntrypointVLD;
				break;

//		case VAProfileH263Baseline:
//				*num_entrypoints = 1;
//				entrypoint_list[0] = VAEntrypointVLD;
//				break;
//
//		case VAProfileJPEGBaseline:
//				*num_entrypoints = 1;
//				entrypoint_list[0] = VAEntrypointVLD;
//				break;

		default:
				*num_entrypoints = 0;
				break;
	}

	/* If the assert fails then CRYSTALHD_MAX_ENTRYPOINTS needs to be bigger */
	assert(*num_entrypoints <= CRYSTALHD_MAX_ENTRYPOINTS);
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_GetConfigAttributes(
		VADriverContextP ctx,
		VAProfile profile,
		VAEntrypoint entrypoint,
		VAConfigAttrib *attrib_list,	/* in/out */
		int num_attribs
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;

	int i;

	/* Other attributes don't seem to be defined */
	/* What to do if we don't know the attribute? */
	for (i = 0; i < num_attribs; i++)
	{
		switch (attrib_list[i].type)
		{
		  case VAConfigAttribRTFormat:
			  attrib_list[i].value = VA_RT_FORMAT_YUV420;
			  break;

		  default:
			  /* Do nothing */
			  attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
			  break;
		}
	}

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

static VAStatus crystalhd__update_attribute(object_config_p obj_config, VAConfigAttrib *attrib)
{
	int i;
	/* Check existing attrbiutes */
	for(i = 0; obj_config->attrib_count < i; i++)
	{
		if (obj_config->attrib_list[i].type == attrib->type)
		{
			/* Update existing attribute */
			obj_config->attrib_list[i].value = attrib->value;
			INSTRUMENT_RET;
			return VA_STATUS_SUCCESS;
		}
	}
	if (obj_config->attrib_count < CRYSTALHD_MAX_CONFIG_ATTRIBUTES)
	{
		i = obj_config->attrib_count;
		obj_config->attrib_list[i].type = attrib->type;
		obj_config->attrib_list[i].value = attrib->value;
		obj_config->attrib_count++;
		INSTRUMENT_RET;
		return VA_STATUS_SUCCESS;
	}
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
}

VAStatus crystalhd_CreateConfig(
		VADriverContextP ctx,
		VAProfile profile,
		VAEntrypoint entrypoint,
		VAConfigAttrib *attrib_list,
		int num_attribs,
		VAConfigID *config_id		/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus;
	int configID;
	object_config_p obj_config;
	int i;

	/* Validate profile & entrypoint */
	switch (profile) {
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
				if ((VAEntrypointVLD == entrypoint) ||
					(VAEntrypointMoComp == entrypoint))
				{
					vaStatus = VA_STATUS_SUCCESS;
				}
				else
				{
					vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
				}
				break;

		case VAProfileMPEG4Simple:
		case VAProfileMPEG4AdvancedSimple:
		case VAProfileMPEG4Main:
				if (VAEntrypointVLD == entrypoint)
				{
					vaStatus = VA_STATUS_SUCCESS;
				}
				else
				{
					vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
				}
				break;

		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
				if (VAEntrypointVLD == entrypoint)
				{
					vaStatus = VA_STATUS_SUCCESS;
				}
				else
				{
					vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
				}
				break;

		case VAProfileVC1Simple:
		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
				if (VAEntrypointVLD == entrypoint)
				{
					vaStatus = VA_STATUS_SUCCESS;
				}
				else
				{
					vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
				}
				break;

		default:
				vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
				break;
	}

	if (VA_STATUS_SUCCESS != vaStatus)
	{
		INSTRUMENT_RET;
		return vaStatus;
	}

	configID = object_heap_allocate( &driver_data->config_heap );
	obj_config = CONFIG(configID);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		INSTRUMENT_RET;
		return vaStatus;
	}

	obj_config->profile = profile;
	obj_config->entrypoint = entrypoint;
	obj_config->attrib_list[0].type = VAConfigAttribRTFormat;
	obj_config->attrib_list[0].value = VA_RT_FORMAT_YUV420;
	obj_config->attrib_count = 1;

	for(i = 0; i < num_attribs; i++)
	{
		vaStatus = crystalhd__update_attribute(obj_config, &(attrib_list[i]));
		if (VA_STATUS_SUCCESS != vaStatus)
		{
			break;
		}
	}

	/* Error recovery */
	if (VA_STATUS_SUCCESS != vaStatus)
	{
		object_heap_free( &driver_data->config_heap, (object_base_p) obj_config);
	}
	else
	{
		*config_id = configID;
	}

	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_DestroyConfig(
		VADriverContextP ctx,
		VAConfigID config_id
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus;
	object_config_p obj_config;

	obj_config = CONFIG(config_id);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
		INSTRUMENT_RET;
		return vaStatus;
	}

	object_heap_free( &driver_data->config_heap, (object_base_p) obj_config);
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_QueryConfigAttributes(
		VADriverContextP ctx,
		VAConfigID config_id,
		VAProfile *profile,		/* out */
		VAEntrypoint *entrypoint, 	/* out */
		VAConfigAttrib *attrib_list,	/* out */
		int *num_attribs		/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_config_p obj_config;
	int i;

	obj_config = CONFIG(config_id);
	assert(obj_config);

	*profile = obj_config->profile;
	*entrypoint = obj_config->entrypoint;
	*num_attribs =  obj_config->attrib_count;
	for(i = 0; i < obj_config->attrib_count; i++)
	{
		attrib_list[i] = obj_config->attrib_list[i];
	}

	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_CreateSurfaces(
		VADriverContextP ctx,
		int width,
		int height,
		int format,
		int num_surfaces,
		VASurfaceID *surfaces		/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	int i;

	/* We only support one format */
	if (VA_RT_FORMAT_YUV420 != format)
	{
		INSTRUMENT_RET;
		return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
	}

	for (i = 0; i < num_surfaces; i++)
	{
		int surfaceID = object_heap_allocate( &driver_data->surface_heap );
		object_surface_p obj_surface = SURFACE(surfaceID);
		if (NULL == obj_surface)
		{
			vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
			break;
		}
		obj_surface->surface_id = surfaceID;
		obj_surface->metadata = NULL;
		obj_surface->data = NULL;
		surfaces[i] = surfaceID;
	}

	/* Error recovery */
	if (VA_STATUS_SUCCESS != vaStatus)
	{
		/* surfaces[i-1] was the last successful allocation */
		for(; i--; )
		{
			object_surface_p obj_surface = SURFACE(surfaces[i]);
			surfaces[i] = VA_INVALID_SURFACE;
			assert(obj_surface);
			object_heap_free( &driver_data->surface_heap, (object_base_p) obj_surface);
		}
	}

	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_DestroySurfaces(
		VADriverContextP ctx,
		VASurfaceID *surface_list,
		int num_surfaces
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	int i;
	for(i = num_surfaces; i--; )
	{
		object_surface_p obj_surface = SURFACE(surface_list[i]);
		if (!obj_surface)
			continue;

		if (obj_surface->metadata)
			free(obj_surface->metadata);
		if (obj_surface->data)
			free(obj_surface->data);

		object_heap_free( &driver_data->surface_heap, (object_base_p) obj_surface);
	}
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

enum {
	CRYSTALHD_SURFACETYPE_RGBA = 1,
	CRYSTALHD_SURFACETYPE_YUV,
	CRYSTALHD_SURFACETYPE_INDEXED,
};

typedef struct {
	unsigned int type;
	VAImageFormat va_format;
} crystalhd_image_format_map_t;

static const crystalhd_image_format_map_t
crystalhd_image_formats_map[CRYSTALHD_MAX_IMAGE_FORMATS + 1] = {
#if 0
	/* TODO: support these image formats... and maybe more */
	{ CRYSTALHD_SURFACETYPE_YUV,
	  { VA_FOURCC('Y','V','1','2'), VA_LSB_FIRST, 12, } },
	{ CRYSTALHD_SURFACETYPE_YUV,
	  { VA_FOURCC('I','4','2','0'), VA_LSB_FIRST, 12, } },
#endif
	{ CRYSTALHD_SURFACETYPE_YUV,
	  { VA_FOURCC('N','V','1','2'), VA_LSB_FIRST, 12, } },
};

VAStatus crystalhd_QueryImageFormats(
	VADriverContextP ctx,
	VAImageFormat *format_list,	/* out */
	int *num_formats		/* out */
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	int n;
	for (n = 0; crystalhd_image_formats_map[n].va_format.fourcc != 0; ++n) {
		const crystalhd_image_format_map_t * const m = &crystalhd_image_formats_map[n];
		if (format_list)
			format_list[n] = m->va_format;
	}

	if (num_formats)
		*num_formats = n;

	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_CreateImage(
	VADriverContextP ctx,
	VAImageFormat *format,
	int width,
	int height,
	VAImage *out_image		/* out */
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	object_image_p obj_image;
	VAImageID image_id;
	unsigned int width2, height2, size2, size;
	
	out_image->image_id = VA_INVALID_ID;
	out_image->buf = VA_INVALID_ID;

	image_id = object_heap_allocate(&driver_data->image_heap);
	if (image_id == VA_INVALID_ID)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	obj_image = IMAGE(image_id);
	if (!obj_image)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;
	obj_image->palette = NULL;

	VAImage * const image = &obj_image->image;
	image->image_id = image_id;
	image->buf = VA_INVALID_ID;

	size = width * height;
	width2 = (width + 1) / 2;
	height2 = (height + 1) / 2;
	size2 = width2 * height2;

	image->num_palette_entries = 0;
	image->entry_bytes = 0;
	memset(image->component_order, 0, sizeof(image->component_order));

	switch (format->fourcc) {
#if 0
		case VA_FOURCC('I','A','4','4'):
		case VA_FOURCC('A','I','4','4'):
			image->num_planes = 1;
			image->pitches[0] = width;
			image->offsets[0] = 0;
			image->data_size = image->offsets[0] + image->pitches[0] * height;
			image->num_palette_entries = 16;
			image->entry_bytes = 3;
			image->component_order[0] = 'R';
			image->component_order[1] = 'G';
			image->component_order[2] = 'B';
			break;
		case VA_FOURCC('A','R','G','B'):
		case VA_FOURCC('A','B','G','R'):
		case VA_FOURCC('B','G','R','A'):
		case VA_FOURCC('R','G','B','A'):
			image->num_planes = 1;
			image->pitches[0] = width * 4;
			image->offsets[0] = 0;
			image->data_size = image->offsets[0] + image->pitches[0] * height;
			break;
		case VA_FOURCC('Y','V','1','2'):
			image->num_planes = 3;
			image->pitches[0] = width;
			image->offsets[0] = 0;
			image->pitches[1] = width2;
			image->offsets[1] = size + size2;
			image->pitches[2] = width2;
			image->offsets[2] = size;
			image->data_size = size + 2 * size2;
			break;
		case VA_FOURCC('I','4','2','0'):
			image->num_planes = 3;
			image->pitches[0] = width;
			image->offsets[0] = 0;
			image->pitches[1] = width2;
			image->offsets[1] = size;
			image->pitches[2] = width2;
			image->offsets[2] = size + size2;
			image->data_size = size + 2 * size2;
			break;
#endif
		case VA_FOURCC('N','V','1','2'):
			image->num_planes = 2;
			image->pitches[0] = width;
			image->offsets[0] = 0;
			image->pitches[1] = width;
			image->offsets[1] = size;
			image->data_size = size + 2 * size2;
			break;
		default:
			goto error;
	};

	if (crystalhd_CreateBuffer(ctx, 0, VAImageBufferType,
		image->data_size, 1, NULL, &image->buf) != VA_STATUS_SUCCESS)
		goto error;

	if (image->num_palette_entries > 0 && image->entry_bytes > 0) {
		obj_image->palette = malloc(image->num_palette_entries * sizeof(obj_image->palette));
		if (!obj_image->palette)
			goto error;
	}

	image->image_id = image_id;
	image->format = *format;
	image->width = width;
	image->height = height;

	*out_image = *image;

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;

error:
	crystalhd_DestroyImage(ctx, image_id);
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_OPERATION_FAILED;
}

VAStatus crystalhd_DeriveImage(
	VADriverContextP ctx,
	VASurfaceID surface,
	VAImage *image			/* out */
)
{
	/* TODO */
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus crystalhd_DestroyImage(
	VADriverContextP ctx,
	VAImageID image
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;

	object_image_p obj_image = IMAGE(image);
	if (obj_image)
	{
		if (obj_image->image.buf != VA_INVALID_ID)
			crystalhd_DestroyBuffer(ctx, obj_image->image.buf);

		if (obj_image->palette)
			free(obj_image->palette);

		object_heap_free(&driver_data->image_heap, (object_base_p)obj_image);
	}

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_SetImagePalette(
	VADriverContextP ctx,
	VAImageID image,
	unsigned char *palette
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_GetImage(
	VADriverContextP ctx,
	VASurfaceID surface,
	int x,			/* coordinates of the upper left source pixel */
	int y,
	unsigned int width,	/* width and height of the region */
	unsigned int height,
	VAImageID image
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;

	object_surface_p obj_surface = SURFACE(surface);
	if (NULL == obj_surface)
	{
		INSTRUMENT_RET;
		return VA_STATUS_ERROR_INVALID_SURFACE;
	}

	object_image_p obj_image = IMAGE(image);
	if (NULL == obj_image)
	{
		INSTRUMENT_RET;
		return VA_STATUS_ERROR_INVALID_IMAGE;
	}

	object_image_p obj_output_image = IMAGE(obj_surface->output_image_id);

	obj_image->image.format = obj_output_image->image.format;
	obj_image->image.width = obj_output_image->image.width;
	obj_image->image.height = obj_output_image->image.height;
	obj_image->image.num_planes = obj_output_image->image.num_planes;
	obj_image->image.entry_bytes = obj_output_image->image.entry_bytes;
	obj_image->image.pitches[0] = obj_output_image->image.pitches[0];
	obj_image->image.pitches[1] = obj_output_image->image.pitches[1];
	obj_image->image.pitches[2] = obj_output_image->image.pitches[2];
	obj_image->image.offsets[0] = obj_output_image->image.offsets[0];
	obj_image->image.offsets[1] = obj_output_image->image.offsets[1];
	obj_image->image.offsets[2] = obj_output_image->image.offsets[2];
	obj_image->image.component_order[0] = obj_output_image->image.component_order[0];
	obj_image->image.component_order[1] = obj_output_image->image.component_order[1];
	obj_image->image.component_order[2] = obj_output_image->image.component_order[2];
	obj_image->image.component_order[3] = obj_output_image->image.component_order[3];

	memcpy(	BUFFER(obj_image->image.buf)->buffer_data,
		BUFFER(obj_output_image->image.buf)->buffer_data,
		CRYSTALHD_MIN(obj_image->image.data_size, obj_output_image->image.data_size));
	obj_image->image.data_size = CRYSTALHD_MIN(obj_image->image.data_size, obj_output_image->image.data_size);
	
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_PutImage(
	VADriverContextP ctx,
	VASurfaceID surface,
	VAImageID image,
	int src_x,
	int src_y,
	unsigned int src_width,
	unsigned int src_height,
	int dest_x,
	int dest_y,
	unsigned int dest_width,
	unsigned int dest_height
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_QuerySubpictureFormats(
	VADriverContextP ctx,
	VAImageFormat *format_list,	/* out */
	unsigned int *flags,		/* out */
	unsigned int *num_formats	/* out */
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_CreateSubpicture(
	VADriverContextP ctx,
	VAImageID image,
	VASubpictureID *subpicture	/* out */
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_DestroySubpicture(
	VADriverContextP ctx,
	VASubpictureID subpicture
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_SetSubpictureImage(
		VADriverContextP ctx,
		VASubpictureID subpicture,
		VAImageID image
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_SetSubpicturePalette(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	/*
	 * pointer to an array holding the palette data.  The size of the array is
	 * num_palette_entries * entry_bytes in size.  The order of the components
	 * in the palette is described by the component_order in VASubpicture struct
	 */
	unsigned char *palette
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_SetSubpictureChromakey(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	unsigned int chromakey_min,
	unsigned int chromakey_max,
	unsigned int chromakey_mask
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_SetSubpictureGlobalAlpha(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	float global_alpha 
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}


VAStatus crystalhd_AssociateSubpicture(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	VASurfaceID *target_surfaces,
	int num_surfaces,
	short src_x, /* upper left offset in subpicture */
	short src_y,
	unsigned short src_width,
	unsigned short src_height,
	short dest_x, /* upper left offset in surface */
	short dest_y,
	unsigned short dest_width,
	unsigned short dest_height,
	/*
	 * whether to enable chroma-keying or global-alpha
	 * see VA_SUBPICTURE_XXX values
	 */
	unsigned int flags
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus crystalhd_DeassociateSubpicture(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	VASurfaceID *target_surfaces,
	int num_surfaces
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus crystalhd_CreateContext(
		VADriverContextP ctx,
		VAConfigID config_id,
		int picture_width,
		int picture_height,
		int flag,
		VASurfaceID *render_targets,
		int num_render_targets,
		VAContextID *context		/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_config_p obj_config;
	int i;
	BC_STATUS sts;

	obj_config = CONFIG(config_id);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
		INSTRUMENT_RET;
		return vaStatus;
	}

	/* Validate flag */
	/* Validate picture dimensions */
	crystalhd__information_message("%s: image size = %d x %d\n", __func__, picture_width, picture_height);

	int contextID = object_heap_allocate( &driver_data->context_heap );
	object_context_p obj_context = CONTEXT(contextID);
	if (NULL == obj_context)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		INSTRUMENT_RET;
		return vaStatus;
	}

	obj_context->context_id  = contextID;
	*context = contextID;
	obj_context->current_render_target = -1;
	obj_context->config_id = config_id;
	obj_context->picture_width = picture_width;
	obj_context->picture_height = picture_height;
	obj_context->num_render_targets = num_render_targets;
	obj_context->render_targets = (VASurfaceID *) malloc(num_render_targets * sizeof(VASurfaceID));
	if (obj_context->render_targets == NULL)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		INSTRUMENT_RET;
		return vaStatus;
	}
	
	for(i = 0; i < num_render_targets; i++)
	{
		if (NULL == SURFACE(render_targets[i]))
		{
			vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
			break;
		}
		obj_context->render_targets[i] = render_targets[i];
	}
	obj_context->flags = flag;

	/* CrystalHD specific */
	sts = DtsOpenDecoder(driver_data->hdev, BC_STREAM_TYPE_ES);
	assert( sts == BC_STS_SUCCESS );

	enum _DtsSetVideoParamsAlgo bc_algo;

	switch (obj_config->profile) {
		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
			bc_algo = BC_VID_ALGO_H264;
			break;

		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			bc_algo = BC_VID_ALGO_MPEG2;
			break;

		case VAProfileVC1Simple:
			bc_algo = BC_VID_ALGO_VC1;
			break;

		//case VAProfileDivx???:
		//	bc_algo = BC_VID_ALGO_DIVX;
		//	break;

		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
			bc_algo = BC_VID_ALGO_VC1MP;
			break;

		default:
			INSTRUMENT_RET;
			return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}
	sts = DtsSetVideoParams(driver_data->hdev, bc_algo, TRUE, FALSE, TRUE, 0);
	assert( sts == BC_STS_SUCCESS );

	sts = DtsStartDecoder(driver_data->hdev);
	assert( sts == BC_STS_SUCCESS );

	sts = DtsStartCapture(driver_data->hdev);
	assert( sts == BC_STS_SUCCESS );

	/* Error recovery */
	if (VA_STATUS_SUCCESS != vaStatus)
	{
		obj_context->context_id = -1;
		obj_context->config_id = -1;
		free(obj_context->render_targets);
		obj_context->render_targets = NULL;
		obj_context->num_render_targets = 0;
		obj_context->flags = 0;
		object_heap_free( &driver_data->context_heap, (object_base_p) obj_context);
	}

	INSTRUMENT_RET;
	return vaStatus;
}


VAStatus crystalhd_DestroyContext(
		VADriverContextP ctx,
		VAContextID context
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	object_context_p obj_context = CONTEXT(context);
	assert(obj_context);

	obj_context->context_id = -1;
	obj_context->config_id = -1;
	obj_context->picture_width = 0;
	obj_context->picture_height = 0;
	if (obj_context->render_targets)
	{
		free(obj_context->render_targets);
	}
	obj_context->render_targets = NULL;
	obj_context->num_render_targets = 0;
	obj_context->flags = 0;

	obj_context->current_render_target = -1;

	/* CrystalHD specific */
	DtsStopDecoder(driver_data->hdev);
	DtsCloseDecoder(driver_data->hdev);

	object_heap_free( &driver_data->context_heap, (object_base_p) obj_context);

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}



static VAStatus crystalhd__allocate_buffer(object_buffer_p obj_buffer, int size)
{
	obj_buffer->buffer_data = realloc(obj_buffer->buffer_data, size);
	if (NULL == obj_buffer->buffer_data)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_CreateBuffer(
		VADriverContextP ctx,
		VAContextID context,		/* in */
		VABufferType type,		/* in */
		unsigned int size,		/* in */
		unsigned int num_elements,	/* in */
		void *data,			/* in */
		VABufferID *buf_id		/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	int bufferID;
	object_buffer_p obj_buffer;

	/* Validate type */
	switch (type)
	{
		case VAPictureParameterBufferType:
		case VAIQMatrixBufferType:
		case VABitPlaneBufferType:
		case VASliceGroupMapBufferType:
		case VASliceParameterBufferType:
		case VASliceDataBufferType:
		case VAMacroblockParameterBufferType:
		case VAResidualDataBufferType:
		case VADeblockingParameterBufferType:
		case VAImageBufferType:
			/* Ok */
			break;
		default:
			INSTRUMENT_CHECKPOINT(type);
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
			INSTRUMENT_RET;
			return vaStatus;
	}

	bufferID = object_heap_allocate( &driver_data->buffer_heap );
	obj_buffer = BUFFER(bufferID);
	crystalhd__information_message("buffer_id = %d (0x%08x), obj_buffer = 0x%08x, obj_buffer->base.id = 0x%08x, BufferType: %d, size: %d * %d = %d\n",
			bufferID, bufferID, obj_buffer, obj_buffer->base.id, type, size, num_elements, size * num_elements);
	if (NULL == obj_buffer)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		INSTRUMENT_RET;
		return vaStatus;
	}

	obj_buffer->type = type;
	obj_buffer->buffer_data = NULL;

	vaStatus = crystalhd__allocate_buffer(obj_buffer, size * num_elements);
	if (VA_STATUS_SUCCESS == vaStatus)
	{
		obj_buffer->element_size = size;
		obj_buffer->num_elements = num_elements;
		obj_buffer->max_num_elements = num_elements;
		if (data)
		{
			memcpy(obj_buffer->buffer_data, data, size * num_elements);
		}
		*buf_id = bufferID;
	}

	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_BufferSetNumElements(
		VADriverContextP ctx,
		VABufferID buf_id,		/* in */
		unsigned int num_elements	/* in */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_buffer_p obj_buffer = BUFFER(buf_id);
	assert(obj_buffer);

	if ((num_elements < 0) || (num_elements > obj_buffer->max_num_elements))
	{
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
	}
	if (VA_STATUS_SUCCESS == vaStatus)
	{
		obj_buffer->num_elements = num_elements;
	}

	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_MapBuffer(
		VADriverContextP ctx,
		VABufferID buf_id,	/* in */
		void **pbuf		/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	object_buffer_p obj_buffer = BUFFER(buf_id);
	assert(obj_buffer);
	if (NULL == obj_buffer)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_BUFFER;
		INSTRUMENT_RET;
		return vaStatus;
	}

	if (NULL != obj_buffer->buffer_data)
	{
		*pbuf = obj_buffer->buffer_data;
		vaStatus = VA_STATUS_SUCCESS;
	}
	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_UnmapBuffer(
		VADriverContextP ctx,
		VABufferID buf_id	/* in */
	)
{
	/* Do nothing */
	return VA_STATUS_SUCCESS;
}

static void crystalhd__destroy_buffer(struct crystalhd_driver_data *driver_data, object_buffer_p obj_buffer)
{
	INSTRUMENT_CALL;
	if (NULL != obj_buffer->buffer_data)
	{
		free(obj_buffer->buffer_data);
		obj_buffer->buffer_data = NULL;
	}

	object_heap_free( &driver_data->buffer_heap, (object_base_p) obj_buffer);
	INSTRUMENT_RET;
}

VAStatus crystalhd_DestroyBuffer(
		VADriverContextP ctx,
		VABufferID buffer_id
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	object_buffer_p obj_buffer = BUFFER(buffer_id);
	crystalhd__information_message("buffer_id = %d (0x%08x), obj_buffer = 0x%08x\n", buffer_id, buffer_id, obj_buffer);
	//assert(obj_buffer);

	if (obj_buffer)
		crystalhd__destroy_buffer(driver_data, obj_buffer);

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_BeginPicture(
		VADriverContextP ctx,
		VAContextID context,
		VASurfaceID render_target
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_context_p obj_context = CONTEXT(context);
	object_surface_p obj_surface = SURFACE(render_target);

	assert(obj_context);
	assert(obj_surface);

	obj_context->current_render_target = obj_surface->base.id;

	INSTRUMENT_RET;
	return vaStatus;
}

#include "bitstream.h"
enum NALU
{
	NAL_DISPOSABLE_EOSEQ	= 0x0a,
	NAL_HIGHEST_SLICE_IDR	= 0x65,
	NAL_HIGHEST_SPS		= 0x67,
	NAL_HIGHEST_PPS		= 0x68,
};

void print_buffer(uint8_t *data, uint32_t size)
{
	unsigned int i = 0;
	printf("printing buffer with size 0x%x\n", size);
	for (i = 0;i < size; ++i)
		printf("0x%02x ", data[i]);
	printf("\n=====\n");
}

/* FIXME: dirty hack of global variable */
object_buffer_p buffered_picture_parameter_buffer;

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

VAStatus crystalhd_render_picture_parameter_buffer_mpeg2(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_render_picture_parameter_buffer_vc1(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_render_picture_parameter_buffer_vc1mp(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_render_picture_parameter_buffer(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	object_config_p obj_config = CONFIG(obj_context->config_id);

	switch (obj_config->profile) {
		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
			vaStatus = crystalhd_render_picture_parameter_buffer_h264(ctx, obj_context, obj_buffer);
			break;

		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			vaStatus = crystalhd_render_picture_parameter_buffer_mpeg2(ctx, obj_context, obj_buffer);
			break;

		case VAProfileVC1Simple:
			vaStatus = crystalhd_render_picture_parameter_buffer_vc1(ctx, obj_context, obj_buffer);
			break;

		//case VAProfileDivx???:
		//	vaStatus = crystalhd_render_picture_parameter_buffer_divx(ctx, obj_context, obj_buffer);
		//	break;

		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
			vaStatus = crystalhd_render_picture_parameter_buffer_vc1mp(ctx, obj_context, obj_buffer);
			break;

		default:
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}

	INSTRUMENT_RET;
	return vaStatus;
}

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

	if (l->level_idc)
		++l;

	return l->level_idc;
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

#if 0
VAStatus crystalhd_render_slice_parameter_buffer_mpeg2(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_render_slice_parameter_buffer_vc1(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_render_slice_parameter_buffer_vc1mp(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	INSTRUMENT_RET;
	return vaStatus;
}
#endif

VAStatus crystalhd_render_slice_parameter_buffer(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	object_config_p obj_config = CONFIG(obj_context->config_id);

	switch (obj_config->profile) {
		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
			vaStatus = crystalhd_render_slice_parameter_buffer_h264(ctx, obj_context, obj_buffer);
			break;

#if 0
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			vaStatus = crystalhd_render_slice_parameter_buffer_mpeg2(ctx, obj_context, obj_buffer);
			break;

		case VAProfileVC1Simple:
			vaStatus = crystalhd_render_slice_parameter_buffer_vc1(ctx, obj_context, obj_buffer);
			break;

		case VAProfileDivx???:
			vaStatus = crystalhd_render_slice_parameter_buffer_divx(ctx, obj_context, obj_buffer);
			break;

		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
			vaStatus = crystalhd_render_slice_parameter_buffer_vc1mp(ctx, obj_context, obj_buffer);
			break;
#endif

		default:
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}

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

VAStatus crystalhd_render_slice_data_buffer(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	object_config_p obj_config = CONFIG(obj_context->config_id);

	switch (obj_config->profile) {
		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
			vaStatus = crystalhd_render_slice_data_buffer_h264(ctx, obj_context, obj_buffer);
			break;

#if 0
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			vaStatus = crystalhd_render_slice_data_buffer_mpeg2(ctx, obj_context, obj_buffer);
			break;

		case VAProfileVC1Simple:
			vaStatus = crystalhd_render_slice_data_buffer_vc1(ctx, obj_context, obj_buffer);
			break;

		case VAProfileDivx???:
			vaStatus = crystalhd_render_slice_data_buffer_divx(ctx, obj_context, obj_buffer);
			break;

		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
			vaStatus = crystalhd_render_slice_data_buffer_vc1mp(ctx, obj_context, obj_buffer);
			break;
#endif

		default:
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}

	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_RenderPicture(
		VADriverContextP ctx,
		VAContextID context,
		VABufferID *buffers,
		int num_buffers
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	object_context_p obj_context = CONTEXT(context);
	object_surface_p obj_surface = SURFACE(obj_context->current_render_target);
	int i;

	assert(obj_context);
	assert(obj_surface);

	/* verify that we got valid buffer references */
	crystalhd__information_message("%s: num_buffers = %d\n", __func__, num_buffers);
	for(i = 0; i < num_buffers; ++i)
	{
		object_buffer_p obj_buffer = BUFFER(buffers[i]);
		assert(obj_buffer);
		if (NULL == obj_buffer)
		{
			return VA_STATUS_ERROR_INVALID_BUFFER;
		}
		crystalhd__information_message("%s: buffer[%d] = 0x%08x, type = %d\n", __func__, i, obj_buffer, obj_buffer->type);
	}

	for(i = 0;i < num_buffers; ++i)
	{
		object_buffer_p obj_buffer = BUFFER(buffers[i]);
		switch (obj_buffer->type)
		{
		case VAIQMatrixBufferType:
			vaStatus = VA_STATUS_SUCCESS;
			break;

		case VAPictureParameterBufferType:
			vaStatus = crystalhd_render_picture_parameter_buffer(ctx, obj_context, obj_buffer);
			break;

		case VASliceParameterBufferType:
			vaStatus = crystalhd_render_slice_parameter_buffer(ctx, obj_context, obj_buffer);
			break;

		case VASliceDataBufferType:
			vaStatus = crystalhd_render_slice_data_buffer(ctx, obj_context, obj_buffer);
			break;

		default:
			crystalhd__information_message("%s: byffer type '%d' not handled.", __func__, obj_buffer->type);
		}
	}

	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_EndPicture(
		VADriverContextP ctx,
		VAContextID context
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	unsigned int width, height, width2, height2, flags;
	VAImage image;
	BC_DTS_PROC_OUT output;
	BC_STATUS sts;
	object_context_p obj_context;
	object_surface_p obj_surface;

	obj_context = CONTEXT(context);
	assert(obj_context);

	obj_surface = SURFACE(obj_context->current_render_target);
	assert(obj_surface);

	sts = DtsProcInput(driver_data->hdev, obj_surface->metadata, obj_surface->metadata_size, 0, FALSE);
	if (sts != BC_STS_SUCCESS)
	{
		crystalhd__error_message("%s: Unable to send metadata. status = %d\n", __func__, sts);
		INSTRUMENT_RET;
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}
	DUMP_BUFFER(obj_surface->metadata, obj_surface->metadata_size, "crystalhd-video_metadata_0x%08x", obj_surface->metadata);

	sts = DtsProcInput(driver_data->hdev, obj_surface->data, obj_surface->data_size, 0, FALSE);
	if (sts != BC_STS_SUCCESS)
	{
		crystalhd__error_message("%s: Unable to send data. status = %d\n", __func__, sts);
		INSTRUMENT_RET;
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}
	DUMP_BUFFER(obj_surface->data, obj_surface->data_size, "crystalhd-video_data_0x%08x", obj_surface->data);

	/* flush the input */
	sts = DtsFlushInput(driver_data->hdev, 0);
	if (sts != BC_STS_SUCCESS)
	{
		crystalhd__error_message("%s: DtsFlushInput failed with status = %d\n", __func__, sts);
		INSTRUMENT_RET;
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	VABufferID obj_image_buf_id = -1;

	width = obj_context->picture_width;
	height = obj_context->picture_height;
again:
	width2 = (width + 1) / 2;
	height2 = (height + 1) / 2;

	if (image.image_id != VA_INVALID_ID)
		vaStatus = crystalhd_DestroyImage(ctx, image.image_id);
	vaStatus = crystalhd_CreateImage(ctx, &(crystalhd_image_formats_map[0].va_format),
			width, height, &image);
	if (vaStatus != VA_STATUS_SUCCESS)
	{
		INSTRUMENT_RET;
		return vaStatus;
	}

	object_buffer_p obj_image_buf = BUFFER(image.buf);

	memset(&output, 0, sizeof(output));
	output.PoutFlags	= BC_POUT_FLAGS_SIZE;
	output.PicInfo.width	= width;
	output.PicInfo.height	= height;
	output.Ybuff		= obj_image_buf->buffer_data;
	output.YbuffSz		= width * height;
	output.UVbuff		= obj_image_buf->buffer_data + output.YbuffSz;
	output.UVbuffSz		= 2 * width2 * height2;

	sts = DtsProcOutput(driver_data->hdev, DTS_OUTPUT_TIMEOUT, &output);
	switch (sts) {
	case BC_STS_SUCCESS:
		if (!(output.PoutFlags & BC_POUT_FLAGS_PIB_VALID))
			goto again;
		obj_surface->output_image_id = image.image_id;
		break;

	case BC_STS_FMT_CHANGE:
		flags = BC_POUT_FLAGS_PIB_VALID | BC_POUT_FLAGS_FMT_CHANGE;
		if ((output.PoutFlags & flags) == flags) {
			width = output.PicInfo.width;
			height = output.PicInfo.height;
			goto again;
		}

	default:
		if (sts != BC_STS_SUCCESS)
		{
			crystalhd_DestroyBuffer(ctx, image.image_id);
			crystalhd__information_message("status = %d\n", sts);
			INSTRUMENT_RET;
			return VA_STATUS_ERROR_OPERATION_FAILED;
		}
	}

	// For now, assume that we are done with rendering right away
	obj_context->current_render_target = -1;

	INSTRUMENT_RET;
	return vaStatus;
}


VAStatus crystalhd_SyncSurface(
		VADriverContextP ctx,
		VASurfaceID render_target
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_surface_p obj_surface;

	obj_surface = SURFACE(render_target);
	assert(obj_surface);

	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_QuerySurfaceStatus(
		VADriverContextP ctx,
		VASurfaceID render_target,
		VASurfaceStatus *status	/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_surface_p obj_surface;

	obj_surface = SURFACE(render_target);
	assert(obj_surface);

	*status = VASurfaceReady;

	INSTRUMENT_RET;
	return vaStatus;
}

VAStatus crystalhd_PutSurface(
   		VADriverContextP ctx,
		VASurfaceID surface,
		Drawable draw, /* X Drawable */
		short srcx,
		short srcy,
		unsigned short srcw,
		unsigned short srch,
		short destx,
		short desty,
		unsigned short destw,
		unsigned short desth,
		VARectangle *cliprects, /* client supplied clip list */
		unsigned int number_cliprects, /* number of clip rects in the clip list */
		unsigned int flags /* de-interlacing flags */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;

	

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

/* 
 * Query display attributes 
 * The caller must provide a "attr_list" array that can hold at
 * least vaMaxNumDisplayAttributes() entries. The actual number of attributes
 * returned in "attr_list" is returned in "num_attributes".
 */
VAStatus crystalhd_QueryDisplayAttributes (
		VADriverContextP ctx,
		VADisplayAttribute *attr_list,	/* out */
		int *num_attributes		/* out */
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;

	if (num_attributes)
		*num_attributes = 0;

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

/* 
 * Get display attributes 
 * This function returns the current attribute values in "attr_list".
 * Only attributes returned with VA_DISPLAY_ATTRIB_GETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can have their values retrieved.  
 */
VAStatus crystalhd_GetDisplayAttributes (
		VADriverContextP ctx,
		VADisplayAttribute *attr_list,	/* in/out */
		int num_attributes
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

/* 
 * Set display attributes 
 * Only attributes returned with VA_DISPLAY_ATTRIB_SETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can be set.  If the attribute is not settable or 
 * the value is out of range, the function returns VA_STATUS_ERROR_ATTR_NOT_SUPPORTED
 */
VAStatus crystalhd_SetDisplayAttributes (
		VADriverContextP ctx,
		VADisplayAttribute *attr_list,
		int num_attributes
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}


VAStatus crystalhd_BufferInfo(
		VADriverContextP ctx,
		VAContextID context,	/* in */
		VABufferID buf_id,	/* in */
		VABufferType *type,	/* out */
		unsigned int *size,		/* out */
		unsigned int *num_elements /* out */
	)
{
	INSTRUMENT_CALL;
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

	

VAStatus crystalhd_LockSurface(
		VADriverContextP ctx,
		VASurfaceID surface,
		unsigned int *fourcc, /* following are output argument */
		unsigned int *luma_stride,
		unsigned int *chroma_u_stride,
		unsigned int *chroma_v_stride,
		unsigned int *luma_offset,
		unsigned int *chroma_u_offset,
		unsigned int *chroma_v_offset,
		unsigned int *buffer_name,
		void **buffer
	)
{
	INSTRUMENT_CALL;
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus crystalhd_UnlockSurface(
		VADriverContextP ctx,
		VASurfaceID surface
	)
{
	INSTRUMENT_CALL;
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus crystalhd_Terminate( VADriverContextP ctx )
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	object_buffer_p obj_buffer;
	object_surface_p obj_surface;
	object_context_p obj_context;
	object_config_p obj_config;
	object_image_p obj_image;
	object_heap_iterator iter;

	/* Clean up left over buffers */
	obj_buffer = (object_buffer_p) object_heap_first( &driver_data->buffer_heap, &iter);
	while (obj_buffer)
	{
		crystalhd__information_message("vaTerminate: bufferID %08x still allocated, destroying\n", obj_buffer->base.id);
		crystalhd__destroy_buffer(driver_data, obj_buffer);
		obj_buffer = (object_buffer_p) object_heap_next( &driver_data->buffer_heap, &iter);
	}
	object_heap_destroy( &driver_data->buffer_heap );

	/* TODO cleanup */
	object_heap_destroy( &driver_data->surface_heap );

	/* TODO cleanup */
	object_heap_destroy( &driver_data->context_heap );

	/* Clean up configIDs */
	obj_config = (object_config_p) object_heap_first( &driver_data->config_heap, &iter);
	while (obj_config)
	{
		object_heap_free( &driver_data->config_heap, (object_base_p) obj_config);
		obj_config = (object_config_p) object_heap_next( &driver_data->config_heap, &iter);
	}
	object_heap_destroy( &driver_data->config_heap );

	/* TODO cleanup */
	obj_image = (object_image_p) object_heap_first( &driver_data->image_heap, &iter);
	while (obj_image)
	{
		object_heap_free( &driver_data->image_heap, (object_base_p) obj_image);
		obj_image = (object_image_p) object_heap_next( &driver_data->image_heap, &iter);
	}
	object_heap_destroy( &driver_data->image_heap );

	if (driver_data->hdev)
	{
		DtsDeviceClose(driver_data->hdev);
		driver_data->hdev = NULL;
	}

	free(ctx->pDriverData);
	ctx->pDriverData = NULL;

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus __vaDriverInit_0_31(  VADriverContextP ctx )
{
	INSTRUMENT_CALL;
	object_base_p obj;
	int result;
	struct crystalhd_driver_data *driver_data;
	int i;

	BC_STATUS sts;

	ctx->version_major = VA_MAJOR_VERSION;
	ctx->version_minor = VA_MINOR_VERSION;
	ctx->max_profiles = CRYSTALHD_MAX_PROFILES;
	ctx->max_entrypoints = CRYSTALHD_MAX_ENTRYPOINTS;
	ctx->max_attributes = CRYSTALHD_MAX_CONFIG_ATTRIBUTES;
	ctx->max_image_formats = CRYSTALHD_MAX_IMAGE_FORMATS;
	ctx->max_subpic_formats = CRYSTALHD_MAX_SUBPIC_FORMATS;
	ctx->max_display_attributes = CRYSTALHD_MAX_DISPLAY_ATTRIBUTES;
	ctx->str_vendor = CRYSTALHD_STR_VENDOR;

	ctx->vtable.vaTerminate = crystalhd_Terminate;
	ctx->vtable.vaQueryConfigEntrypoints = crystalhd_QueryConfigEntrypoints;
	ctx->vtable.vaQueryConfigProfiles = crystalhd_QueryConfigProfiles;
	ctx->vtable.vaQueryConfigEntrypoints = crystalhd_QueryConfigEntrypoints;
	ctx->vtable.vaQueryConfigAttributes = crystalhd_QueryConfigAttributes;
	ctx->vtable.vaCreateConfig = crystalhd_CreateConfig;
	ctx->vtable.vaDestroyConfig = crystalhd_DestroyConfig;
	ctx->vtable.vaGetConfigAttributes = crystalhd_GetConfigAttributes;
	ctx->vtable.vaCreateSurfaces = crystalhd_CreateSurfaces;
	ctx->vtable.vaDestroySurfaces = crystalhd_DestroySurfaces;
	ctx->vtable.vaCreateContext = crystalhd_CreateContext;
	ctx->vtable.vaDestroyContext = crystalhd_DestroyContext;
	ctx->vtable.vaCreateBuffer = crystalhd_CreateBuffer;
	ctx->vtable.vaBufferSetNumElements = crystalhd_BufferSetNumElements;
	ctx->vtable.vaMapBuffer = crystalhd_MapBuffer;
	ctx->vtable.vaUnmapBuffer = crystalhd_UnmapBuffer;
	ctx->vtable.vaDestroyBuffer = crystalhd_DestroyBuffer;
	ctx->vtable.vaBeginPicture = crystalhd_BeginPicture;
	ctx->vtable.vaRenderPicture = crystalhd_RenderPicture;
	ctx->vtable.vaEndPicture = crystalhd_EndPicture;
	ctx->vtable.vaSyncSurface = crystalhd_SyncSurface;
	ctx->vtable.vaQuerySurfaceStatus = crystalhd_QuerySurfaceStatus;
	ctx->vtable.vaPutSurface = crystalhd_PutSurface;
	ctx->vtable.vaQueryImageFormats = crystalhd_QueryImageFormats;
	ctx->vtable.vaCreateImage = crystalhd_CreateImage;
	ctx->vtable.vaDeriveImage = crystalhd_DeriveImage;
	ctx->vtable.vaDestroyImage = crystalhd_DestroyImage;
	ctx->vtable.vaSetImagePalette = crystalhd_SetImagePalette;
	ctx->vtable.vaGetImage = crystalhd_GetImage;
	ctx->vtable.vaPutImage = crystalhd_PutImage;
	ctx->vtable.vaQuerySubpictureFormats = crystalhd_QuerySubpictureFormats;
	ctx->vtable.vaCreateSubpicture = crystalhd_CreateSubpicture;
	ctx->vtable.vaDestroySubpicture = crystalhd_DestroySubpicture;
	ctx->vtable.vaSetSubpictureImage = crystalhd_SetSubpictureImage;
	ctx->vtable.vaSetSubpictureChromakey = crystalhd_SetSubpictureChromakey;
	ctx->vtable.vaSetSubpictureGlobalAlpha = crystalhd_SetSubpictureGlobalAlpha;
	ctx->vtable.vaAssociateSubpicture = crystalhd_AssociateSubpicture;
	ctx->vtable.vaDeassociateSubpicture = crystalhd_DeassociateSubpicture;
	ctx->vtable.vaQueryDisplayAttributes = crystalhd_QueryDisplayAttributes;
	ctx->vtable.vaGetDisplayAttributes = crystalhd_GetDisplayAttributes;
	ctx->vtable.vaSetDisplayAttributes = crystalhd_SetDisplayAttributes;
	//ctx->vtable.vaLockSurface = crystalhd_LockSurface;
	//ctx->vtable.vaUnlockSurface = crystalhd_UnlockSurface;
	//ctx->vtable.vaBufferInfo = crystalhd_BufferInfo;

	driver_data = (struct crystalhd_driver_data *) malloc( sizeof(*driver_data) );
	ctx->pDriverData = (void *) driver_data;

	sts = DtsDeviceOpen(&(driver_data->hdev),
			DTS_PLAYBACK_MODE | DTS_LOAD_FILE_PLAY_FW |
			DTS_DFLT_RESOLUTION(vdecRESOLUTION_CUSTOM));
	assert( sts == BC_STS_SUCCESS );

	result = object_heap_init( &driver_data->config_heap, sizeof(struct object_config), CONFIG_ID_OFFSET );
	assert( result == 0 );

	result = object_heap_init( &driver_data->context_heap, sizeof(struct object_context), CONTEXT_ID_OFFSET );
	assert( result == 0 );

	result = object_heap_init( &driver_data->surface_heap, sizeof(struct object_surface), SURFACE_ID_OFFSET );
	assert( result == 0 );

	result = object_heap_init( &driver_data->buffer_heap, sizeof(struct object_buffer), BUFFER_ID_OFFSET );
	assert( result == 0 );

	result = object_heap_init( &driver_data->image_heap, sizeof(struct object_image), IMAGE_ID_OFFSET );
	assert( result == 0 );

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

