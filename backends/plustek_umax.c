// plustek_umax.c: Plustek device backend for UMAX models
// This file is part of scanbuttond.
// Copyleft )c( 2005 by Hans Verkuil
// Copyleft )c( 2005-2006 by Bernhard Stiftner
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

#define BACKEND_NAME	"Plustek USB for UMAX"

#define NUM_SUPPORTED_USB_DEVICES 1

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x1606, 0x0060, 4 }  // UMAX Astra 3400 (3450?)
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "UMAX", "Astra 3400/3450" }
};


GENERIC_GLOBALS
GENERIC_MATCH_LIBUSB_SCANNER_FUNC
GENERIC_ATTACH_LIBUSB_SCANNER_FUNC("plustek:libusb:")
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
	int button = 0;
	
	bytes[0] = 1;
	bytes[1] = 2;
	bytes[2] = 0;
	bytes[3] = 1;

	if (!scanner->is_open)
		return -EINVAL;

	num_bytes = scanbtnd_write(scanner, (void*)bytes, 4);
	if (num_bytes != 4)  {
		scanbtnd_flush(scanner);
		return 0;
	}
	num_bytes = scanbtnd_read(scanner, (void*)bytes, 1);
	if (num_bytes != 1)  {
		scanbtnd_flush(scanner);
		return 0;
	}
	
	switch (scanner->num_buttons) {
	case 1: // not tested
		if ((bytes[0] & 0x04) != 0) button = 1;
		break;
	case 2: // not tested
		if ((bytes[0] & 0x08) != 0) button = 1;
		if ((bytes[0] & 0x04) != 0) button = 2;
		break;
	case 3: // not tested
		if ((bytes[0] & 0x10) != 0) button = 1;
		if ((bytes[0] & 0x08) != 0) button = 2;
		if ((bytes[0] & 0x04) != 0) button = 3;
		break;	
	case 4: // only tested on UMAX Astra 3400
		if ((bytes[0] & 0x04) != 0) button = 1;
		if ((bytes[0] & 0x08) != 0) button = 2;
		if ((bytes[0] & 0x40) != 0) button = 3;
		if ((bytes[0] & 0x20) != 0) button = 4;
		break;
	}
	return button;
}


GENERIC_GET_SANE_DEVICE_DESCRIPTOR_FUNC
GENERIC_EXIT_FUNC
