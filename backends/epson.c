// epson.c: Epson ESC/I device backend
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
//
// Thanks to:
//  - James Gilliland (Epson CX3200 support)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include "epson.h"
#include "scanbuttond/interface_usb.h"
#include "generic_backend.h"

#define BACKEND_NAME	"Epson USB"
#define	ESC        	0x1B		/* ASCII value for ESC */


#define NUM_SUPPORTED_USB_DEVICES 14

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x04B8, 0x0107, 1 },	// Epson Expression 1600
	{ 0x04B8, 0x010E, 1 },	// Epson Expression 1680
	{ 0x04B8, 0x0103, 1 },	// Epson Perfection 610
	{ 0x04B8, 0x0101, 3 },	// Epson Perfection 636U
	{ 0x04B8, 0x010C, 3 },	// Epson Perfection 640
	{ 0x04B8, 0x0104, 1 },	// Epson Perfection 1200U
	{ 0x04B8, 0x010B, 3 },	// Epson Perfection 1240
	{ 0x04B8, 0x010A, 1 },	// Epson Perfection 1640
	{ 0x04B8, 0x0110, 4 },	// Epson Perfection 1650
	{ 0x04B8, 0x011E, 4 },	// Epson Perfection 1660
	{ 0x04B8, 0x011B, 4 },	// Epson Perfection 2400
	{ 0x04B8, 0x0112, 1 },	// Epson Perfection 2450
	{ 0x04B8, 0x011C, 1 },	// Epson Perfection 3200
	{ 0x04B8, 0x0802, 1 }	// Epson CX3200 (note: is the button number
				// really correct?)
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Epson", "Expression 1600"},
	{ "Epson", "Expression 1680"},
	{ "Epson", "Perfection 610"},
	{ "Epson", "Perfection 636U"},
	{ "Epson", "Perfection 640"},
	{ "Epson", "Perfection 1200U"},
	{ "Epson", "Perfection 1240"},
	{ "Epson", "Perfection 1640"},
	{ "Epson", "Perfection 1650"},
	{ "Epson", "Perfection 1660"},
	{ "Epson", "Perfection 2400"},
	{ "Epson", "Perfection 2450"},
	{ "Epson", "Perfection 3200"},
	{ "Epson", "Stylus CX3200"}
};


GENERIC_GLOBALS
GENERIC_MATCH_LIBUSB_SCANNER_FUNC
GENERIC_ATTACH_LIBUSB_SCANNER_FUNC("epson:libusb:")
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
	int rcv_len;
	int num_bytes;

	bytes[0] = ESC;
	bytes[1] = '!';
	bytes[2] = '\0';

	if (!scanner->is_open)
		return -EINVAL;

	num_bytes = scanbtnd_write(scanner, (void*)bytes, 2);
	if (num_bytes != 2) {
		scanbtnd_flush(scanner);
		return 0;
	}
	num_bytes = scanbtnd_read(scanner, (void*)bytes, 4);
	if (num_bytes != 4)  {
		scanbtnd_flush(scanner);
		return 0;
	}
	rcv_len = bytes[3] << 8 | bytes[2];
	num_bytes = scanbtnd_read(scanner, (void*)bytes, rcv_len);
	if (num_bytes != rcv_len)  {
		scanbtnd_flush(scanner);
		return 0;
	}
	return bytes[0];
}


GENERIC_GET_SANE_DEVICE_DESCRIPTOR_FUNC
GENERIC_EXIT_FUNC

