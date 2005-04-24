//
// Canon N1220U (plustek based) scanner button backend
// Copyleft )c( 2005 by Hans Verkuil
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
#include "backends/plustek.h"

static char* backend_name = "Plustek USB (experimental)";

#define NUM_SUPPORTED_USB_DEVICES 1

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ 0x04a9, 0x2207 }	// CanonScan N1220U
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Canon", "CanonScan N1220U"}
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


void attach_libusb_scanner(usb_scanner* scanner) {
  const char* descriptor_prefix = "plustek:libusb:";
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


int plustek_read(scanner_device* scanner, void* buffer, int bytecount) {
  switch (scanner->connection) {
    case CONNECTION_LIBUSB:
      return libusb_read((usb_scanner*)scanner->internal_dev_ptr, buffer, bytecount);
    break;
  }
  return -1;
}

int plustek_write(scanner_device* scanner, void* buffer, int bytecount) {
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
  int button_mask = 4;

  /* Note 1: I strongly suspect that the command 0x01 0x69 0x00 0x01 will return
     a button bitmask. For my Canon N1220U it returns 0x04, which happens to
     be the bit I have to test against to see if the scanner button was pressed.
     However, this has to be tested on other scanners to see if this is true. */
  
  /* Note 2: This works on my Canon N1220U. Whether this is Canon specific or
     if it works for all 'plustek usb' type scanners is something I don't know. */

  /* Note 3: You must have run sane-find-scanner once. Sane apparently initializes 
     something on the scanner allowing this to work. Otherwise all you get is 0x00. */
  bytes[0] = 1;
  bytes[1] = 2;
  bytes[2] = 0;
  bytes[3] = 1;
  
  num_bytes = plustek_write(scanner, (void*)bytes, 4);
  if (num_bytes != 4) return 0;
  num_bytes = plustek_read(scanner, (void*)bytes, 1);
  if (num_bytes != 1) return 0;
  return ((bytes[0] & button_mask) == 0) ? 0 : 1;
}


char* scanbtnd_get_sane_device_descriptor(scanner_device* scanner) {
  return scanner->sane_device;
}


int scanbtnd_exit(void) {
  detach_scanners();
  libusb_exit();
  return 0;
}

