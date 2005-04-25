// Niash scanner backend
// Copyleft )c( 2005 by Bernhard Stiftner
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
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scanbuttond.h"
#include "interface/libusbi.h"
#include "backends/niash.h"
#include <syslog.h>

static char* backend_name = "Niash USB (experimental)";

#define NUM_SUPPORTED_USB_DEVICES 4

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ 0x06bd, 0x0100 },	// Agfa Snapscan Touch
	{ 0x03f0, 0x0205 },	// HP Scanjet 3300c
	{ 0x03f0, 0x0405 },	// HP Scanjet 3400c
	{ 0x03f0, 0x0305 },	// HP Scanjet 4300c
};

// TODO: check if this backend really works on the Epson 2580 too...
static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Agfa", "Snapscan Touch" },
	{ "Hewlett-Packard", "Scanjet 3300c" },
	{ "Hewlett-Packard", "Scanjet 3400c" },
	{ "Hewlett-Packard", "Scanjet 4300c" },
};


scanner_device* detected_scanners = NULL;


// returns -1 if the scanner is unsupported, or the index of the
// corresponding vendor-product pair in the supported_usb_devices array.
int match_libusb_scanner(usb_scanner* scanner) {
  int index;
  for (index = 0; index < NUM_SUPPORTED_USB_DEVICES; index++) {
    if (supported_usb_devices[index][0] == scanner->vendorID &&
        supported_usb_devices[index][1] == scanner->productID) {
      break;
    }
  }
  if (index >= NUM_SUPPORTED_USB_DEVICES) return -1;
  return index;
}

// TODO: check if the descriptor matches the SANE device name!
void attach_libusb_scanner(usb_scanner* scanner) {
  const char* descriptor_prefix = "niash:libusb:";
  int index = match_libusb_scanner(scanner);
  if (index < 0) return; // unsupported
  scanner_device* dev = (scanner_device*)malloc(sizeof(scanner_device));
  dev->vendor = usb_device_descriptions[index][0];
  dev->product = usb_device_descriptions[index][1];
  dev->connection = CONNECTION_LIBUSB;
  dev->internal_dev_ptr = (void*)scanner;
  dev->sane_device = (char*)malloc(strlen(scanner->location) + strlen(descriptor_prefix) + 1);
  strcpy(dev->sane_device, descriptor_prefix);
  strcat(dev->sane_device, scanner->location);  
  dev->next = detected_scanners;
  detected_scanners = dev;
}


void detach_scanners(void) {
  scanner_device* next;
  while (detected_scanners != NULL) {
    next = detected_scanners->next;
    free(detected_scanners->sane_device);
    free(detected_scanners);
    detected_scanners = next;
  }
}


void scanbtnd_scan_devices(usb_scanner* scanner) {
  int index;
  while (scanner != NULL) {
    index = match_libusb_scanner(scanner);
    if (index >= 0) attach_libusb_scanner(scanner);
    scanner = scanner->next;
  }
}


int scanbtnd_init_libusb(void) {
  usb_scanner* scanner;
  
  libusb_init();
  scanner = libusb_get_devices();
  scanbtnd_scan_devices(scanner);
  return 0;
}

char* scanbtnd_get_backend_name(void) {
  return backend_name;
}

int scanbtnd_init(void) {
  detected_scanners = NULL;
  return scanbtnd_init_libusb();
}


int scanbtnd_rescan(void) {
  usb_scanner* scanner;

  detach_scanners();
  detected_scanners = NULL;
  libusb_rescan();
  scanner = libusb_get_devices();
  scanbtnd_scan_devices(scanner);
  return 0;
}


scanner_device* scanbtnd_get_supported_devices(void) {
  return detected_scanners;
}


int scanbtnd_open(scanner_device* scanner) {  
  switch (scanner->connection) {
    case CONNECTION_LIBUSB:
      return libusb_open((usb_scanner*)scanner->internal_dev_ptr);
    break;
  }
  return -1;
}


int scanbtnd_close(scanner_device* scanner) {
  switch (scanner->connection) {
    case CONNECTION_LIBUSB:
      return libusb_close((usb_scanner*)scanner->internal_dev_ptr);
    break;
  }
  return -1;
}


int niash_control_msg(scanner_device* scanner, int requesttype, int request, int value, int index, void* buffer, int bytecount) {
  switch (scanner->connection) {
    case CONNECTION_LIBUSB:
      return libusb_control_msg((usb_scanner*)scanner->internal_dev_ptr, requesttype, request, value, index, buffer, bytecount);
    break;
  }
  return -1;
}


int scanbtnd_get_button(scanner_device* scanner) {
  unsigned char bytes[255];
  int value[255];
  int requesttype[255];
  int num_bytes;
  int button;
  int i;
  /* The button status seems to be held in Register 0x2e of the
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


char* scanbtnd_get_sane_device_descriptor(scanner_device* scanner) {
  return scanner->sane_device;
}


int scanbtnd_exit(void) {
  detach_scanners();
  libusb_exit();
  return 0;
}

