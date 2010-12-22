DRIVERS  =

INCLUDES = \
	$(CRYSTALHD_VIDEO_CFLAGS) -std=c99

LDADD = \
	$(CRYSTALHD_VIDEO_LT_LDFLAGS) -ldl -module \
	-no-undefined -module -Wl,--no-undefined

if USE_GLX
source_glx_h = crystalhd_video_glx.h
source_glx_c = crystalhd_video_glx.c
endif

source_x11_h = crystalhd_video_x11.h
source_x11_c = crystalhd_video_x11.c

source_h =			\
	bitstream.h		\
	debug.h			\
	crystalhd_video_mpeg2.h	\
	crystalhd_video_h264.h	\
	object_heap.h		\
	queue.h			\
	crystalhd_drv_video.h	\
	x264_encoder_set.h	\
	$(source_glx_h)		\
	$(source_x11_h)

source_c =			\
	crystalhd_drv_video.c	\
	crystalhd_video_mpeg2.c	\
	crystalhd_video_h264.c	\
	x264_encoder_set.c	\
	object_heap.c		\
	queue.c			\
	$(source_glx_c)		\
	$(source_x11_c)

crystalhd_drv_video_la_LTLIBRARIES	= crystalhd_drv_video.la
crystalhd_drv_video_ladir		= @LIBVA_DRIVERS_PATH@
crystalhd_drv_video_la_SOURCES		= $(source_c)
crystalhd_drv_video_la_LIBADD		= $(CRYSTALHD_VIDEO_LIBS) -lX11
crystalhd_drv_video_la_LDFLAGS		= $(LDADD)

noinst_HEADERS = $(source_h)

EXTRA_DIST = \
	$(source_glx_c) \
	$(source_glx_h)	\
	$(source_x11_c)	\
	$(source_x11_h)

install-data-hook:
	cd $(DESTDIR)$(LIBVA_DRIVERS_PATH) ;			\
	for drv in $(DRIVERS); do				\
	    ln -s crystalhd_drv_video.so $${drv}_drv_video.so;	\
	done

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in config.h.in
