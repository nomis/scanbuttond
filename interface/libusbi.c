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

#define TIMEOUT	   	10 * 1000	/* 10 seconds */

libusb_device_t* usb_devices = NULL;
int invocation_count = 0;


int libusb_init(void) {
  syslog(LOG_INFO, "libusbi: libusb_init, invocation counter=%d", invocation_count);
  invocation_count++;
  if (invocation_count != 1) return 0;
  syslog(LOG_INFO, "libusbi: initializing...");
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
  libusb_device_t* libusb_device = (libusb_device_t*)malloc(sizeof(libusb_device_t));
  libusb_device->vendorID = device->descriptor.idVendor;
  libusb_device->productID = device->descriptor.idProduct;
  
  // the location string consists of bus number, followed by a colon (":"), and the device number
  libusb_device->location = (char*)malloc(strlen(device->bus->dirname) + strlen(device->filename) + 2);
  strcpy(libusb_device->location, device->bus->dirname);
  strcat(libusb_device->location, ":");
  strcat(libusb_device->location, device->filename);
  
  libusb_device->device = device;
  libusb_device->handle = NULL;
  libusb_device->interface = libusb_search_interface(device);
  if (libusb_device->interface < 0) {
    syslog(LOG_ERR, "libusbi: no valid interface found for device %s", libusb_device->location);
    free(libusb_device->location);
    free(libusb_device);
    return;
  }
  libusb_device->out_endpoint = libusb_search_out_endpoint(device);
  if (libusb_device->out_endpoint < 0) {
    syslog(LOG_ERR, "libusbi: no valid outbound endpoint found for device %s", libusb_device->location);
    free(libusb_device->location);
    free(libusb_device);
    return;
  }  
  libusb_device->in_endpoint = libusb_search_in_endpoint(device);
  if (libusb_device->in_endpoint < 0) {
    syslog(LOG_ERR, "libusbi: no valid inbound endpoint found for device %s", libusb_device->location);
    free(libusb_device->location);
    free(libusb_device);
    return;
  }
  libusb_device->next = usb_devices;
  usb_devices = libusb_device;
}


void libusb_detach_devices(void) {
  libusb_device_t* next;
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
  
  syslog(LOG_DEBUG, "libusbi: rescanning usb devices...");

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
  
  syslog(LOG_DEBUG, "libusbi: rescan complete");
  
}


int libusb_get_changed_device_count(void) {
  usb_find_busses();
  return usb_find_devices();
}  


libusb_device_t* libusb_get_devices(void) {
  return usb_devices;
}


int libusb_open(libusb_device_t* device) {
  int result;
  
  if (!device->device)
    return -ENODEV;
  
  syslog(LOG_DEBUG, "libusbi: opening device %s", device->location);
  
  device->handle = usb_open(device->device);
  if (device->handle == NULL) {
    syslog(LOG_ERR, "libusbi: could not open device %s", device->location);
    return -ENODEV;
  }
  
  syslog(LOG_DEBUG, "libusbi: claiming interface...");
  
  // Calling usb_set_configuration should not be necessary. It is even considered harmful, since
  // it disturbs other processes which are currently communicating with the scanner!
  // usb_set_configuration(scanner->handle,
  //   usb_device(scanner->handle)->config[0].bConfigurationValue);
   
  result = usb_claim_interface(device->handle, device->interface);
  switch (result) {
    case 0:
      syslog(LOG_DEBUG, "libusbi: usb_open completed");
      return 0;
    case -ENOMEM:
      syslog(LOG_ERR, "libusbi: could not claim interface for device %s. (ENOMEM)", device->location);
      usb_close(device->handle);
      return -ENODEV;
    case -EBUSY:
      syslog(LOG_ERR, "libusbi: could not claim interface for device %s. (EBUSY)", device->location);
      usb_close(device->handle);
      return -EBUSY;
    default:
      syslog(LOG_ERR, "libusbi: could not claim interface for device %s. (code=%d)", device->location, result);
      usb_close(device->handle);
      return -ENODEV;
  }
}


int libusb_close(libusb_device_t* device) {
  int result;
  result = usb_release_interface(device->handle, device->interface);
  if (result < 0) {
    syslog(LOG_ERR, "libusbi: could not release interface, error code=%d, device=%s", result, device->location);
    return result;
  }
  result = usb_close(device->handle);
  if (result < 0) {
    syslog(LOG_ERR, "libusbi: could not close usb device, error code=%d, device=%s", result, device->location);
    return result;
  }
  return 0;
}


int libusb_read(libusb_device_t* device, void* buffer, int bytecount) {
  int num_bytes = usb_bulk_read(device->handle, device->in_endpoint, buffer, bytecount, TIMEOUT);  
  if (num_bytes<0) {
    usb_clear_halt(device->handle, device->in_endpoint);
    return 0;
  }
  return num_bytes;
}


int libusb_write(libusb_device_t* device, void* buffer, int bytecount) {
  int num_bytes = usb_bulk_write(device->handle, device->out_endpoint, buffer, bytecount, TIMEOUT);
  if (num_bytes<0) {
    usb_clear_halt(device->handle, device->in_endpoint);
    return 0;
  }
  return num_bytes;
}


int libusb_control_msg(libusb_device_t* device, int requesttype, int request, int value, int index, void* bytes, int size) {
  int num_bytes = usb_control_msg(device->handle, requesttype, request, value, index, bytes, size, TIMEOUT);
  if (num_bytes<0) {
    usb_clear_halt(device->handle, device->in_endpoint);
    return 0;
  }
  return num_bytes;
}


int libusb_exit(void) {
  syslog(LOG_INFO, "libusbi: libusb_exit, invocation counter=%d", invocation_count);
  invocation_count--;
  if (invocation_count != 0) return 0;
  syslog(LOG_INFO, "libusbi: shutting down...");
  libusb_detach_devices();
  return 0;
}

