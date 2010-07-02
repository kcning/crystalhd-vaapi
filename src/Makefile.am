DRIVERS  = crystalhd

INCLUDES = \
	$(CRYSTALHD_VIDEO_CFLAGS)

LDADD = \
	$(CRYSTALHD_VIDEO_LT_LDFLAGS) -module \
	-no-undefined -module -Wl,--no-undefined

if USE_GLX
source_glx_h = crystalhd_video_glx.h
source_glx_c = crystalhd_video_glx.c
endif

source_x11_h = crystalhd_video_x11.h
source_x11_c = crystalhd_video_x11.c

source_h = \
	object_heap.h	\
	crystalhd_video.h \
	$(source_glx_h)	\
	$(source_x11_h)

source_c = \
	object_heap.c	\
	crystalhd_video.c	\
	$(source_glx_c)	\
	$(source_x11_c)

crystalhd_drv_video_la_LTLIBRARIES	= crystalhd_drv_video.la
crystalhd_drv_video_ladir		= @LIBVA_DRIVERS_PATH@
crystalhd_drv_video_la_SOURCES	= $(source_c)
crystalhd_drv_video_la_LIBADD	= $(CRYSTALHD_VIDEO_LIBS) -lX11
crystalhd_drv_video_la_LDFLAGS	= $(LDADD)

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
