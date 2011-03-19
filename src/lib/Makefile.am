###################################
# Copyright 2010 Szymon Jakubczak
#
# RawOFDM

include $(top_srcdir)/Makefile.common

SUBDIRS = downlink
DIST_SUBDIRS = $(SUBDIRS)

lib_LTLIBRARIES = libgnuradio-rfidbts.la

libgnuradio_rfidbts_la_SOURCES =

libgnuradio_rfidbts_la_LIBADD = \
        downlink/librfidbts-downlink.la 

if PYTHON
###################################
# SWIG Python interface and library

TOP_SWIG_IFILES = \
        rfidbts.i

rfidbts_la_swig_libadd = \
        libgnuradio-rfidbts.la

include $(top_srcdir)/Makefile.swig

# add some of the variables generated inside the Makefile.swig.gen
BUILT_SOURCES = $(swig_built_sources)

# Do not distribute the output of SWIG
no_dist_files = $(swig_built_sources)

# additional SWIG files to be installed
# FIXME: they are installed in swig/ without subdirectories!
rfidbts_swiginclude_headers = \
        downlink/rfidbts_downlink.i 

endif
