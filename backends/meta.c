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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scanbuttond.h"
#include "meta.h"


static char* backend_name = "Dynamic Module Loader";

scanner_t* scanners = NULL;
backend_t* backends = NULL;


char* scanbtnd_get_backend_name(void) {
  return backend_name;
}


void attach_scanner(scanner_t* scanner, backend_t* backend) {
  scanner_t* dev = (scanner_t*)malloc(sizeof(scanner_t));
  dev->vendor = scanner->vendor;
  dev->product = scanner->product;
  dev->connection = scanner->connection;
  dev->internal_dev_ptr = scanner->internal_dev_ptr;
  dev->sane_device = scanner->sane_device;
  dev->meta_info = (void*)backend;
  dev->next = scanners;
  scanners = dev;
}


void attach_scanners(scanner_t* scanners, backend_t* backend) {
  scanner_t* dev = scanners;
  while (dev != NULL) {
    attach_scanner(dev, backend);
    dev = dev->next;
  }
}


void detach_scanners(void) {
  scanner_t* next;
  while (scanners != NULL) {
    next = scanners->next;    
    free(scanners);
    scanners = next;
  }
}


void attach_backend(backend_t* backend) {
  // don't attach another meta backend
  if (strcmp(backend->scanbtnd_get_backend_name(), scanbtnd_get_backend_name()) == 0) {
    return;
  }
  backend->next = backends;
  backends = backend;
  backend->scanbtnd_init();
  attach_scanners(backend->scanbtnd_get_supported_devices(), backend);
}


void detach_backend(backend_t* backend) {
  backend->scanbtnd_exit();
  free(backend);
}

		
void detach_backends(void) {
  backend_t* next;
  while (backends != NULL) {
    next = backends->next;    
    detach_backend(backends);
    backends = next;
  }
  detach_scanners();
}


backend_t* lookup_backend(scanner_t* scanner) {
  return (backend_t*)scanner->meta_info;
}


int scanbtnd_init(void) {
  scanners = NULL;
  // TODO: load all modules (using dlopen(...) or whatever)
  // TODO: for every module, create a backend_t structure
  // TODO: call attach_backend(...) for every backend
  return 0;
}


int scanbtnd_rescan(void) {
  // TODO: implement
  return 0;
}


scanner_t* scanbtnd_get_supported_devices(void) {
  return scanners;
}


int scanbtnd_open(scanner_t* scanner) {  
  backend_t* backend = lookup_backend(scanner);
  if (backend == NULL) return -1;
  return backend->scanbtnd_open(scanner);
}


int scanbtnd_close(scanner_t* scanner) {
  backend_t* backend = lookup_backend(scanner);
  if (backend == NULL) return -1;
  return backend->scanbtnd_close(scanner);
}


int scanbtnd_get_button(scanner_t* scanner) {
  backend_t* backend = lookup_backend(scanner);
  if (backend == NULL) return 0;
  return backend->scanbtnd_get_button(scanner);
}


char* scanbtnd_get_sane_device_descriptor(scanner_t* scanner) {
  backend_t* backend = lookup_backend(scanner);
  if (backend == NULL) return NULL;
  return backend->scanbtnd_get_sane_device_descriptor(scanner);
}


int scanbtnd_exit(void) {
  detach_backends();
  return 0;
}

