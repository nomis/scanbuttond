//
// scanbuttond
// libusb interface
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
#include <unistd.h>
#include <usb.h>

#define TIMEOUT	   	30 * 1000	/* 30 seconds */

struct usb_devices**;


int libusb_init(void) {
  usb_init();
  usb_find_busses();
  usb_find_devices();
  usb_devices = NULL;
}


struct usb_device** libusb_get_devices(void) {
  int count = 0;
  int i = 0;
  struct usb_bus *bus;
  struct usb_device *device;
  struct usb_interface_descriptor *interface;
  
  bus = usb_busses;
  while (bus != NULL) {
    device = usb_busses->devices;
    while (device != NULL) {
      count++;
      device = device->next;
    }
    bus = bus->next;
  }
  if (count == 0) return NULL;
  
  if (usb_devices != NULL) free(usb_devices);
  usb_devices = (struct usb_devices**)malloc(count * sizeof(void*));
  
  bus = usb_busses;
  while (bus != NULL) {
    device = usb_busses->devices;
    while (device != NULL) {
      usb_devices[i] = device;
      i++;
      device = device->next;
    }
    bus = bus->next;
  }
}


int libusb_open(usb_scanner* scanner) {
  scanner->handle = usb_open(scanner->device);
  if (scanner->handle == NULL) {
    usb_close(scanner->handle);
    return -1;
  }
  if (usb_claim_interface(scanner->handle, scanner->interface) != 0) {
    usb_close(scanner->handle);
    return -2;
  }
  return 0;
}


int libusb_close(usb_scanner* scanner) {
  return usb_close(scanner->handle);
}


int libusb_read(usb_scanner* scanner, void* buffer, int bytecount) {
  int num_bytes = usb_bulk_read(scanner->device, scanner->in_endpoint, buffer, bytecount, TIMEOUT);  
  if (num_bytes<0) {
    usb_clear_halt(scanner->device, scanner->in_endpoint);
    return 0;
  }
  return num_bytes;
}


int libusb_write(usb_scanner* scanner, void* buffer, int bytecount) {
  int num_bytes = usb_bulk_write(scanner->device, scanner->out_endpoint, buffer, bytecount, TIMEOUT);
  if (num_bytes<0) {
    usb_clear_halt(scanner->device, scanner->in_endpoint);
    return 0;
  }
  return num_bytes;
}


int libusb_exit(void) {
  free(usb_devices);
}

