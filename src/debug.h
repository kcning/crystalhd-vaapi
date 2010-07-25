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

#include "osdep.h"

#ifdef USE_DEBUG
inline void
crystalhd__print_buffer(uint8_t *data, uint32_t size);

inline void
crystalhd__error_message(const char *msg, ...);

inline void
crystalhd__information_message(const char *msg, ...);

#define DUMP_BUFFER(BUF, SIZE, FILENAME, ...) \
	do { \
		char dump_buf_file[200] = { 0 }; \
		sprintf(dump_buf_file, FILENAME, __VA_ARGS__); \
		FILE * dump_buf = fopen(dump_buf_file, "w"); \
		if (fwrite(BUF, SIZE, 1, dump_buf) < 0) \
			crystalhd__error_message("cannot dump buffer to %s\n", dump_buf_file); \
		fclose(dump_buf); \
	} while (0);

#define INSTRUMENT_CALL			crystalhd__information_message("%s (#%d): being called\n", __func__, __LINE__);
#define INSTRUMENT_RET			crystalhd__information_message("%s (#%d): returned\n", __func__, __LINE__);
#define INSTRUMENT_CHECKPOINT(n)	crystalhd__information_message("%s (#%d): checkpoint %d\n", __func__, __LINE__, n);

#else

#define crystalhd__print_buffer(data,size)
#define crystalhd__error_message(msg, ...)
#define crystalhd__information_message(msg, ...)
#define DUMP_BUFFER(BUF, SIZE, FILENAME, ...)

#define INSTRUMENT_CALL
#define INSTRUMENT_RET
#define INSTRUMENT_CHECKPOINT(n)

#endif

#endif
