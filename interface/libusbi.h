// scanbuttond
// libusb interface
// Copyleft )c( 2004 by Bernhard Stiftner
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
// along with this program;a if not, write to the Free Software
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
	struct usb_device* device;
	struct usb_dev_handle* handle; // automatically set by libusb_open(...)
	int interface;
	int out_endpoint;
	int in_endpoint;
	usb_scanner* next;
};

int libusb_init(void);

void libusb_rescan(void);

usb_scanner* libusb_get_devices(void);

int libusb_open(usb_scanner* scanner);

int libusb_close(usb_scanner* scanner);

int libusb_read(usb_scanner* scanner, void* buffer, int bytecount);

int libusb_write(usb_scanner* scanner, void* buffer, int bytecount);

int libusb_exit(void);

#endif
