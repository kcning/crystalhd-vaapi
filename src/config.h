/* src/config.h.  Generated from config.h.in by configure.  */
/* src/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

#warning "not a warning: config.h included"

/* Major version of the driver */
#define CRYSTALHD_VIDEO_MAJOR_VERSION 0

/* Micro version of the driver */
#define CRYSTALHD_VIDEO_MICRO_VERSION 9

/* Minor version of the driver */
#define CRYSTALHD_VIDEO_MINOR_VERSION 6

/* Define to 1 if you have the `clock_gettime' function. */
#define HAVE_CLOCK_GETTIME 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <GL/glext.h> header file. */
#define HAVE_GL_GLEXT_H 1

/* Define to 1 if you have the <GL/glx.h> header file. */
#define HAVE_GL_GLX_H 1

/* Define to 1 if you have the <GL/gl.h> header file. */
#define HAVE_GL_GL_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `c_r' library (-lc_r). */
/* #undef HAVE_LIBC_R */

/* Define to 1 if you have the `pthread' library (-lpthread). */
#define HAVE_LIBPTHREAD 1

/* Define to 1 if you have the `PTL' library (-lPTL). */
/* #undef HAVE_LIBPTL */

/* Define to 1 if you have the `rt' library (-lrt). */
#define HAVE_LIBRT 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Defined if pthreads are available */
#define HAVE_PTHREADS 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to use the __attribute__((visibility())) extension */
#define HAVE_VISIBILITY_ATTRIBUTE 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "crystalhd-video"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "gbeauchesne@splitted-desktop.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "crystalhd_video"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "crystalhd_video 0.6.9"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "crystalhd-video"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.6.9"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable debugging information */
#define USE_DEBUG 1

/* Defined to 1 if GLX is enabled */
#define USE_GLX 1

/* Enable CRYSTALHD tracer */
#define USE_TRACER 1

/* Define driver entry-point */
#define VA_DRIVER_INIT_FUNC __vaDriverInit_0_31_0_sds6

/* Version number of package */
#define VERSION "0.6.9"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */
