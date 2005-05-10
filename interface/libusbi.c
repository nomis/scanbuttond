// scanbuttond
// libusb interface
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <usb.h>
#include <syslog.h>
#include "libusbi.h"

#define TIMEOUT	   	30 * 1000	/* 30 seconds */

usb_scanner* usb_devices = NULL;


int libusb_init(void) {
  usb_init();
  libusb_rescan();
  return 0;
}


int libusb_search_interface(struct usb_device* device) {
  int found = 0;
  int interface;
  for (interface = 0; interface < device->config[0].bNumInterfaces && !found; interface++) {
    switch (device->descriptor.bDeviceClass) {
      case USB_CLASS_VENDOR_SPEC:
        found = 1;
        break;
      case USB_CLASS_PER_INTERFACE:
        switch (device->config[0].interface[interface].altsetting[0].bInterfaceClass) {
          case USB_CLASS_VENDOR_SPEC:
          case USB_CLASS_PER_INTERFACE:
          case 16: /* data? */
            found = 1;
            break;
          }
        break;
       }
     }
  interface--;
  if (!found) return -1;
  return interface;
}


int libusb_search_in_endpoint(struct usb_device* device) {
  int usb_in_ep = 0;
  int usb_out_ep = 0;
  struct usb_interface_descriptor *interface;
  interface = &device->config[0].interface->altsetting[0];

  int num;
  for (num = 0; num < interface->bNumEndpoints; num++) {
    struct usb_endpoint_descriptor *endpoint;
    int address, direction, transfer_type;
            
    endpoint = &interface->endpoint[num];
    address = endpoint->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
    direction = endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK;
    transfer_type = endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK;
          
    if (transfer_type == USB_ENDPOINT_TYPE_BULK) {
      if (direction) {	/* in */
        if (!usb_in_ep)
          usb_in_ep = endpoint->bEndpointAddress;
      } else {	/* out */
        if (!usb_out_ep)
          usb_out_ep = endpoint->bEndpointAddress;
      }
    }
  }
  return usb_in_ep;
}


int libusb_search_out_endpoint(struct usb_device* device) {
  int usb_in_ep = 0;
  int usb_out_ep = 0;
  struct usb_interface_descriptor *interface;
  interface = &device->config[0].interface->altsetting[0];

  int num;
  for (num = 0; num < interface->bNumEndpoints; num++) {
    struct usb_endpoint_descriptor *endpoint;
    int address, direction, transfer_type;
            
    endpoint = &interface->endpoint[num];
    address = endpoint->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
    direction = endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK;
    transfer_type = endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK;
          
    if (transfer_type == USB_ENDPOINT_TYPE_BULK) {
      if (direction) {	/* in */
        if (!usb_in_ep)
          usb_in_ep = endpoint->bEndpointAddress;
      } else {	/* out */
        if (!usb_out_ep)
          usb_out_ep = endpoint->bEndpointAddress;
      }
    }
  }
  return usb_out_ep;
}


void libusb_attach_device(struct usb_device* device) {
  usb_scanner* scanner = (usb_scanner*)malloc(sizeof(usb_scanner));
  scanner->vendorID = device->descriptor.idVendor;
  scanner->productID = device->descriptor.idProduct;
  
  // the location string consists of bus number, followed by a colon (":"), and the device number
  scanner->location = (char*)malloc(strlen(device->bus->dirname) + strlen(device->filename) + 2);
  strcpy(scanner->location, device->bus->dirname);
  strcat(scanner->location, ":");
  strcat(scanner->location, device->filename);
  
  scanner->device = device;
  scanner->handle = NULL;
  scanner->interface = libusb_search_interface(device);
  if (scanner->interface < 0) {
    free(scanner);
    return;
  }
  scanner->out_endpoint = libusb_search_out_endpoint(device);
  if (scanner->out_endpoint < 0) {
    free(scanner);
    return;
  }  
  scanner->in_endpoint = libusb_search_in_endpoint(device);
  if (scanner->in_endpoint < 0) {
    free(scanner);
    return;
  }
  scanner->next = usb_devices;
  usb_devices = scanner;
}


void libusb_detach_devices(void) {
  usb_scanner* next;
  while (usb_devices != NULL) {
    next = usb_devices->next;
    free(usb_devices->location);
    free(usb_devices);
    usb_devices = next;
  }
}


void libusb_rescan(void) {  
  struct usb_bus *bus;
  struct usb_device *device;

  libusb_detach_devices();
    
  usb_find_busses();
  usb_find_devices();
  usb_devices = NULL;
  
  bus = usb_busses;
  while (bus != NULL) {
    device = bus->devices;
    while (device != NULL) {
      libusb_attach_device(device);
      device = device->next;
    }
    bus = bus->next;
  }
}
  

usb_scanner* libusb_get_devices(void) {
  return usb_devices;
}


int libusb_open(usb_scanner* scanner) {
  int result;
  
  // if usb busses or devices have been added or removed, we return
  // an error to make scanbuttond update its device list
  usb_find_busses();
  if (usb_find_devices() != 0) {
    syslog(LOG_INFO, "usb devices have been added/removed. forcing update of device list...");
    return -ENODEV;
  }
  
  scanner->handle = usb_open(scanner->device);
  if (scanner->handle == NULL) {
    syslog(LOG_INFO, "usb_open failed!");
    return -ENODEV;
  }
  
  result = usb_claim_interface(scanner->handle, scanner->interface);
  //syslog(LOG_INFO, "usb_claim_interface returned %d", result);
  if (result != 0 && result != -EBUSY) {
    // Claiming the device failed, but it's not busy!?!
    // Perhaps we have to explicitly set its configuration and try again...
    syslog(LOG_INFO, "claiming the device failed, but it's not busy. Trying to set configuration.");
    if (usb_set_configuration(scanner->handle, 
        usb_device(scanner->handle)->config[0].bConfigurationValue) != 0) {
      syslog(LOG_INFO, "usb_set_configuration failed!");
      usb_close(scanner->handle);
      return -ENODEV;
    }
    result = usb_claim_interface(scanner->handle, scanner->interface);
  }
  if (result != 0) {
    if (result == -EBUSY) 
    {
     syslog(LOG_INFO, "usb_claim_interface failed. the device is busy.");
    } else
    { 
     syslog(LOG_INFO, "usb_claim_interface failed. the device is not? busy.");
    }
    usb_close(scanner->handle);
    return -EBUSY;
  }
  
  return 0;
}


int libusb_close(usb_scanner* scanner) {
  return usb_close(scanner->handle);
}


int libusb_read(usb_scanner* scanner, void* buffer, int bytecount) {
  int num_bytes = usb_bulk_read(scanner->handle, scanner->in_endpoint, buffer, bytecount, TIMEOUT);  
  if (num_bytes<0) {
    usb_clear_halt(scanner->handle, scanner->in_endpoint);
    return 0;
  }
  return num_bytes;
}


int libusb_write(usb_scanner* scanner, void* buffer, int bytecount) {
  int num_bytes = usb_bulk_write(scanner->handle, scanner->out_endpoint, buffer, bytecount, TIMEOUT);
  if (num_bytes<0) {
    usb_clear_halt(scanner->handle, scanner->in_endpoint);
    return 0;
  }
  return num_bytes;
}

int libusb_control_msg(usb_scanner* scanner, int requesttype, int request, int value, int index, void* bytes, int size) {
  int num_bytes = usb_control_msg(scanner->handle, requesttype, request, value, index, bytes, size, TIMEOUT);
  if (num_bytes<0) {
    usb_clear_halt(scanner->handle, scanner->in_endpoint);
    return 0;
  }
  return num_bytes;
}



int libusb_exit(void) {
  libusb_detach_devices();
  return 0;
}

