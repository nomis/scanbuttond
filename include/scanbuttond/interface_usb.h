// libscanbtnd-interface_usb.h: libusb wrapper
// This file is part of scanbuttond.
// Copyleft )c( 2004-2006 by Bernhard Stiftner
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

#ifndef __SCANBTND_LIBSCANBTND_INTERFACE_USB_H_INCLUDED
#define __SCANBTND_LIBSCANBTND_INTERFACE_USB_H_INCLUDED

#include <usb.h>


struct scanbtnd_libusb_device;
typedef struct scanbtnd_libusb_device scanbtnd_libusb_device_t;

struct scanbtnd_libusb_device {
	int vendorID;
	int productID;
	char* location; // bus number + ":" + device number
	struct usb_device* device;
	struct usb_dev_handle* handle; // automatically set by libusb_open(...)
	int interface;
	int out_endpoint;
	int in_endpoint;
	scanbtnd_libusb_device_t* next;
};

struct scanbtnd_libusb_handle;
typedef struct scanbtnd_libusb_handle scanbtnd_libusb_handle_t;

struct scanbtnd_libusb_handle {
	scanbtnd_libusb_device_t* devices;
	// rescanning info, timestamps???
};

scanbtnd_libusb_handle_t* scanbtnd_libusb_init(void);

// GLOBAL number of changed devices (does not require a handle!)
int scanbtnd_libusb_get_changed_device_count(void);

void scanbtnd_libusb_rescan(scanbtnd_libusb_handle_t* handle);

scanbtnd_libusb_device_t* scanbtnd_libusb_get_devices(scanbtnd_libusb_handle_t* handle);

// returns 0 on success, -EBUSY if the scanner is currently in use,
// or -ENODEV if the scanner does no longer exist
int scanbtnd_libusb_open(scanbtnd_libusb_device_t* device);

int scanbtnd_libusb_close(scanbtnd_libusb_device_t* device);

int scanbtnd_libusb_read(scanbtnd_libusb_device_t* device, void* buffer, int bytecount);

int scanbtnd_libusb_write(scanbtnd_libusb_device_t* device, void* buffer, int bytecount);

int scanbtnd_libusb_control_msg(scanbtnd_libusb_device_t* device, int requesttype,
	int request, int value, int index, void* bytes, int size);

void scanbtnd_libusb_exit(scanbtnd_libusb_handle_t* handle);

#endif
