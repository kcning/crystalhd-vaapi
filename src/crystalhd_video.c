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

#include <libcrystalhd/bc_dts_types.h>
#include "crystalhd_video.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define INIT_DRIVER_DATA;	struct crystalhd_driver_data *driver_data = (struct crystalhd_driver_data *) ctx->pDriverData;

#define CONFIG(id)	((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id)	((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define SURFACE(id)	((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define BUFFER(id)	((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))

#define CONFIG_ID_OFFSET		0x01000000
#define CONTEXT_ID_OFFSET		0x02000000
#define SURFACE_ID_OFFSET		0x04000000
#define BUFFER_ID_OFFSET		0x08000000

#define INSTRUMENT_CALL	crystalhd__information_message("%s: being called\n", __func__);
#define INSTRUMENT_RET	crystalhd__information_message("%s: returned\n", __func__);

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

	profile_list[i++] = VAProfileMPEG2Simple;
	profile_list[i++] = VAProfileMPEG2Main;
	profile_list[i++] = VAProfileMPEG4Simple;
	profile_list[i++] = VAProfileMPEG4AdvancedSimple;
	profile_list[i++] = VAProfileMPEG4Main;
	profile_list[i++] = VAProfileH264Baseline;
	profile_list[i++] = VAProfileH264Main;
	profile_list[i++] = VAProfileH264High;
	profile_list[i++] = VAProfileVC1Simple;
	profile_list[i++] = VAProfileVC1Main;
	profile_list[i++] = VAProfileVC1Advanced;
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
		assert(obj_surface);
		object_heap_free( &driver_data->surface_heap, (object_base_p) obj_surface);
	}
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_QueryImageFormats(
	VADriverContextP ctx,
	VAImageFormat *format_list,	/* out */
	int *num_formats		/* out */
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_CreateImage(
	VADriverContextP ctx,
	VAImageFormat *format,
	int width,
	int height,
	VAImage *image			/* out */
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_DeriveImage(
	VADriverContextP ctx,
	VASurfaceID surface,
	VAImage *image			/* out */
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

VAStatus crystalhd_DestroyImage(
	VADriverContextP ctx,
	VAImageID image
)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	
	/* TODO */
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
	
	/* TODO */
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
	return VA_STATUS_SUCCESS;
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
	return VA_STATUS_SUCCESS;
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

	obj_config = CONFIG(config_id);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
		INSTRUMENT_RET;
		return vaStatus;
	}

	/* Validate flag */
	/* Validate picture dimensions */

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

	object_heap_free( &driver_data->context_heap, (object_base_p) obj_context);

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}



static VAStatus crystalhd__allocate_buffer(object_buffer_p obj_buffer, int size)
{
	VAStatus vaStatus = VA_STATUS_SUCCESS;

	obj_buffer->buffer_data = realloc(obj_buffer->buffer_data, size);
	if (NULL == obj_buffer->buffer_data)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
	}
	INSTRUMENT_RET;
	return vaStatus;
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
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
			INSTRUMENT_RET;
			return vaStatus;
	}

	bufferID = object_heap_allocate( &driver_data->buffer_heap );
	obj_buffer = BUFFER(bufferID);
	if (NULL == obj_buffer)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		INSTRUMENT_RET;
		return vaStatus;
	}

	obj_buffer->buffer_data = NULL;

	vaStatus = crystalhd__allocate_buffer(obj_buffer, size * num_elements);
	if (VA_STATUS_SUCCESS == vaStatus)
	{
		obj_buffer->max_num_elements = num_elements;
		obj_buffer->num_elements = num_elements;
		if (data)
		{
			memcpy(obj_buffer->buffer_data, data, size * num_elements);
		}
	}

	if (VA_STATUS_SUCCESS == vaStatus)
	{
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
		void **pbuf		 /* out */
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
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	/* Do nothing */
	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

static void crystalhd__destroy_buffer(struct crystalhd_driver_data *driver_data, object_buffer_p obj_buffer)
{
	if (NULL != obj_buffer->buffer_data)
	{
		free(obj_buffer->buffer_data);
		obj_buffer->buffer_data = NULL;
	}

	object_heap_free( &driver_data->buffer_heap, (object_base_p) obj_buffer);
}

VAStatus crystalhd_DestroyBuffer(
		VADriverContextP ctx,
		VABufferID buffer_id
	)
{
	INIT_DRIVER_DATA;
	INSTRUMENT_CALL;
	object_buffer_p obj_buffer = BUFFER(buffer_id);
	assert(obj_buffer);

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
	object_context_p obj_context;
	object_surface_p obj_surface;

	obj_context = CONTEXT(context);
	assert(obj_context);

	obj_surface = SURFACE(render_target);
	assert(obj_surface);

	obj_context->current_render_target = obj_surface->base.id;

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
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_context_p obj_context;
	object_surface_p obj_surface;
	int i;

	obj_context = CONTEXT(context);
	assert(obj_context);

	obj_surface = SURFACE(obj_context->current_render_target);
	assert(obj_surface);

	/* verify that we got valid buffer references */
	for(i = 0; i < num_buffers; i++)
	{
		object_buffer_p obj_buffer = BUFFER(buffers[i]);
		assert(obj_buffer);
		if (NULL == obj_buffer)
		{
			vaStatus = VA_STATUS_ERROR_INVALID_BUFFER;
			break;
		}
	}
	
	/* Release buffers */
	for(i = 0; i < num_buffers; i++)
	{
		object_buffer_p obj_buffer = BUFFER(buffers[i]);
		assert(obj_buffer);
		crystalhd__destroy_buffer(driver_data, obj_buffer);
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
	object_context_p obj_context;
	object_surface_p obj_surface;

	obj_context = CONTEXT(context);
	assert(obj_context);

	obj_surface = SURFACE(obj_context->current_render_target);
	assert(obj_surface);

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
	/* TODO */
	INSTRUMENT_RET;
	return VA_STATUS_ERROR_UNKNOWN;
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

	if (driver_data->hdev)
	{
		DtsCloseDecoder(driver_data->hdev);
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
			DTS_SKIP_TX_CHK_CPB | DTS_DFLT_RESOLUTION(vdecRESOLUTION_720p29_97));
	assert( sts == BC_STS_SUCCESS );

	sts = DtsOpenDecoder(driver_data->hdev, BC_STREAM_TYPE_ES);
	assert( sts == BC_STS_SUCCESS );

	result = object_heap_init( &driver_data->config_heap, sizeof(struct object_config), CONFIG_ID_OFFSET );
	assert( result == 0 );

	result = object_heap_init( &driver_data->context_heap, sizeof(struct object_context), CONTEXT_ID_OFFSET );
	assert( result == 0 );

	result = object_heap_init( &driver_data->surface_heap, sizeof(struct object_surface), SURFACE_ID_OFFSET );
	assert( result == 0 );

	result = object_heap_init( &driver_data->buffer_heap, sizeof(struct object_buffer), BUFFER_ID_OFFSET );
	assert( result == 0 );

	INSTRUMENT_RET;
	return VA_STATUS_SUCCESS;
}

