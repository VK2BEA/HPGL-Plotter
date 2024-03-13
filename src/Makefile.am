MAINTAINERCLEANFILES = Makefile.in

if DEBUG
	AM_CFLAGS = -g3 -O0
	AM_CXXFLAGS = -g3 -O0
else
	AM_CFLAGS = -O2
	AM_CXXFLAGS = -O2
endif

HPGLplotter_CPPFLAGS = "-I$(top_srcdir)/include"
HPGLplotter_CPPFLAGS += @GLIB_CFLAGS@ @GTK4_CFLAGS@

HPGLplotter_CFLAGS = $(AM_CFLAGS)
hp8753_CXXFLAGS = $(AM_CXXFLAGS)

HPGLplotter_LDFLAGS = -lgpib -lm -lgs -rdynamic
HPGLplotter_LDFLAGS += @GLIB_LIBS@ @GTK4_LIBS@

#
# bin program
#

# Program name
bin_PROGRAMS = HPGLplotter

HPGLplotter_SOURCES = CairoPlot.c  GPIBcommsThread.c \
				 GTKcallbacks.c GTKcallbacksOptions.c \
                 HPGLplotter.c HPGLplotter-GTK4.c \
                 HPlogo.c messageEvent.c \
                 parseHPGL.c PDF+SVG+PNGwidgetCallback.c \
                 printWidgetCallbacks.c settings.c \
                 utility.c

HPGLplotter_SOURCES += $(top_srcdir)/include/GPIBcomms.h \
				  $(top_srcdir)/include/HPGLplotter.h \
				  $(top_srcdir)/include/messageEvent.h

