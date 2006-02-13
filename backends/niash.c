// niash.c: Niash device backend
// This file is part of scanbuttond.
// Copyleft )c( 2005-2006 by Bernhard Stiftner
// Copyleft )c( 2005 by Dirk Wriedt
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
#include "niash.h"
#include "scanbuttond/interface_usb.h"
#include "generic_backend.h"

#define BACKEND_NAME	"Niash USB"

#define NUM_SUPPORTED_USB_DEVICES 4

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x06bd, 0x0100, 4 },	// Agfa Snapscan Touch
	{ 0x03f0, 0x0205, 2 },	// HP Scanjet 3300c
	{ 0x03f0, 0x0405, 3 },	// HP Scanjet 3400c
	{ 0x03f0, 0x0305, 3 }	// HP Scanjet 4300c
};

// TODO: check if this backend really works on the Epson 2580 too...
static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Agfa", "Snapscan Touch" },
	{ "Hewlett-Packard", "Scanjet 3300c" },
	{ "Hewlett-Packard", "Scanjet 3400c" },
	{ "Hewlett-Packard", "Scanjet 4300c" }
};


GENERIC_GLOBALS
GENERIC_MATCH_LIBUSB_SCANNER_FUNC
GENERIC_ATTACH_LIBUSB_SCANNER_FUNC("niash:libusb:")
GENERIC_DETACH_SCANNERS_FUNC
GENERIC_SCAN_DEVICES_FUNC
GENERIC_INIT_LIBUSB_FUNC
GENERIC_GET_BACKEND_NAME_FUNC
GENERIC_INIT_FUNC
GENERIC_RESCAN_FUNC
GENERIC_GET_SUPPORTED_DEVICES_FUNC
GENERIC_OPEN_FUNC
GENERIC_CLOSE_FUNC


int niash_control_msg(scanbtnd_scanner_t* scanner, int requesttype, int request,
	int value, int index, void* buffer, int bytecount)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			return scanbtnd_libusb_control_msg(
				(scanbtnd_libusb_device_t*)scanner->internal_dev_ptr,
				requesttype, request, value, index, buffer, bytecount);
			break;
	}
	return -1;
}


int scanbtnd_get_button(scanbtnd_scanner_t* scanner)
{
	unsigned char bytes[255];
	int value[255];
	int requesttype[255];
	int num_bytes;
	int button;
	int i;

	/*
	The button status seems to be held in Register 0x2e of the
	scanner's USB - IEEE1284 bridge
	I checked the usb sniffer logs against hp3300c_xfer.h (hp3300 sane backend)
	and learned that the requests being submitted by the windows driver for
	my Agfa Snapscan Touch seem to follow this schema:

	request value                data   datasize
	0x40    SPP_CONTROL   (0x87) 0x14        1
	0x40    EPP_ADDR      (0x83) 0x2e        1
	0x40    SPP_CONTROL   (0x87) 0x34        1
	0xc0    EPP_DATA_READ (0x84) returned    1
	0x40    SPP_CONTROL   (0x87) 0x14        1

	The register can be read by setting the address with an EPP_ADDR call,
	then issuing an EPP_DATA_READ call.
	I don't know what the last request is for.
	*/

	if (!scanner->is_open)
		return -EINVAL;

	requesttype[0]=0x40; bytes[0] = 0x14; value[0]=0x87; /* SPP_CONTROL */
	requesttype[1]=0x40; bytes[1] = 0x2e; value[1]=0x83; /* EPP_ADDR */
	requesttype[2]=0x40; bytes[2] = 0x34; value[2]=0x87; /* SPP_CONTROL */
	requesttype[3]=0xc0; bytes[3] = 0xff; value[3]=0x84; /* EPP_DATA_READ */
	requesttype[4]=0x40; bytes[4] = 0x14; value[4]=0x87; /* SPP_CONTROL */
	for(i=0;i<5;i++) {
		num_bytes=niash_control_msg(scanner, requesttype[i], 0x0c, value[i], 0, (void*)&bytes + i, 0x01);
		if (num_bytes < 0 ) return 0;
	}
	switch (bytes[3]) {
		case 0x02: button = 1; break;
		case 0x04: button = 2; break;
		case 0x08: button = 3; break;
		case 0x10: button = 4; break;
		default: button = 0; break;
	};

	return button;
}


GENERIC_GET_SANE_DEVICE_DESCRIPTOR_FUNC
GENERIC_EXIT_FUNC
