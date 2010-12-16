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
#define FUNC_WRAPPER(FUNCTION, RET_T, ARG_LIST, ARGS_T, ARGS, format, ...) \
	RET_T FUNCTION(LIST(ARG_LIST)) { \
		pthread_mutex_lock( &mutex_func ); \
		enter(format, ARGS); \
		if (_indent_level < 30) \
			++_indent_level; \
		pthread_mutex_unlock( &mutex_func ); \
		RET_T (*func)(ARGS_T); \
		* (void **) (&func) = dlsym(__dlopen_crystalhd_so(), __func__); \
		RET_T ret = func(ARGS); \
		pthread_mutex_lock( &mutex_func ); \
		if (_indent_level > 0) \
			--_indent_level; \
		leave(ret); \
		__VA_ARGS__; \
		pthread_mutex_unlock( &mutex_func ); \
		return ret; \
	}

#define dump_data(header, data, size) \
	do { \
		int i; \
		eprintf("%s: data = %p, size = %u\n", header, data, size); \
		eprintf("  "); \
		for (i = 0;i < MIN(size, 100);++i) \
		{ \
			if (i && (i+3 < MIN(size, 100))) \
			{ \
				if (i && data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) \
				{ \
					printf("(%d)\n", i); \
					eprintf("  "); \
				} \
			} \
			if (i && (i+2 < MIN(size, 100))) \
			{ \
				if (i && data[i-1] != 0x00 && data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01) \
				{ \
					printf("(%d)\n", i); \
					eprintf("  "); \
				} \
			} \
			printf("%02x ", data[i]); \
		} \
		if (size > 100) \
			printf("(%d) ... ", i); \
		else \
			printf("(%d) ", i); \
		printf("\b\n"); \
	} while (0);

#endif
