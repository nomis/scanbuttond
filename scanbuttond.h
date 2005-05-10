//
// Epson GT-9300 scanner button daemon
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
//

#ifndef SCANBUTTOND_H_INCLUDED
#define SCANBUTTOND_H_INCLUDED

// uncomment to enable lots of debug statements...
// #define DEBUG

// connection types
#define CONNECTIONS_COUNT	2
#define CONNECTION_NONE		0
#define CONNECTION_LIBUSB 	1

char* scanbtnd_get_connection_name(int connection);

struct scanner_device;
typedef struct scanner_device scanner_device;

struct scanner_device {
	char* vendor;
	char* product;
	int connection;
	void* internal_dev_ptr;
	char* sane_device;
	void* meta_info;
        int lastbutton;
	
        scanner_device* next;
};

#endif
