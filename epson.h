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

#ifndef __EPSON_H_INCLUDED
#define __EPSON_H_INCLUDED

#include "scanbuttond.h"

// Initializes the epson scanner driver using the given connection type
int epson_init(int connection);

// Searches for supported devices. Returns the number of found & supported scanners.
int epson_get_num_devices();

// Returns a device handle for scanner number 'index'.
scanner_device* epson_get_device(int index);

// Opens the given scanner device
int epson_open(scanner_device* scanner);

// Closes the given scanner device
int epson_close(scanner_device* scanner);

// Query the given scanner's button status. Returns the number of the pressed button 
// or 0 if no button is currently pressed.
int epson_get_button(scanner_device* scanner);

#endif

