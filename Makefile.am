SUBDIRS = src

DISTCLEANFILES  = Makefile.in aclocal.m4 config.guess config.h.in config.sub
DISTCLEANFILES += configure depcomp install-sh ltmain.sh missing

distclean-local:
	-rm -rf autom4te.cache

dist-hook:
	cd ${top_srcdir} && \
	    git log --pretty=medium | grep -v '^commit' >${distdir}/ChangeLog

user_install:
	if [ ! -d $(HOME)/.irssi/modules ]; then \
		mkdir -p $(HOME)/.irssi/modules; \
	fi
	cp -f src/core/.libs/libicb_core.so $(HOME)/.irssi/modules/
	cp -f src/fe-common/.libs/libfe_icb.so $(HOME)/.irssi/modules/
