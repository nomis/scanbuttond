lib_LTLIBRARIES = 	libscanbtnd-backend_epson.la \
			libscanbtnd-backend_meta.la \
			libscanbtnd-backend_mustek.la \
			libscanbtnd-backend_niash.la \
			libscanbtnd-backend_plustek.la \
			libscanbtnd-backend_plustek_umax.la \
			libscanbtnd-backend_snapscan.la

libscanbtnd_backend_epson_la_SOURCES = epson.c
libscanbtnd_backend_epson_la_LIBADD = ../interface/libscanbtnd-interface_usb.la
libscanbtnd_backend_epson_la_LDFLAGS = -version-info 1:0:0
libscanbtnd_backend_meta_la_SOURCES = meta.c ../lib/loader.c
libscanbtnd_backend_meta_la_LIBADD = ../interface/libscanbtnd-interface_usb.la
libscanbtnd_backend_meta_la_LDFLAGS = -version-info 1:0:0 -ldl
libscanbtnd_backend_mustek_la_SOURCES = mustek.c
libscanbtnd_backend_mustek_la_LIBADD = ../interface/libscanbtnd-interface_usb.la
libscanbtnd_backend_mustek_la_LDFLAGS = -version-info 1:0:0
libscanbtnd_backend_niash_la_SOURCES = niash.c
libscanbtnd_backend_niash_la_LIBADD = ../interface/libscanbtnd-interface_usb.la
libscanbtnd_backend_niash_la_LDFLAGS = -version-info 1:0:0
libscanbtnd_backend_plustek_la_SOURCES = plustek.c
libscanbtnd_backend_plustek_la_LIBADD = ../interface/libscanbtnd-interface_usb.la
libscanbtnd_backend_plustek_la_LDFLAGS = -version-info 1:0:0
libscanbtnd_backend_plustek_umax_la_SOURCES = plustek_umax.c
libscanbtnd_backend_plustek_umax_la_LIBADD = ../interface/libscanbtnd-interface_usb.la
libscanbtnd_backend_plustek_umax_la_LDFLAGS = -version-info 1:0:0
libscanbtnd_backend_snapscan_la_SOURCES = snapscan.c
libscanbtnd_backend_snapscan_la_LIBADD = ../interface/libscanbtnd-interface_usb.la
libscanbtnd_backend_snapscan_la_LDFLAGS = -version-info 1:0:0

pkgsysconfdir = $(sysconfdir)/$(PACKAGE)
pkgsysconf_DATA = meta.conf

INCLUDES = $(all_includes) -I$(top_builddir)/include -I.
