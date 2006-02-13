// generic_backend.h: stuff common to all backends
// This file is part of scanbuttond.
// Copyleft )c( 2006 by Bernhard Stiftner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "scanbuttond/backend.h"
#include "scanbuttond/interface_usb.h"


#define GENERIC_GLOBALS \
scanbtnd_libusb_handle_t* libusb_handle;\
scanbtnd_scanner_t* scanners = NULL;


#define GENERIC_MATCH_LIBUSB_SCANNER_FUNC \
int scanbtnd_match_libusb_scanner(scanbtnd_libusb_device_t* device)\
{\
	int index;\
	for (index = 0; index < NUM_SUPPORTED_USB_DEVICES; index++) {\
		if (supported_usb_devices[index][0] == device->vendorID &&\
		    supported_usb_devices[index][1] == device->productID) {\
			break;\
		}\
	}\
	if (index >= NUM_SUPPORTED_USB_DEVICES) return -1;\
	return index;\
}

#define GENERIC_ATTACH_LIBUSB_SCANNER_FUNC(descriptorprefix) \
void scanbtnd_attach_libusb_scanner(scanbtnd_libusb_device_t* device)\
{\
	int index = scanbtnd_match_libusb_scanner(device);\
	if (index < 0) return; \
	scanbtnd_scanner_t* scanner = (scanbtnd_scanner_t*)malloc(sizeof(scanbtnd_scanner_t));\
	scanner->vendor = usb_device_descriptions[index][0];\
	scanner->product = usb_device_descriptions[index][1];\
	scanner->connection = CONNECTION_LIBUSB;\
	scanner->internal_dev_ptr = (void*)device;\
	scanner->lastbutton = 0;\
	scanner->sane_device = (char*)malloc(strlen(device->location) + \
		strlen(descriptorprefix) + 1);\
	strcpy(scanner->sane_device, descriptorprefix);\
	strcat(scanner->sane_device, device->location);\
	scanner->num_buttons = supported_usb_devices[index][2];\
	scanner->is_open = 0;\
	scanner->next = scanners;\
	scanners = scanner;\
}

#define GENERIC_DETACH_SCANNERS_FUNC \
void scanbtnd_detach_scanners(void)\
{\
	scanbtnd_scanner_t* next;\
	while (scanners != NULL) {\
		next = scanners->next;\
		free(scanners->sane_device);\
		free(scanners);\
		scanners = next;\
	}\
}

#define GENERIC_SCAN_DEVICES_FUNC \
void scanbtnd_scan_devices(scanbtnd_libusb_device_t* devices)\
{\
	int index;\
	scanbtnd_libusb_device_t* device = devices;\
	while (device != NULL) {\
		index = scanbtnd_match_libusb_scanner(device);\
		if (index >= 0)\
			scanbtnd_attach_libusb_scanner(device);\
		device = device->next;\
	}\
}

#define GENERIC_INIT_LIBUSB_FUNC \
int scanbtnd_init_libusb(void)\
{\
	scanbtnd_libusb_device_t* devices;\
\
	libusb_handle = scanbtnd_libusb_init();\
	devices = scanbtnd_libusb_get_devices(libusb_handle);\
	scanbtnd_scan_devices(devices);\
	return 0;\
}

#define GENERIC_GET_BACKEND_NAME_FUNC \
const char* scanbtnd_get_backend_name(void)\
{\
	return BACKEND_NAME;\
}

#define GENERIC_INIT_FUNC \
int scanbtnd_init(void)\
{\
	scanners = NULL;\
\
	return scanbtnd_init_libusb();\
}

#define GENERIC_RESCAN_FUNC \
int scanbtnd_rescan(void)\
{\
	scanbtnd_libusb_device_t *devices;\
\
	scanbtnd_detach_scanners();\
	scanners = NULL;\
	scanbtnd_libusb_rescan(libusb_handle);\
	devices = scanbtnd_libusb_get_devices(libusb_handle);\
	scanbtnd_scan_devices(devices);\
	return 0;\
}

#define GENERIC_GET_SUPPORTED_DEVICES_FUNC \
const scanbtnd_scanner_t* scanbtnd_get_supported_devices(void)\
{\
	return scanners;\
}

#define GENERIC_OPEN_FUNC \
int scanbtnd_open(scanbtnd_scanner_t* scanner)\
{\
	int result = -ENOSYS;\
	if (scanner->is_open)\
		return -EINVAL;\
	switch (scanner->connection) {\
		case CONNECTION_LIBUSB:\
			if (scanbtnd_libusb_get_changed_device_count() != 0)\
				return -ENODEV;\
			result = scanbtnd_libusb_open(\
				(scanbtnd_libusb_device_t*)scanner->internal_dev_ptr);\
			break;\
	}\
	if (result == 0)\
		scanner->is_open = 1;\
	return result;\
}

#define GENERIC_CLOSE_FUNC \
int scanbtnd_close(scanbtnd_scanner_t* scanner)\
{\
	int result = -ENOSYS;\
	if (!scanner->is_open)\
		return -EINVAL;\
	switch (scanner->connection) {\
		case CONNECTION_LIBUSB:\
			result = scanbtnd_libusb_close(\
				(scanbtnd_libusb_device_t*)scanner->internal_dev_ptr);\
			break;\
	}\
	if (result == 0)\
		scanner->is_open = 0;\
	return result;\
}

#define GENERIC_READ_FUNC \
int scanbtnd_read(scanbtnd_scanner_t* scanner, void* buffer, int bytecount)\
{\
	switch (scanner->connection) {\
		case CONNECTION_LIBUSB:\
			return scanbtnd_libusb_read(\
				(scanbtnd_libusb_device_t*)scanner->internal_dev_ptr, \
				buffer, bytecount);\
			break;\
	}\
	return -1;\
}

#define GENERIC_WRITE_FUNC \
int scanbtnd_write(scanbtnd_scanner_t* scanner, void* buffer, int bytecount)\
{\
	switch (scanner->connection) {\
		case CONNECTION_LIBUSB:\
			return scanbtnd_libusb_write(\
				(scanbtnd_libusb_device_t*)scanner->internal_dev_ptr, \
				buffer, bytecount);\
			break;\
	}\
	return -1;\
}

#define GENERIC_FLUSH_FUNC \
int scanbtnd_flush(scanbtnd_scanner_t* scanner)\
{\
	switch (scanner->connection) {\
		case CONNECTION_LIBUSB:\
			scanbtnd_libusb_flush((scanbtnd_libusb_device_t*)scanner->internal_dev_ptr);\
			break;\
	}\
}

#define GENERIC_GET_SANE_DEVICE_DESCRIPTOR_FUNC \
const char* scanbtnd_get_sane_device_descriptor(scanbtnd_scanner_t* scanner)\
{\
	return scanner->sane_device;\
}

#define GENERIC_EXIT_FUNC \
int scanbtnd_exit(void)\
{\
	scanbtnd_detach_scanners();\
	scanbtnd_libusb_exit(libusb_handle);\
	return 0;\
}
