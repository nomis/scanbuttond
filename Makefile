#
# Makefile for scanbuttond
# Copyleft )c( 2004 by Bernhard Stiftner
#

prefix = /usr/local
exec_prefix = ${prefix}
sbindir = ${exec_prefix}/sbin
includedir = /usr/include
libdir = /usr/lib

CC = gcc
INCLUDES = -I. -I$(includedir)
CFLAGS = -g -O2 -W -I$(includedir)
LD = ld
LDFLAGS =  -L$(libdir) -lusb
INSTALL = /bin/install -c
REMOVE = rm -f
DISTFILES = Makefile scanbuttond.c

all: scanbuttond

install: scanbuttond epson.so
	$(INSTALL) scanbuttond $(DESTDIR)$(sbindir)/scanbuttond

.c.o:
	$(CC) -c $(CFLAGS) $<

epson.so:
	$(CC) -c -fPIC libusbi.c -o libusbi.o
	$(CC) -c -fPIC epson.c -o epson.o
	$(CC) -shared -Wl,-soname,libepson.so -o libepson.so epson.o libusbi.o -lusb

scanbuttond: scanbuttond.o
	$(CC) $(LDFLAGS) -o scanbuttond scanbuttond.o

clean:
	$(REMOVE) scanbuttond
	$(REMOVE) scanbuttond.o

