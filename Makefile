.POSIX:
DESTDIR ?= /usr/local
sxmag: sxmag.c config.h
	$(CC) $(CFLAGS) -lX11 sxmag.c -o sxmag

install: sxmag
	cp -- sxmag $(DESTDIR)/bin/

