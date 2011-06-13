SUBDIRS = src

user_install:
	if [ ! -d $(HOME)/.irssi/modules ]; then mkdir -p $(HOME)/.irssi/modules; fi
	cp -f src/core/.libs/libicb_core.so $(HOME)/.irssi/modules/
	cp -f src/fe-common/.libs/libfe_icb.so $(HOME)/.irssi/modules/
