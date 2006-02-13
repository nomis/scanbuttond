#
# Makefile for scanbuttond
# Copyleft )c( 2004-2006 by Bernhard Stiftner
#

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
includedir = /usr/include
libdir = ${exec_prefix}/lib/scanbuttond

CC = gcc
INCLUDES = -I. -I$(includedir)
CFLAGS = -g -O2 -Wall -I$(includedir) -I.
LD = ld
LDFLAGS =  -L$(libdir)
INSTALL = /usr/bin/install -c
REMOVE = rm -f
DISTFILES = Makefile scanbuttond.c

all: scanbuttond backends/libmeta.so.1.0 backends/libepson.so.1.0 \
backends/libplustek.so.1.0 backends/libplustek_umax.so.1.0 \
backends/libsnapscan.so.1.0 backends/libniash.so.1.0 backends/libmustek.so.1.0 \
interface/libusbi.so.1.0

install: scanbuttond backends/libepson.so.1.0 backends/libplustek.so.1.0 \
backends/libsnapscan.so.1.0 backends/libniash.so.1.0 backends/libmeta.so.1.0 \
backends/libmustek.so.1.0 interface/libusbi.so.1.0
	$(INSTALL) scanbuttond $(DESTDIR)$(bindir)/scanbuttond
	mkdir -p $(libdir)
	$(INSTALL) interface/libusbi.so.1.0 $(DESTDIR)$(libdir)/libusbi.so.1.0
	$(INSTALL) backends/libepson.so.1.0 $(DESTDIR)$(libdir)/libepson.so.1.0
	$(INSTALL) backends/libplustek.so.1.0 $(DESTDIR)$(libdir)/libplustek.so.1.0
	$(INSTALL) backends/libplustek_umax.so.1.0 $(DESTDIR)$(libdir)/libplustek_umax.so.1.0
	$(INSTALL) backends/libsnapscan.so.1.0 $(DESTDIR)$(libdir)/libsnapscan.so.1.0
	$(INSTALL) backends/libniash.so.1.0 $(DESTDIR)$(libdir)/libniash.so.1.0
	$(INSTALL) backends/libmustek.so.1.0 $(DESTDIR)$(libdir)/libmustek.so.1.0
	$(INSTALL) backends/libmeta.so.1.0 $(DESTDIR)$(libdir)/libmeta.so.1.0
	/sbin/ldconfig $(DESTDIR)$(libdir)
	ln -sf libusbi.so.1 $(DESTDIR)$(libdir)/libusbi.so
	ln -sf libepson.so.1 $(DESTDIR)$(libdir)/libepson.so
	ln -sf libplustek.so.1 $(DESTDIR)$(libdir)/libplustek.so
	ln -sf libplustek_umax.so.1 $(DESTDIR)$(libdir)/libplustek_umax.so
	ln -sf libsnapscan.so.1 $(DESTDIR)$(libdir)/libsnapscan.so
	ln -sf libniash.so.1 $(DESTDIR)$(libdir)/libniash.so
	ln -sf libmustek.so.1 $(DESTDIR)$(libdir)/libmustek.so
	ln -sf libmeta.so.1 $(DESTDIR)$(libdir)/libmeta.so
	if [ ! -d /etc/scanbuttond ]; then mkdir /etc/scanbuttond; fi
	if [ ! -f /etc/scanbuttond/buttonpressed.sh ]; then cp buttonpressed.sh /etc/scanbuttond; fi
	if [ ! -f /etc/scanbuttond/initscanner.sh ]; then cp initscanner.sh /etc/scanbuttond; fi
	if [ ! -f /etc/scanbuttond/meta.conf ]; then $(INSTALL) backends/meta.conf /etc/scanbuttond/meta.conf; fi

.c.o:
	$(CC) -c $(CFLAGS) $<

interface/libusbi.so.1.0: interface/libusbi.h interface/libusbi.c
	$(CC) $(CFLAGS) -c -fPIC $(CFLAGS) interface/libusbi.c -o interface/libusbi.o
	$(CC) -rdynamic -shared -Wl,-soname,libusbi.so.1 -o interface/libusbi.so.1.0 interface/libusbi.o -lusb
	/sbin/ldconfig -n ./interface
	ln -sf libusbi.so.1 interface/libusbi.so

backends/libepson.so.1.0: interface/libusbi.so.1.0 backends/epson.c backends/epson.h backends/backend.h
	$(CC) -c -fPIC $(CFLAGS) backends/epson.c -o backends/epson.o
	$(CC) -shared -L./interface -Wl,-soname,libepson.so.1 -Wl,-rpath,./interface:$(libdir) -o backends/libepson.so.1.0 backends/epson.o -lusbi
	/sbin/ldconfig -n ./backends
	ln -sf libepson.so.1 backends/libepson.so

backends/libplustek.so.1.0: interface/libusbi.so.1.0 backends/plustek.c backends/plustek.h backends/backend.h
	$(CC) -c -fPIC $(CFLAGS) backends/plustek.c -o backends/plustek.o
	$(CC) -shared -L./interface -Wl,-soname,libplustek.so.1 -Wl,-rpath,./interface:$(libdir)  -o backends/libplustek.so.1.0 backends/plustek.o -lusbi
	/sbin/ldconfig -n ./backends
	ln -sf libplustek.so.1 backends/libplustek.so

backends/libplustek_umax.so.1.0: interface/libusbi.so.1.0 backends/plustek_umax.c backends/plustek_umax.h backends/backend.h
	$(CC) -c -fPIC $(CFLAGS) backends/plustek_umax.c -o backends/plustek_umax.o
	$(CC) -shared -L./interface -Wl,-soname,libplustek_umax.so.1 -Wl,-rpath,./interface:$(libdir)  -o backends/libplustek_umax.so.1.0 backends/plustek_umax.o -lusbi
	/sbin/ldconfig -n ./backends
	ln -sf libplustek_umax.so.1 backends/libplustek_umax.so

backends/libsnapscan.so.1.0: interface/libusbi.so.1.0 backends/snapscan.c backends/snapscan.h backends/backend.h
	$(CC) -c -fPIC $(CFLAGS) backends/snapscan.c -o backends/snapscan.o
	$(CC) -shared -L./interface -Wl,-soname,libsnapscan.so.1 -Wl,-rpath,./interface:$(libdir) -o backends/libsnapscan.so.1.0 backends/snapscan.o -lusbi
	/sbin/ldconfig -n ./backends
	ln -sf libsnapscan.so.1 backends/libsnapscan.so

backends/libniash.so.1.0: interface/libusbi.so.1.0 backends/niash.c backends/niash.h backends/backend.h
	$(CC) -c -fPIC $(CFLAGS) backends/niash.c -o backends/niash.o
	$(CC) -shared -L./interface -Wl,-soname,libniash.so.1 -Wl,-rpath,./interface:$(libdir) -o backends/libniash.so.1.0 backends/niash.o -lusbi
	/sbin/ldconfig -n ./backends
	ln -sf libniash.so.1 backends/libniash.so

backends/libmustek.so.1.0: interface/libusbi.so.1.0 backends/mustek.c backends/mustek.h backends/backend.h
	$(CC) -c -fPIC $(CFLAGS) backends/mustek.c -o backends/mustek.o
	$(CC) -shared -L./interface -Wl,-soname,libmustek.so.1 -Wl,-rpath,./interface:$(libdir) -o backends/libmustek.so.1.0 backends/mustek.o -lusbi
	/sbin/ldconfig -n ./backends
	ln -sf libmustek.so.1 backends/libmustek.so

backends/libmeta.so.1.0: interface/libusbi.so.1.0 lib/loader.o backends/meta.c backends/meta.h backends/backend.h
	$(CC) -c -fPIC $(CFLAGS) backends/meta.c -o backends/meta.o
	$(CC) -shared -L./interface -Wl,-soname,libmeta.so.1 -Wl,-rpath,./interface:$(libdir) -o backends/libmeta.so.1.0 backends/meta.o lib/loader.o -ldl -lusbi
	/sbin/ldconfig -n ./backends
	ln -sf libmeta.so.1 backends/libmeta.so

lib/loader.o: lib/loader.c lib/loader.h
	$(CC) -c -fPIC $(CFLAGS) -o lib/loader.o lib/loader.c

scanbuttond: scanbuttond.o lib/loader.o
	$(CC) -rdynamic -o scanbuttond scanbuttond.o lib/loader.o -ldl

clean:
	$(REMOVE) scanbuttond
	$(REMOVE) scanbuttond.o
	$(REMOVE) backends/epson.o
	$(REMOVE) backends/plustek.o
	$(REMOVE) backends/plustek_umax.so
	$(REMOVE) backends/snapscan.o
	$(REMOVE) backends/niash.o
	$(REMOVE) backends/meta.o
	$(REMOVE) backends/libepson.so*
	$(REMOVE) backends/libplustek.so*
	$(REMOVE) backends/libplustek_umax.so*
	$(REMOVE) backends/libsnapscan.so*
	$(REMOVE) backends/libniash.so*
	$(REMOVE) backends/libmustek.so*
	$(REMOVE) backends/libmeta.so*
	$(REMOVE) interface/libusbi.o
	$(REMOVE) interface/libusbi.so*
	$(REMOVE) lib/loader.o

