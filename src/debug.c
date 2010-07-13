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

#include "debug.h"

#ifdef USE_DEBUG
void
crystalhd__print_buffer(uint8_t *data, uint32_t size)
{
	unsigned int i = 0;
	printf("printing buffer with size 0x%x\n", size);
	for (i = 0;i < size; ++i)
		printf("0x%02x ", data[i]);
	printf("\n=====\n");
}

void
crystalhd__error_message(const char *msg, ...)
{
	va_list args;

	fprintf(stderr, "crystalhd_drv_video error: ");
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}

void
crystalhd__information_message(const char *msg, ...)
{
	va_list args;

	fprintf(stderr, "crystalhd_drv_video: ");
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}
#endif
