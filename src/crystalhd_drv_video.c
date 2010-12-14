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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define _GNU_SOURCE
#define __USE_GNU
#include <dlfcn.h>

#include "crystalhd_drv_video.h"
#include "crystalhd_video_h264.h"

#include "debug.h"

#define CONFIG_ID_OFFSET		0x01000000
#define CONTEXT_ID_OFFSET		0x02000000
#define SURFACE_ID_OFFSET		0x04000000
#define BUFFER_ID_OFFSET		0x08000000
#define IMAGE_ID_OFFSET			0x10000000

object_buffer_p buffered_picture_parameter_buffer;
object_buffer_p buffered_slice_parameter_buffer;

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

VAStatus crystalhd_QueryConfigProfiles(
		VADriverContextP ctx,
		VAProfile *profile_list,	/* out */
		int *num_profiles		/* out */
	)
{
	INIT_DRIVER_DATA;
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

	switch (profile) {
#if 0
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
#endif
		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
				*num_entrypoints = 1;
				entrypoint_list[0] = VAEntrypointVLD;
				break;

#if 0
		case VAProfileVC1Simple:
		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
				*num_entrypoints = 1;
				entrypoint_list[0] = VAEntrypointVLD;
				break;

		case VAProfileH263Baseline:
				*num_entrypoints = 1;
				entrypoint_list[0] = VAEntrypointVLD;
				break;

		case VAProfileJPEGBaseline:
				*num_entrypoints = 1;
				entrypoint_list[0] = VAEntrypointVLD;
				break;
#endif

		default:
				*num_entrypoints = 0;
				break;
	}

	/* If the assert fails then CRYSTALHD_MAX_ENTRYPOINTS needs to be bigger */
	assert(*num_entrypoints <= CRYSTALHD_MAX_ENTRYPOINTS);

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
			return VA_STATUS_SUCCESS;
		}
	}
	if (obj_config->attrib_count < CRYSTALHD_MAX_CONFIG_ATTRIBUTES)
	{
		i = obj_config->attrib_count;
		obj_config->attrib_list[i].type = attrib->type;
		obj_config->attrib_list[i].value = attrib->value;
		obj_config->attrib_count++;
		return VA_STATUS_SUCCESS;
	}
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
	VAStatus vaStatus;
	int configID;
	object_config_p obj_config;
	int i;

	/* Validate profile & entrypoint */
	switch (profile) {
#if 0
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
#endif
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

#if 0
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
#endif
	}

	if (VA_STATUS_SUCCESS != vaStatus)
	{
		return vaStatus;
	}

	configID = object_heap_allocate( &driver_data->config_heap );
	obj_config = CONFIG(configID);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
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

	return vaStatus;
}

VAStatus crystalhd_DestroyConfig(
		VADriverContextP ctx,
		VAConfigID config_id
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus;
	object_config_p obj_config;

	obj_config = CONFIG(config_id);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
		return vaStatus;
	}

	object_heap_free( &driver_data->config_heap, (object_base_p) obj_config);
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

	VAStatus vaStatus = VA_STATUS_SUCCESS;
	int i;

	/* We only support one format */
	if (VA_RT_FORMAT_YUV420 != format)
	{
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

	return vaStatus;
}

VAStatus crystalhd_DestroySurfaces(
		VADriverContextP ctx,
		VASurfaceID *surface_list,
		int num_surfaces
	)
{
	INIT_DRIVER_DATA;

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
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_QueryImageFormats(
	VADriverContextP ctx,
	VAImageFormat *format_list,	/* out */
	int *num_formats		/* out */
)
{
	INIT_DRIVER_DATA;
	
	int n;
	for (n = 0; crystalhd_image_formats_map[n].va_format.fourcc != 0; ++n) {
		const crystalhd_image_format_map_t * const m = &crystalhd_image_formats_map[n];
		if (format_list)
			format_list[n] = m->va_format;
	}

	if (num_formats)
		*num_formats = n;

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

	return VA_STATUS_SUCCESS;

error:
	crystalhd_DestroyImage(ctx, image_id);

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

	object_image_p obj_image = IMAGE(image);
	if (obj_image)
	{
		if (obj_image->image.buf != VA_INVALID_ID)
			crystalhd_DestroyBuffer(ctx, obj_image->image.buf);

		if (obj_image->palette)
			free(obj_image->palette);

		object_heap_free(&driver_data->image_heap, (object_base_p)obj_image);
	}

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_SetImagePalette(
	VADriverContextP ctx,
	VAImageID image,
	unsigned char *palette
)
{
	INIT_DRIVER_DATA;
	
	/* TODO */

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

	object_surface_p obj_surface = SURFACE(surface);
	if (NULL == obj_surface)
	{
		return VA_STATUS_ERROR_INVALID_SURFACE;
	}

	object_image_p obj_output_image = IMAGE(obj_surface->output_image_id);
	if (NULL == obj_output_image)
	{
		return VA_STATUS_ERROR_INVALID_IMAGE;
	}

	object_image_p obj_image = IMAGE(image);
	if (NULL == obj_image)
	{
		return VA_STATUS_ERROR_INVALID_IMAGE;
	}

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

	object_buffer_p buf = BUFFER(obj_image->image.buf),
			obuf = BUFFER(obj_output_image->image.buf);
	crystalhd__information_message("0x%08x\n", obj_image->image.buf);
	crystalhd__information_message("0x%08x\n", obj_output_image->image.buf);
	
	INSTRUMENT_CHECKPOINT(1);
	memcpy(	buf->buffer_data, obuf->buffer_data,
		CRYSTALHD_MIN(obj_image->image.data_size, obj_output_image->image.data_size));
	crystalhd__information_message("0x%08x 0x%08x\n", obj_image->image.data_size, obj_output_image->image.data_size);
	INSTRUMENT_CHECKPOINT(2);
	
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
	
	/* TODO */

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
	
	/* TODO */

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_CreateSubpicture(
	VADriverContextP ctx,
	VAImageID image,
	VASubpictureID *subpicture	/* out */
)
{
	INIT_DRIVER_DATA;
	
	/* TODO */

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_DestroySubpicture(
	VADriverContextP ctx,
	VASubpictureID subpicture
)
{
	INIT_DRIVER_DATA;
	
	/* TODO */

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_SetSubpictureImage(
		VADriverContextP ctx,
		VASubpictureID subpicture,
		VAImageID image
)
{
	INIT_DRIVER_DATA;
	
	/* TODO */

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
	
	/* TODO */

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
	
	/* TODO */

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_SetSubpictureGlobalAlpha(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	float global_alpha 
)
{
	INIT_DRIVER_DATA;
	
	/* TODO */

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
	
	/* TODO */

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
	
	/* TODO */

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

	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_config_p obj_config;
	int i;
	BC_STATUS sts;

	obj_config = CONFIG(config_id);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
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
		return vaStatus;
	}

	obj_context->context_id  = contextID;
	*context = contextID;
	obj_context->current_render_target = -1;
	obj_context->config_id = config_id;
	obj_context->picture_width = picture_width;
	obj_context->picture_height = picture_height;
	obj_context->num_render_targets = num_render_targets;
	obj_context->last_iqmatrix_buffer_id = 0;
	obj_context->last_slice_param_buffer_id = 0;
	obj_context->last_picture_param_buffer_id = 0;
	obj_context->last_h264_sps_id = 0;
	obj_context->last_h264_pps_id = 0;

	obj_context->render_targets = (VASurfaceID *) malloc(num_render_targets * sizeof(VASurfaceID));
	if (obj_context->render_targets == NULL)
	{
		return VA_STATUS_ERROR_ALLOCATION_FAILED;
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
#if 0
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			bc_algo = BC_VID_ALGO_MPEG2;
			break;

		case VAProfileVC1Simple:
			bc_algo = BC_VID_ALGO_VC1;
			break;

		case VAProfileDivx???:
			bc_algo = BC_VID_ALGO_DIVX;
			break;

		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
			bc_algo = BC_VID_ALGO_VC1MP;
			break;
#endif
		default:
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

	return vaStatus;
}


VAStatus crystalhd_DestroyContext(
		VADriverContextP ctx,
		VAContextID context
	)
{
	INIT_DRIVER_DATA;

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
			return vaStatus;
	}

	bufferID = object_heap_allocate( &driver_data->buffer_heap );
	obj_buffer = BUFFER(bufferID);
	crystalhd__information_message("buffer_id = %d (0x%08x), obj_buffer = 0x%08x, obj_buffer->base.id = 0x%08x, BufferType: %d, size: %d * %d = %d\n",
			bufferID, bufferID, obj_buffer, obj_buffer->base.id, type, size, num_elements, size * num_elements);
	if (NULL == obj_buffer)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
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

	return vaStatus;
}

VAStatus crystalhd_BufferSetNumElements(
		VADriverContextP ctx,
		VABufferID buf_id,		/* in */
		unsigned int num_elements	/* in */
	)
{
	INIT_DRIVER_DATA;

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

	return vaStatus;
}

VAStatus crystalhd_MapBuffer(
		VADriverContextP ctx,
		VABufferID buffer_id,	/* in */
		void **pbuf		/* out */
	)
{
	INIT_DRIVER_DATA;

	object_buffer_p obj_buffer = BUFFER(buffer_id);

	if (NULL == obj_buffer)
	{
		return VA_STATUS_ERROR_INVALID_BUFFER;
	}

	if (NULL != obj_buffer->buffer_data)
	{
		*pbuf = obj_buffer->buffer_data;
	}

	return VA_STATUS_SUCCESS;
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
	if (NULL != obj_buffer->buffer_data)
		free(obj_buffer->buffer_data);

	object_heap_free( &driver_data->buffer_heap, (object_base_p) obj_buffer);
}

VAStatus crystalhd_DestroyBuffer(
		VADriverContextP ctx,
		VABufferID buffer_id
	)
{
	INIT_DRIVER_DATA;
	object_buffer_p obj_buffer = BUFFER(buffer_id);
	if (NULL == obj_buffer)
		return VA_STATUS_SUCCESS;

	Dl_info dlinfo;
	void * caller = __builtin_return_address(0);
	dladdr(caller, &dlinfo);

	if (dlinfo.dli_sname != NULL && strcmp("ff_vaapi_common_end_frame", dlinfo.dli_sname) == 0)
	{
		// do not destroy image buffer
		if (obj_buffer->type == VAImageBufferType)
			return VA_STATUS_SUCCESS;
	}

	crystalhd__destroy_buffer(driver_data, obj_buffer);

	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_BeginPicture(
		VADriverContextP ctx,
		VAContextID context,
		VASurfaceID render_target
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_context_p obj_context = CONTEXT(context);
	assert(obj_context);
	object_surface_p obj_surface = SURFACE(render_target);
	assert(obj_surface);
	object_config_p obj_config = CONFIG(obj_context->config_id);
	assert(obj_config);

	obj_context->current_render_target = obj_surface->base.id;

	switch (obj_config->profile) {
		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
			vaStatus = crystalhd_begin_picture_h264(ctx, context, render_target);
			break;
#if 0
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			vaStatus = crystalhd_begin_picture_mpeg2(ctx, context, render_target);
			break;

		case VAProfileVC1Simple:
			vaStatus = crystalhd_begin_picture_vc1(ctx, context, render_target);
			break;

		case VAProfileDivx???:
			vaStatus = crystalhd_begin_picture_divx(ctx, context, render_target);
			break;

		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
			vaStatus = crystalhd_begin_picture_vc1mp(ctx, context, render_target);
			break;
#endif
		default:
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}

	return vaStatus;
}

#if 0
VAStatus crystalhd_render_picture_parameter_buffer_mpeg2(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	return vaStatus;
}

VAStatus crystalhd_render_picture_parameter_buffer_vc1(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	return vaStatus;
}

VAStatus crystalhd_render_picture_parameter_buffer_vc1mp(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	return vaStatus;
}
#endif

VAStatus crystalhd_render_iqmatrix_buffer(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	object_config_p obj_config = CONFIG(obj_context->config_id);

	switch (obj_config->profile) {
		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
			vaStatus = crystalhd_render_iqmatrix_buffer_h264(ctx, obj_context, obj_buffer);
			break;
#if 0
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			vaStatus = crystalhd_render_iqmatrix_buffer_mpeg2(ctx, obj_context, obj_buffer);
			break;

		case VAProfileVC1Simple:
			vaStatus = crystalhd_render_iqmatrix_buffer_vc1(ctx, obj_context, obj_buffer);
			break;

		case VAProfileDivx???:
			vaStatus = crystalhd_render_iqmatrix_buffer_divx(ctx, obj_context, obj_buffer);
			break;

		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
			vaStatus = crystalhd_render_iqmatrix_buffer_vc1mp(ctx, obj_context, obj_buffer);
			break;
#endif
		default:
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}

	return vaStatus;
}

VAStatus crystalhd_render_picture_parameter_buffer(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	object_config_p obj_config = CONFIG(obj_context->config_id);

	switch (obj_config->profile) {
		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
			vaStatus = crystalhd_render_picture_parameter_buffer_h264(ctx, obj_context, obj_buffer);
			break;
#if 0
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			vaStatus = crystalhd_render_picture_parameter_buffer_mpeg2(ctx, obj_context, obj_buffer);
			break;

		case VAProfileVC1Simple:
			vaStatus = crystalhd_render_picture_parameter_buffer_vc1(ctx, obj_context, obj_buffer);
			break;

		case VAProfileDivx???:
			vaStatus = crystalhd_render_picture_parameter_buffer_divx(ctx, obj_context, obj_buffer);
			break;

		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
			vaStatus = crystalhd_render_picture_parameter_buffer_vc1mp(ctx, obj_context, obj_buffer);
			break;
#endif
		default:
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}

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

	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	return vaStatus;
}

VAStatus crystalhd_render_slice_parameter_buffer_vc1(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
	return vaStatus;
}

VAStatus crystalhd_render_slice_parameter_buffer_vc1mp(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
	/* TODO */
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

	return vaStatus;
}

VAStatus crystalhd_render_slice_data_buffer(
		VADriverContextP ctx,
		object_context_p obj_context,
		object_buffer_p obj_buffer
	)
{
	INIT_DRIVER_DATA;

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

	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	int i;

	object_context_p obj_context = CONTEXT(context);
	assert(obj_context);

	object_surface_p obj_surface = SURFACE(obj_context->current_render_target);
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
			vaStatus = crystalhd_render_iqmatrix_buffer(ctx, obj_context, obj_buffer);
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
			crystalhd__information_message("%s: byffer type '%d' not handled.\n", __func__, obj_buffer->type);
		}
	}

	return vaStatus;
}

VAStatus crystalhd_EndPicture(
		VADriverContextP ctx,
		VAContextID context
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_SUCCESS;
	unsigned int width, height, width2, height2, flags;
	VAImage image;
	BC_DTS_PROC_OUT output;
	BC_STATUS sts;

	image.image_id = VA_INVALID_ID;

	object_context_p obj_context = CONTEXT(context);
	assert(obj_context);

	object_config_p obj_config = CONFIG(obj_context->config_id);
	assert(obj_config);

	object_surface_p obj_surface = SURFACE(obj_context->current_render_target);
	assert(obj_surface);

	switch (obj_config->profile) {
		case VAProfileH264Baseline:
		case VAProfileH264Main:
		case VAProfileH264High:
			vaStatus = crystalhd_end_picture_h264(ctx, obj_context, obj_surface);
			break;

#if 0
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			vaStatus = crystalhd_end_picture_mpeg2(ctx, obj_context, obj_surface);
			break;

		case VAProfileVC1Simple:
			vaStatus = crystalhd_end_picture_vc1(ctx, obj_context, obj_surface);
			break;

		case VAProfileDivx???:
			vaStatus = crystalhd_end_picture_divx(ctx, obj_context, obj_surface);
			break;

		case VAProfileVC1Main:
		case VAProfileVC1Advanced:
			vaStatus = crystalhd_end_picture_vc1mp(ctx, obj_context, obj_surface);
			break;
#endif

		default:
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}

	width = obj_context->picture_width;
	height = obj_context->picture_height;
again:
	width2 = (width + 1) / 2;
	height2 = (height + 1) / 2;

	if (image.image_id != VA_INVALID_ID)
		vaStatus = crystalhd_DestroyImage(ctx, image.image_id);
	vaStatus = crystalhd_CreateImage(ctx, &(crystalhd_image_formats_map[0].va_format), width, height, &image);
	if (vaStatus != VA_STATUS_SUCCESS)
	{
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
		{
			crystalhd__information_message("%s: checking BC_POUT_FLAG_PIB_VALID failed. trying again.\n", __func__);
			goto again;
		}
		break;

	case BC_STS_FMT_CHANGE:
		flags = BC_POUT_FLAGS_PIB_VALID | BC_POUT_FLAGS_FMT_CHANGE;
		if ((output.PoutFlags & flags) == flags) {
			width = output.PicInfo.width;
			height = output.PicInfo.height;
		}
		goto again;

	default:
		if (sts != BC_STS_SUCCESS)
		{
			crystalhd__information_message("status = %d\n", sts);
			return VA_STATUS_ERROR_OPERATION_FAILED;
		}
	}
	obj_surface->output_image_id = image.image_id;

	// For now, assume that we are done with rendering right away
	obj_context->current_render_target = -1;

	static int xxx = 0;

	crystalhd__information_message("%s (#%d): I'm being called %d times.", __func__, __LINE__, ++xxx);

	return vaStatus;
}

VAStatus crystalhd_SyncSurface(
		VADriverContextP ctx,
		VASurfaceID render_target
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_SUCCESS;
	//object_surface_p obj_surface;

	//obj_surface = SURFACE(render_target);

	return vaStatus;
}

VAStatus crystalhd_QuerySurfaceStatus(
		VADriverContextP ctx,
		VASurfaceID render_target,
		VASurfaceStatus *status	/* out */
	)
{
	INIT_DRIVER_DATA;

	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_surface_p obj_surface;

	obj_surface = SURFACE(render_target);
	assert(obj_surface);

	*status = VASurfaceReady;

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

	if (num_attributes)
		*num_attributes = 0;

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

	/* TODO */

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

	/* TODO */

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}


VAStatus crystalhd_BufferInfo(
		VADriverContextP ctx,
		VAContextID context,		/* in */
		VABufferID buffer_id,		/* in */
		VABufferType *type,		/* out */
		unsigned int *size,		/* out */
		unsigned int *num_elements	/* out */
	)
{
	INIT_DRIVER_DATA;

	object_buffer_p obj_buffer = BUFFER(buffer_id);
	if (NULL == obj_buffer)
	{
		return VA_STATUS_ERROR_INVALID_BUFFER;
	}

	if (NULL != type)
		*type = obj_buffer->type;

	if (NULL != size)
		*size = obj_buffer->element_size;

	if (NULL != num_elements)
		*num_elements = obj_buffer->num_elements;

	return VA_STATUS_SUCCESS;
}

#if 0
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
	/* TODO */
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus crystalhd_UnlockSurface(
		VADriverContextP ctx,
		VASurfaceID surface
	)
{
	/* TODO */
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}
#endif

VAStatus
crystalhd_Terminate(
		VADriverContextP ctx
	)
{
	INIT_DRIVER_DATA;

	object_buffer_p obj_buffer;
	object_surface_p obj_surface;
	object_context_p obj_context;
	object_config_p obj_config;
	object_image_p obj_image;
	object_heap_iterator iter;

	/* close device */
	if (driver_data->hdev)
	{
		DtsDeviceClose(driver_data->hdev);
	}

	/* Clean up images */
	obj_image = (object_image_p) object_heap_first( &driver_data->image_heap, &iter);
	while (obj_image)
	{
		object_heap_free( &driver_data->image_heap, (object_base_p) obj_image);
		obj_image = (object_image_p) object_heap_next( &driver_data->image_heap, &iter);
	}
	object_heap_destroy( &driver_data->image_heap );

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

	/* Clean up configs */
	obj_config = (object_config_p) object_heap_first( &driver_data->config_heap, &iter);
	while (obj_config)
	{
		object_heap_free( &driver_data->config_heap, (object_base_p) obj_config);
		obj_config = (object_config_p) object_heap_next( &driver_data->config_heap, &iter);
	}
	object_heap_destroy( &driver_data->config_heap );

	free(ctx->pDriverData);

	return VA_STATUS_SUCCESS;
}

VAStatus
__vaDriverInit_0_31(
		VADriverContextP ctx
	)
{
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
	if ( sts != BC_STS_SUCCESS )
	{
		DtsDeviceClose(driver_data->hdev);
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

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

	return VA_STATUS_SUCCESS;
}
