// interface_libusb.c: libusb wrapper
// This file is part of scanbuttond.
// Copyleft )c( 2004-2006 by Bernhard Stiftner
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
#include "scanbuttond/interface_usb.h"

#define TIMEOUT	   	10 * 1000	/* 10 seconds */

int invocation_count = 0;


scanbtnd_libusb_handle_t* scanbtnd_libusb_init(void)
{
	scanbtnd_libusb_handle_t* handle;
	invocation_count++;
	if (invocation_count == 1) {
		syslog(LOG_INFO, "interface_libusb: initializing...");
		usb_init();
	}
	handle = (scanbtnd_libusb_handle_t*)malloc(sizeof(scanbtnd_libusb_handle_t));
	handle->devices = NULL;
	scanbtnd_libusb_rescan(handle);
	return handle;
}


int scanbtnd_libusb_search_interface(struct usb_device* device)
{
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


int scanbtnd_libusb_search_in_endpoint(struct usb_device* device)
{
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


int scanbtnd_libusb_search_out_endpoint(struct usb_device* device)
{
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


void scanbtnd_libusb_attach_device(struct usb_device* device, scanbtnd_libusb_handle_t* handle)
{
	scanbtnd_libusb_device_t* libusb_device = (scanbtnd_libusb_device_t*)malloc(sizeof(scanbtnd_libusb_device_t));
	libusb_device->vendorID = device->descriptor.idVendor;
	libusb_device->productID = device->descriptor.idProduct;

	// the location string consists of bus number, followed by a colon (":"), and the device number
	libusb_device->location = (char*)malloc(strlen(device->bus->dirname) + strlen(device->filename) + 2);
	strcpy(libusb_device->location, device->bus->dirname);
	strcat(libusb_device->location, ":");
	strcat(libusb_device->location, device->filename);

	libusb_device->device = device;
	libusb_device->handle = NULL;
	libusb_device->interface = scanbtnd_libusb_search_interface(device);
	if (libusb_device->interface < 0) {
		free(libusb_device->location);
		free(libusb_device);
		return;
	}
	libusb_device->out_endpoint = scanbtnd_libusb_search_out_endpoint(device);
	if (libusb_device->out_endpoint < 0) {
		free(libusb_device->location);
		free(libusb_device);
		return;
	}
	libusb_device->in_endpoint = scanbtnd_libusb_search_in_endpoint(device);
	if (libusb_device->in_endpoint < 0) {
		free(libusb_device->location);
		free(libusb_device);
		return;
	}
	libusb_device->next = handle->devices;
	handle->devices = libusb_device;
}


void scanbtnd_libusb_detach_devices(scanbtnd_libusb_handle_t* handle)
{
	scanbtnd_libusb_device_t* next;
	while (handle->devices != NULL) {
		next = handle->devices->next;
		free(handle->devices->location);
		free(handle->devices);
		handle->devices = next;
	}
}


void scanbtnd_libusb_rescan(scanbtnd_libusb_handle_t* handle)
{
	struct usb_bus *bus;
	struct usb_device *device;

	scanbtnd_libusb_detach_devices(handle);

	usb_find_busses();
	usb_find_devices();
	handle->devices = NULL;

	bus = usb_busses;
	while (bus != NULL) {
		device = bus->devices;
		while (device != NULL) {
			scanbtnd_libusb_attach_device(device, handle);
			device = device->next;
		}
		bus = bus->next;
	}

}


int scanbtnd_libusb_get_changed_device_count(void)
{
	usb_find_busses();
	return usb_find_devices();
}


scanbtnd_libusb_device_t* scanbtnd_libusb_get_devices(scanbtnd_libusb_handle_t* handle)
{
	return handle->devices;
}


int scanbtnd_libusb_open(scanbtnd_libusb_device_t* device)
{
	int result;

	if (!device || !device->device)
		return -ENODEV;

	device->handle = usb_open(device->device);
	if (device->handle == NULL) {
		syslog(LOG_ERR, "interface_libusb: could not open device %s", device->location);
		return -ENODEV;
	}

	// Calling usb_set_configuration should not be necessary.
	// It is even considered harmful, since it may disturb other processes
	// which are currently communicating with the scanner!
	//
	// usb_set_configuration(device->handle,
	//     usb_device(device->handle)->config[0].bConfigurationValue);

	result = usb_claim_interface(device->handle, device->interface);
	switch (result) {
		case 0:
			return 0;
		case -ENOMEM:
			syslog(LOG_ERR, "interface_libusb: could not claim interface for device %s. (ENOMEM)",
				   device->location);
			usb_close(device->handle);
			return -ENODEV;
		case -EBUSY:
			syslog(LOG_ERR, "interface_libusb: could not claim interface for device %s. (EBUSY)",
				   device->location);
			usb_close(device->handle);
			return -EBUSY;
		default:
			syslog(LOG_ERR, "interface_libusb: could not claim interface for device %s. (code=%d)",
				   device->location, result);
			usb_close(device->handle);
			return -ENODEV;
	}
}


int scanbtnd_libusb_close(scanbtnd_libusb_device_t* device)
{
	int result;
	result = usb_release_interface(device->handle, device->interface);
	if (result < 0) {
		syslog(LOG_ERR, "interface_libusb: could not release interface, error code=%d, device=%s",
			   result, device->location);
		return result;
	}
	result = usb_close(device->handle);
	if (result < 0) {
		syslog(LOG_ERR, "interface_libusb: could not close usb device, error code=%d, device=%s",
			   result, device->location);
		return result;
	}
	return 0;
}


int scanbtnd_libusb_read(scanbtnd_libusb_device_t* device, void* buffer, int bytecount)
{
	int num_bytes = usb_bulk_read(device->handle, device->in_endpoint,
								  buffer, bytecount, TIMEOUT);
	if (num_bytes<0) {
		usb_clear_halt(device->handle, device->in_endpoint);
		return 0;
	}
	return num_bytes;
}


int scanbtnd_libusb_write(scanbtnd_libusb_device_t* device, void* buffer, int bytecount)
{
	int num_bytes = usb_bulk_write(device->handle, device->out_endpoint,
								   buffer, bytecount, TIMEOUT);
	if (num_bytes<0) {
		usb_clear_halt(device->handle, device->in_endpoint);
		return 0;
	}
	return num_bytes;
}


void scanbtnd_libusb_flush(scanbtnd_libusb_device_t* device)
{
	char buffer[16];
	while (usb_bulk_read(device->handle, device->in_endpoint, buffer, 16, 500) > 0) {};
}


int scanbtnd_libusb_control_msg(scanbtnd_libusb_device_t* device, int requesttype, int request,
					   int value, int index, void* bytes, int size)
{
	int num_bytes = usb_control_msg(device->handle, requesttype, request, value,
									index, bytes, size, TIMEOUT);
	if (num_bytes<0) {
		// Doesn't seem to be needed... (bs, Jun 07 2005)
		// usb_clear_halt(device->handle, device->in_endpoint);
		return 0;
	}
	return num_bytes;
}


void scanbtnd_libusb_exit(scanbtnd_libusb_handle_t* handle)
{
	invocation_count--;
	if (invocation_count == 0)
		syslog(LOG_INFO, "interface_libusb: shutting down...");
	scanbtnd_libusb_detach_devices(handle);
	free(handle);
}

