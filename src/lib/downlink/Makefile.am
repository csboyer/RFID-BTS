###################################
# Copyright 2010 Szymon Jakubczak
#
# modulation and FEC for RawOFDM

include $(top_srcdir)/Makefile.common

modinclude_HEADERS = \
        rfidbts_pie_encoder.h

noinst_LTLIBRARIES = librfidbts-downlink.la

librfidbts_downlink_la_SOURCES =  \
        rfidbts_pie_encoder.cc

librfidbts_downlink_la_LIBADD = \
        $(GNURADIO_CORE_LA)

