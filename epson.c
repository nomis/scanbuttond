//
// Epson GT-9300 scanner button daemon
// Copyleft )c( 2004 by Bernhard Stiftner
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
// along with this program;a if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scanbuttond.h"

#define	ESC        	0x1B		/* ASCII value for ESC */

#define NUM_SUPPORTED_USB_DEVICES 1
static int supported_usb_devices[NUM_SUPPORTED_DEVICES][2] = {
	{ 0x04B8, 0x011B }	// Epson GT-9300 (aka Perfection 2400)
};

int connectiontype;
int num_detected_scanners;
scanner_device** detected_scanners;


int epson_init(int connection) {
  connectiontype = connection;
  
  switch (connectiontype) {
    case CONNECTION_LIBUSB:
    // TODO: implement!
    break;
  }
  
}


int epson_get_num_devices() {
  return num_detected_scanners;
}


scanner_device* epson_get_device(int index) {
  if (index < 0 || index >= num_detected_scanners) return NULL;
  return detected_scanners[index];
}


int epson_open(scanner_device* scanner) {  
  switch (connectiontype) {
    case CONNECTION_LIBUSB:
      return libusb_open((usb_scanner*)scanner->internal_dev_ptr);
    break;
  }
  return -1;
}


int epson_close(scanner_device* scanner) {
  switch (connectiontype) {
    case CONNECTION_LIBUSB:
      return libusb_close((usb_scanner*)scanner->internal_dev_ptr);
    break;
  }
  return -1;
}


int epson_read(scanner* scanner, void* buffer, int bytecount) {
  switch (connectiontype) {
    case CONNECTION_LIBUSB:
      return libusb_read((usb_scanner*)scanner->internal_dev_ptr, buffer, bytecount);
    break;
  }
  return -1;
}

int epson_write(scanner* scanner, void* buffer, int bytecount) {
  switch (connectiontype) {
    case CONNECTION_LIBUSB:
      return libusb_write((usb_scanner*)scanner->internal_dev_ptr, buffer, bytecount);
    break;
  }
  return -1;
}


int epson_get_button(scanner_device* scanner) {
  char bytes[255];
  int rcv_len;
  int num_bytes;
  
  bytes[0] = ESC;
  bytes[1] = '!';
  bytes[2] = '\0';
  
  num_bytes = epson_write(scanner, (void*)bytes, 2);
  if (num_bytes != 2) return 0;
  num_bytes = epson_read(scanner, (void*)bytes, 4);
  if (num_bytes != 2) return 0;
  rcv_len = bytes[3] << 8 | bytes[2];
  num_bytes = epson_read(scanner, (void*)bytes, rcv_len);
  if (num_bytes != rcv_len) return 0;
  return bytes[0];
}

