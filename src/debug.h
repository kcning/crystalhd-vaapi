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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <stdarg.h>
#include <va/va.h>

#ifdef USE_DEBUG
static inline const char * string_of_VABufferType(VABufferType type)
{
	switch (type)
	{
#define BUFFERTYPE(buftype) case buftype: return #buftype
		BUFFERTYPE(VAPictureParameterBufferType);
		BUFFERTYPE(VAIQMatrixBufferType);
		BUFFERTYPE(VABitPlaneBufferType);
		BUFFERTYPE(VASliceGroupMapBufferType);
		BUFFERTYPE(VASliceParameterBufferType);
		BUFFERTYPE(VASliceDataBufferType);
		BUFFERTYPE(VAMacroblockParameterBufferType);
		BUFFERTYPE(VAResidualDataBufferType);
		BUFFERTYPE(VADeblockingParameterBufferType);
		BUFFERTYPE(VAImageBufferType);
		BUFFERTYPE(VAProtectedSliceDataBufferType);
		BUFFERTYPE(VAQMatrixBufferType);
		/* Following are encode buffer types */
		BUFFERTYPE(VAEncCodedBufferType);
		BUFFERTYPE(VAEncSequenceParameterBufferType);
		BUFFERTYPE(VAEncPictureParameterBufferType);
		BUFFERTYPE(VAEncSliceParameterBufferType);
		BUFFERTYPE(VAEncH264VUIBufferType);
		BUFFERTYPE(VAEncH264SEIBufferType);
#undef BUFFERTYPE
	}
	return "<unknown>";
}

#define crystalhd__print_buffer(data, size) \
	{ \
		unsigned int i = 0; \
		printf("printing buffer with size 0x%08x\n", size); \
		for (int i = 0;i < size; ++i) \
			printf("0x%02x ", data[i]); \
		printf("\n=====\n"); \
	}

#define crystalhd__error_message(format, ...) \
	fprintf(stderr, "crystalhd_drv_video error: " format, ##__VA_ARGS__);

#define crystalhd__information_message(format, ...) \
	fprintf(stderr, "crystalhd_drv_video info: " format, ##__VA_ARGS__);

#define DUMP_BUFFER(BUF, SIZE, FILENAME, ...) \
	do { \
		char dump_buf_file[200] = { 0 }; \
		sprintf(dump_buf_file, "dump_" FILENAME, __VA_ARGS__); \
		FILE * dump_buf = fopen(dump_buf_file, "w"); \
		size_t wrote_bytes = fwrite(BUF, SIZE, 1, dump_buf); \
		if (wrote_bytes < 0) \
			crystalhd__error_message("cannot dump buffer to %s\n", dump_buf_file); \
		fclose(dump_buf); \
	} while (0);

#define INSTRUMENT_CHECKPOINT(n) \
	crystalhd__information_message("%s (%s:%d): checkpoint %d\n", __func__, __FILE__, __LINE__, n);

#else

#define string_of_VABufferType(type)
#define crystalhd__print_buffer(data, size)
#define crystalhd__error_message(msg, ...)
#define crystalhd__information_message(msg, ...)
#define DUMP_BUFFER(BUF, SIZE, FILENAME, ...)

#define INSTRUMENT_CHECKPOINT(n)

#endif

#endif
