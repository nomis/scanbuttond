#
# Makefile for scanbuttond
# Copyleft )c( 2004 by Bernhard Stiftner
#

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
includedir = /usr/include
libdir = ${exec_prefix}/lib/scanbtnd

CC = gcc
INCLUDES = -I. -I$(includedir)
CFLAGS = -g -O2 -W -I$(includedir) -I.
LD = ld
LDFLAGS =  -L$(libdir) -lusb
INSTALL = /bin/install -c
REMOVE = rm -f
DISTFILES = Makefile scanbuttond.c

all: scanbuttond

install: scanbuttond backends/libepson.so.1.0
	$(INSTALL) scanbuttond $(DESTDIR)$(bindir)/scanbuttond
	mkdir -p $(libdir)
	$(INSTALL) backends/libepson.so.1.0 $(DESTDIR)$(libdir)/libepson.so.1.0
	/sbin/ldconfig $(DESTDIR)$(libdir)
	ln -sf libepson.so.1 $(DESTDIR)$(libdir)/libepson.so
	if [ ! -f /root/buttonpressed.sh ]; then cp buttonpressed.sh /root; fi

.c.o:
	$(CC) -c $(CFLAGS) $<

backends/libepson.so.1.0:
	$(CC) -c -fPIC $(CFLAGS) interface/libusbi.c -o interface/libusbi.o
	$(CC) -c -fPIC $(CFLAGS) backends/epson.c -o backends/epson.o
	$(CC) -shared -Wl,-soname,libepson.so.1 -o backends/libepson.so.1.0 backends/epson.o interface/libusbi.o -lusb
	/sbin/ldconfig -n ./backends
	ln -sf libepson.so.1 backends/libepson.so

scanbuttond: scanbuttond.o backends/libepson.so.1.0
	$(CC) -L./backends -Wl,-rpath,./backends:$(libdir) -lepson -o scanbuttond scanbuttond.o

clean:
	$(REMOVE) scanbuttond
	$(REMOVE) scanbuttond.o
	$(REMOVE) backends/epson.o
	$(REMOVE) backends/libepson.so*
	$(REMOVE) interface/libusbi.o

