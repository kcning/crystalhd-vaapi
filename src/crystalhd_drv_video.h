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

#ifndef _CRYSTALHD_DRIVER_H_
#define _CRYSTALHD_DRIVER_H_

#include <va/va.h>
#include <va/va_backend.h>

#include <libcrystalhd/bc_dts_types.h>
#include <libcrystalhd/bc_dts_defs.h>
#include <libcrystalhd/libcrystalhd_if.h>
#include <libcrystalhd/libcrystalhd_version.h>

#include "common.h"
#include "object_heap.h"

#define INIT_DRIVER_DATA	struct crystalhd_driver_data *driver_data = get_crystalhd_driver_data(ctx);

#define CONFIG(id)	((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id)	((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define SURFACE(id)	((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define BUFFER(id)	((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))
#define IMAGE(id)	((object_image_p) object_heap_lookup( &driver_data->image_heap, id ))

struct crystalhd_driver_data
{
	struct object_heap	config_heap;
	struct object_heap	context_heap;
	struct object_heap	surface_heap;
	struct object_heap	buffer_heap;
	struct object_heap	image_heap;

	HANDLE			hdev;
};

struct object_config {
	struct object_base base;
	VAProfile profile;
	VAEntrypoint entrypoint;
	VAConfigAttrib attrib_list[CRYSTALHD_MAX_CONFIG_ATTRIBUTES];
	int attrib_count;
};

struct object_context {
	struct object_base base;
	VAContextID context_id;
	VAConfigID config_id;
	VASurfaceID current_render_target;
	int picture_width;
	int picture_height;
	int num_render_targets;
	int flags;
	VASurfaceID *render_targets;

	VABufferID last_iqmatrix_buffer_id;
	VABufferID last_slice_param_buffer_id;
	VABufferID last_picture_param_buffer_id;

	int last_h264_sps_id;
	int last_h264_pps_id;

	uint8_t *metadata;
	int metadata_size;
};

struct object_surface {
	struct object_base base;
	VASurfaceID surface_id;
	uint8_t *metadata;
	int metadata_size;
	uint8_t *data;
	int data_size;
	VAImageID output_image_id;
};

struct object_buffer {
	struct object_base base;
	VABufferType type;
	void *buffer_data;
	int element_size;
	int num_elements;
	int max_num_elements;
};

struct object_image {
	struct object_base base;
	VAImage image;
	unsigned int *palette;
};

typedef struct object_config *object_config_p;
typedef struct object_context *object_context_p;
typedef struct object_surface *object_surface_p;
typedef struct object_buffer *object_buffer_p;
typedef struct object_image *object_image_p;

static inline struct crystalhd_driver_data *
get_crystalhd_driver_data(VADriverContextP ctx)
{
	if (ctx)
		return (struct crystalhd_driver_data *)ctx->pDriverData;
	return NULL;
}

VAStatus crystalhd_QueryConfigProfiles(
	VADriverContextP ctx,
	VAProfile *profile_list,	/* out */
	int *num_profiles		/* out */
);

VAStatus crystalhd_QueryConfigEntrypoints(
	VADriverContextP ctx,
	VAProfile profile,
	VAEntrypoint  *entrypoint_list,	/* out */
	int *num_entrypoints		/* out */
);

VAStatus crystalhd_GetConfigAttributes(
	VADriverContextP ctx,
	VAProfile profile,
	VAEntrypoint entrypoint,
	VAConfigAttrib *attrib_list,	/* in/out */
	int num_attribs
);

VAStatus crystalhd_CreateConfig(
	VADriverContextP ctx,
	VAProfile profile,
	VAEntrypoint entrypoint,
	VAConfigAttrib *attrib_list,
	int num_attribs,
	VAConfigID *config_id		/* out */
);

VAStatus crystalhd_DestroyConfig(
	VADriverContextP ctx,
	VAConfigID config_id
);

VAStatus crystalhd_QueryConfigAttributes(
	VADriverContextP ctx,
	VAConfigID config_id,
	VAProfile *profile,		/* out */
	VAEntrypoint *entrypoint, 	/* out */
	VAConfigAttrib *attrib_list,	/* out */
	int *num_attribs		/* out */
);

VAStatus crystalhd_CreateSurfaces(
	VADriverContextP ctx,
	int width,
	int height,
	int format,
	int num_surfaces,
	VASurfaceID *surfaces		/* out */
);

VAStatus crystalhd_DestroySurfaces(
	VADriverContextP ctx,
	VASurfaceID *surface_list,
	int num_surfaces
);

VAStatus crystalhd_QueryImageFormats(
	VADriverContextP ctx,
	VAImageFormat *format_list,	/* out */
	int *num_formats		/* out */
);

VAStatus crystalhd_CreateImage(
	VADriverContextP ctx,
	VAImageFormat *format,
	int width,
	int height,
	VAImage *out_image		/* out */
);

VAStatus crystalhd_DeriveImage(
	VADriverContextP ctx,
	VASurfaceID surface,
	VAImage *image			/* out */
);

VAStatus crystalhd_DestroyImage(
	VADriverContextP ctx,
	VAImageID image
);

VAStatus crystalhd_SetImagePalette(
	VADriverContextP ctx,
	VAImageID image,
	unsigned char *palette
);

VAStatus crystalhd_GetImage(
	VADriverContextP ctx,
	VASurfaceID surface,
	int x,			/* coordinates of the upper left source pixel */
	int y,
	unsigned int width,	/* width and height of the region */
	unsigned int height,
	VAImageID image
);

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
);

VAStatus crystalhd_QuerySubpictureFormats(
	VADriverContextP ctx,
	VAImageFormat *format_list,	/* out */
	unsigned int *flags,		/* out */
	unsigned int *num_formats	/* out */
);

VAStatus crystalhd_CreateSubpicture(
	VADriverContextP ctx,
	VAImageID image,
	VASubpictureID *subpicture	/* out */
);

VAStatus crystalhd_DestroySubpicture(
	VADriverContextP ctx,
	VASubpictureID subpicture
);

VAStatus crystalhd_SetSubpictureImage(
		VADriverContextP ctx,
		VASubpictureID subpicture,
		VAImageID image
);

VAStatus crystalhd_SetSubpicturePalette(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	/*
	 * pointer to an array holding the palette data.  The size of the array is
	 * num_palette_entries * entry_bytes in size.  The order of the components
	 * in the palette is described by the component_order in VASubpicture struct
	 */
	unsigned char *palette
);

VAStatus crystalhd_SetSubpictureChromakey(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	unsigned int chromakey_min,
	unsigned int chromakey_max,
	unsigned int chromakey_mask
);

VAStatus crystalhd_SetSubpictureGlobalAlpha(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	float global_alpha 
);


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
);

VAStatus crystalhd_DeassociateSubpicture(
	VADriverContextP ctx,
	VASubpictureID subpicture,
	VASurfaceID *target_surfaces,
	int num_surfaces
);

VAStatus crystalhd_CreateContext(
	VADriverContextP ctx,
	VAConfigID config_id,
	int picture_width,
	int picture_height,
	int flag,
	VASurfaceID *render_targets,
	int num_render_targets,
	VAContextID *context		/* out */
);

VAStatus crystalhd_DestroyContext(
	VADriverContextP ctx,
	VAContextID context
);

VAStatus crystalhd_CreateBuffer(
	VADriverContextP ctx,
	VAContextID context,		/* in */
	VABufferType type,		/* in */
	unsigned int size,		/* in */
	unsigned int num_elements,	/* in */
	void *data,			/* in */
	VABufferID *buf_id		/* out */
);

VAStatus crystalhd_BufferSetNumElements(
	VADriverContextP ctx,
	VABufferID buf_id,		/* in */
	unsigned int num_elements	/* in */
);

VAStatus crystalhd_MapBuffer(
	VADriverContextP ctx,
	VABufferID buffer_id,	/* in */
	void **pbuf		/* out */
);

VAStatus crystalhd_UnmapBuffer(
	VADriverContextP ctx,
	VABufferID buf_id	/* in */
);

VAStatus crystalhd_DestroyBuffer(
	VADriverContextP ctx,
	VABufferID buffer_id
);
VAStatus crystalhd_BeginPicture(
	VADriverContextP ctx,
	VAContextID context,
	VASurfaceID render_target
);

VAStatus crystalhd_RenderPicture(
	VADriverContextP ctx,
	VAContextID context,
	VABufferID *buffers,
	int num_buffers
);

VAStatus crystalhd_EndPicture(
	VADriverContextP ctx,
	VAContextID context
);

VAStatus crystalhd_SyncSurface(
	VADriverContextP ctx,
	VASurfaceID render_target
);

VAStatus crystalhd_QuerySurfaceStatus(
	VADriverContextP ctx,
	VASurfaceID render_target,
	VASurfaceStatus *status	/* out */
);

VAStatus crystalhd_PutSurface(
	VADriverContextP ctx,
	VASurfaceID surface,
	void *draw, /* Drawable of window system */
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
);

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
);

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
);

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
);

VAStatus crystalhd_BufferInfo(
	VADriverContextP ctx,
	VAContextID context,		/* in */
	VABufferID buffer_id,		/* in */
	VABufferType *type,		/* out */
	unsigned int *size,		/* out */
	unsigned int *num_elements	/* out */
);

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
);

VAStatus crystalhd_UnlockSurface(
	VADriverContextP ctx,
	VASurfaceID surface
);
#endif

VAStatus
crystalhd_Terminate(
	VADriverContextP ctx
);

VAStatus
__vaDriverInit_0_31(
	VADriverContextP ctx
);
#endif
