// scanbuttond
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

#ifndef __BACKEND_H_INCLUDED
#define __BACKEND_H_INCLUDED

#include "scanbuttond.h"

// Get the name of this backend.
char* scanbtnd_get_backend_name(void);

// Initialize backend, search for supported devices
int scanbtnd_init(void);

// Rescan for supported scanners.
int scanbtnd_rescan(void);

// Returns a linked list of scanner devices which are supported by this backend.
scanner_device* scanbtnd_get_supported_devices(void);

// Opens the given scanner. WARNING! Access to the scanner will be blocked for 
// other applications (like SANE) until you call scanbtnd_close(,,,), 
int scanbtnd_open(scanner_device* scanner);

// Closes the given scanner device. After that, other applications may access the 
// scanner again.
int scanbtnd_close(scanner_device* scanner);

// Query the given scanner's button status. Returns the number of the pressed button 
// or 0 if no button is currently pressed.
int scanbtnd_get_button(scanner_device* scanner);

// Get a valid SANE device name for this scanner, e.g. 'epson:libusb:003:017'.
// Returns NULL if such a device name cannot be determined.
// The memory for the string is managed by scanbtnd and should not be free'd.
char* scanbtnd_get_sane_device_descriptor(scanner_device* scanner);

// Clean up internal data structures, free some memory
int scanbtnd_exit(void);

#endif
