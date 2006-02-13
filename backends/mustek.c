// mustek.c : scanbuttond backend for mustek/gt68xx devices
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
#include <syslog.h>
#include "mustek.h"
#include "scanbuttond/interface_usb.h"
#include "generic_backend.h"

#define BACKEND_NAME	"Mustek USB"

#define NUM_SUPPORTED_USB_DEVICES 1

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x055f, 0x0409, 5 } // Mustek BearPaw 2448TA
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Mustek", "BearPaw 2448TA"}
};


GENERIC_GLOBALS
GENERIC_MATCH_LIBUSB_SCANNER_FUNC
GENERIC_ATTACH_LIBUSB_SCANNER_FUNC("mustek:libusb:")
GENERIC_DETACH_SCANNERS_FUNC
GENERIC_SCAN_DEVICES_FUNC
GENERIC_INIT_LIBUSB_FUNC
GENERIC_GET_BACKEND_NAME_FUNC
GENERIC_INIT_FUNC
GENERIC_RESCAN_FUNC
GENERIC_GET_SUPPORTED_DEVICES_FUNC
GENERIC_OPEN_FUNC
GENERIC_CLOSE_FUNC
GENERIC_READ_FUNC
GENERIC_WRITE_FUNC
GENERIC_FLUSH_FUNC


int scanbtnd_get_button(scanbtnd_scanner_t* scanner)
{
	unsigned char bytes[255];
	int num_bytes;

	bytes[0] = 0x74; // check function key

	if (!scanner->is_open)
		return -EINVAL;

	num_bytes = scanbtnd_write(scanner, (void*)bytes, 1);
	if (num_bytes != 1)  {
		scanbtnd_flush(scanner);
		return 0;
	}
	num_bytes = scanbtnd_read(scanner, (void*)bytes, 4);
	if (num_bytes != 4)  {
		scanbtnd_flush(scanner);
		return 0;
	}
	switch (bytes[2]) {
	case 0x10: // scan button
		return 1;
	case 0x14: // copy
		return 2;
	case 0x12: // fax
		return 3;
	case 0x18: // email
		return 4;
	case 0x11: // panel
		return 5;
	}
	return 0;
}


GENERIC_GET_SANE_DEVICE_DESCRIPTOR_FUNC
GENERIC_EXIT_FUNC
