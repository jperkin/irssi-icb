moduledir = $(libdir)/irssi/modules

module_LTLIBRARIES = libfe_icb.la
libfe_icb_la_LDFLAGS = -module -avoid-version

DISTCLEANFILES = Makefile.in

AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(IRSSI_INCLUDE) -I$(IRSSI_INCLUDE)/src \
	-I$(IRSSI_INCLUDE)/src/core -I$(IRSSI_INCLUDE)/src/fe-common/core \
	-I$(top_srcdir)/src/core

libfe_icb_la_SOURCES = \
	fe-icb.c \
	module-formats.c

noinst_HEADERS = \
	module.h \
	module-formats.h
