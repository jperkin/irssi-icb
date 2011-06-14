moduledir = $(libdir)/irssi/modules

module_LTLIBRARIES = libicb_core.la
libicb_core_la_LDFLAGS = -module -avoid-version

DISTCLEANFILES = Makefile.in

AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(IRSSI_INCLUDE) -I$(IRSSI_INCLUDE)/src \
	-I$(IRSSI_INCLUDE)/src/core

libicb_core_la_SOURCES = \
	icb-channels.c \
	icb-commands.c \
	icb-core.c \
	icb-nicklist.c \
	icb-queries.c \
	icb-servers-reconnect.c \
	icb-protocol.c \
	icb-servers.c \
	icb-session.c

noinst_HEADERS = \
	icb.h \
	icb-channels.h \
	icb-commands.h \
	icb-nicklist.h \
	icb-protocol.h \
	icb-queries.h \
	icb-servers.h \
	module.h
