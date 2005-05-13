// Snapscan scanner backend
// Copyleft )c( 2005 by Bernhard Stiftner
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
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include "scanbuttond.h"
#include "interface/libusbi.h"
#include "backends/snapscan.h"

static char* backend_name = "Snapscan USB";

#define NUM_SUPPORTED_USB_DEVICES 2

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ 0x04b8, 0x0121 },	// Epson Perfection 2480
	{ 0x04b8, 0x011f }	// Epson Perfection 1670
};

// TODO: check if this backend really works on the Epson 2580 too...
static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Epson", "Perfection 2480 / 2580"},
	{ "Epson", "Perfection 1670"}
};


scanner_device* snapscan_scanners = NULL;


// returns -1 if the scanner is unsupported, or the index of the
// corresponding vendor-product pair in the supported_usb_devices array.
int snapscan_match_libusb_scanner(usb_scanner* scanner) {
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
void snapscan_attach_libusb_scanner(usb_scanner* scanner) {
  const char* descriptor_prefix = "snapscan:libusb:";
  int index = snapscan_match_libusb_scanner(scanner);
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
  dev->next = snapscan_scanners;
  snapscan_scanners = dev;
}


void snapscan_detach_scanners(void) {
  scanner_device* next;
  while (snapscan_scanners != NULL) {
    next = snapscan_scanners->next;
    free(snapscan_scanners->sane_device);
    free(snapscan_scanners);
    snapscan_scanners = next;
  }
}


void snapscan_scan_devices(usb_scanner* scanner) {
  int index;
  while (scanner != NULL) {
    index = snapscan_match_libusb_scanner(scanner);
    if (index >= 0) snapscan_attach_libusb_scanner(scanner);
    scanner = scanner->next;
  }
}


int snapscan_init_libusb(void) {
  usb_scanner* scanner;
  
  libusb_init();
  scanner = libusb_get_devices();
  snapscan_scan_devices(scanner);
  return 0;
}


char* scanbtnd_get_backend_name(void) {
  return backend_name;
}


int scanbtnd_init(void) {
  snapscan_scanners = NULL;
  
  syslog(LOG_INFO, "snapscan-backend: init");
  return snapscan_init_libusb();
}


int scanbtnd_rescan(void) {
  usb_scanner* scanner;
  
  snapscan_detach_scanners();
  snapscan_scanners = NULL;
  libusb_rescan();
  scanner = libusb_get_devices();
  snapscan_scan_devices(scanner);
  return 0;
}


scanner_device* scanbtnd_get_supported_devices(void) {
  return snapscan_scanners;
}


int scanbtnd_open(scanner_device* scanner) {  
  switch (scanner->connection) {
    case CONNECTION_LIBUSB:
      // if devices have been added/removed, return -ENODEV to
      // make scanbuttond update its device list
      if (libusb_get_changed_device_count() != 0) {
        return -ENODEV;
      }
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


int snapscan_read(scanner_device* scanner, void* buffer, int bytecount) {
  switch (scanner->connection) {
    case CONNECTION_LIBUSB:
      return libusb_read((usb_scanner*)scanner->internal_dev_ptr, buffer, bytecount);
    break;
  }
  return -1;
}


int snapscan_write(scanner_device* scanner, void* buffer, int bytecount) {
  switch (scanner->connection) {
    case CONNECTION_LIBUSB:
      return libusb_write((usb_scanner*)scanner->internal_dev_ptr, buffer, bytecount);
    break;
  }
  return -1;
}


int scanbtnd_get_button(scanner_device* scanner) {
  unsigned char bytes[255];
  int num_bytes;
  int button;
  
  bytes[0] = 0x03;
  bytes[1] = 0x00;
  bytes[2] = 0x00;
  bytes[3] = 0x00;
  bytes[4] = 0x14;
  bytes[5] = 0x00;
  num_bytes = snapscan_write(scanner, (void*)bytes, 6);
  if (num_bytes != 6) return 0;
  
  num_bytes = snapscan_read(scanner, (void*)bytes, 8);
  if (num_bytes != 8 || bytes[0] != 0xF9) return 0;

  num_bytes = snapscan_read(scanner, (void*)bytes, 20);
  if (num_bytes != 20 || bytes[0] != 0xF0) return 0;
  switch (bytes[18] & 0xF0) {
    case 0x10: button = 1; break;
    case 0x20: button = 2; break;
    case 0x40: button = 3; break;
    case 0x80: button = 4; break;
    default: button = 0; break;
  };
  
  num_bytes = snapscan_read(scanner, (void*)bytes, 8);
  if (num_bytes != 8 || bytes[0] != 0xFB) return 0;
  
  return button;
}


char* scanbtnd_get_sane_device_descriptor(scanner_device* scanner) {
  return scanner->sane_device;
}


int scanbtnd_exit(void) {
  syslog(LOG_INFO, "snapscan-backend: exit");
  snapscan_detach_scanners();
  libusb_exit();
  return 0;
}

