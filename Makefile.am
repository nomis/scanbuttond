SUBDIRS = interface backends scripts .

pkgsysconfdir = $(sysconfdir)/$(PACKAGE)

bin_PROGRAMS = scanbuttond
scanbuttond_SOURCES = scanbuttond.c lib/loader.c
scanbuttond_LDFLAGS = -ldl

INCLUDES = $(all_includes) -I$(top_builddir)/include -I.
