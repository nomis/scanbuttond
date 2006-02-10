// snapscan.c: Snapscan device backend
// This file is part of scanbuttond.
// Copyleft )c( 2005-2006 by Bernhard Stiftner
// Thanks to J. Javier Maestro for sniffing the button codes ;-)
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
#include "plustek.h"
#include "scanbuttond/interface_usb.h"
#include "generic_backend.h"

#define BACKEND_NAME	"Snapscan USB"

#define NUM_SUPPORTED_USB_DEVICES 3

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	{ 0x04b8, 0x0121, 4 },	// Epson Perfection 2480
	{ 0x04b8, 0x011f, 4 },	// Epson Perfection 1670
	{ 0x04b8, 0x0122, 4 }	// Epson Perfection 3490
};

// TODO: check if this backend really works on the Epson 2580 too...
static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	   { "Epson", "Perfection 2480 / 2580" },
	   { "Epson", "Perfection 1670" },
	   { "Epson", "Perfection 3490" }
};


GENERIC_GLOBALS
GENERIC_MATCH_LIBUSB_SCANNER_FUNC
GENERIC_ATTACH_LIBUSB_SCANNER_FUNC("snapscan:libusb:")
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


int scanbtnd_get_button(scanbtnd_scanner_t* scanner)
{
	unsigned char bytes[255];
	int num_bytes;
	int button;

	bytes[0] = 0x03;
	bytes[1] = 0x00;
	bytes[2] = 0x00;
	bytes[3] = 0x00;
	bytes[4] = 0x14;
	bytes[5] = 0x00;
	num_bytes = scanbtnd_write(scanner, (void*)bytes, 6);
	if (num_bytes != 6) return 0;

	num_bytes = scanbtnd_read(scanner, (void*)bytes, 8);
	if (num_bytes != 8 || bytes[0] != 0xF9) return 0;

	num_bytes = scanbtnd_read(scanner, (void*)bytes, 20);
	if (num_bytes != 20 || bytes[0] != 0xF0) return 0;
	switch (bytes[18] & 0xF0) {
		case 0x10: button = 1; break;
		case 0x20: button = 2; break;
		case 0x40: button = 3; break;
		case 0x80: button = 4; break;
		default: button = 0; break;
	};

	num_bytes = scanbtnd_read(scanner, (void*)bytes, 8);
	if (num_bytes != 8 || bytes[0] != 0xFB) return 0;

	return button;
}


GENERIC_GET_SANE_DEVICE_DESCRIPTOR_FUNC
GENERIC_EXIT_FUNC
