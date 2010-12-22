#ifndef _CRYSTALHD_COMMON_H_
#define _CRYSTALHD_COMMON_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

#include <libcrystalhd/bc_dts_types.h>
#include <libcrystalhd/libcrystalhd_if.h>

extern int _indent_level;
extern pthread_mutex_t mutex_func;

void * __dlopen_crystalhd_so();
const char *string_of_BC_STATUS(BC_STATUS);
const char *string_of_BC_MEDIA_SUBTYPE(BC_MEDIA_SUBTYPE);

#define MIN(x, y) (((x)<(y))?(x):(y))

#define LIST(...) __VA_ARGS__

#define enter(args_format, ...) \
	printf("__crystalhd[0x%08x] => ", (unsigned int)pthread_self()); \
	{ int i = _indent_level; while (i--) printf("  "); } \
	printf("%s(" args_format ")\n", __func__, ##__VA_ARGS__)

#define leave(ret) \
	printf("__crystalhd[0x%08x] <= ", (unsigned int)pthread_self()); \
	{ int i = _indent_level; while (i--) printf("  "); } \
	printf("%s() = %s\n", __func__, string_of_BC_STATUS(ret))

#define eprintf(args_format, ...) \
	printf("__crystalhd[0x%08x]    ", (unsigned int)pthread_self()); \
	{ int i = _indent_level; while (i--) printf("  "); } \
	printf("%s: " args_format, __func__, ##__VA_ARGS__)

/**
 * Macro FUNC_WRAPPER():
 * 	FUNCTION: function name
 * 	RET_T	: list of return types
 * 	ARG_LIST: argument (type-name pair) list
 * 	ARGS_T	: list of argument types
 * 	ARGS	: list of arguments
 * 	format	: format for printf()
 * 	...	: codes to run after function return
 */
#define PRE_FUNC_CMDS_NULL	{ }
#define POST_FUNC_CMDS_NULL	{ }
#define FUNC_WRAPPER(FUNCTION, RET_T, ARG_LIST, ARGS_T, ARGS, format, PRE_FUNC_CMDS, POST_FUNC_CMDS) \
	RET_T FUNCTION(LIST(ARG_LIST)) { \
		pthread_mutex_lock( &mutex_func ); \
		enter(format, ARGS); \
		PRE_FUNC_CMDS; \
		if (_indent_level < 30) \
			++_indent_level; \
		pthread_mutex_unlock( &mutex_func ); \
		RET_T (*func)(ARGS_T); \
		* (void **) (&func) = dlsym(__dlopen_crystalhd_so(), __func__); \
		RET_T ret = func(ARGS); \
		pthread_mutex_lock( &mutex_func ); \
		if (_indent_level > 0) \
			--_indent_level; \
		POST_FUNC_CMDS; \
		leave(ret); \
		pthread_mutex_unlock( &mutex_func ); \
		return ret; \
	}

#define dump_to_file(BUF, SIZE, FILENAME, ...) \
	do { \
		char dump_buf_file[200] = { 0 }; \
		sprintf(dump_buf_file, FILENAME, __VA_ARGS__); \
		FILE * dump_buf = fopen(dump_buf_file, "w"); \
		size_t wrote_bytes = fwrite(BUF, SIZE, 1, dump_buf); \
		fclose(dump_buf); \
	} while (0);

#define dump_data(header, data, size) \
	do { \
		int i, last_startcode = 0, *n; \
		eprintf("%s: data = %016p, size = %u\n", header, data, size); \
		eprintf("  "); \
		for (i = 0;i < size; ++i) \
		{ \
			n = (int *)(data + i); \
			/* little endian on my x86-64 machine, */ \
			/* 0x01000000 == 0x00 0x00 0x00 0x01, 0x00010000 == 0x00 0x00 0x01 0x00 */ \
			if (i != 0 && i != 1 && (*n == 0x01000000 || (*n & 0x00ffffff) == 0x00010000)) \
			{ \
				printf("%s(%d bytes)\n", ((i > last_startcode + 100)?"... ":""), i - last_startcode); \
				if (*n == 0x01000000) \
				{ \
					eprintf("  00 00 00 01 "); \
					i += 4; \
				} else {\
					eprintf("  00 00 01 "); \
					i += 3; \
				} \
				last_startcode = i; \
			} \
			if (i < last_startcode + 100) \
				printf("%02x ", data[i]); \
		} \
		if (i > last_startcode + 100) \
			printf("... "); \
		printf("(%d bytes)\n", i - last_startcode); \
	} while (0);

#endif
