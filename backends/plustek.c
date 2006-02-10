// plustek.c: Plustek device backend
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

#define BACKEND_NAME	"Plustek USB"

#define NUM_SUPPORTED_USB_DEVICES 8

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x04a9, 0x2207, 1 },	// CanoScan N1220U
	{ 0x04a9, 0x2208, 1 },	// CanoScan CanoScan D660U
	{ 0x04a9, 0x2206, 1 },	// CanoScan N650U
	{ 0x04a9, 0x220d, 3 },	// CanoScan LiDE 20
	{ 0x04a9, 0x2220, 3 },  // CanoScan LiDE 25
	{ 0x04a9, 0x220e, 3 },	// CanoScan LiDE 30
	{ 0x04b8, 0x011d, 4 },  // Epson Perfection 1260
	{ 0x03f0, 0x0605, 2 }   // HP ScanJet 2200c (maybe only 1 button?)
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Canon", "CanoScan N1220U" },
	{ "Canon", "CanoScan D660U"  },
	{ "Canon", "CanoScan N650U" },
	{ "Canon", "CanoScan LiDE 20" },
	{ "Canon", "CanoScan LiDE 25" },
	{ "Canon", "CanoScan LiDE 30" },
	{ "Epson", "Perfection 1260" },
	{ "Hewlett-Packard", "ScanJet 2200c" }
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


int scanbtnd_get_button(scanbtnd_scanner_t* scanner)
{
	/*
	Note 1: I strongly suspect that the command 0x01 0x69 0x00 0x01 will return
	a button bitmask. For my Canon N1220U it returns 0x04, which happens to
	be the bit I have to test against to see if the scanner button was pressed.
	However, this has to be tested on other scanners to see if this is true.
	UPDATE by BS: The LIDE 20 also returns 0x04, but it has three buttons!
	So this guess is probably wrong. (Thanks to Christian Bucher for this info)
	
	Note 2: This works on my Canon N1220U. Whether this is Canon specific or
	if it works for all 'plustek usb' type scanners is something I don't know.

	Note 3: You must have run sane-find-scanner once. Sane apparently initializes
	something on the scanner allowing this to work. Otherwise all you get is 0x00.
	
	Note 4: by /cbx
	On my CanoScan LIDE20, the default value is $62 and the bits for the
	buttons are as follows:
	Scan: $72 ==> 0x10
	Copy: $6a ==> 0x08
	Mail: $66 ==> 0x04
	*/

	unsigned char bytes[255];
	int num_bytes;
	int button = 0;
	
	bytes[0] = 1;
	bytes[1] = 2;
	bytes[2] = 0;
	bytes[3] = 1;

	num_bytes = scanbtnd_write(scanner, (void*)bytes, 4);
	if (num_bytes != 4) return 0;
	num_bytes = scanbtnd_read(scanner, (void*)bytes, 1);
	if (num_bytes != 1) return 0;
	
	// by BS: This is my first attempt to get rid of the 
	// hardcoded button bitmask. Note that I do not own any device
	// supported by this backend, so this code is based on guessing.
	// Tested on the LIDE 20, should work for 1-button devices, too.
	switch (scanner->num_buttons) {
	case 1:
		if ((bytes[0] & 0x04) != 0) button = 1;
		break;
	case 2:
		if ((bytes[0] & 0x08) != 0) button = 1;
		if ((bytes[0] & 0x04) != 0) button = 2;
		break;
	case 3: 
		if ((bytes[0] & 0x10) != 0) button = 1;
		if ((bytes[0] & 0x08) != 0) button = 2;
		if ((bytes[0] & 0x04) != 0) button = 3;
		break;	
	case 4: // only tested for the Epson Perfection 1260...
		// seems to be a bit odd compared to the other cases...
		if ((bytes[0] & 0x08) != 0) button = 1;
		if ((bytes[0] & 0x10) != 0) button = 2;
		if ((bytes[0] & 0x20) != 0) button = 3;
		if ((bytes[0] & 0x40) != 0) button = 4;
		break;
	}
	return button;
}


GENERIC_GET_SANE_DEVICE_DESCRIPTOR_FUNC
GENERIC_EXIT_FUNC
