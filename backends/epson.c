//
// Epson GT-9300 scanner button daemon
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
//
// Thanks to:
//  - James Gilliland (Epson CX3200 support)
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include "scanbuttond.h"
#include "interface/libusbi.h"
#include "backends/epson.h"

#define	ESC        	0x1B		/* ASCII value for ESC */

static char* backend_name = "Epson USB";

#define NUM_SUPPORTED_USB_DEVICES 14

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ 0x04B8, 0x0107 },	// Epson Expression 1600 
	{ 0x04B8, 0x010E },	// Epson Expression 1680
	{ 0x04B8, 0x0103 },	// Epson Perfection 610
	{ 0x04B8, 0x0101 },	// Epson Perfection 636U
	{ 0x04B8, 0x010C },	// Epson Perfection 640
	{ 0x04B8, 0x0104 },	// Epson Perfection 1200U
	{ 0x04B8, 0x010B },	// Epson Perfection 1240
	{ 0x04B8, 0x010A },	// Epson Perfection 1640
	{ 0x04B8, 0x0110 },	// Epson Perfection 1650
	{ 0x04B8, 0x011E },	// Epson Perfection 1660
	{ 0x04B8, 0x011B },	// Epson Perfection 2400
	{ 0x04B8, 0x0112 },	// Epson Perfection 2450
	{ 0x04B8, 0x011C },	// Epson Perfection 3200
	{ 0x04B8, 0x0802 }	// Epson CX3200
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


scanner_device* epson_scanners = NULL;


// returns -1 if the scanner is unsupported, or the index of the
// corresponding vendor-product pair in the supported_usb_devices array.
int epson_match_libusb_scanner(usb_scanner* scanner) {
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


void epson_attach_libusb_scanner(usb_scanner* scanner) {
  const char* descriptor_prefix = "epson:libusb:";
  int index = epson_match_libusb_scanner(scanner);
  if (index < 0) return; // unsupported
  scanner_device* dev = (scanner_device*)malloc(sizeof(scanner_device));
  dev->vendor = usb_device_descriptions[index][0];
  dev->product = usb_device_descriptions[index][1];
  dev->connection = CONNECTION_LIBUSB;
  dev->internal_dev_ptr = (void*)scanner;
  dev->lastbutton = 0;
  dev->sane_device = (char*)malloc(strlen(scanner->location) + strlen(descriptor_prefix) + 1);
  strcpy(dev->sane_device, descriptor_prefix);
  strcat(dev->sane_device, scanner->location);  
  dev->next = epson_scanners;
  epson_scanners = dev;
}


void epson_detach_scanners(void) {
  scanner_device* next;
  while (epson_scanners != NULL) {
    next = epson_scanners->next;
    free(epson_scanners->sane_device);
    free(epson_scanners);
    epson_scanners = next;
  }
}


void epson_scan_devices(usb_scanner* scanner) {
  int index;
  while (scanner != NULL) {
    index = epson_match_libusb_scanner(scanner);
    if (index >= 0) epson_attach_libusb_scanner(scanner);
    scanner = scanner->next;
  }
}


int epson_init_libusb(void) {
  usb_scanner* scanner;
  
  libusb_init();
  scanner = libusb_get_devices();
  epson_scan_devices(scanner);
  return 0;
}


char* scanbtnd_get_backend_name(void) {
  return backend_name;
}


int scanbtnd_init(void) {
  epson_scanners = NULL;
  
  syslog(LOG_INFO, "epson-backend: init");
  return epson_init_libusb();
}


int scanbtnd_rescan(void) {
  usb_scanner* scanner;
  
  epson_detach_scanners();
  epson_scanners = NULL;
  libusb_rescan();
  scanner = libusb_get_devices();
  epson_scan_devices(scanner);
  return 0;
}


scanner_device* scanbtnd_get_supported_devices(void) {
  return epson_scanners;
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


int epson_read(scanner_device* scanner, void* buffer, int bytecount) {
  switch (scanner->connection) {
    case CONNECTION_LIBUSB:
      return libusb_read((usb_scanner*)scanner->internal_dev_ptr, buffer, bytecount);
    break;
  }
  return -1;
}


int epson_write(scanner_device* scanner, void* buffer, int bytecount) {
  switch (scanner->connection) {
    case CONNECTION_LIBUSB:
      return libusb_write((usb_scanner*)scanner->internal_dev_ptr, buffer, bytecount);
    break;
  }
  return -1;
}


int scanbtnd_get_button(scanner_device* scanner) {
  unsigned char bytes[255];
  int rcv_len;
  int num_bytes;
  
  bytes[0] = ESC;
  bytes[1] = '!';
  bytes[2] = '\0';
  
  num_bytes = epson_write(scanner, (void*)bytes, 2);
  if (num_bytes != 2) return 0;
  num_bytes = epson_read(scanner, (void*)bytes, 4);
  if (num_bytes != 4) return 0;
  rcv_len = bytes[3] << 8 | bytes[2];
  num_bytes = epson_read(scanner, (void*)bytes, rcv_len);
  if (num_bytes != rcv_len) return 0;
  return bytes[0];
}


char* scanbtnd_get_sane_device_descriptor(scanner_device* scanner) {
  return scanner->sane_device;
}


int scanbtnd_exit(void) {
  syslog(LOG_INFO, "epson-backend: exit");
  epson_detach_scanners();
  libusb_exit();
  return 0;
}

