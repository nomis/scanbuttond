// scanbuttond
// meta backend
// Copyleft )c( 2005 by Bernhard Stiftner
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

#ifndef __META_H_INCLUDED
#define __META_H_INCLUDED

#include "backend.h"

struct backend;
typedef struct backend backend_t;

struct backend {
  char* (*scanbtnd_get_backend_name)(void);
  int (*scanbtnd_init)(void);
  int (*scanbtnd_rescan)(void);
  scanner_t* (*scanbtnd_get_supported_devices)(void);
  int (*scanbtnd_open)(scanner_t* scanner);
  int (*scanbtnd_close)(scanner_t* scanner);
  int (*scanbtnd_get_button)(scanner_t* scanner);
  char* (*scanbtnd_get_sane_device_descriptor)(scanner_t* scanner);
  int (*scanbtnd_exit)(void);
  
  backend_t* next;
};
  

#endif
