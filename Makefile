#
# Makefile for scanbuttond
# Copyleft )c( 2004, 2005 by Bernhard Stiftner
#

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
includedir = /usr/include
libdir = ${exec_prefix}/lib/scanbuttond
confdir = /etc/scanbuttond

CC = gcc
INCLUDES = -I. -I$(includedir)
CFLAGS = -g -O2 -W -I$(includedir) -I.
LD = ld
LDFLAGS =  -L$(libdir) -lusb
INSTALL = /bin/install -c
REMOVE = rm -f
SYMLINK = ln -sf
MKDIR = mkdir -p
LDCONFIG = /sbin/ldconfig
DISTFILES = Makefile COPYING README \
            buttonpressed.sh scanbuttond.c scanbuttond.h \
	    backends/backend.h \
            backends/epson.h backends/epson.c \
	    backends/meta.h backends/meta.c \
	    interface/libusbi.h interface/libusbi.c

all: scanbuttond backends/libscanbtnd-epson.so.1.0 backends/libscanbtnd-meta.so.1.0

install: scanbuttond backends/libscanbtnd-epson.so.1.0 backends/libscanbtnd-meta.so.1.0
	$(INSTALL) scanbuttond $(DESTDIR)$(bindir)/scanbuttond
	$(MKDIR) $(libdir)
	$(INSTALL) backends/libscanbtnd-epson.so.1.0 $(DESTDIR)$(libdir)/libscanbtnd-epson.so.1.0
	$(INSTALL) backends/libscanbtnd-meta.so.1.0 $(DESTDIR)$(libdir)/libscanbtnd-meta.so.1.0
	$(LDCONFIG) $(DESTDIR)$(libdir)
	$(SYMLINK) libscanbtnd-epson.so.1 $(DESTDIR)$(libdir)/libscanbtnd-epson.so
	$(SYMLINK) libscanbtnd-meta.so.1 $(DESTDIR)$(libdir)/libscanbtnd-meta.so
	$(MKDIR) $(confdir)
	if [ ! -f $(confdir)/buttonpressed.sh ]; then $(INSTALL) buttonpressed.sh $(confdir); fi
	if [ ! -f $(confdir)/meta.conf ]; then $(INSTALL) backends/meta.conf $(confdir); fi

interface/libusbi.o: interface/libusbi.c
	$(CC) $(CFLAGS) -c -fPIC $(CFLAGS) interface/libusbi.c -o interface/libusbi.o
	
backends/epson.o: backends/epson.c
	$(CC) $(CFLAGS) -c -fPIC $(CFLAGS) backends/epson.c -o backends/epson.o

backends/meta.o: backends/meta.c
	$(CC) $(CFLAGS) -c -fPIC $(CFLAGS) backends/meta.c -o backends/meta.o
		
backends/libscanbtnd-epson.so.1.0: backends/epson.o interface/libusbi.o
	$(CC) -shared -Wl,-soname,libscanbtnd-epson.so.1 -o backends/libscanbtnd-epson.so.1.0 backends/epson.o interface/libusbi.o -lusb
	$(LDCONFIG) -n ./backends
	$(SYMLINK) libscanbtnd-epson.so.1 backends/libscanbtnd-epson.so

backends/libscanbtnd-meta.so.1.0: backends/meta.o
	$(CC) -rdynamic -shared -Wl,-soname,libscanbtnd-meta.so.1 -o backends/libscanbtnd-meta.so.1.0 backends/meta.o -ldl
	$(LDCONFIG) -n ./backends
	$(SYMLINK) libscanbtnd-meta.so.1 backends/libscanbtnd-meta.so

scanbuttond: scanbuttond.o backends/libscanbtnd-epson.so.1.0 backends/libscanbtnd-meta.so.1.0
	$(CC) $(CFLAGS) -L./backends -Wl,-rpath,./backends:$(libdir) -lscanbtnd-meta -o scanbuttond scanbuttond.o
	
.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	$(REMOVE) scanbuttond
	$(REMOVE) scanbuttond.o
	$(REMOVE) backends/epson.o
	$(REMOVE) backends/meta.o
	$(REMOVE) backends/libscanbtnd*
	$(REMOVE) interface/libusbi.o

