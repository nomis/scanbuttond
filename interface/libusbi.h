// scanbuttond
// libusb interface
// Copyleft )c( 2004-2005 by Bernhard Stiftner
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

#ifndef __LIBUSBI_H_INCLUDED
#define __LIBUSBI_H_INCLUDED

#include <usb.h>
#include "scanbuttond.h"

struct usb_scanner;
typedef struct usb_scanner usb_scanner;

struct usb_scanner {
	int vendorID;
	int productID;
	char* location; // bus number + ":" + device number
	struct usb_device* device;
	struct usb_dev_handle* handle; // automatically set by libusb_open(...)
	int interface;
	int out_endpoint;
	int in_endpoint;
	usb_scanner* next;
};

int libusb_init(void);

int libusb_get_changed_device_count(void);

void libusb_rescan(void);

usb_scanner* libusb_get_devices(void);

// returns 0 on success, -EBUSY if the scanner is currently in use, 
// or -ENODEV if the scanner does no longer exist
int libusb_open(usb_scanner* scanner);

int libusb_close(usb_scanner* scanner);

int libusb_read(usb_scanner* scanner, void* buffer, int bytecount);

int libusb_write(usb_scanner* scanner, void* buffer, int bytecount);

int libusb_control_msg(usb_scanner* scanner, int requesttype, int request, int value, int index, void* bytes, int size);

int libusb_exit(void);

#endif
