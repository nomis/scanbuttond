// genesys.c: Genesys device backend
// This file is part of scanbuttond.
// Copyleft )c( 2005 by Hans Verkuil
// Copyleft )c( 2005-2006 by Bernhard Stiftner
// Copyright 2020 Simon Arlott
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
#include "scanbuttond/scanbuttond.h"
#include "scanbuttond/libusbi.h"
#include "genesys.h"

static char* backend_name = "Genesys USB";

#define NUM_SUPPORTED_USB_DEVICES 3

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x04a9, 0x221c, 4 }, // CanoScan LiDE 60
	{ 0x04a9, 0x2213, 4 }, // CanoScan LiDE 35
	{ 0x04a9, 0x190f, 5 }  // CanoScan LiDE 220
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Canon", "CanoScan LiDE 60" },
	{ "Canon", "CanoScan LiDE 35" },
	{ "Canon", "CanoScan LiDE 220" }
};

static libusb_handle_t* libusb_handle;
static scanner_t* genesys_scanners = NULL;

// Button Map for CanonScan LiDE 60
// button 1 = 0x08 copy
// button 2 = 0x01 scan
// button 3 = 0x02 pdf
// button 4 = 0x04 mail
static int button_map_lide60[256] = {  0,  2,  3,  0,
				       4,  0,  0,  0,
				       1,  0,  0,  0,
				       0,  0,  0,  0};

// Button Map for CanoScan LiDE 220
// button 1 = 0x10 pdf1
// button 2 = 0x01 pdf2
// button 3 = 0x02 auto
// button 4 = 0x04 copy
// button 5 = 0x08 send
static int button_map_lide220[256] = {
				       0,  2,  3,  0,
				       4,  0,  0,  0,
				       5,  0,  0,  0,
				       0,  0,  0,  0,
				       1,  0,  0,  0,
				       0,  0,  0,  0,
				       0,  0,  0,  0,
				       0,  0,  0,  0,
				     };

struct genesys_config {
	int reg;
	int mask;
	int* button_map;
};

static struct genesys_config usb_device_config[NUM_SUPPORTED_USB_DEVICES] = {
	{ 0x6d, 0x0f, button_map_lide60 },
	{ 0x6d,	0x0f, button_map_lide60 },
	{ 0x31,	0x1f, button_map_lide220 }
};

// returns -1 if the scanner is unsupported, or the index of the
// corresponding vendor-product pair in the supported_usb_devices array.
int genesys_match_libusb_scanner(libusb_device_t* device)
{
   int index;
   for (index = 0; index < NUM_SUPPORTED_USB_DEVICES; index++) {
      if (supported_usb_devices[index][0] == device->vendorID &&
	  supported_usb_devices[index][1] == device->productID) {
	 break;
      }
   }
   if (index >= NUM_SUPPORTED_USB_DEVICES) return -1;
   return index;
}


void genesys_attach_libusb_scanner(libusb_device_t* device)
{
   const char* descriptor_prefix = "genesys:libusb:";
   int index = genesys_match_libusb_scanner(device);
   if (index < 0) return; // unsupported
   scanner_t* scanner = (scanner_t*)malloc(sizeof(scanner_t));
   scanner->vendor = usb_device_descriptions[index][0];
   scanner->product = usb_device_descriptions[index][1];
   scanner->connection = CONNECTION_LIBUSB;
   scanner->internal_dev_ptr = (void*)device;
   scanner->lastbutton = 0;
   scanner->sane_device = (char*)malloc(strlen(device->location) +
					strlen(descriptor_prefix) + 1);
   strcpy(scanner->sane_device, descriptor_prefix);
   strcat(scanner->sane_device, device->location);
   scanner->num_buttons = supported_usb_devices[index][2];
   scanner->backend_config = &usb_device_config[index];
   scanner->is_open = 0;
   scanner->next = genesys_scanners;
   genesys_scanners = scanner;
}


void genesys_detach_scanners(void)
{
   scanner_t* next;
   while (genesys_scanners != NULL) {
      next = genesys_scanners->next;
      free(genesys_scanners->sane_device);
      free(genesys_scanners);
      genesys_scanners = next;
   }
}


void genesys_scan_devices(libusb_device_t* devices)
{
   int index;
   libusb_device_t* device = devices;
   while (device != NULL) {
      index = genesys_match_libusb_scanner(device);
      if (index >= 0)
	 genesys_attach_libusb_scanner(device);
      device = device->next;
   }
}


int genesys_init_libusb(void)
{
   libusb_device_t* devices;
   
   libusb_handle = libusb_init();
   devices = libusb_get_devices(libusb_handle);
   genesys_scan_devices(devices);
   return 0;
}


const char* scanbtnd_get_backend_name(void)
{
   return backend_name;
}


int scanbtnd_init(void)
{
   genesys_scanners = NULL;
   
   syslog(LOG_INFO, "genesys-backend: init");
   return genesys_init_libusb();
}


int scanbtnd_rescan(void)
{
   libusb_device_t* devices;
   
   genesys_detach_scanners();
   genesys_scanners = NULL;
   libusb_rescan(libusb_handle);
   devices = libusb_get_devices(libusb_handle);
   genesys_scan_devices(devices);
   return 0;
}


const scanner_t* scanbtnd_get_supported_devices(void)
{
   return genesys_scanners;
}


int scanbtnd_open(scanner_t* scanner)
{
   int result = -ENOSYS;
   if (scanner->is_open)
      return -EINVAL;
   switch (scanner->connection) {
      case CONNECTION_LIBUSB:
	 // if devices have been added/removed, return -ENODEV to
	 // make scanbuttond update its device list
	 if (libusb_get_changed_device_count() != 0)
	    return -ENODEV;
	 result = libusb_open((libusb_device_t*)scanner->internal_dev_ptr);
	 break;
	}
   if (result == 0)
      scanner->is_open = 1;
   return result;
}


int scanbtnd_close(scanner_t* scanner)
{
   int result = -ENOSYS;
   if (!scanner->is_open)
      return -EINVAL;
   switch (scanner->connection) {
      case CONNECTION_LIBUSB:
	 result = libusb_close((libusb_device_t*)scanner->internal_dev_ptr);
	 break;
   }
   if (result == 0)
      scanner->is_open = 0;
   return result;
}

int scanbtnd_get_button(scanner_t* scanner)
{
   unsigned char bytes[2];
   int num_bytes;
   int button = 0;
   struct genesys_config *config = scanner->backend_config;
   
   if (!scanner->is_open)
      return -EINVAL;

   // every time we want to read the key currently pressed we need to send
   // some specific data to the scanner first
   // 40 0c 83 00 00 00 01 00  -> 0x6d
   bytes[0] = config->reg;
   bytes[1] = 0x00; // not really needed, but just to be sure ;-)
   num_bytes = libusb_control_msg((libusb_device_t*)scanner->internal_dev_ptr,
				  0x40, 0x0c, 0x0083, 0x0000, (void *)bytes, 0x0001);
   
   if (num_bytes != 1) {
      syslog(LOG_WARNING, "genesys-backend: communication error: "
			"read length:%d (expected:%d)", num_bytes, 1);
      return 0;
   }

   usleep(10000);

   // now we can ask for the current state
   // only the currently pressed keys are reported, if some key was pressed _and_ release between
   // the last an the current query it is not reported here, depending on the query frequence
   // the key needs to be holded for some time to be recognised
   
   // c0 0c 84 00 00 00 01 00
   // returns 1f xored with the keys pressed
   // - this mean any key that is pressed gets it bit removed from 0x1f
   // - if multiple keys are pressed at the same time multiple bits will be removed
   num_bytes = libusb_control_msg((libusb_device_t*)scanner->internal_dev_ptr,
				  0xc0, 0x0c, 0x0084, 0x0000, (void *)bytes, 0x0001);
   if (num_bytes != 1) {
      syslog(LOG_WARNING, "genesys-backend: communication error: "
         "could not read status register");
      return 0;
   }

   // xor with mask and use only lower 4 bit
   // lookup button in button map and return
   return config->button_map[(bytes[0] ^ config->mask) & config->mask];
}

const char* scanbtnd_get_sane_device_descriptor(scanner_t* scanner)
{
   return scanner->sane_device;
}


int scanbtnd_exit(void)
{
   syslog(LOG_INFO, "genesys-backend: exit");
   genesys_detach_scanners();
   libusb_exit(libusb_handle);
   return 0;
}

